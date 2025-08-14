#ifndef FMUS_PROTOCOLS_ISO9141_H
#define FMUS_PROTOCOLS_ISO9141_H

/**
 * @file iso9141.h
 * @brief ISO 9141 Protocol Implementation
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
 * @brief ISO 9141 Message structure
 */
struct ISO9141Message {
    uint8_t format = 0x68;              ///< Format byte
    uint8_t targetAddress = 0x6A;       ///< Target address
    uint8_t sourceAddress = 0xF1;       ///< Source address
    std::vector<uint8_t> data;          ///< Message data
    uint8_t checksum = 0;               ///< Checksum byte
    bool isResponse = false;
    std::chrono::system_clock::time_point timestamp;
    
    ISO9141Message() = default;
    ISO9141Message(uint8_t target, uint8_t source, const std::vector<uint8_t>& msgData)
        : targetAddress(target), sourceAddress(source), data(msgData),
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
    static ISO9141Message fromBytes(const std::vector<uint8_t>& bytes);
    
    /**
     * @brief Get message length
     */
    uint8_t getLength() const;
    
    std::string toString() const;
    bool isValid() const;
};

/**
 * @brief ISO 9141 Configuration
 */
struct ISO9141Config {
    uint32_t baudRate = 10400;          ///< Baud rate (typically 10400)
    uint32_t timeout = 1000;            ///< Response timeout (ms)
    uint32_t p1Max = 20;                ///< P1 timing (ms)
    uint32_t p2Min = 25;                ///< P2 minimum timing (ms)
    uint32_t p2Max = 50;                ///< P2 maximum timing (ms)
    uint32_t p3Min = 55;                ///< P3 minimum timing (ms)
    uint32_t p4Min = 5;                 ///< P4 minimum timing (ms)
    bool useChecksum = true;            ///< Enable checksum validation
    uint8_t sourceAddress = 0xF1;       ///< Default source address
    
    std::string toString() const;
};

/**
 * @brief ISO 9141 Initialization sequence
 */
struct ISO9141InitSequence {
    std::vector<uint8_t> initBytes = {0x33, 0x6B, 0x8F, 0x40, 0x87};
    uint32_t initBaudRate = 5;          ///< 5 baud initialization
    uint32_t syncPattern = 0x55;        ///< Sync pattern
    bool fastInit = false;              ///< Use fast initialization
    
    std::string toString() const;
};

/**
 * @brief ISO 9141 Protocol Implementation
 */
class FMUS_AUTO_API ISO9141Protocol {
public:
    /**
     * @brief Constructor
     */
    ISO9141Protocol();
    
    /**
     * @brief Destructor
     */
    ~ISO9141Protocol();
    
    /**
     * @brief Initialize protocol
     */
    bool initialize(const ISO9141Config& config);
    
    /**
     * @brief Shutdown protocol
     */
    void shutdown();
    
    /**
     * @brief Check if initialized
     */
    bool isInitialized() const;
    
    /**
     * @brief Perform 5-baud initialization
     */
    bool performInit(const ISO9141InitSequence& initSeq = ISO9141InitSequence{});
    
    /**
     * @brief Send ISO 9141 message
     */
    bool sendMessage(const ISO9141Message& message);
    
    /**
     * @brief Send request and wait for response
     */
    ISO9141Message sendRequest(const ISO9141Message& request);
    
    /**
     * @brief Start message monitoring
     */
    bool startMonitoring(std::function<void(const ISO9141Message&)> callback);
    
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
        uint64_t initAttempts = 0;
        uint64_t initSuccesses = 0;
        std::chrono::system_clock::time_point startTime;
    };
    
    Statistics getStatistics() const;
    void resetStatistics();
    
    /**
     * @brief Get current configuration
     */
    ISO9141Config getConfiguration() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief ISO 9141 Error class
 */
class FMUS_AUTO_API ISO9141Error : public std::runtime_error {
public:
    enum class ErrorCode {
        INITIALIZATION_FAILED,
        INIT_SEQUENCE_FAILED,
        SEND_FAILED,
        RECEIVE_TIMEOUT,
        CHECKSUM_ERROR,
        INVALID_MESSAGE,
        TIMING_ERROR
    };
    
    ISO9141Error(ErrorCode code, const std::string& message)
        : std::runtime_error(message), errorCode(code) {}
    
    ErrorCode getErrorCode() const { return errorCode; }

private:
    ErrorCode errorCode;
};

// Utility functions
FMUS_AUTO_API uint8_t calculateISO9141Checksum(const std::vector<uint8_t>& data);
FMUS_AUTO_API bool isValidISO9141Address(uint8_t address);
FMUS_AUTO_API std::vector<uint8_t> createISO9141InitSequence(uint8_t ecuAddress);

} // namespace protocols
} // namespace fmus

#endif // FMUS_PROTOCOLS_ISO9141_H
