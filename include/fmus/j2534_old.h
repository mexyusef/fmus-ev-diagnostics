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
class MessageBuilder;
class FilterBuilder;

/**
 * @brief J2534 API version
 */
enum class APIVersion {
    V02_02,  ///< J2534-1 v2.02
    V04_04,  ///< J2534-1 v4.04
    V05_00   ///< J2534-2 v5.00
};

/**
 * @brief J2534 protocol identifier
 */
enum class Protocol {
    J1850VPW = 0x01,      ///< J1850 VPW (10.4kbps)
    J1850PWM = 0x02,      ///< J1850 PWM (41.6kbps)
    ISO9141 = 0x03,       ///< ISO 9141 (5 baud init, 10.4kbps)
    ISO14230 = 0x04,      ///< ISO 14230 (KWP2000, 5 baud init)
    CAN = 0x05,           ///< CAN (ISO 11898, 11/29-bit, 125-500kbps)
    ISO15765 = 0x06,      ///< ISO 15765 (CAN with transport layer)
    SCI_A_ENGINE = 0x07,  ///< SCI A (Chrysler)
    SCI_A_TRANS = 0x08,   ///< SCI A (Chrysler)
    SCI_B_ENGINE = 0x09,  ///< SCI B (Chrysler)
    SCI_B_TRANS = 0x0A,   ///< SCI B (Chrysler)

    // J2534-2 additions
    J1850VPW_PS = 0x41,   ///< J1850VPW with checksum
    J1850PWM_PS = 0x42,   ///< J1850PWM with checksum
    ISO9141_PS = 0x43,    ///< ISO9141 with checksum
    ISO14230_PS = 0x44,   ///< ISO14230 with checksum
    ISO15765_PS = 0x46,   ///< ISO15765 with checksum
    J2610_PS = 0x48,      ///< J2610 (Intellibus) with checksum
    SW_CAN_PS = 0x49,     ///< SW_CAN with checksum
    SW_ISO15765_PS = 0x4A,///< SW_ISO15765 with checksum
    GM_UART_PS = 0x4B,    ///< GM_UART with checksum
    UART_ECHO_BYTE_PS = 0x4C, ///< UART with echo byte with checksum
    HONDA_DIAG_PS = 0x4D, ///< Honda diagnostic with checksum

    // Vendor-specific additions
    CAN_XON_XOFF = 0x80,  ///< CAN with XON/XOFF flow control
    CAN_CH1 = 0x81,       ///< CAN on channel 1
    CAN_CH2 = 0x82        ///< CAN on channel 2
};

/**
 * @brief J2534 baud rate flags
 */
enum class BaudRate : unsigned long {
    AUTO = 0,          ///< Automatic baud rate detection
    STANDARD_9600 = 9600,
    STANDARD_14400 = 14400,
    STANDARD_19200 = 19200,
    STANDARD_38400 = 38400,
    STANDARD_57600 = 57600,
    STANDARD_115200 = 115200,

    CAN_125000 = 125000,    ///< CAN 125 kbps
    CAN_250000 = 250000,    ///< CAN 250 kbps
    CAN_500000 = 500000,    ///< CAN 500 kbps
    CAN_1000000 = 1000000   ///< CAN 1 Mbps
};

/**
 * @brief J2534 message flags
 */
enum class MessageFlag : unsigned long {
    NO_FLAGS = 0x0000,
    // Direction flags
    TX_MSG = 0x0001,      ///< Transmit message (from PC to vehicle)
    RX_MSG = 0x0002,      ///< Receive message (from vehicle to PC)

    // Message type flags
    ISO15765_EXTENDED_ADDR = 0x0004,  ///< ISO15765 extended addressing
    ISO15765_FRAME_PAD = 0x0008,      ///< ISO15765 pad frame to full size
    TX_INDICATION = 0x0010,           ///< Transmit indication (echo tx msg)
    WAIT_P3_MIN_ONLY = 0x0020,        ///< Wait P3 minimum only (ISO14230)
    CAN_29BIT_ID = 0x0100,            ///< Use 29-bit CAN ID
    CAN_REMOTE_FRAME = 0x0200,        ///< CAN remote frame
    ISO15765_ADDR_TYPE = 0x0400,      ///< ISO15765 address type (0 = no ex, 1 = ex)

