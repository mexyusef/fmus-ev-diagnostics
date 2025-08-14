#ifndef FMUS_DIAGNOSTICS_OBDII_H
#define FMUS_DIAGNOSTICS_OBDII_H

/**
 * @file obdii.h
 * @brief OBD-II (On-Board Diagnostics) implementation
 */

#include <fmus/protocols/can.h>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <map>
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
namespace diagnostics {

/**
 * @brief OBD-II Modes
 */
enum class OBDMode : uint8_t {
    CURRENT_DATA = 0x01,                    ///< Mode 1: Current powertrain data
    FREEZE_FRAME_DATA = 0x02,               ///< Mode 2: Freeze frame data
    STORED_DTCS = 0x03,                     ///< Mode 3: Stored DTCs
    CLEAR_DTCS = 0x04,                      ///< Mode 4: Clear DTCs
    O2_SENSOR_MONITORING = 0x05,            ///< Mode 5: O2 sensor monitoring
    ON_BOARD_MONITORING = 0x06,             ///< Mode 6: On-board monitoring
    PENDING_DTCS = 0x07,                    ///< Mode 7: Pending DTCs
    CONTROL_OPERATIONS = 0x08,              ///< Mode 8: Control operations
    VEHICLE_INFORMATION = 0x09,             ///< Mode 9: Vehicle information
    PERMANENT_DTCS = 0x0A                   ///< Mode 10: Permanent DTCs
};

/**
 * @brief Common OBD-II PIDs for Mode 1
 */
enum class OBDPID : uint8_t {
    SUPPORTED_PIDS_01_20 = 0x00,           ///< PIDs supported [01-20]
    MONITOR_STATUS = 0x01,                  ///< Monitor status since DTCs cleared
    FREEZE_DTC = 0x02,                      ///< Freeze DTC
    FUEL_SYSTEM_STATUS = 0x03,              ///< Fuel system status
    ENGINE_LOAD = 0x04,                     ///< Calculated engine load
    COOLANT_TEMP = 0x05,                    ///< Engine coolant temperature
    SHORT_TERM_FUEL_TRIM_1 = 0x06,          ///< Short term fuel trim—Bank 1
    LONG_TERM_FUEL_TRIM_1 = 0x07,           ///< Long term fuel trim—Bank 1
    SHORT_TERM_FUEL_TRIM_2 = 0x08,          ///< Short term fuel trim—Bank 2
    LONG_TERM_FUEL_TRIM_2 = 0x09,           ///< Long term fuel trim—Bank 2
    FUEL_PRESSURE = 0x0A,                   ///< Fuel pressure
    INTAKE_MANIFOLD_PRESSURE = 0x0B,        ///< Intake manifold absolute pressure
    ENGINE_RPM = 0x0C,                      ///< Engine RPM
    VEHICLE_SPEED = 0x0D,                   ///< Vehicle speed
    TIMING_ADVANCE = 0x0E,                  ///< Timing advance
    INTAKE_AIR_TEMP = 0x0F,                 ///< Intake air temperature
    MAF_AIRFLOW_RATE = 0x10,                ///< MAF air flow rate
    THROTTLE_POSITION = 0x11,               ///< Throttle position
    COMMANDED_SECONDARY_AIR_STATUS = 0x12,   ///< Commanded secondary air status
    O2_SENSORS_PRESENT = 0x13,              ///< Oxygen sensors present
    O2_SENSOR_1_VOLTAGE = 0x14,             ///< Oxygen sensor 1 voltage
    O2_SENSOR_2_VOLTAGE = 0x15,             ///< Oxygen sensor 2 voltage
    O2_SENSOR_3_VOLTAGE = 0x16,             ///< Oxygen sensor 3 voltage
    O2_SENSOR_4_VOLTAGE = 0x17,             ///< Oxygen sensor 4 voltage
    O2_SENSOR_5_VOLTAGE = 0x18,             ///< Oxygen sensor 5 voltage
    O2_SENSOR_6_VOLTAGE = 0x19,             ///< Oxygen sensor 6 voltage
    O2_SENSOR_7_VOLTAGE = 0x1A,             ///< Oxygen sensor 7 voltage
    O2_SENSOR_8_VOLTAGE = 0x1B,             ///< Oxygen sensor 8 voltage
    OBD_STANDARDS = 0x1C,                   ///< OBD standards this vehicle conforms to
    O2_SENSORS_PRESENT_4_BANKS = 0x1D,      ///< Oxygen sensors present (4 banks)
    AUXILIARY_INPUT_STATUS = 0x1E,          ///< Auxiliary input status
    RUNTIME_SINCE_ENGINE_START = 0x1F,      ///< Runtime since engine start
    SUPPORTED_PIDS_21_40 = 0x20,           ///< PIDs supported [21-40]
    DISTANCE_WITH_MIL_ON = 0x21,            ///< Distance traveled with MIL on
    FUEL_RAIL_PRESSURE = 0x22,              ///< Fuel Rail Pressure (relative to manifold vacuum)
    FUEL_RAIL_GAUGE_PRESSURE = 0x23,        ///< Fuel Rail Gauge Pressure (diesel, or gasoline direct injection)
    O2_SENSOR_1_FUEL_AIR_RATIO = 0x24,      ///< Oxygen sensor 1 fuel-air equivalence ratio
    O2_SENSOR_2_FUEL_AIR_RATIO = 0x25,      ///< Oxygen sensor 2 fuel-air equivalence ratio
    O2_SENSOR_3_FUEL_AIR_RATIO = 0x26,      ///< Oxygen sensor 3 fuel-air equivalence ratio
    O2_SENSOR_4_FUEL_AIR_RATIO = 0x27,      ///< Oxygen sensor 4 fuel-air equivalence ratio
    O2_SENSOR_5_FUEL_AIR_RATIO = 0x28,      ///< Oxygen sensor 5 fuel-air equivalence ratio
    O2_SENSOR_6_FUEL_AIR_RATIO = 0x29,      ///< Oxygen sensor 6 fuel-air equivalence ratio
    O2_SENSOR_7_FUEL_AIR_RATIO = 0x2A,      ///< Oxygen sensor 7 fuel-air equivalence ratio
    O2_SENSOR_8_FUEL_AIR_RATIO = 0x2B,      ///< Oxygen sensor 8 fuel-air equivalence ratio
    COMMANDED_EGR = 0x2C,                   ///< Commanded EGR
    EGR_ERROR = 0x2D,                       ///< EGR Error
    COMMANDED_EVAPORATIVE_PURGE = 0x2E,     ///< Commanded evaporative purge
    FUEL_TANK_LEVEL = 0x2F,                 ///< Fuel Tank Level Input
    WARMUPS_SINCE_CODES_CLEARED = 0x30,     ///< Warm-ups since codes cleared
    DISTANCE_SINCE_CODES_CLEARED = 0x31,    ///< Distance traveled since codes cleared
    EVAP_SYSTEM_VAPOR_PRESSURE = 0x32,      ///< Evap. System Vapor Pressure
    ABSOLUTE_BAROMETRIC_PRESSURE = 0x33,    ///< Absolute Barometric Pressure
    O2_SENSOR_1_CURRENT = 0x34,             ///< Oxygen sensor 1 current
    O2_SENSOR_2_CURRENT = 0x35,             ///< Oxygen sensor 2 current
    O2_SENSOR_3_CURRENT = 0x36,             ///< Oxygen sensor 3 current
    O2_SENSOR_4_CURRENT = 0x37,             ///< Oxygen sensor 4 current
    O2_SENSOR_5_CURRENT = 0x38,             ///< Oxygen sensor 5 current
    O2_SENSOR_6_CURRENT = 0x39,             ///< Oxygen sensor 6 current
    O2_SENSOR_7_CURRENT = 0x3A,             ///< Oxygen sensor 7 current
    O2_SENSOR_8_CURRENT = 0x3B,             ///< Oxygen sensor 8 current
    CATALYST_TEMP_BANK1_SENSOR1 = 0x3C,     ///< Catalyst Temperature: Bank 1, Sensor 1
    CATALYST_TEMP_BANK2_SENSOR1 = 0x3D,     ///< Catalyst Temperature: Bank 2, Sensor 1
    CATALYST_TEMP_BANK1_SENSOR2 = 0x3E,     ///< Catalyst Temperature: Bank 1, Sensor 2
    CATALYST_TEMP_BANK2_SENSOR2 = 0x3F,     ///< Catalyst Temperature: Bank 2, Sensor 2
    SUPPORTED_PIDS_41_60 = 0x40            ///< PIDs supported [41-60]
};

/**
 * @brief OBD-II DTC structure
 */
struct OBDDTC {
    std::string code;                       ///< DTC code (e.g., "P0171")
    std::string description;                ///< Human-readable description
    bool isPending = false;                 ///< Whether the DTC is pending
    bool isConfirmed = false;               ///< Whether the DTC is confirmed
    bool isPermanent = false;               ///< Whether the DTC is permanent
    std::chrono::system_clock::time_point timestamp;
    
