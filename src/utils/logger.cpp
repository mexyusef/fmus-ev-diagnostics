#include <fmus/logger.h>
#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace fmus {

// Singleton implementation for Logger
std::shared_ptr<Logger> Logger::instance = nullptr;
std::mutex Logger::instanceMutex;

// Mendapatkan timestamp saat ini dalam format yang bisa dibaca
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

// Mendapatkan string untuk level log yang sesuai
std::string levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARNING";
        case LogLevel::Error:   return "ERROR";
        default:                return "UNKNOWN";
    }
}

// Implementasi constructor Logger
Logger::Logger()
    : currentLevel(LogLevel::Info),
      logToConsole(true),
      logToFile(false) {
}

// Implementasi destructor Logger
Logger::~Logger() {
    if (fileStream.is_open()) {
        fileStream.close();
    }
}

// Implementasi getInstance untuk singleton pattern
std::shared_ptr<Logger> Logger::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance) {
        instance = std::shared_ptr<Logger>(new Logger());
    }
    return instance;
}

// Implementasi setLogLevel
void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex);
    currentLevel = level;
}

// Implementasi enableConsoleLogging
void Logger::enableConsoleLogging(bool enable) {
    std::lock_guard<std::mutex> lock(logMutex);
    logToConsole = enable;
}

// Implementasi enableFileLogging
bool Logger::enableFileLogging(const std::string& filename) {
    std::lock_guard<std::mutex> lock(logMutex);

    // Tutup file lama jika sudah ada yang terbuka
    if (fileStream.is_open()) {
        fileStream.close();
    }

    // Buka file baru untuk writing
    fileStream.open(filename, std::ios::out | std::ios::app);
    if (!fileStream.is_open()) {
        if (logToConsole) {
            std::cerr << "Failed to open log file: " << filename << std::endl;
        }
        logToFile = false;
        return false;
    }

    logToFile = true;
    return true;
}

// Implementasi log
void Logger::log(LogLevel level, const std::string& message) {
    // Hanya log jika level sesuai
    if (level < currentLevel) {
        return;
    }

    std::lock_guard<std::mutex> lock(logMutex);

    // Format pesan log
    std::string timestamp = getCurrentTimestamp();
    std::string levelStr = levelToString(level);
    std::string formattedMessage = timestamp + " [" + levelStr + "] " + message;

    // Output ke console jika diaktifkan
    if (logToConsole) {
        if (level == LogLevel::Error) {
            std::cerr << formattedMessage << std::endl;
        } else {
            std::cout << formattedMessage << std::endl;
        }
    }

    // Output ke file jika diaktifkan
    if (logToFile && fileStream.is_open()) {
        fileStream << formattedMessage << std::endl;
        fileStream.flush();
    }
}

// Implementasi debug
void Logger::debug(const std::string& message) {
    log(LogLevel::Debug, message);
}

// Implementasi info
void Logger::info(const std::string& message) {
    log(LogLevel::Info, message);
}

// Implementasi warning
void Logger::warning(const std::string& message) {
    log(LogLevel::Warning, message);
}

// Implementasi error
void Logger::error(const std::string& message) {
    log(LogLevel::Error, message);
}

} // namespace fmus