    // Filter type flags
    PASS_FILTER = 0x1000000,          ///< Pass messages matching pattern
    BLOCK_FILTER = 0x2000000,         ///< Block messages matching pattern
    FLOW_CONTROL_FILTER = 0x3000000   ///< Flow control filter for ISO15765
};

/**
 * @brief J2534 message structure
 *
 * Represents a message sent or received via J2534
 */
struct Message {
    Protocol protocol;                     ///< Protocol ID
    uint32_t flags;                        ///< Message flags
    uint32_t timestamp;                    ///< Timestamp (in ms)
    uint32_t id;                           ///< Message ID (arbitration ID for CAN)
    std::vector<uint8_t> data;             ///< Message data bytes
    std::optional<uint8_t> extraDataIndex; ///< Extra data index (for ISO15765)

    /**
     * @brief Create a new message
     *
     * @return Message builder object
     */
    static MessageBuilder create();
};

/**
 * @brief Builder class for constructing J2534 messages
 */
class MessageBuilder {
public:
    /**
     * @brief Set the protocol
     *
     * @param protocol Protocol to use
     * @return Reference to this builder
     */
    MessageBuilder& protocol(Protocol protocol);

    /**
     * @brief Set the message ID
     *
     * @param id Message ID (arbitration ID for CAN)
     * @return Reference to this builder
     */
    MessageBuilder& id(uint32_t id);

    /**
     * @brief Set message data
     *
     * @param data Vector of data bytes
     * @return Reference to this builder
     */
    MessageBuilder& data(const std::vector<uint8_t>& data);

    /**
     * @brief Set message flags
     *
     * @param flags Message flags
     * @return Reference to this builder
     */
    MessageBuilder& flags(uint32_t flags);

    /**
     * @brief Use 29-bit CAN identifier
     *
     * @param use29Bit Whether to use 29-bit ID
     * @return Reference to this builder
     */
    MessageBuilder& use29BitId(bool use29Bit = true);

    /**
     * @brief Set as transmit message
     *
     * @return Reference to this builder
     */
    MessageBuilder& transmit();

    /**
     * @brief Set as receive message
     *
     * @return Reference to this builder
     */
    MessageBuilder& receive();

    /**
     * @brief Build the message
     *
     * @return Constructed Message object
     */
    Message build();

private:
    Message msg;
};

/**
 * @brief J2534 filter structure
 *
 * Represents a message filter for J2534
 */
struct Filter {
    Protocol protocol;       ///< Protocol ID
    uint32_t flags;          ///< Filter flags
    uint32_t maskId;         ///< Mask for ID filtering
    uint32_t patternId;      ///< Pattern for ID filtering
    std::vector<uint8_t> maskData;    ///< Mask for data filtering
    std::vector<uint8_t> patternData; ///< Pattern for data filtering

    /**
     * @brief Create a new filter
     *
     * @return Filter builder object
     */
    static FilterBuilder create();
};

/**
 * @brief Builder class for constructing J2534 filters
 */
class FilterBuilder {
public:
    /**
     * @brief Set the protocol
     *
     * @param protocol Protocol to use
     * @return Reference to this builder
     */
    FilterBuilder& protocol(Protocol protocol);

    /**
     * @brief Set as pass filter
     *
     * @return Reference to this builder
     */
    FilterBuilder& passFilter();

    /**
     * @brief Set as block filter
     *
     * @return Reference to this builder
     */
    FilterBuilder& blockFilter();

    /**
     * @brief Set as flow control filter
     *
     * @return Reference to this builder
     */
    FilterBuilder& flowControlFilter();

    /**
     * @brief Set the ID mask
     *
     * @param mask Mask to apply to message IDs
     * @return Reference to this builder
     */
    FilterBuilder& maskId(uint32_t mask);

    /**
     * @brief Set the ID pattern
     *
     * @param pattern Pattern to match against message IDs
     * @return Reference to this builder
     */
    FilterBuilder& patternId(uint32_t pattern);

    /**
     * @brief Set the data mask
     *
     * @param mask Mask to apply to message data
     * @return Reference to this builder
     */
    FilterBuilder& maskData(const std::vector<uint8_t>& mask);

    /**
     * @brief Set the data pattern
     *
     * @param pattern Pattern to match against message data
     * @return Reference to this builder
     */
    FilterBuilder& patternData(const std::vector<uint8_t>& pattern);