    OBDDTC() = default;
    OBDDTC(const std::string& dtcCode, const std::string& desc = "")
        : code(dtcCode), description(desc), timestamp(std::chrono::system_clock::now()) {}
    
    /**
     * @brief Get DTC category (P, B, C, U)
     */
    char getCategory() const;
    
    /**
     * @brief Check if this is an emissions-related DTC
     */
    bool isEmissionsRelated() const;
    
    /**
     * @brief Convert raw DTC bytes to string
     */
    static std::string bytesToDTCString(uint16_t dtcBytes);
    
    /**
     * @brief Convert DTC string to raw bytes
     */
    static uint16_t dtcStringToBytes(const std::string& dtcString);
    
    std::string toString() const;
};

/**
 * @brief OBD-II Parameter data
 */
struct OBDParameter {
    OBDPID pid;                             ///< Parameter ID
    std::string name;                       ///< Parameter name
    std::string description;                ///< Parameter description
    std::vector<uint8_t> rawData;           ///< Raw response data
    double value = 0.0;                     ///< Calculated value
    std::string unit;                       ///< Unit of measurement
    std::chrono::system_clock::time_point timestamp;
    
    OBDParameter() = default;
    OBDParameter(OBDPID paramPid, const std::string& paramName, const std::string& paramUnit = "")
        : pid(paramPid), name(paramName), unit(paramUnit), timestamp(std::chrono::system_clock::now()) {}
    
