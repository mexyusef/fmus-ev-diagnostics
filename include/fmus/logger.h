#ifndef FMUS_LOGGER_H
#define FMUS_LOGGER_H

/**
 * @file logger.h
 * @brief Simple logging system for FMUS-AUTO
 */

#include <string>
#include <memory>
#include <mutex>
#include <fstream>

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
 * @brief Log levels
 */
enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3
};

/**
 * @brief Simple logger class
 */
class FMUS_AUTO_API Logger {
public:
    /**
     * @brief Get the singleton logger instance
     */
    static std::shared_ptr<Logger> getInstance();
    
    /**
     * @brief Set the minimum log level
     */
    void setLogLevel(LogLevel level);
    
    /**
     * @brief Enable/disable console logging
     */
    void enableConsoleLogging(bool enable);
    
    /**
     * @brief Enable file logging
     */
    bool enableFileLogging(const std::string& filename);
    
    /**
     * @brief Log a message
     */
    void log(LogLevel level, const std::string& message);
    
    /**
     * @brief Log debug message
     */
    void debug(const std::string& message);
    
    /**
     * @brief Log info message
     */
    void info(const std::string& message);
    
    /**
     * @brief Log warning message
     */
    void warning(const std::string& message);
    
    /**
     * @brief Log error message
     */
    void error(const std::string& message);

    // Destructor needs to be public for shared_ptr
    ~Logger();

private:
    Logger();
    
    static std::shared_ptr<Logger> instance;
    static std::mutex instanceMutex;
    
    LogLevel currentLevel = LogLevel::Info;
    bool logToConsole = true;
    bool logToFile = false;
    std::ofstream fileStream;
    std::mutex logMutex;
};

// Convenience functions
FMUS_AUTO_API void debug(const std::string& message);
FMUS_AUTO_API void info(const std::string& message);
FMUS_AUTO_API void warning(const std::string& message);
FMUS_AUTO_API void error(const std::string& message);
FMUS_AUTO_API bool enableFileLogging(const std::string& filename);

} // namespace fmus

#endif // FMUS_LOGGER_H