    /**
     * @brief Build the filter
     *
     * @return Constructed Filter object
     */
    Filter build();

private:
    Filter filter;
};

/**
 * @brief Adapter information structure
 *
 * Contains information about a J2534 adapter
 */
struct AdapterInfo {
    std::string name;            ///< Adapter name
    std::string vendor;          ///< Vendor name
    std::string version;         ///< Firmware version
    std::string dllPath;         ///< Path to the PassThru DLL
    std::string description;     ///< Description
    APIVersion apiVersion;       ///< Supported API version
    std::vector<Protocol> supportedProtocols; ///< Supported protocols
    bool isConnected;            ///< Connection status

    /**
     * @brief Check if the adapter supports a specific protocol
     *
     * @param protocol Protocol to check
     * @return true if supported, false otherwise
     */
    bool supportsProtocol(Protocol protocol) const;
};

/**
 * @brief Connection options for J2534 devices
 */
struct ConnectionOptions {
    std::optional<std::string> adapterName;        ///< Specific adapter name to connect to
    std::optional<std::string> vendorName;         ///< Specific vendor to use
    std::optional<Protocol> requiredProtocol;      ///< Protocol that must be supported
    bool autoDetect = true;                        ///< Whether to auto-detect adapters
    std::chrono::milliseconds connectTimeout = std::chrono::milliseconds(5000); ///< Connection timeout

    /**
     * @brief Create default connection options
     *
     * @return Default ConnectionOptions
     */
    static ConnectionOptions defaultOptions();

    /**
     * @brief Require a specific adapter by name
     *
     * @param name Adapter name
     * @return Updated ConnectionOptions
     */
    ConnectionOptions& requireAdapter(const std::string& name);

    /**
     * @brief Require a specific vendor
     *
     * @param vendor Vendor name
     * @return Updated ConnectionOptions
     */
    ConnectionOptions& requireVendor(const std::string& vendor);

    /**
     * @brief Require support for a specific protocol
     *
     * @param protocol Protocol that must be supported
     * @return Updated ConnectionOptions
     */
    ConnectionOptions& requireProtocol(Protocol protocol);

    /**
     * @brief Set connection timeout
     *
     * @param timeout Timeout in milliseconds
     * @return Updated ConnectionOptions
     */
    ConnectionOptions& setTimeout(std::chrono::milliseconds timeout);
};

/**
 * @brief Channel configuration options
 */
struct ChannelConfig {
    Protocol protocol;                       ///< Protocol to use
    BaudRate baudRate = BaudRate::AUTO;      ///< Baud rate
    std::map<std::string, uint32_t> params;  ///< Additional parameters

    /**
     * @brief Create a new channel configuration
     *
     * @param protocol Protocol to use
     * @return ChannelConfig object
     */
    static ChannelConfig create(Protocol protocol);

    /**
     * @brief Set the baud rate
     *
     * @param rate Baud rate
     * @return Reference to this object
     */
    ChannelConfig& baudRate(BaudRate rate);

    /**
     * @brief Set a configuration parameter
     *
     * @param name Parameter name
     * @param value Parameter value
     * @return Reference to this object
     */
    ChannelConfig& parameter(const std::string& name, uint32_t value);
};

/**
 * @brief Exception class for J2534 device errors
 */
class FMUS_AUTO_API DeviceError : public std::runtime_error {
public:
    /**
     * @brief Create a new device error
     *
     * @param message Error message
     * @param errorCode Raw J2534 error code
     */
    DeviceError(const std::string& message, int errorCode);

    /**
     * @brief Get the error code
     *
     * @return Raw J2534 error code
     */
    int getErrorCode() const;

private:
    int errorCode;
};

/**
 * @brief Formats a J2534 error message
 *
 * @param errorCode The J2534 error code
 * @param operation The operation that caused the error
 * @return std::string Formatted error message
 */
FMUS_AUTO_API std::string formatErrorMessage(int errorCode, const std::string& operation);

/**
 * @brief Information about a discovered J2534 adapter
 */
class FMUS_AUTO_API AdapterInfo {
public:
    /**
     * @brief Construct a new Adapter Info object with default values
     */
    AdapterInfo();

    /**
     * @brief Copy constructor
     */
    AdapterInfo(const AdapterInfo& other);

    /**
     * @brief Move constructor
     */
    AdapterInfo(AdapterInfo&& other) noexcept;