    /**
     * @brief Calculate value from raw data
     */
    void calculateValue();
    
    /**
     * @brief Get formatted value string
     */
    std::string getFormattedValue() const;
    
    std::string toString() const;
};

/**
 * @brief OBD-II Configuration
 */
struct OBDConfig {
    uint32_t requestId = 0x7DF;             ///< Functional request ID
    uint32_t responseId = 0x7E8;            ///< Response ID (ECU specific)
    uint32_t timeout = 1000;                ///< Response timeout in ms
    bool useExtendedIds = false;            ///< Use 29-bit CAN IDs
    std::vector<uint32_t> ecuIds;           ///< List of ECU response IDs to monitor
    
    std::string toString() const;
};

/**
 * @brief OBD-II Client for vehicle diagnostics
 */
class FMUS_AUTO_API OBDClient {
public:
    /**
     * @brief Constructor
     */
    OBDClient();
    
    /**
     * @brief Destructor
     */
    ~OBDClient();
    
    /**
     * @brief Initialize OBD client
     */
    bool initialize(const OBDConfig& config, std::shared_ptr<protocols::CANProtocol> canProtocol);
    
    /**
     * @brief Shutdown OBD client
     */
    void shutdown();
    
    /**
     * @brief Check if client is initialized
     */
    bool isInitialized() const;
    
    // Mode 1: Current Data
    std::vector<OBDPID> getSupportedPIDs();
    OBDParameter readParameter(OBDPID pid);
    std::vector<OBDParameter> readMultipleParameters(const std::vector<OBDPID>& pids);
    
    // Specific parameter readers
    double getEngineRPM();
    double getVehicleSpeed();
    double getEngineCoolantTemp();
    double getEngineLoad();
    double getThrottlePosition();
    double getFuelLevel();
    double getIntakeAirTemp();
    double getMAFAirflowRate();
    
    // Mode 2: Freeze Frame Data
    std::vector<OBDParameter> readFreezeFrameData(uint8_t frameNumber = 0);
    
    // Mode 3: Stored DTCs
    std::vector<OBDDTC> readStoredDTCs();
    
    // Mode 4: Clear DTCs
    bool clearDTCs();
    
    // Mode 7: Pending DTCs
    std::vector<OBDDTC> readPendingDTCs();
    
    // Mode 9: Vehicle Information
    std::string getVIN();
    std::string getCalibrationID();
    std::string getECUName();
    
    // Mode 10: Permanent DTCs
    std::vector<OBDDTC> readPermanentDTCs();
    
    /**
     * @brief Start continuous parameter monitoring
     */
    bool startMonitoring(const std::vector<OBDPID>& pids, 
                        std::function<void(const std::vector<OBDParameter>&)> callback,
                        std::chrono::milliseconds interval = std::chrono::milliseconds(1000));
    
    /**
     * @brief Stop parameter monitoring
     */
    void stopMonitoring();
    
    /**
     * @brief Check if monitoring is active
     */
    bool isMonitoring() const;
    
    /**
     * @brief Get client statistics
     */
    struct Statistics {
        uint64_t requestsSent = 0;
        uint64_t responsesReceived = 0;
        uint64_t timeouts = 0;
        uint64_t errors = 0;
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
    OBDConfig getConfiguration() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

// Utility functions
FMUS_AUTO_API std::string obdModeToString(OBDMode mode);
FMUS_AUTO_API std::string obdPidToString(OBDPID pid);
FMUS_AUTO_API std::string getPIDDescription(OBDPID pid);
FMUS_AUTO_API std::string getPIDUnit(OBDPID pid);
FMUS_AUTO_API bool isPIDSupported(const std::vector<uint8_t>& supportedPIDs, OBDPID pid);
FMUS_AUTO_API std::vector<OBDPID> parseSupportedPIDs(const std::vector<uint8_t>& data, uint8_t baseRange);

/**
 * @brief OBD-II Error class
 */
class FMUS_AUTO_API OBDError : public std::runtime_error {
public:
    OBDError(const std::string& message, OBDMode mode = OBDMode::CURRENT_DATA, OBDPID pid = OBDPID::SUPPORTED_PIDS_01_20)
        : std::runtime_error(message), mode(mode), pid(pid) {}

    OBDMode getMode() const { return mode; }
    OBDPID getPID() const { return pid; }

private:
    OBDMode mode;
    OBDPID pid;
};

} // namespace diagnostics
} // namespace fmus

#endif // FMUS_DIAGNOSTICS_OBDII_H
