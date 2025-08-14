#ifndef FMUS_DIAGNOSTICS_UDS_H
#define FMUS_DIAGNOSTICS_UDS_H

/**
 * @file uds.h
 * @brief UDS (Unified Diagnostic Services) implementation
 */

#include <fmus/protocols/can.h>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <map>

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
namespace diagnostics {

/**
 * @brief UDS Service Identifiers
 */
enum class UDSService : uint8_t {
    DIAGNOSTIC_SESSION_CONTROL = 0x10,
    ECU_RESET = 0x11,
    SECURITY_ACCESS = 0x27,
    COMMUNICATION_CONTROL = 0x28,
    TESTER_PRESENT = 0x3E,
    ACCESS_TIMING_PARAMETER = 0x83,
    SECURED_DATA_TRANSMISSION = 0x84,
    CONTROL_DTC_SETTING = 0x85,
    RESPONSE_ON_EVENT = 0x86,
    LINK_CONTROL = 0x87,
    READ_DATA_BY_IDENTIFIER = 0x22,
    READ_MEMORY_BY_ADDRESS = 0x23,
    READ_SCALING_DATA_BY_IDENTIFIER = 0x24,
    READ_DATA_BY_PERIODIC_IDENTIFIER = 0x2A,
    DYNAMICALLY_DEFINE_DATA_IDENTIFIER = 0x2C,
    WRITE_DATA_BY_IDENTIFIER = 0x2E,
    WRITE_MEMORY_BY_ADDRESS = 0x3D,
    CLEAR_DIAGNOSTIC_INFORMATION = 0x14,
    READ_DTC_INFORMATION = 0x19,
    INPUT_OUTPUT_CONTROL_BY_IDENTIFIER = 0x2F,
    ROUTINE_CONTROL = 0x31,
    REQUEST_DOWNLOAD = 0x34,
    REQUEST_UPLOAD = 0x35,
    TRANSFER_DATA = 0x36,
    REQUEST_TRANSFER_EXIT = 0x37
};

/**
 * @brief UDS Session Types
 */
enum class UDSSession : uint8_t {
    DEFAULT = 0x01,
    PROGRAMMING = 0x02,
    EXTENDED_DIAGNOSTIC = 0x03,
    SAFETY_SYSTEM_DIAGNOSTIC = 0x04
};

/**
 * @brief UDS Negative Response Codes
 */
enum class UDSNegativeResponse : uint8_t {
    GENERAL_REJECT = 0x10,
    SERVICE_NOT_SUPPORTED = 0x11,
    SUB_FUNCTION_NOT_SUPPORTED = 0x12,
    INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT = 0x13,
    RESPONSE_TOO_LONG = 0x14,
    BUSY_REPEAT_REQUEST = 0x21,
    CONDITIONS_NOT_CORRECT = 0x22,
    REQUEST_SEQUENCE_ERROR = 0x24,
    NO_RESPONSE_FROM_SUBNET_COMPONENT = 0x25,
    FAILURE_PREVENTS_EXECUTION_OF_REQUESTED_ACTION = 0x26,
    REQUEST_OUT_OF_RANGE = 0x31,
    SECURITY_ACCESS_DENIED = 0x33,
    INVALID_KEY = 0x35,
    EXCEED_NUMBER_OF_ATTEMPTS = 0x36,
    REQUIRED_TIME_DELAY_NOT_EXPIRED = 0x37,
    UPLOAD_DOWNLOAD_NOT_ACCEPTED = 0x70,
    TRANSFER_DATA_SUSPENDED = 0x71,
    GENERAL_PROGRAMMING_FAILURE = 0x72,
    WRONG_BLOCK_SEQUENCE_COUNTER = 0x73,
    REQUEST_CORRECTLY_RECEIVED_RESPONSE_PENDING = 0x78,
    SUB_FUNCTION_NOT_SUPPORTED_IN_ACTIVE_SESSION = 0x7E,
    SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION = 0x7F
};

/**
 * @brief UDS Request/Response structure
 */
struct UDSMessage {
    UDSService service;
    std::vector<uint8_t> data;
    bool isResponse = false;
    bool isNegativeResponse = false;
    UDSNegativeResponse negativeResponseCode = UDSNegativeResponse::GENERAL_REJECT;
    std::chrono::system_clock::time_point timestamp;
    
    UDSMessage() = default;
    UDSMessage(UDSService svc, const std::vector<uint8_t>& payload = {})
        : service(svc), data(payload), timestamp(std::chrono::system_clock::now()) {}
    
    /**
     * @brief Convert to CAN message
     */
    protocols::CANMessage toCANMessage(uint32_t requestId) const;
    
    /**
     * @brief Create from CAN message
     */
    static UDSMessage fromCANMessage(const protocols::CANMessage& canMsg);
    
    /**
     * @brief Get string representation
     */
    std::string toString() const;
    
