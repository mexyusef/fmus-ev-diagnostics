#ifndef FMUS_PROTOCOLS_CAN_H
#define FMUS_PROTOCOLS_CAN_H

/**
 * @file can.h
 * @brief CAN (Controller Area Network) protocol implementation
 */

#include <fmus/j2534.h>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>

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
namespace protocols {

/**
 * @brief CAN frame types
 */
enum class CANFrameType {
    DATA,           ///< Data frame
    REMOTE,         ///< Remote transmission request
    ERROR,          ///< Error frame
    OVERLOAD        ///< Overload frame
};

/**
 * @brief CAN message structure
 */
struct CANMessage {
    uint32_t id = 0;                    ///< CAN identifier
    std::vector<uint8_t> data;          ///< Data payload (0-8 bytes for CAN 2.0)
    bool extended = false;              ///< Extended frame format (29-bit ID)
    bool rtr = false;                   ///< Remote transmission request
    CANFrameType frameType = CANFrameType::DATA;
    std::chrono::system_clock::time_point timestamp;
    
    CANMessage() = default;
    CANMessage(uint32_t canId, const std::vector<uint8_t>& payload, bool isExtended = false)
        : id(canId), data(payload), extended(isExtended), timestamp(std::chrono::system_clock::now()) {}
    
    /**
     * @brief Check if this is a valid CAN message
     */
    bool isValid() const;
    
    /**
     * @brief Get string representation
     */
    std::string toString() const;
    
    /**
     * @brief Convert to J2534 message
     */
    j2534::Message toJ2534Message() const;
    
    /**
     * @brief Create from J2534 message
     */
    static CANMessage fromJ2534Message(const j2534::Message& msg);
};

/**
 * @brief CAN bus configuration
 */
struct CANConfig {
    uint32_t baudRate = 500000;         ///< Baud rate in bps
    bool listenOnly = false;            ///< Listen-only mode
    bool loopback = false;              ///< Loopback mode
    bool extendedFrames = true;         ///< Support extended frames
    uint32_t txTimeout = 1000;          ///< Transmission timeout in ms
    uint32_t rxTimeout = 1000;          ///< Reception timeout in ms
    
    std::string toString() const;
};

/**
 * @brief CAN message filter
 */
struct CANFilter {
    uint32_t id = 0;                    ///< Filter ID
    uint32_t mask = 0x7FF;              ///< Filter mask (11-bit default)
    bool extended = false;              ///< Extended frame filter
    bool passThrough = true;            ///< Pass or block matching messages
    
    CANFilter() = default;
    CANFilter(uint32_t filterId, uint32_t filterMask, bool isExtended = false)
        : id(filterId), mask(filterMask), extended(isExtended) {}
    
    /**
     * @brief Check if a message matches this filter
     */
    bool matches(const CANMessage& message) const;
    
    /**
     * @brief Convert to J2534 filter
     */
    j2534::Filter toJ2534Filter() const;
    
    std::string toString() const;
};

/**
 * @brief CAN protocol handler
 */
class FMUS_AUTO_API CANProtocol {
public:
    /**
     * @brief Constructor
     */
    CANProtocol();
    
    /**
     * @brief Destructor
     */
    ~CANProtocol();
    
    /**
     * @brief Initialize CAN protocol with configuration
     */
    bool initialize(const CANConfig& config);
    
    /**
     * @brief Shutdown CAN protocol
     */
    void shutdown();
    
    /**
     * @brief Check if protocol is initialized
     */
    bool isInitialized() const;
    
    /**
     * @brief Send a CAN message
     */
    bool sendMessage(const CANMessage& message);
    
    /**
     * @brief Send multiple CAN messages
     */
    bool sendMessages(const std::vector<CANMessage>& messages);
    
    /**
     * @brief Receive CAN messages
     */
    std::vector<CANMessage> receiveMessages(uint32_t timeout = 1000);
    
    /**
     * @brief Add a message filter
     */
    bool addFilter(const CANFilter& filter);
    
    /**
     * @brief Remove a message filter
     */
    bool removeFilter(const CANFilter& filter);
    
    /**
     * @brief Clear all filters
     */
    void clearFilters();
    
    /**
     * @brief Get current filters
     */
    std::vector<CANFilter> getFilters() const;
    
    /**
     * @brief Start continuous message monitoring
     */
    bool startMonitoring(std::function<void(const CANMessage&)> callback);
    
    /**
     * @brief Stop message monitoring
     */
    void stopMonitoring();
    
    /**
     * @brief Check if monitoring is active
     */
    bool isMonitoring() const;
    
    /**
     * @brief Get protocol statistics
     */
    struct Statistics {
        uint64_t messagesSent = 0;
        uint64_t messagesReceived = 0;
        uint64_t errorsDetected = 0;
        uint64_t filtersApplied = 0;
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
    CANConfig getConfiguration() const;
    
    /**
     * @brief Update configuration (requires reinitialization)
     */
    bool updateConfiguration(const CANConfig& config);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

// Utility functions
FMUS_AUTO_API bool isValidCANId(uint32_t id, bool extended = false);
FMUS_AUTO_API bool isValidCANBaudRate(uint32_t baudRate);
FMUS_AUTO_API std::vector<uint32_t> getStandardCANBaudRates();
FMUS_AUTO_API std::string canIdToString(uint32_t id, bool extended = false);
FMUS_AUTO_API uint32_t stringToCANId(const std::string& str);

} // namespace protocols
} // namespace fmus

#endif // FMUS_PROTOCOLS_CAN_H