    /**
     * @brief Copy assignment operator
     */
    AdapterInfo& operator=(const AdapterInfo& other);

    /**
     * @brief Move assignment operator
     */
    AdapterInfo& operator=(AdapterInfo&& other) noexcept;

    /**
     * @brief Get a string representation of this adapter
     *
     * @return std::string Adapter info as string
     */
    std::string toString() const;

    /**
     * @brief Get the vendor name
     *
     * @return const std::string& Vendor name
     */
    const std::string& getVendorName() const;

    /**
     * @brief Get the device name
     *
     * @return const std::string& Device name
     */
    const std::string& getDeviceName() const;

    /**
     * @brief Get the library path
     *
     * @return const std::string& Library path
     */
    const std::string& getLibraryPath() const;

    /**
     * @brief Get the device ID
     *
     * @return unsigned long Device ID
     */
    unsigned long getDeviceId() const;

    /**
     * @brief Check if this adapter is connected
     *
     * @return true If connected
     * @return false If not connected
     */
    bool isConnected() const;

    /**
     * @brief Set the vendor name
     *
     * @param name New vendor name
     */
    void setVendorName(const std::string& name);

    /**
     * @brief Set the device name
     *
     * @param name New device name
     */
    void setDeviceName(const std::string& name);

    /**
     * @brief Set the library path
     *
     * @param path New library path
     */
    void setLibraryPath(const std::string& path);

    /**
     * @brief Set the device ID
     *
     * @param id New device ID
     */
    void setDeviceId(unsigned long id);

    /**
     * @brief Set the connected state
     *
     * @param isConnected New connected state
     */
    void setConnected(bool isConnected);

    // Member variables
    std::string vendorName;
    std::string deviceName;
    std::string libraryPath;

private:
    unsigned long deviceId;
    bool connected;
};

/**
 * @brief A J2534 message
 */
class FMUS_AUTO_API Message {
public:
    friend class MessageBuilder;

    /**
     * @brief Default constructor
     */
    Message();

    /**
     * @brief Copy constructor
     */
    Message(const Message& other);

    /**
     * @brief Move constructor
     */
    Message(Message&& other) noexcept;

    /**
     * @brief Copy assignment operator
     */
    Message& operator=(const Message& other);

    /**
     * @brief Move assignment operator
     */
    Message& operator=(Message&& other) noexcept;

    /**
     * @brief Get the protocol ID
     *
     * @return unsigned long Protocol ID
     */
    unsigned long getProtocol() const;

    /**
     * @brief Get the message ID
     *
     * @return unsigned long Message ID
     */
    unsigned long getId() const;

    /**
     * @brief Get the message data
     *
     * @return const std::vector<uint8_t>& Message data
     */
    const std::vector<uint8_t>& getData() const;

    /**
     * @brief Get the message flags
     *
     * @return unsigned long Message flags
     */
    unsigned long getFlags() const;

    /**
     * @brief Get the message timestamp
     *
     * @return unsigned long Message timestamp
     */
    unsigned long getTimestamp() const;

    /**
     * @brief Get the size of the data
     *
     * @return size_t Data size in bytes
     */
    size_t getDataSize() const;

    /**
     * @brief Get a string representation of this message
     *
     * @return std::string Message as string
     */
    std::string toString() const;

    /**
     * @brief Get a hex string representation of this message
     *
     * @return std::string Message as hex string (ID#DATA format)
     */
    std::string toHexString() const;

    /**
     * @brief Create a Message object from a hex string
     *
     * @param hexStr Hex string in ID#DATA format
     * @param protocol Protocol ID
     * @return Message New message
     */
    static Message fromHexString(const std::string& hexStr, unsigned long protocol);

    /**
     * @brief Equality operator
     */
    bool operator==(const Message& other) const;

    /**
     * @brief Inequality operator
     */
    bool operator!=(const Message& other) const;

private:
    /**
     * @brief Constructor from builder
     */
    explicit Message(const MessageBuilder& builder);

    // Member variables
    unsigned long protocol;
    unsigned long id;
    std::vector<uint8_t> data;
    unsigned long flags;
    unsigned long timestamp;
};

/**
 * @brief Builder for Message objects
 */
class FMUS_AUTO_API MessageBuilder {
public:
    friend class Message;

    /**
     * @brief Construct a new Message Builder object
     */
    MessageBuilder();