    /**
     * @brief Check if this is a valid UDS message
     */
    bool isValid() const;
};

/**
 * @brief UDS Configuration
 */
struct UDSConfig {
    uint32_t requestId = 0x7E0;         ///< Request CAN ID
    uint32_t responseId = 0x7E8;        ///< Response CAN ID
    uint32_t timeout = 1000;            ///< Response timeout in ms
    uint32_t p2ClientMax = 50;          ///< P2*Client timing in ms
    uint32_t p2StarClientMax = 5000;    ///< P2*Client timing in ms
    bool extendedAddressing = false;    ///< Use extended addressing
    uint8_t sourceAddress = 0xF1;       ///< Source address for extended addressing
    uint8_t targetAddress = 0x10;       ///< Target address for extended addressing
    
    std::string toString() const;
};

/**
 * @brief UDS Client for diagnostic communication
 */
class FMUS_AUTO_API UDSClient {
public:
    /**
     * @brief Constructor
     */
    UDSClient();
    
    /**
     * @brief Destructor
     */
    ~UDSClient();
    
    /**
     * @brief Initialize UDS client
     */
    bool initialize(const UDSConfig& config, std::shared_ptr<protocols::CANProtocol> canProtocol);
    
    /**
     * @brief Shutdown UDS client
     */
    void shutdown();
    
    /**
     * @brief Check if client is initialized
     */
    bool isInitialized() const;
    
    /**
     * @brief Send UDS request and wait for response
     */
    UDSMessage sendRequest(const UDSMessage& request);
    
    /**
     * @brief Send UDS request asynchronously
     */
    void sendRequestAsync(const UDSMessage& request, 
                         std::function<void(const UDSMessage&)> callback);
    
    // Diagnostic Session Control (0x10)
    bool startDiagnosticSession(UDSSession session);
    UDSSession getCurrentSession() const;
    
    // ECU Reset (0x11)
    bool resetECU(uint8_t resetType = 0x01);
    
    // Security Access (0x27)
    std::vector<uint8_t> requestSeed(uint8_t level);
    bool sendKey(uint8_t level, const std::vector<uint8_t>& key);
    bool unlockSecurityAccess(uint8_t level, const std::vector<uint8_t>& key);
    
    // Tester Present (0x3E)
    bool sendTesterPresent(bool suppressResponse = false);
    
    // Read Data by Identifier (0x22)
    std::vector<uint8_t> readDataByIdentifier(uint16_t dataIdentifier);
    std::map<uint16_t, std::vector<uint8_t>> readMultipleDataByIdentifier(
        const std::vector<uint16_t>& identifiers);
    
    // Write Data by Identifier (0x2E)
    bool writeDataByIdentifier(uint16_t dataIdentifier, const std::vector<uint8_t>& data);
    
    // Clear Diagnostic Information (0x14)
    bool clearDiagnosticInformation(uint32_t groupOfDTC = 0xFFFFFF);
    
    // Read DTC Information (0x19)
    struct DTCInfo {
        uint32_t dtcNumber;
        uint8_t statusMask;
        std::vector<uint8_t> snapshotData;
        std::vector<uint8_t> extendedData;
    };
    
    std::vector<DTCInfo> readDTCInformation(uint8_t subFunction, uint8_t statusMask = 0xFF);
    std::vector<DTCInfo> readStoredDTCs();
    std::vector<DTCInfo> readPendingDTCs();
    std::vector<DTCInfo> readConfirmedDTCs();
    
    // Routine Control (0x31)
    enum class RoutineControlType : uint8_t {
        START = 0x01,
        STOP = 0x02,
        REQUEST_RESULTS = 0x03
    };
    
    std::vector<uint8_t> routineControl(RoutineControlType controlType, 
                                       uint16_t routineIdentifier,
                                       const std::vector<uint8_t>& parameters = {});
    
    // Input/Output Control (0x2F)
    bool inputOutputControl(uint16_t dataIdentifier, uint8_t controlParameter,
                           const std::vector<uint8_t>& controlState = {});
    
    /**
     * @brief Get last error information
     */
    struct ErrorInfo {
        bool hasError = false;
        UDSNegativeResponse errorCode = UDSNegativeResponse::GENERAL_REJECT;
        std::string description;
        std::chrono::system_clock::time_point timestamp;
    };
    
    ErrorInfo getLastError() const;
    
    /**
     * @brief Get client statistics
     */
    struct Statistics {
        uint64_t requestsSent = 0;
        uint64_t responsesReceived = 0;
        uint64_t negativeResponses = 0;
        uint64_t timeouts = 0;
        std::chrono::system_clock::time_point startTime;
    };
    
    Statistics getStatistics() const;
    
    /**
     * @brief Reset statistics
     */
    void resetStatistics();
    
    /**
     * @brief Get current configuration
     */
    UDSConfig getConfiguration() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

// Utility functions
FMUS_AUTO_API std::string udsServiceToString(UDSService service);
FMUS_AUTO_API std::string udsSessionToString(UDSSession session);
FMUS_AUTO_API std::string udsNegativeResponseToString(UDSNegativeResponse nrc);
FMUS_AUTO_API bool isValidDataIdentifier(uint16_t did);
FMUS_AUTO_API std::vector<uint8_t> encodeDataIdentifier(uint16_t did);
FMUS_AUTO_API uint16_t decodeDataIdentifier(const std::vector<uint8_t>& data, size_t offset = 0);

} // namespace diagnostics
} // namespace fmus

#endif // FMUS_DIAGNOSTICS_UDS_H
