#ifndef FMUS_PROTOCOLS_J1850_H
#define FMUS_PROTOCOLS_J1850_H

/**
 * @file j1850.h
 * @brief J1850 VPW/PWM Protocol Implementation
 */

#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <string>

// Define exports macro
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
namespace protocols {

/**
 * @brief J1850 Protocol Types
 */
enum class J1850Type {
    VPW,    ///< Variable Pulse Width (10.4 kbps)
    PWM     ///< Pulse Width Modulation (41.6 kbps)
};

/**
 * @brief J1850 Message Priority
 */
enum class J1850Priority : uint8_t {
    HIGHEST = 0x00,
    HIGH = 0x01,
    MEDIUM = 0x02,
    LOW = 0x03
};

/**
 * @brief J1850 Message structure
 */
struct J1850Message {
    J1850Priority priority = J1850Priority::MEDIUM;
    uint8_t sourceAddress = 0xF1;
    uint8_t targetAddress = 0x10;
    std::vector<uint8_t> data;
    uint8_t checksum = 0;
    bool isResponse = false;
    std::chrono::system_clock::time_point timestamp;
    
    J1850Message() = default;
    J1850Message(uint8_t src, uint8_t target, const std::vector<uint8_t>& msgData)
        : sourceAddress(src), targetAddress(target), data(msgData), 
          timestamp(std::chrono::system_clock::now()) {
        calculateChecksum();
    }
    
    /**
     * @brief Calculate and set checksum
     */
    void calculateChecksum();
    
    /**
     * @brief Verify checksum
     */
    bool verifyChecksum() const;
    
    /**
     * @brief Get message as raw bytes
     */
    std::vector<uint8_t> toBytes() const;
    
    /**
     * @brief Create from raw bytes
     */
    static J1850Message fromBytes(const std::vector<uint8_t>& bytes);
    
    /**
     * @brief Get header byte
     */
    uint8_t getHeaderByte() const;
    
    std::string toString() const;
    bool isValid() const;
};

/**
 * @brief J1850 Configuration
 */
struct J1850Config {
    J1850Type protocolType = J1850Type::VPW;
    uint32_t timeout = 1000;            ///< Response timeout (ms)
    bool useChecksum = true;            ///< Enable checksum validation
    uint8_t sourceAddress = 0xF1;       ///< Default source address
    
    std::string toString() const;
};

/**
 * @brief J1850 Protocol Implementation
 */
class FMUS_AUTO_API J1850Protocol {
public:
    /**
     * @brief Constructor
     */
    J1850Protocol();
    
    /**
     * @brief Destructor
     */
    ~J1850Protocol();
    
    /**
     * @brief Initialize protocol
     */
    bool initialize(const J1850Config& config);
    
    /**
     * @brief Shutdown protocol
     */
    void shutdown();
    
    /**
     * @brief Check if initialized
     */
    bool isInitialized() const;
    
    /**
     * @brief Send J1850 message
     */
    bool sendMessage(const J1850Message& message);
    
    /**
     * @brief Send request and wait for response
     */
    J1850Message sendRequest(const J1850Message& request);
    
    /**
     * @brief Start message monitoring
     */
    bool startMonitoring(std::function<void(const J1850Message&)> callback);
    
    /**
     * @brief Stop message monitoring
     */
    void stopMonitoring();
    
    /**
     * @brief Check if monitoring
     */
    bool isMonitoring() const;
    
    /**
     * @brief Get protocol statistics
     */
    struct Statistics {
        uint64_t messagesSent = 0;
        uint64_t messagesReceived = 0;
        uint64_t checksumErrors = 0;
        uint64_t timeouts = 0;
        std::chrono::system_clock::time_point startTime;
    };
    
    Statistics getStatistics() const;
    void resetStatistics();
    
    /**
     * @brief Get current configuration
     */
    J1850Config getConfiguration() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief J1850 Error class
 */
class FMUS_AUTO_API J1850Error : public std::runtime_error {
public:
    enum class ErrorCode {
        INITIALIZATION_FAILED,
        SEND_FAILED,
        RECEIVE_TIMEOUT,
        CHECKSUM_ERROR,
        INVALID_MESSAGE
    };
    
    J1850Error(ErrorCode code, const std::string& message)
        : std::runtime_error(message), errorCode(code) {}
    
    ErrorCode getErrorCode() const { return errorCode; }

private:
    ErrorCode errorCode;
};

// Utility functions
FMUS_AUTO_API std::string j1850TypeToString(J1850Type type);
FMUS_AUTO_API std::string j1850PriorityToString(J1850Priority priority);
FMUS_AUTO_API uint8_t calculateJ1850Checksum(const std::vector<uint8_t>& data);
FMUS_AUTO_API bool isValidJ1850Address(uint8_t address);

} // namespace protocols
} // namespace fmus

#endif // FMUS_PROTOCOLS_J1850_H
