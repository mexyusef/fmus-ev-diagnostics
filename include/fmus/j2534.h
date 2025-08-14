#ifndef FMUS_J2534_H
#define FMUS_J2534_H

/**
 * @file j2534.h
 * @brief Modern C++ wrapper for the J2534 PassThru API
 */

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <optional>
#include <variant>
#include <map>
#include <stdexcept>
#include <unordered_map>
#include <cstdint>

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
namespace j2534 {

// Forward declarations
class Device;

/**
 * @brief J2534 Protocol identifiers
 */
enum class Protocol : uint32_t {
    J1850VPW = 1,
    J1850PWM = 2,
    ISO9141 = 3,
    ISO14230_4 = 4,
    CAN = 5,
    ISO15765 = 6,
    SCI_A_ENGINE = 7,
    SCI_A_TRANS = 8,
    SCI_B_ENGINE = 9,
    SCI_B_TRANS = 10
};

/**
 * @brief Baud rate options
 */
enum class BaudRate : uint32_t {
    AUTO = 0,
    BAUD_4800 = 4800,
    BAUD_9600 = 9600,
    BAUD_10400 = 10400,
    BAUD_19200 = 19200,
    BAUD_38400 = 38400,
    BAUD_57600 = 57600,
    BAUD_115200 = 115200,
    BAUD_125000 = 125000,
    BAUD_250000 = 250000,
    BAUD_500000 = 500000,
    BAUD_1000000 = 1000000
};

/**
 * @brief Filter types
 */
enum class FilterType : uint32_t {
    PASS_FILTER = 1,
    BLOCK_FILTER = 2,
    FLOW_CONTROL_FILTER = 3
};

/**
 * @brief J2534 Message structure
 */
struct Message {
    Protocol protocol = Protocol::CAN;
    uint32_t id = 0;
    std::vector<uint8_t> data;
    uint32_t flags = 0;
    uint32_t timestamp = 0;
    
    Message() = default;
    Message(Protocol proto, uint32_t msg_id, const std::vector<uint8_t>& msg_data)
        : protocol(proto), id(msg_id), data(msg_data) {}
    
    std::string toString() const;
    std::string toHexString() const;
    static Message fromHexString(const std::string& hexStr, Protocol proto = Protocol::CAN);
};

/**
 * @brief Message builder for fluent interface
 */
class MessageBuilder {
public:
    MessageBuilder() = default;
    
    MessageBuilder& protocol(Protocol proto) { mProtocol = proto; return *this; }
    MessageBuilder& id(uint32_t msg_id) { mId = msg_id; return *this; }
    MessageBuilder& data(const std::vector<uint8_t>& msg_data) { mData = msg_data; return *this; }
    MessageBuilder& flags(uint32_t msg_flags) { mFlags = msg_flags; return *this; }
    
    Message build() {
        Message msg;
        msg.protocol = mProtocol;
        msg.id = mId;
        msg.data = mData;
        msg.flags = mFlags;
        return msg;
    }

private:
    Protocol mProtocol = Protocol::CAN;
    uint32_t mId = 0;
    std::vector<uint8_t> mData;
    uint32_t mFlags = 0;
};

/**
 * @brief J2534 Filter structure
 */
struct Filter {
    Protocol protocol = Protocol::CAN;
    FilterType filterType = FilterType::PASS_FILTER;
    uint32_t maskId = 0;
    uint32_t patternId = 0;
    std::vector<uint8_t> maskData;
    std::vector<uint8_t> patternData;
    std::vector<uint8_t> flowControlData;
    uint32_t flags = 0;
    
    Filter() = default;
    Filter(Protocol proto, FilterType type, uint32_t mask, uint32_t pattern)
        : protocol(proto), filterType(type), maskId(mask), patternId(pattern) {}
    
    std::string toString() const;
};

/**
 * @brief Filter builder for fluent interface
 */
class FilterBuilder {
public:
    FilterBuilder() = default;
    
    FilterBuilder& protocol(Protocol proto) { mProtocol = proto; return *this; }
    FilterBuilder& filterType(FilterType type) { mFilterType = type; return *this; }
    FilterBuilder& maskId(uint32_t mask) { mMaskId = mask; return *this; }
    FilterBuilder& patternId(uint32_t pattern) { mPatternId = pattern; return *this; }
    FilterBuilder& maskData(const std::vector<uint8_t>& mask) { mMaskData = mask; return *this; }
    FilterBuilder& patternData(const std::vector<uint8_t>& pattern) { mPatternData = pattern; return *this; }
    FilterBuilder& flowControlData(const std::vector<uint8_t>& fc) { mFlowControlData = fc; return *this; }
    FilterBuilder& flags(uint32_t filter_flags) { mFlags = filter_flags; return *this; }
    
