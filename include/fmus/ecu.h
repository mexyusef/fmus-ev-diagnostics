#pragma once

/**
 * @file ecu.h
 * @brief ECU interface for diagnostics and programming
 */

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <functional>
#include <chrono>
#include <map>

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

// Forward declarations
namespace j2534 {
    class Device;
    class Message;
}

class Message;
class DTC;
class LiveData;
class SecurityAccessParams;
class FlashOptions;

/**
 * @enum ECUType
 * @brief Enumeration of common ECU types
 */
enum class ECUType {
    Engine,
    Transmission,
    ABS,
    SRS,
    BCM,     // Body Control Module
    ICM,     // Instrument Cluster Module
    TCM,     // Transmission Control Module
    PCM,     // Powertrain Control Module
    ECM,     // Engine Control Module
    HVAC,    // Heating, Ventilation, and Air Conditioning
    Radio,
    Immobilizer,
    EPS,     // Electric Power Steering
    VCM,     // Vehicle Control Module
    Custom
};

/**
 * @struct DTCInfo
 * @brief Information about a Diagnostic Trouble Code
 */
struct FMUS_AUTO_API DTCInfo {
    std::string code;           ///< DTC code (e.g., "P0301")
    std::string description;    ///< Human-readable description
    std::string status;         ///< Status flags (e.g., "Confirmed", "Pending")
    std::chrono::system_clock::time_point timestamp; ///< When the DTC was set
    uint32_t occurrenceCount;   ///< Number of occurrences
    std::optional<std::vector<uint8_t>> freezeFrameData; ///< Associated freeze frame data

    // Convenience methods
    bool isActive() const;
    bool isPending() const;
    bool isHistoric() const;
    bool isConfirmed() const;
};

/**
 * @struct Parameter
 * @brief Represents a vehicle parameter (sensor or calculated value)
 */
struct FMUS_AUTO_API Parameter {
    std::string name;           ///< Parameter name
    std::string unit;           ///< Unit of measurement
    double value;               ///< Current value
    double minValue;            ///< Minimum possible value
    double maxValue;            ///< Maximum possible value
    bool isAvailable;           ///< Whether the parameter is available

    // Convenience methods
    double getPercentage() const;   ///< Calculate percentage within range
    std::string getFormattedValue() const; ///< Get value with unit
};

/**
 * @class ECU
 * @brief Interface for ECU diagnostics and programming
 *
 * This class provides a high-level interface for communicating with
 * and programming automotive ECUs (Electronic Control Units).
 */
class FMUS_AUTO_API ECU {
public:
    /**
     * @brief Read all Diagnostic Trouble Codes (DTCs)
     *
     * @return Vector of DTCInfo objects
     */
    std::vector<DTCInfo> readDTCs() const;

    /**
     * @brief Read DTCs with a specific status
     *
     * @param statusMask Bit mask for DTC status filtering
     * @return Vector of filtered DTCInfo objects
     */
    std::vector<DTCInfo> readDTCsByStatus(uint8_t statusMask) const;

    /**
     * @brief Clear all DTCs and freeze frame data
     *
     * @return true if successful, false otherwise
     */
    bool clearDTCs();

    /**
     * @brief Read freeze frame data for a specific DTC
     *
     * @param dtcCode The DTC code to read freeze frame data for
     * @return Vector of Parameters representing the freeze frame
     */
    std::vector<Parameter> readFreezeFrame(const std::string& dtcCode) const;

    /**
     * @brief Read a single parameter by ID
     *
     * @param pid Parameter ID
     * @return Parameter object with current value
     */
    Parameter readParameter(uint16_t pid) const;

    /**
     * @brief Read multiple parameters in one request
     *
     * @param pids Vector of parameter IDs to read
     * @return Vector of Parameter objects with current values
     */
    std::vector<Parameter> readParameters(const std::vector<uint16_t>& pids) const;

    /**
     * @brief Start a live data stream for continuous parameter updates
     *
     * @param pids Vector of parameter IDs to monitor
     * @param updateCallback Function called when parameter values update
     * @param updateRate Desired update rate in milliseconds
     * @return LiveData object that can be used to control the stream
     */
    LiveData startLiveData(
        const std::vector<uint16_t>& pids,
        std::function<void(const std::vector<Parameter>&)> updateCallback,
        std::chrono::milliseconds updateRate = std::chrono::milliseconds(100)
    );

    /**
     * @brief Get ECU information (VIN, calibration IDs, etc.)
     *
     * @return Map of information name to value
     */
    std::map<std::string, std::string> getInfo() const;

    /**
     * @brief Get the Vehicle Identification Number (VIN)
     *
     * @return VIN string
     */
    std::string getVIN() const;

    /**
     * @brief Perform a specific actuator test
     *
     * @param actuatorId ID of the actuator to test
     * @param parameters Optional parameters for the test
     * @return true if successful, false otherwise
     */
    bool performActuatorTest(uint16_t actuatorId, const std::vector<uint8_t>& parameters = {});