    /**
     * @brief Reset the builder to default values
     */
    void reset();

    /**
     * @brief Set the protocol ID
     *
     * @param protocol Protocol ID
     * @return MessageBuilder& This builder
     */
    MessageBuilder& protocol(unsigned long protocol);

    /**
     * @brief Set the message ID
     *
     * @param id Message ID
     * @return MessageBuilder& This builder
     */
    MessageBuilder& id(unsigned long id);

    /**
     * @brief Set the message data from a vector
     *
     * @param data Data vector
     * @return MessageBuilder& This builder
     */
    MessageBuilder& data(const std::vector<uint8_t>& data);

    /**
     * @brief Set the message data from a pointer and size
     *
     * @param data Data pointer
     * @param size Data size
     * @return MessageBuilder& This builder
     */
    MessageBuilder& data(const uint8_t* data, size_t size);

    /**
     * @brief Set the message flags
     *
     * @param flags Message flags
     * @return MessageBuilder& This builder
     */
    MessageBuilder& flags(unsigned long flags);

    /**
     * @brief Set the message timestamp
     *
     * @param timestamp Message timestamp
     * @return MessageBuilder& This builder
     */
    MessageBuilder& timestamp(unsigned long timestamp);

    /**
     * @brief Build the message
     *
     * @return Message The built message
     */
    Message build() const;

private:
    // Builder private variables
    unsigned long mProtocol;
    unsigned long mId;
    std::vector<uint8_t> mData;
    unsigned long mFlags;
    unsigned long mTimestamp;
};

/**
 * @brief A J2534 message filter
 */
class FMUS_AUTO_API Filter {
public:
    friend class FilterBuilder;

    /**
     * @brief Default constructor
     */
    Filter();

    /**
     * @brief Copy constructor
     */
    Filter(const Filter& other);

    /**
     * @brief Move constructor
     */
    Filter(Filter&& other) noexcept;

    /**
     * @brief Copy assignment operator
     */
    Filter& operator=(const Filter& other);

    /**
     * @brief Move assignment operator
     */
    Filter& operator=(Filter&& other) noexcept;

    /**
     * @brief Get the protocol ID
     *
     * @return unsigned long Protocol ID
     */
    unsigned long getProtocol() const;

    /**
     * @brief Get the filter type
     *
     * @return unsigned long Filter type
     */
    unsigned long getFilterType() const;

    /**
     * @brief Get the mask ID
     *
     * @return unsigned long Mask ID
     */
    unsigned long getMaskId() const;

    /**
     * @brief Get the pattern ID
     *
     * @return unsigned long Pattern ID
     */
    unsigned long getPatternId() const;

    /**
     * @brief Get the mask data
     *
     * @return const std::vector<uint8_t>& Mask data
     */
    const std::vector<uint8_t>& getMaskData() const;

    /**
     * @brief Get the pattern data
     *
     * @return const std::vector<uint8_t>& Pattern data
     */
    const std::vector<uint8_t>& getPatternData() const;

    /**
     * @brief Get the flow control data
     *
     * @return const std::vector<uint8_t>& Flow control data
     */
    const std::vector<uint8_t>& getFlowControlData() const;

    /**
     * @brief Get the filter flags
     *
     * @return unsigned long Filter flags
     */
    unsigned long getFlags() const;

    /**
     * @brief Get a string representation of this filter
     *
     * @return std::string Filter as string
     */
    std::string toString() const;

    /**
     * @brief Equality operator
     */
    bool operator==(const Filter& other) const;

    /**
     * @brief Inequality operator
     */
    bool operator!=(const Filter& other) const;

private:
    /**
     * @brief Constructor from builder
     */
    explicit Filter(const FilterBuilder& builder);

    // Member variables
    unsigned long protocol;
    unsigned long filterType;
    unsigned long maskId;
    unsigned long patternId;
    std::vector<uint8_t> maskData;
    std::vector<uint8_t> patternData;
    std::vector<uint8_t> flowControlData;
    unsigned long flags;
};

/**
 * @brief Builder for Filter objects
 */
class FMUS_AUTO_API FilterBuilder {
public:
    friend class Filter;

    /**
     * @brief Construct a new Filter Builder object
     */
    FilterBuilder();

    /**
     * @brief Reset the builder to default values
     */
    void reset();

