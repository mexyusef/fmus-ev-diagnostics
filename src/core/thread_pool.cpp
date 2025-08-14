#include <fmus/thread_pool.h>
#include <fmus/logger.h>

namespace fmus {

// Global thread pool instance
static std::shared_ptr<ThreadPool> globalThreadPool = nullptr;
static std::mutex globalThreadPoolMutex;

ThreadPool::ThreadPool(size_t threads) 
    : stop_flag(false), activeTasks(0) {
    
    // Auto-detect thread count if not specified
    if (threads == 0) {
        threads = std::thread::hardware_concurrency();
        if (threads == 0) {
            threads = 4; // Fallback
        }
    }
    
    auto logger = Logger::getInstance();
    logger->info("Creating thread pool with " + std::to_string(threads) + " threads");
    
    // Create worker threads
    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back([this] {
            for (;;) {
                std::function<void()> task;
                
                {
                    std::unique_lock<std::mutex> lock(this->queueMutex);
                    this->condition.wait(lock, [this] { 
                        return this->stop_flag || !this->tasks.empty(); 
                    });
                    
                    if (this->stop_flag && this->tasks.empty()) {
                        return;
                    }
                    
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                
                // Execute the task
                try {
                    task();
                } catch (const std::exception& e) {
                    auto logger = Logger::getInstance();
                    logger->error("Thread pool task exception: " + std::string(e.what()));
                } catch (...) {
                    auto logger = Logger::getInstance();
                    logger->error("Thread pool task unknown exception");
                }
                
                // Notify that a task is finished
                {
                    std::unique_lock<std::mutex> lock(this->queueMutex);
                    this->activeTasks--;
                    if (this->activeTasks == 0 && this->tasks.empty()) {
                        this->finished.notify_all();
                    }
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    stop();
}

size_t ThreadPool::getThreadCount() const {
    return workers.size();
}

size_t ThreadPool::getPendingTaskCount() const {
    std::unique_lock<std::mutex> lock(queueMutex);
    return tasks.size() + activeTasks;
}

bool ThreadPool::isStopping() const {
    std::unique_lock<std::mutex> lock(queueMutex);
    return stop_flag;
}

void ThreadPool::waitForAll() {
    std::unique_lock<std::mutex> lock(queueMutex);
    finished.wait(lock, [this] {
        return this->tasks.empty() && this->activeTasks == 0;
    });
}

void ThreadPool::stop() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (stop_flag) {
            return; // Already stopping
        }
        stop_flag = true;
    }
    
    condition.notify_all();
    
    for (std::thread &worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    auto logger = Logger::getInstance();
    logger->info("Thread pool stopped");
}

std::shared_ptr<ThreadPool> getGlobalThreadPool() {
    std::lock_guard<std::mutex> lock(globalThreadPoolMutex);
    if (!globalThreadPool) {
        globalThreadPool = std::make_shared<ThreadPool>();
    }
    return globalThreadPool;
}

void setGlobalThreadPool(std::shared_ptr<ThreadPool> pool) {
    std::lock_guard<std::mutex> lock(globalThreadPoolMutex);
    globalThreadPool = pool;
}

} // namespace fmus