    /**
     * @brief Authenticate for secure operations
     *
     * @param securityLevel Security level to access
     * @param params Additional parameters for the security algorithm
     * @return true if authentication successful, false otherwise
     */
    bool authenticate(uint8_t securityLevel, const SecurityAccessParams& params = {});

    /**
     * @brief Check if security access is currently granted
     *
     * @param securityLevel Security level to check
     * @return true if the security level is active
     */
    bool isSecurityAccessActive(uint8_t securityLevel) const;

    /**
     * @brief Flash (reprogram) the ECU with new firmware
     *
     * @param firmwarePath Path to the firmware file
     * @param options Optional flashing parameters
     * @return true if flashing was successful
     */
    bool flashFirmware(
        const std::string& firmwarePath,
        const FlashOptions& options = {}
    );

    /**
     * @brief Flash with progress reporting
     *
     * @param firmwarePath Path to the firmware file
     * @param progressCallback Function called with progress updates (0-100)
     * @param options Optional flashing parameters
     * @return true if flashing was successful
     */
    bool flashFirmware(
        const std::string& firmwarePath,
        std::function<void(int)> progressCallback,
        const FlashOptions& options = {}
    );

    /**
     * @brief Read ECU memory directly
     *
     * @param address Memory address to read from
     * @param size Number of bytes to read
     * @return Vector of bytes read from memory
     */
    std::vector<uint8_t> readMemory(uint32_t address, size_t size);

    /**
     * @brief Write directly to ECU memory
     *
     * @param address Memory address to write to
     * @param data Data to write
     * @return true if successful, false otherwise
     */
    bool writeMemory(uint32_t address, const std::vector<uint8_t>& data);

    /**
     * @brief Send a raw diagnostic message
     *
     * @param message The message to send
     * @param timeoutMs Timeout in milliseconds
     * @return Response message(s)
     */
    std::vector<Message> sendMessage(
        const Message& message,
        std::chrono::milliseconds timeoutMs = std::chrono::milliseconds(1000)
    );

    /**
     * @brief Get the ECU type
     *
     * @return ECU type enum value
     */
    ECUType getType() const;

    /**
     * @brief Get the CAN address of this ECU
     *
     * @return CAN address
     */
    uint32_t getAddress() const;

    /**
     * @brief Check if the ECU is responding to messages
     *
     * @return true if the ECU is responding, false otherwise
     */
    bool isResponding() const;

    /**
     * @brief Set a session timeout handler
     *
     * @param callback Function to call when session is about to timeout
     */
    void setSessionTimeoutHandler(std::function<void()> callback);

    // Constructors and assignment operators
    ~ECU();
    ECU(ECU&& other) noexcept;
    ECU& operator=(ECU&& other) noexcept;

    // Deleted copy constructor and assignment
    ECU(const ECU&) = delete;
    ECU& operator=(const ECU&) = delete;

private:
    // Private implementation
    class Impl;
    std::unique_ptr<Impl> pImpl;

    // Private constructor used by Auto class
    friend class Auto;
    explicit ECU(std::unique_ptr<Impl> impl);
};

/**
 * @class LiveData
 * @brief Controls a live data stream of parameters
 */
class FMUS_AUTO_API LiveData {
public:
    /**
     * @brief Start the data stream
     */
    void start();

    /**
     * @brief Pause the data stream
     */
    void pause();

    /**
     * @brief Resume a paused data stream
     */
    void resume();

    /**
     * @brief Stop the data stream and release resources
     */
    void stop();

    /**
     * @brief Check if the data stream is active
     *
     * @return true if active, false otherwise
     */
    bool isActive() const;

    /**
     * @brief Add parameters to the existing stream
     *
     * @param pids Additional parameter IDs to monitor
     */
    void addParameters(const std::vector<uint16_t>& pids);

    /**
     * @brief Remove parameters from the stream
     *
     * @param pids Parameter IDs to remove
     */
    void removeParameters(const std::vector<uint16_t>& pids);

    /**
     * @brief Set the update rate
     *
     * @param updateRate New update rate in milliseconds
     */
    void setUpdateRate(std::chrono::milliseconds updateRate);

    /**
     * @brief Get the latest parameter values
     *
     * @return Vector of Parameter objects with current values
     */
    std::vector<Parameter> getLatestValues() const;

    // Constructors and assignment operators
    ~LiveData();
    LiveData(LiveData&& other) noexcept;
    LiveData& operator=(LiveData&& other) noexcept;

    // Deleted copy constructor and assignment
    LiveData(const LiveData&) = delete;
    LiveData& operator=(const LiveData&) = delete;

private:
    // Private implementation
    class Impl;
    std::unique_ptr<Impl> pImpl;

    // Private constructor used by ECU class
    friend class ECU;
    explicit LiveData(std::unique_ptr<Impl> impl);
};

} // namespace fmus

#endif // FMUS_ECU_H