    /**
     * @brief Set the protocol ID
     *
     * @param protocol Protocol ID
     * @return FilterBuilder& This builder
     */
    FilterBuilder& protocol(unsigned long protocol);

    /**
     * @brief Set the filter type
     *
     * @param type Filter type
     * @return FilterBuilder& This builder
     */
    FilterBuilder& filterType(unsigned long type);

    /**
     * @brief Set the mask ID
     *
     * @param id Mask ID
     * @return FilterBuilder& This builder
     */
    FilterBuilder& maskId(unsigned long id);

    /**
     * @brief Set the pattern ID
     *
     * @param id Pattern ID
     * @return FilterBuilder& This builder
     */
    FilterBuilder& patternId(unsigned long id);

    /**
     * @brief Set the mask data from a vector
     *
     * @param data Mask data
     * @return FilterBuilder& This builder
     */
    FilterBuilder& maskData(const std::vector<uint8_t>& data);

    /**
     * @brief Set the mask data from a pointer and size
     *
     * @param data Mask data pointer
     * @param size Mask data size
     * @return FilterBuilder& This builder
     */
    FilterBuilder& maskData(const uint8_t* data, size_t size);

    /**
     * @brief Set the pattern data from a vector
     *
     * @param data Pattern data
     * @return FilterBuilder& This builder
     */
    FilterBuilder& patternData(const std::vector<uint8_t>& data);

    /**
     * @brief Set the pattern data from a pointer and size
     *
     * @param data Pattern data pointer
     * @param size Pattern data size
     * @return FilterBuilder& This builder
     */
    FilterBuilder& patternData(const uint8_t* data, size_t size);

    /**
     * @brief Set the flow control data from a vector
     *
     * @param data Flow control data
     * @return FilterBuilder& This builder
     */
    FilterBuilder& flowControlData(const std::vector<uint8_t>& data);

    /**
     * @brief Set the flow control data from a pointer and size
     *
     * @param data Flow control data pointer
     * @param size Flow control data size
     * @return FilterBuilder& This builder
     */
    FilterBuilder& flowControlData(const uint8_t* data, size_t size);

    /**
     * @brief Set the filter flags
     *
     * @param flags Filter flags
     * @return FilterBuilder& This builder
     */
    FilterBuilder& flags(unsigned long flags);

    /**
     * @brief Build the filter
     *
     * @return Filter The built filter
     */
    Filter build() const;

private:
    // Builder private variables
    unsigned long mProtocol;
    unsigned long mFilterType;
    unsigned long mMaskId;
    unsigned long mPatternId;
    std::vector<uint8_t> mMaskData;
    std::vector<uint8_t> mPatternData;
    std::vector<uint8_t> mFlowControlData;
    unsigned long mFlags;
};

/**
 * @brief Connection options for J2534 devices
 */
class FMUS_AUTO_API ConnectionOptions {
public:
    /**
     * @brief Default constructor
     */
    ConnectionOptions();

    /**
     * @brief Constructor with parameters
     *
     * @param vendorName Vendor name
     * @param protocol Protocol ID
     * @param baudRate Baud rate
     * @param flags Connection flags
     * @param timeout Connection timeout in ms
     */
    ConnectionOptions(const std::string& vendorName,
                     unsigned long protocol,
                     unsigned long baudRate = 0,
                     unsigned long flags = 0,
                     unsigned long timeout = 5000);

    /**
     * @brief Copy constructor
     */
    ConnectionOptions(const ConnectionOptions& other);

    /**
     * @brief Move constructor
     */
    ConnectionOptions(ConnectionOptions&& other) noexcept;

    /**
     * @brief Copy assignment operator
     */
    ConnectionOptions& operator=(const ConnectionOptions& other);

    /**
     * @brief Move assignment operator
     */
    ConnectionOptions& operator=(ConnectionOptions&& other) noexcept;

    /**
     * @brief Get the vendor name
     *
     * @return const std::string& Vendor name
     */
    const std::string& getVendorName() const;

    /**
     * @brief Get the protocol ID
     *
     * @return unsigned long Protocol ID
     */
    unsigned long getProtocol() const;

    /**
     * @brief Get the baud rate
     *
     * @return unsigned long Baud rate
     */
    unsigned long getBaudRate() const;

    /**
     * @brief Get the connection flags
     *
     * @return unsigned long Connection flags
     */
    unsigned long getFlags() const;

