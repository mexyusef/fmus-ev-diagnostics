#ifndef FMUS_PROTOCOLS_KWP2000_H
#define FMUS_PROTOCOLS_KWP2000_H

/**
 * @file kwp2000.h
 * @brief KWP2000 (Keyword Protocol 2000) Implementation
 */

#include <fmus/protocols/can.h>
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
 * @brief KWP2000 Service IDs
 */
enum class KWP2000Service : uint8_t {
    START_DIAGNOSTIC_SESSION = 0x10,
    ECU_RESET = 0x11,
    READ_FAULT_MEMORY = 0x12,
    CLEAR_FAULT_MEMORY = 0x14,
    READ_STATUS_OF_FAULT_MEMORY = 0x17,
    READ_FAULT_MEMORY_BY_STATUS = 0x18,
    READ_DATA_BY_IDENTIFIER = 0x21,
    READ_DATA_BY_ADDRESS = 0x23,
    SECURITY_ACCESS = 0x27,
    DISABLE_NORMAL_MESSAGE_TRANSMISSION = 0x28,
    ENABLE_NORMAL_MESSAGE_TRANSMISSION = 0x29,
    DYNAMICALLY_DEFINE_MESSAGE = 0x2C,
    WRITE_DATA_BY_IDENTIFIER = 0x2E,
    INPUT_OUTPUT_CONTROL_BY_IDENTIFIER = 0x30,
    START_ROUTINE_BY_IDENTIFIER = 0x31,
    STOP_ROUTINE_BY_IDENTIFIER = 0x32,
    REQUEST_ROUTINE_RESULTS_BY_IDENTIFIER = 0x33,
    REQUEST_DOWNLOAD = 0x34,
    REQUEST_UPLOAD = 0x35,
    TRANSFER_DATA = 0x36,
    REQUEST_TRANSFER_EXIT = 0x37,
    TESTER_PRESENT = 0x3E
};

/**
 * @brief KWP2000 Negative Response Codes
 */
enum class KWP2000NRC : uint8_t {
    GENERAL_REJECT = 0x10,
    SERVICE_NOT_SUPPORTED = 0x11,
    SUB_FUNCTION_NOT_SUPPORTED = 0x12,
    BUSY_REPEAT_REQUEST = 0x21,
    CONDITIONS_NOT_CORRECT = 0x22,
    REQUEST_SEQUENCE_ERROR = 0x24,
    REQUEST_OUT_OF_RANGE = 0x31,
    SECURITY_ACCESS_DENIED = 0x33,
    INVALID_KEY = 0x35,
    EXCEED_NUMBER_OF_ATTEMPTS = 0x36,
    REQUIRED_TIME_DELAY_NOT_EXPIRED = 0x37
};

/**
 * @brief KWP2000 Message structure
 */
struct KWP2000Message {
    KWP2000Service service;
    std::vector<uint8_t> data;
    bool isResponse = false;
    bool isNegativeResponse = false;
    KWP2000NRC negativeResponseCode = KWP2000NRC::GENERAL_REJECT;
    std::chrono::system_clock::time_point timestamp;
    
    KWP2000Message() = default;
    KWP2000Message(KWP2000Service svc, const std::vector<uint8_t>& msgData = {})
        : service(svc), data(msgData), timestamp(std::chrono::system_clock::now()) {}
    
    /**
     * @brief Convert to CAN message
     */
    protocols::CANMessage toCANMessage(uint32_t canId) const;
    
    /**
     * @brief Create from CAN message
     */
    static KWP2000Message fromCANMessage(const protocols::CANMessage& canMsg);
    
    /**
     * @brief Get message as raw bytes
     */
    std::vector<uint8_t> toBytes() const;
    
    /**
     * @brief Create from raw bytes
     */
    static KWP2000Message fromBytes(const std::vector<uint8_t>& bytes);
    
    std::string toString() const;
    bool isValid() const;
};

/**
 * @brief KWP2000 Configuration
 */
struct KWP2000Config {
    uint32_t requestId = 0x200;         ///< Request CAN ID
    uint32_t responseId = 0x201;        ///< Response CAN ID
    uint32_t timeout = 1000;            ///< Response timeout (ms)
    uint32_t p2ClientMax = 50;          ///< P2 timing (ms)
    uint32_t p2StarClientMax = 5000;    ///< P2* timing (ms)
    bool useExtendedAddressing = false;  ///< Extended addressing
    uint8_t sourceAddress = 0xF1;       ///< Source address
    uint8_t targetAddress = 0x10;       ///< Target address
    
    std::string toString() const;
};

/**
 * @brief KWP2000 Protocol Implementation
 */
class FMUS_AUTO_API KWP2000Protocol {
public:
    /**
     * @brief Constructor
     */
    KWP2000Protocol();
    
    /**
     * @brief Destructor
     */
    ~KWP2000Protocol();
    
    /**
     * @brief Initialize protocol
     */
    bool initialize(const KWP2000Config& config, std::shared_ptr<CANProtocol> canProtocol);
    
    /**
     * @brief Shutdown protocol
     */
    void shutdown();
    
    /**
     * @brief Check if initialized
     */
    bool isInitialized() const;
    
    /**
     * @brief Send KWP2000 message
     */
    bool sendMessage(const KWP2000Message& message);
    
    /**
     * @brief Send request and wait for response
     */
    KWP2000Message sendRequest(const KWP2000Message& request);
    
    /**
     * @brief Start message monitoring
     */
    bool startMonitoring(std::function<void(const KWP2000Message&)> callback);
    
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
        uint64_t negativeResponses = 0;
        uint64_t timeouts = 0;
        std::chrono::system_clock::time_point startTime;
    };
    
    Statistics getStatistics() const;
    void resetStatistics();
    
    /**
     * @brief Get current configuration
     */
    KWP2000Config getConfiguration() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief KWP2000 Error class
 */
class FMUS_AUTO_API KWP2000Error : public std::runtime_error {
public:
    KWP2000Error(const std::string& message, KWP2000Service service = KWP2000Service::TESTER_PRESENT, 
                 KWP2000NRC nrc = KWP2000NRC::GENERAL_REJECT)
        : std::runtime_error(message), service(service), nrc(nrc) {}
    
    KWP2000Service getService() const { return service; }
    KWP2000NRC getNRC() const { return nrc; }

private:
    KWP2000Service service;
    KWP2000NRC nrc;
};

// Utility functions
FMUS_AUTO_API std::string kwp2000ServiceToString(KWP2000Service service);
FMUS_AUTO_API std::string kwp2000NRCToString(KWP2000NRC nrc);
FMUS_AUTO_API bool isValidKWP2000Service(uint8_t serviceId);
FMUS_AUTO_API uint8_t calculateKWP2000Checksum(const std::vector<uint8_t>& data);

} // namespace protocols
} // namespace fmus

#endif // FMUS_PROTOCOLS_KWP2000_H