    Filter build() {
        Filter filter;
        filter.protocol = mProtocol;
        filter.filterType = mFilterType;
        filter.maskId = mMaskId;
        filter.patternId = mPatternId;
        filter.maskData = mMaskData;
        filter.patternData = mPatternData;
        filter.flowControlData = mFlowControlData;
        filter.flags = mFlags;
        return filter;
    }

private:
    Protocol mProtocol = Protocol::CAN;
    FilterType mFilterType = FilterType::PASS_FILTER;
    uint32_t mMaskId = 0;
    uint32_t mPatternId = 0;
    std::vector<uint8_t> mMaskData;
    std::vector<uint8_t> mPatternData;
    std::vector<uint8_t> mFlowControlData;
    uint32_t mFlags = 0;
};

/**
 * @brief Adapter information
 */
struct AdapterInfo {
    std::string vendorName;
    std::string deviceName;
    std::string libraryPath;
    uint32_t deviceId = 0;
    std::vector<Protocol> supportedProtocols;
    bool connected = false;
    
    AdapterInfo() = default;
    AdapterInfo(const std::string& vendor, const std::string& device, const std::string& path)
        : vendorName(vendor), deviceName(device), libraryPath(path) {}
    
    std::string toString() const;
    bool supportsProtocol(Protocol protocol) const;
};

/**
 * @brief Connection options
 */
struct ConnectionOptions {
    std::string vendorName;
    uint32_t deviceId = 0;
    Protocol protocol = Protocol::CAN;
    BaudRate baudRate = BaudRate::AUTO;
    uint32_t flags = 0;
    
    ConnectionOptions() = default;
    ConnectionOptions(const std::string& vendor, uint32_t id, Protocol proto, BaudRate baud)
        : vendorName(vendor), deviceId(id), protocol(proto), baudRate(baud) {}
    
    std::string toString() const;
};

/**
 * @brief Channel configuration
 */
struct ChannelConfig {
    Protocol protocol = Protocol::CAN;
    BaudRate baudRate = BaudRate::AUTO;
    uint32_t flags = 0;
    std::map<std::string, uint32_t> parameters;
    
    ChannelConfig() = default;
    ChannelConfig(Protocol proto, BaudRate baud) : protocol(proto), baudRate(baud) {}
    
    std::string toString() const;
};

/**
 * @brief J2534 Error codes
 */
enum class ErrorCode : int32_t {
    STATUS_NOERROR = 0x00,
    ERR_NOT_SUPPORTED = 0x01,
    ERR_INVALID_CHANNEL_ID = 0x02,
    ERR_INVALID_PROTOCOL_ID = 0x03,
    ERR_NULL_PARAMETER = 0x04,
    ERR_INVALID_IOCTL_VALUE = 0x05,
    ERR_INVALID_FLAGS = 0x06,
    ERR_FAILED = 0x07,
    ERR_DEVICE_NOT_CONNECTED = 0x08,
    ERR_TIMEOUT = 0x09,
    ERR_INVALID_MSG = 0x0A,
    ERR_INVALID_TIME_INTERVAL = 0x0B,
    ERR_EXCEEDED_LIMIT = 0x0C,
    ERR_INVALID_MSG_ID = 0x0D,
    ERR_DEVICE_IN_USE = 0x0E,
    ERR_INVALID_IOCTL_ID = 0x0F,
    ERR_BUFFER_EMPTY = 0x10,
    ERR_BUFFER_FULL = 0x11,
    ERR_BUFFER_OVERFLOW = 0x12,
    ERR_PIN_INVALID = 0x13,
    ERR_CHANNEL_IN_USE = 0x14,
    ERR_MSG_PROTOCOL_ID = 0x15,
    ERR_INVALID_FILTER_ID = 0x16,
    ERR_NO_FLOW_CONTROL = 0x17,
    ERR_NOT_UNIQUE = 0x18,
    ERR_INVALID_BAUDRATE = 0x19,
    ERR_INVALID_DEVICE_ID = 0x1A
};

/**
 * @brief J2534 Exception class
 */
class FMUS_AUTO_API J2534Error : public std::runtime_error {
public:
    J2534Error(ErrorCode code, const std::string& message);
    J2534Error(int code, const std::string& message);
    
    ErrorCode getErrorCode() const { return errorCode; }
    int getRawErrorCode() const { return static_cast<int>(errorCode); }

private:
    ErrorCode errorCode;
};

// Utility functions
FMUS_AUTO_API std::string protocolToString(Protocol protocol);
FMUS_AUTO_API Protocol stringToProtocol(const std::string& str);
FMUS_AUTO_API std::string errorCodeToString(ErrorCode code);
FMUS_AUTO_API std::string formatErrorMessage(ErrorCode code, const std::string& operation);

// Device management functions
FMUS_AUTO_API std::vector<AdapterInfo> discoverAdapters();
FMUS_AUTO_API bool connectToDevice(const AdapterInfo& adapter);
FMUS_AUTO_API void disconnectFromDevice();
FMUS_AUTO_API bool sendMessage(const Message& message);
FMUS_AUTO_API std::vector<Message> receiveMessages(uint32_t timeout = 1000);

} // namespace j2534
} // namespace fmus

#endif // FMUS_J2534_H