    /**
     * @brief Get the timeout
     *
     * @return unsigned long Timeout in ms
     */
    unsigned long getTimeout() const;

    /**
     * @brief Set the vendor name
     *
     * @param name New vendor name
     */
    void setVendorName(const std::string& name);

    /**
     * @brief Set the protocol ID
     *
     * @param protocol New protocol ID
     */
    void setProtocol(unsigned long protocol);

    /**
     * @brief Set the baud rate
     *
     * @param rate New baud rate
     */
    void setBaudRate(unsigned long rate);

    /**
     * @brief Set the connection flags
     *
     * @param flags New connection flags
     */
    void setFlags(unsigned long flags);

    /**
     * @brief Set the timeout
     *
     * @param timeout New timeout in ms
     */
    void setTimeout(unsigned long timeout);

    /**
     * @brief Get a string representation of these options
     *
     * @return std::string Options as string
     */
    std::string toString() const;

    /**
     * @brief Equality operator
     */
    bool operator==(const ConnectionOptions& other) const;

    /**
     * @brief Inequality operator
     */
    bool operator!=(const ConnectionOptions& other) const;

private:
    // Member variables
    std::string vendorName;
    unsigned long protocol;
    unsigned long baudRate;
    unsigned long flags;
    unsigned long timeout;
};

/**
 * @brief Configuration for a J2534 channel
 */
class FMUS_AUTO_API ChannelConfig {
public:
    /**
     * @brief Default constructor
     */
    ChannelConfig();

    /**
     * @brief Constructor with parameters
     *
     * @param baudRate Baud rate
     * @param flags Channel flags
     */
    ChannelConfig(unsigned long baudRate, unsigned long flags = 0);

    /**
     * @brief Copy constructor
     */
    ChannelConfig(const ChannelConfig& other);

    /**
     * @brief Move constructor
     */
    ChannelConfig(ChannelConfig&& other) noexcept;

    /**
     * @brief Copy assignment operator
     */
    ChannelConfig& operator=(const ChannelConfig& other);

    /**
     * @brief Move assignment operator
     */
    ChannelConfig& operator=(ChannelConfig&& other) noexcept;

    /**
     * @brief Get the baud rate
     *
     * @return unsigned long Baud rate
     */
    unsigned long getBaudRate() const;

    /**
     * @brief Get the channel flags
     *
     * @return unsigned long Channel flags
     */
    unsigned long getFlags() const;

    /**
     * @brief Set the baud rate
     *
     * @param rate New baud rate
     */
    void setBaudRate(unsigned long rate);

    /**
     * @brief Set the channel flags
     *
     * @param flags New channel flags
     */
    void setFlags(unsigned long flags);

    /**
     * @brief Set a channel parameter
     *
     * @param parameter Parameter ID
     * @param value Parameter value
     */
    void setParameter(unsigned long parameter, unsigned long value);

    /**
     * @brief Get a channel parameter
     *
     * @param parameter Parameter ID
     * @return unsigned long Parameter value, or 0 if not set
     */
    unsigned long getParameter(unsigned long parameter) const;

    /**
     * @brief Check if a parameter is set
     *
     * @param parameter Parameter ID
     * @return true If the parameter is set
     * @return false If the parameter is not set
     */
    bool hasParameter(unsigned long parameter) const;

    /**
     * @brief Clear all parameters
     */
    void clearParameters();

    /**
     * @brief Get all parameters
     *
     * @return const std::unordered_map<unsigned long, unsigned long>& Parameters map
     */
    const std::unordered_map<unsigned long, unsigned long>& getParameters() const;

    /**
     * @brief Get a string representation of this configuration
     *
     * @return std::string Configuration as string
     */
    std::string toString() const;

    /**
     * @brief Create a configuration for a standard CAN channel
     *
     * @param baudRate Baud rate
     * @return ChannelConfig The created configuration
     */
    static ChannelConfig forCAN(unsigned long baudRate);

    /**
     * @brief Create a configuration for an extended CAN channel
     *
     * @param baudRate Baud rate
     * @return ChannelConfig The created configuration
     */
    static ChannelConfig forCANExtended(unsigned long baudRate);

    /**
     * @brief Create a configuration for a standard ISO15765 channel
     *
     * @param baudRate Baud rate
     * @return ChannelConfig The created configuration
     */
    static ChannelConfig forISO15765(unsigned long baudRate);

