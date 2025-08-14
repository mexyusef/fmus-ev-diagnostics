#ifndef FMUS_THREAD_POOL_H
#define FMUS_THREAD_POOL_H

/**
 * @file thread_pool.h
 * @brief Thread pool for asynchronous operations
 */

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

// Define exports macro for cross-platform compatibility
#ifdef _WIN32
    #ifdef FMUS_AUTO_EXPORTS
        #define FMUS_AUTO_API __declspec(dllexport)
    #else
        #define FMUS_AUTO_API __declspec(dllimport)
    #endif
#else
    #define FMUS_AUTO_API
#endif

namespace fmus {

/**
 * @brief Thread pool for executing tasks asynchronously
 */
class FMUS_AUTO_API ThreadPool {
public:
    /**
     * @brief Constructor
     * @param threads Number of worker threads (0 = auto-detect)
     */
    explicit ThreadPool(size_t threads = 0);
    
    /**
     * @brief Destructor - waits for all tasks to complete
     */
    ~ThreadPool();
    
    /**
     * @brief Enqueue a task for execution
     * @param f Function to execute
     * @param args Arguments to pass to the function
     * @return Future that will contain the result
     */
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;
    
    /**
     * @brief Get the number of worker threads
     */
    size_t getThreadCount() const;
    
    /**
     * @brief Get the number of pending tasks
     */
    size_t getPendingTaskCount() const;
    
    /**
     * @brief Check if the thread pool is stopping
     */
    bool isStopping() const;
    
    /**
     * @brief Wait for all pending tasks to complete
     */
    void waitForAll();
    
    /**
     * @brief Stop the thread pool (no new tasks accepted)
     */
    void stop();

private:
    // Worker threads
    std::vector<std::thread> workers;
    
    // Task queue
    std::queue<std::function<void()>> tasks;
    
    // Synchronization
    mutable std::mutex queueMutex;
    std::condition_variable condition;
    std::condition_variable finished;
    
    // State
    bool stop_flag;
    size_t activeTasks;
};

/**
 * @brief Get the global thread pool instance
 */
FMUS_AUTO_API std::shared_ptr<ThreadPool> getGlobalThreadPool();

/**
 * @brief Set the global thread pool instance
 */
FMUS_AUTO_API void setGlobalThreadPool(std::shared_ptr<ThreadPool> pool);

// Template implementation
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type> {

    using return_type = typename std::result_of<F(Args...)>::type;
    
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<return_type> res = task->get_future();
    
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        
        // Don't allow enqueueing after stopping the pool
        if (stop_flag) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        
        tasks.emplace([task](){ (*task)(); });
        activeTasks++;
    }
    
    condition.notify_one();
    return res;
}

} // namespace fmus

#endif // FMUS_THREAD_POOL_H