    /**
     * @brief Create a configuration for an extended ISO15765 channel
     *
     * @param baudRate Baud rate
     * @return ChannelConfig The created configuration
     */
    static ChannelConfig forISO15765Extended(unsigned long baudRate);

    /**
     * @brief Equality operator
     */
    bool operator==(const ChannelConfig& other) const;

    /**
     * @brief Inequality operator
     */
    bool operator!=(const ChannelConfig& other) const;

private:
    // Member variables
    unsigned long baudRate;
    unsigned long flags;
    std::unordered_map<unsigned long, unsigned long> parameters;
};

/**
 * @brief Main interface to J2534 devices
 */
class FMUS_AUTO_API Device {
public:
    /**
     * @brief Default constructor
     */
    Device();

    /**
     * @brief Copy constructor
     */
    Device(const Device& other);

    /**
     * @brief Move constructor
     */
    Device(Device&& other) noexcept;

    /**
     * @brief Copy assignment operator
     */
    Device& operator=(const Device& other);

    /**
     * @brief Move assignment operator
     */
    Device& operator=(Device&& other) noexcept;

    /**
     * @brief Destructor
     */
    ~Device();

    /**
     * @brief Discover available J2534 adapters
     *
     * @return std::vector<AdapterInfo> Available adapters
     */
    std::vector<AdapterInfo> discoverAdapters();

    /**
     * @brief Connect to an adapter
     *
     * @param adapter Adapter to connect to
     * @return true If connection was successful
     * @return false If connection failed
     */
    bool connect(const AdapterInfo& adapter);

    /**
     * @brief Disconnect from the current adapter
     */
    void disconnect();

    /**
     * @brief Check if connected to an adapter
     *
     * @return true If connected
     * @return false If not connected
     */
    bool isConnected() const;

    /**
     * @brief Open a channel with the specified protocol
     *
     * @param protocol Protocol ID
     * @param config Channel configuration
     * @return unsigned long Channel ID
     * @throws DeviceError If the channel could not be opened
     */
    unsigned long openChannel(unsigned long protocol, const ChannelConfig& config);

    /**
     * @brief Close a channel
     *
     * @param channelId Channel ID to close
     * @throws DeviceError If the channel could not be closed
     */
    void closeChannel(unsigned long channelId);

    /**
     * @brief Send a message on a channel
     *
     * @param channelId Channel ID
     * @param message Message to send
     * @param timeout Timeout in ms
     * @throws DeviceError If the message could not be sent
     */
    void sendMessage(unsigned long channelId, const Message& message, unsigned long timeout = 1000);

    /**
     * @brief Receive messages from a channel
     *
     * @param channelId Channel ID
     * @param timeout Timeout in ms
     * @param maxMessages Maximum number of messages to receive
     * @return std::vector<Message> Received messages
     * @throws DeviceError If messages could not be received
     */
    std::vector<Message> receiveMessages(unsigned long channelId, unsigned long timeout = 1000, unsigned long maxMessages = 10);

    /**
     * @brief Start a message filter on a channel
     *
     * @param channelId Channel ID
     * @param filter Filter to start
     * @return unsigned long Filter ID
     * @throws DeviceError If the filter could not be started
     */
    unsigned long startMsgFilter(unsigned long channelId, const Filter& filter);

    /**
     * @brief Stop a message filter
     *
     * @param channelId Channel ID
     * @param filterId Filter ID to stop
     * @throws DeviceError If the filter could not be stopped
     */
    void stopMsgFilter(unsigned long channelId, unsigned long filterId);

    /**
     * @brief Execute an IOCTL command
     *
     * @param channelId Channel ID (0 for device-specific commands)
     * @param ioctlId IOCTL command ID
     * @param input Input data
     * @param output Output data
     * @throws DeviceError If the IOCTL command failed
     */
    void ioctl(unsigned long channelId, unsigned long ioctlId, const void* input, void* output);

    /**
     * @brief Get information about the connected adapter
     *
     * @return AdapterInfo Adapter information
     * @throws DeviceError If not connected to an adapter
     */
    AdapterInfo getAdapterInfo() const;

private:
    // Private implementation (PIMPL idiom)
    class DeviceImpl;
    std::unique_ptr<DeviceImpl> impl;
};

} // namespace j2534
} // namespace fmus

#endif // FMUS_J2534_H