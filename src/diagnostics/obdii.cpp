#include <fmus/diagnostics/obdii.h>
#include <fmus/logger.h>
#include <fmus/utils.h>
#include <fmus/thread_pool.h>
#include <sstream>
#include <iomanip>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <algorithm>

namespace fmus {
namespace diagnostics {

// OBDDTC implementation
char OBDDTC::getCategory() const {
    if (code.empty()) return '?';
    return code[0];
}

bool OBDDTC::isEmissionsRelated() const {
    return getCategory() == 'P';
}

std::string OBDDTC::bytesToDTCString(uint16_t dtcBytes) {
    char category;
    uint16_t number = dtcBytes & 0x3FFF;
    
    switch ((dtcBytes >> 14) & 0x03) {
        case 0: category = 'P'; break;
        case 1: category = 'C'; break;
        case 2: category = 'B'; break;
        case 3: category = 'U'; break;
        default: category = '?'; break;
    }
    
    std::ostringstream ss;
    ss << category << std::setw(4) << std::setfill('0') << number;
    return ss.str();
}

uint16_t OBDDTC::dtcStringToBytes(const std::string& dtcString) {
    if (dtcString.length() != 5) return 0;
    
    uint16_t categoryBits = 0;
    switch (dtcString[0]) {
        case 'P': categoryBits = 0; break;
        case 'C': categoryBits = 1; break;
        case 'B': categoryBits = 2; break;
        case 'U': categoryBits = 3; break;
        default: return 0;
    }
    
    try {
        uint16_t number = std::stoul(dtcString.substr(1), nullptr, 10);
        return (categoryBits << 14) | (number & 0x3FFF);
    } catch (...) {
        return 0;
    }
}

std::string OBDDTC::toString() const {
    std::ostringstream ss;
    ss << "DTC[" << code;
    if (!description.empty()) {
        ss << ": " << description;
    }
    if (isPending) ss << " PENDING";
    if (isConfirmed) ss << " CONFIRMED";
    if (isPermanent) ss << " PERMANENT";
    ss << "]";
    return ss.str();
}

// OBDParameter implementation
void OBDParameter::calculateValue() {
    if (rawData.empty()) return;
    
    switch (pid) {
        case OBDPID::ENGINE_LOAD:
            if (rawData.size() >= 1) {
                value = (rawData[0] * 100.0) / 255.0;
                unit = "%";
            }
            break;
            
        case OBDPID::COOLANT_TEMP:
        case OBDPID::INTAKE_AIR_TEMP:
            if (rawData.size() >= 1) {
                value = rawData[0] - 40;
                unit = "°C";
            }
            break;
            
        case OBDPID::ENGINE_RPM:
            if (rawData.size() >= 2) {
                value = ((rawData[0] * 256) + rawData[1]) / 4.0;
                unit = "RPM";
            }
            break;
            
        case OBDPID::VEHICLE_SPEED:
            if (rawData.size() >= 1) {
                value = rawData[0];
                unit = "km/h";
            }
            break;
            
        case OBDPID::THROTTLE_POSITION:
            if (rawData.size() >= 1) {
                value = (rawData[0] * 100.0) / 255.0;
                unit = "%";
            }
            break;
            
        case OBDPID::FUEL_PRESSURE:
            if (rawData.size() >= 1) {
                value = rawData[0] * 3;
                unit = "kPa";
            }
            break;
            
        case OBDPID::INTAKE_MANIFOLD_PRESSURE:
            if (rawData.size() >= 1) {
                value = rawData[0];
                unit = "kPa";
            }
            break;
            
        case OBDPID::TIMING_ADVANCE:
            if (rawData.size() >= 1) {
                value = (rawData[0] / 2.0) - 64.0;
                unit = "°";
            }
            break;
            
        case OBDPID::MAF_AIRFLOW_RATE:
            if (rawData.size() >= 2) {
                value = ((rawData[0] * 256) + rawData[1]) / 100.0;
                unit = "g/s";
            }
            break;
            
        case OBDPID::FUEL_TANK_LEVEL:
            if (rawData.size() >= 1) {
                value = (rawData[0] * 100.0) / 255.0;
                unit = "%";
            }
            break;
            
        case OBDPID::ABSOLUTE_BAROMETRIC_PRESSURE:
            if (rawData.size() >= 1) {
                value = rawData[0];
                unit = "kPa";
            }
            break;
            
        case OBDPID::RUNTIME_SINCE_ENGINE_START:
            if (rawData.size() >= 2) {
                value = (rawData[0] * 256) + rawData[1];
                unit = "s";
            }
            break;
            
        case OBDPID::DISTANCE_WITH_MIL_ON:
        case OBDPID::DISTANCE_SINCE_CODES_CLEARED:
            if (rawData.size() >= 2) {
                value = (rawData[0] * 256) + rawData[1];
                unit = "km";
            }
            break;
            
        default:
            // For unsupported PIDs, just store raw value
            if (!rawData.empty()) {
                value = rawData[0];
                unit = "raw";
            }
            break;
    }
}

std::string OBDParameter::getFormattedValue() const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << value;
    if (!unit.empty()) {
        ss << " " << unit;
    }
    return ss.str();
}

std::string OBDParameter::toString() const {
    std::ostringstream ss;
    ss << "OBD[" << name << ": " << getFormattedValue() << "]";
    return ss.str();
}

// OBDConfig implementation
std::string OBDConfig::toString() const {
    std::ostringstream ss;
    ss << "OBDConfig[ReqID:0x" << std::hex << requestId
       << ", RspID:0x" << responseId
       << ", Timeout:" << std::dec << timeout << "ms"
       << ", ExtIDs:" << (useExtendedIds ? "Yes" : "No")
       << ", ECUs:" << ecuIds.size() << "]";
    return ss.str();
}

// OBDClient implementation
class OBDClient::Impl {
public:
    OBDConfig config;
    std::shared_ptr<protocols::CANProtocol> canProtocol;
    std::atomic<bool> initialized{false};
    std::atomic<bool> monitoring{false};
    
    // Monitoring
    std::vector<OBDPID> monitoringPIDs;
    std::function<void(const std::vector<OBDParameter>&)> monitoringCallback;
    std::thread monitoringThread;
    std::chrono::milliseconds monitoringInterval{1000};
    
    // Statistics
    Statistics stats;
    mutable std::mutex statsMutex;
    
    // Request/Response handling
    std::mutex requestMutex;
    std::condition_variable responseCondition;
    std::vector<uint8_t> pendingResponse;
    bool responseReceived = false;
    
    // Supported PIDs cache
    std::vector<OBDPID> supportedPIDs;
    bool supportedPIDsCached = false;
    
    Impl() {
        stats.startTime = std::chrono::system_clock::now();
    }
    
    ~Impl() {
        if (monitoring) {
            stopMonitoring();
        }
    }
    
    void updateStats(bool isRequest, bool isTimeout = false, bool isError = false) {
        std::lock_guard<std::mutex> lock(statsMutex);
        if (isRequest) {
            stats.requestsSent++;
        } else {
            stats.responsesReceived++;
        }
        if (isTimeout) {
            stats.timeouts++;
        }
        if (isError) {
            stats.errors++;
        }
    }
    
    void onCANMessage(const protocols::CANMessage& canMsg) {
        // Check if this is a response to our request
        bool isResponse = false;
        if (canMsg.id == config.responseId) {
            isResponse = true;
        } else {
            // Check if it's from any of the configured ECU IDs
            for (uint32_t ecuId : config.ecuIds) {
                if (canMsg.id == ecuId) {
                    isResponse = true;
                    break;
                }
            }
        }
        
        if (!isResponse || canMsg.data.size() < 2) {
            return;
        }
        
        // Check if it's a positive response (mode + 0x40)
        uint8_t responseMode = canMsg.data[0];
        if (responseMode < 0x40) {
            return; // Not a positive response
        }
        
        std::lock_guard<std::mutex> lock(requestMutex);
        pendingResponse = canMsg.data;
        responseReceived = true;
        responseCondition.notify_one();
    }
    
    std::vector<uint8_t> sendOBDRequest(OBDMode mode, uint8_t pid = 0) {
        if (!initialized) {
            return {};
        }
        
        // Prepare request
        protocols::CANMessage request;
        request.id = config.requestId;
        request.data = {static_cast<uint8_t>(mode), pid};
        
        {
            std::unique_lock<std::mutex> lock(requestMutex);
            responseReceived = false;
        }
        
        // Send request
        if (!canProtocol->sendMessage(request)) {
            updateStats(true, false, true);
            return {};
        }
        
        updateStats(true);
        
        // Wait for response
        std::unique_lock<std::mutex> lock(requestMutex);
        bool received = responseCondition.wait_for(lock, 
            std::chrono::milliseconds(config.timeout),
            [this] { return responseReceived; });
        
        if (!received) {
            updateStats(false, true);
            return {};
        }
        
        updateStats(false);
        return pendingResponse;
    }
    
    void stopMonitoring() {
        monitoring = false;
        if (monitoringThread.joinable()) {
            monitoringThread.join();
        }
    }
    
    void monitoringLoop() {
        auto logger = Logger::getInstance();
        logger->debug("OBD monitoring thread started");
        
        while (monitoring) {
            try {
                std::vector<OBDParameter> parameters;
                
                for (OBDPID pid : monitoringPIDs) {
                    auto response = sendOBDRequest(OBDMode::CURRENT_DATA, static_cast<uint8_t>(pid));
                    if (response.size() >= 3 && response[0] == 0x41 && response[1] == static_cast<uint8_t>(pid)) {
                        OBDParameter param(pid, getPIDDescription(pid), getPIDUnit(pid));
                        param.rawData.assign(response.begin() + 2, response.end());
                        param.calculateValue();
                        parameters.push_back(param);
                    }
                }
                
                if (!parameters.empty() && monitoringCallback) {
                    monitoringCallback(parameters);
                }
                
                std::this_thread::sleep_for(monitoringInterval);
                
            } catch (const std::exception& e) {
                logger->error("OBD monitoring error: " + std::string(e.what()));
                updateStats(false, false, true);
            }
        }
        
        logger->debug("OBD monitoring thread stopped");
    }
};

OBDClient::OBDClient() : pImpl(std::make_unique<Impl>()) {}

OBDClient::~OBDClient() = default;

bool OBDClient::initialize(const OBDConfig& config, std::shared_ptr<protocols::CANProtocol> canProtocol) {
    auto logger = Logger::getInstance();
    logger->info("Initializing OBD client: " + config.toString());
    
    if (!canProtocol || !canProtocol->isInitialized()) {
        logger->error("CAN protocol not initialized");
        return false;
    }
    
    pImpl->config = config;
    pImpl->canProtocol = canProtocol;
    
    // Start monitoring CAN messages for responses
    if (!pImpl->canProtocol->startMonitoring([this](const protocols::CANMessage& msg) {
        pImpl->onCANMessage(msg);
    })) {
        logger->error("Failed to start CAN monitoring for OBD");
        return false;
    }
    
    pImpl->initialized = true;
    logger->info("OBD client initialized successfully");
    return true;
}

void OBDClient::shutdown() {
    if (pImpl->monitoring) {
        stopMonitoring();
    }
    
    if (pImpl->canProtocol && pImpl->canProtocol->isMonitoring()) {
        pImpl->canProtocol->stopMonitoring();
    }
    
    pImpl->initialized = false;
    
    auto logger = Logger::getInstance();
    logger->info("OBD client shutdown");
}

bool OBDClient::isInitialized() const {
    return pImpl->initialized;
}

std::vector<OBDPID> OBDClient::getSupportedPIDs() {
    if (pImpl->supportedPIDsCached) {
        return pImpl->supportedPIDs;
    }
    
    std::vector<OBDPID> supported;
    
    // Read supported PIDs 01-20
    auto response = pImpl->sendOBDRequest(OBDMode::CURRENT_DATA, 0x00);
    if (response.size() >= 6 && response[0] == 0x41 && response[1] == 0x00) {
        std::vector<uint8_t> pidData(response.begin() + 2, response.begin() + 6);
        auto pids = parseSupportedPIDs(pidData, 0);
        supported.insert(supported.end(), pids.begin(), pids.end());
    }
    
    // Read supported PIDs 21-40 if PID 20 is supported
    if (std::find(supported.begin(), supported.end(), OBDPID::SUPPORTED_PIDS_21_40) != supported.end()) {
        response = pImpl->sendOBDRequest(OBDMode::CURRENT_DATA, 0x20);
        if (response.size() >= 6 && response[0] == 0x41 && response[1] == 0x20) {
            std::vector<uint8_t> pidData(response.begin() + 2, response.begin() + 6);
            auto pids = parseSupportedPIDs(pidData, 32);
            supported.insert(supported.end(), pids.begin(), pids.end());
        }
    }
    
    // Read supported PIDs 41-60 if PID 40 is supported
    if (std::find(supported.begin(), supported.end(), OBDPID::SUPPORTED_PIDS_41_60) != supported.end()) {
        response = pImpl->sendOBDRequest(OBDMode::CURRENT_DATA, 0x40);
        if (response.size() >= 6 && response[0] == 0x41 && response[1] == 0x40) {
            std::vector<uint8_t> pidData(response.begin() + 2, response.begin() + 6);
            auto pids = parseSupportedPIDs(pidData, 64);
            supported.insert(supported.end(), pids.begin(), pids.end());
        }
    }
    
    pImpl->supportedPIDs = supported;
    pImpl->supportedPIDsCached = true;
    
    return supported;
}

OBDParameter OBDClient::readParameter(OBDPID pid) {
    OBDParameter param(pid, getPIDDescription(pid), getPIDUnit(pid));
    
    auto response = pImpl->sendOBDRequest(OBDMode::CURRENT_DATA, static_cast<uint8_t>(pid));
    if (response.size() >= 3 && response[0] == 0x41 && response[1] == static_cast<uint8_t>(pid)) {
        param.rawData.assign(response.begin() + 2, response.end());
        param.calculateValue();
    }
    
    return param;
}

std::vector<OBDParameter> OBDClient::readMultipleParameters(const std::vector<OBDPID>& pids) {
    std::vector<OBDParameter> parameters;
    
    for (OBDPID pid : pids) {
        parameters.push_back(readParameter(pid));
    }
    
    return parameters;
}

// Specific parameter readers
double OBDClient::getEngineRPM() {
    auto param = readParameter(OBDPID::ENGINE_RPM);
    return param.value;
}

double OBDClient::getVehicleSpeed() {
    auto param = readParameter(OBDPID::VEHICLE_SPEED);
    return param.value;
}

double OBDClient::getEngineCoolantTemp() {
    auto param = readParameter(OBDPID::COOLANT_TEMP);
    return param.value;
}

double OBDClient::getEngineLoad() {
    auto param = readParameter(OBDPID::ENGINE_LOAD);
    return param.value;
}

double OBDClient::getThrottlePosition() {
    auto param = readParameter(OBDPID::THROTTLE_POSITION);
    return param.value;
}

double OBDClient::getFuelLevel() {
    auto param = readParameter(OBDPID::FUEL_TANK_LEVEL);
    return param.value;
}

double OBDClient::getIntakeAirTemp() {
    auto param = readParameter(OBDPID::INTAKE_AIR_TEMP);
    return param.value;
}

double OBDClient::getMAFAirflowRate() {
    auto param = readParameter(OBDPID::MAF_AIRFLOW_RATE);
    return param.value;
}

// Mode 3: Stored DTCs
std::vector<OBDDTC> OBDClient::readStoredDTCs() {
    std::vector<OBDDTC> dtcs;
    
    auto response = pImpl->sendOBDRequest(OBDMode::STORED_DTCS);
    if (response.size() >= 2 && response[0] == 0x43) {
        uint8_t numDTCs = response[1];
        
        for (size_t i = 0; i < numDTCs && (i * 2 + 4) <= response.size(); ++i) {
            uint16_t dtcBytes = (response[i * 2 + 2] << 8) | response[i * 2 + 3];
            if (dtcBytes != 0) {
                OBDDTC dtc;
                dtc.code = OBDDTC::bytesToDTCString(dtcBytes);
                dtc.isConfirmed = true;
                dtcs.push_back(dtc);
            }
        }
    }
    
    return dtcs;
}

// Mode 4: Clear DTCs
bool OBDClient::clearDTCs() {
    auto response = pImpl->sendOBDRequest(OBDMode::CLEAR_DTCS);
    return !response.empty() && response[0] == 0x44;
}

// Mode 7: Pending DTCs
std::vector<OBDDTC> OBDClient::readPendingDTCs() {
    std::vector<OBDDTC> dtcs;
    
    auto response = pImpl->sendOBDRequest(OBDMode::PENDING_DTCS);
    if (response.size() >= 2 && response[0] == 0x47) {
        uint8_t numDTCs = response[1];
        
        for (size_t i = 0; i < numDTCs && (i * 2 + 4) <= response.size(); ++i) {
            uint16_t dtcBytes = (response[i * 2 + 2] << 8) | response[i * 2 + 3];
            if (dtcBytes != 0) {
                OBDDTC dtc;
                dtc.code = OBDDTC::bytesToDTCString(dtcBytes);
                dtc.isPending = true;
                dtcs.push_back(dtc);
            }
        }
    }
    
    return dtcs;
}

// Mode 9: Vehicle Information
std::string OBDClient::getVIN() {
    auto response = pImpl->sendOBDRequest(OBDMode::VEHICLE_INFORMATION, 0x02);
    if (response.size() >= 3 && response[0] == 0x49 && response[1] == 0x02) {
        // VIN is typically returned in multiple frames
        std::string vin;
        for (size_t i = 3; i < response.size() && vin.length() < 17; ++i) {
            if (std::isalnum(response[i])) {
                vin += static_cast<char>(response[i]);
            }
        }
        return vin;
    }
    return "";
}

// Mode 10: Permanent DTCs
std::vector<OBDDTC> OBDClient::readPermanentDTCs() {
    std::vector<OBDDTC> dtcs;
    
    auto response = pImpl->sendOBDRequest(OBDMode::PERMANENT_DTCS);
    if (response.size() >= 2 && response[0] == 0x4A) {
        uint8_t numDTCs = response[1];
        
        for (size_t i = 0; i < numDTCs && (i * 2 + 4) <= response.size(); ++i) {
            uint16_t dtcBytes = (response[i * 2 + 2] << 8) | response[i * 2 + 3];
            if (dtcBytes != 0) {
                OBDDTC dtc;
                dtc.code = OBDDTC::bytesToDTCString(dtcBytes);
                dtc.isPermanent = true;
                dtcs.push_back(dtc);
            }
        }
    }
    
    return dtcs;
}

bool OBDClient::startMonitoring(const std::vector<OBDPID>& pids, 
                               std::function<void(const std::vector<OBDParameter>&)> callback,
                               std::chrono::milliseconds interval) {
    if (!pImpl->initialized || pImpl->monitoring) {
        return false;
    }
    
    pImpl->monitoringPIDs = pids;
    pImpl->monitoringCallback = callback;
    pImpl->monitoringInterval = interval;
    pImpl->monitoring = true;
    
    pImpl->monitoringThread = std::thread(&OBDClient::Impl::monitoringLoop, pImpl.get());
    
    return true;
}

void OBDClient::stopMonitoring() {
    pImpl->stopMonitoring();
}

bool OBDClient::isMonitoring() const {
    return pImpl->monitoring;
}

OBDClient::Statistics OBDClient::getStatistics() const {
    std::lock_guard<std::mutex> lock(pImpl->statsMutex);
    return pImpl->stats;
}

void OBDClient::resetStatistics() {
    std::lock_guard<std::mutex> lock(pImpl->statsMutex);
    pImpl->stats = Statistics{};
    pImpl->stats.startTime = std::chrono::system_clock::now();
}

OBDConfig OBDClient::getConfiguration() const {
    return pImpl->config;
}

// Utility functions
std::string obdModeToString(OBDMode mode) {
    switch (mode) {
        case OBDMode::CURRENT_DATA: return "CurrentData";
        case OBDMode::FREEZE_FRAME_DATA: return "FreezeFrameData";
        case OBDMode::STORED_DTCS: return "StoredDTCs";
        case OBDMode::CLEAR_DTCS: return "ClearDTCs";
        case OBDMode::O2_SENSOR_MONITORING: return "O2SensorMonitoring";
        case OBDMode::ON_BOARD_MONITORING: return "OnBoardMonitoring";
        case OBDMode::PENDING_DTCS: return "PendingDTCs";
        case OBDMode::CONTROL_OPERATIONS: return "ControlOperations";
        case OBDMode::VEHICLE_INFORMATION: return "VehicleInformation";
        case OBDMode::PERMANENT_DTCS: return "PermanentDTCs";
        default: return "Unknown";
    }
}

std::string obdPidToString(OBDPID pid) {
    std::ostringstream ss;
    ss << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(pid);
    return ss.str();
}

std::string getPIDDescription(OBDPID pid) {
    switch (pid) {
        case OBDPID::ENGINE_LOAD: return "Engine Load";
        case OBDPID::COOLANT_TEMP: return "Coolant Temperature";
        case OBDPID::ENGINE_RPM: return "Engine RPM";
        case OBDPID::VEHICLE_SPEED: return "Vehicle Speed";
        case OBDPID::THROTTLE_POSITION: return "Throttle Position";
        case OBDPID::FUEL_PRESSURE: return "Fuel Pressure";
        case OBDPID::INTAKE_MANIFOLD_PRESSURE: return "Intake Manifold Pressure";
        case OBDPID::TIMING_ADVANCE: return "Timing Advance";
        case OBDPID::INTAKE_AIR_TEMP: return "Intake Air Temperature";
        case OBDPID::MAF_AIRFLOW_RATE: return "MAF Air Flow Rate";
        case OBDPID::FUEL_TANK_LEVEL: return "Fuel Tank Level";
        case OBDPID::ABSOLUTE_BAROMETRIC_PRESSURE: return "Barometric Pressure";
        case OBDPID::RUNTIME_SINCE_ENGINE_START: return "Runtime Since Engine Start";
        case OBDPID::DISTANCE_WITH_MIL_ON: return "Distance with MIL On";
        case OBDPID::DISTANCE_SINCE_CODES_CLEARED: return "Distance Since Codes Cleared";
        default: return "Unknown Parameter";
    }
}

std::string getPIDUnit(OBDPID pid) {
    switch (pid) {
        case OBDPID::ENGINE_LOAD:
        case OBDPID::THROTTLE_POSITION:
        case OBDPID::FUEL_TANK_LEVEL:
            return "%";
        case OBDPID::COOLANT_TEMP:
        case OBDPID::INTAKE_AIR_TEMP:
            return "°C";
        case OBDPID::ENGINE_RPM:
            return "RPM";
        case OBDPID::VEHICLE_SPEED:
            return "km/h";
        case OBDPID::FUEL_PRESSURE:
        case OBDPID::INTAKE_MANIFOLD_PRESSURE:
        case OBDPID::ABSOLUTE_BAROMETRIC_PRESSURE:
            return "kPa";
        case OBDPID::TIMING_ADVANCE:
            return "°";
        case OBDPID::MAF_AIRFLOW_RATE:
            return "g/s";
        case OBDPID::RUNTIME_SINCE_ENGINE_START:
            return "s";
        case OBDPID::DISTANCE_WITH_MIL_ON:
        case OBDPID::DISTANCE_SINCE_CODES_CLEARED:
            return "km";
        default:
            return "";
    }
}

bool isPIDSupported(const std::vector<uint8_t>& supportedPIDs, OBDPID pid) {
    if (supportedPIDs.size() != 4) return false;
    
    uint8_t pidValue = static_cast<uint8_t>(pid);
    uint8_t byteIndex = pidValue / 8;
    uint8_t bitIndex = 7 - (pidValue % 8);
    
    if (byteIndex >= supportedPIDs.size()) return false;
    
    return (supportedPIDs[byteIndex] & (1 << bitIndex)) != 0;
}

std::vector<OBDPID> parseSupportedPIDs(const std::vector<uint8_t>& data, uint8_t baseRange) {
    std::vector<OBDPID> pids;
    
    if (data.size() != 4) return pids;
    
    for (int byte = 0; byte < 4; ++byte) {
        for (int bit = 0; bit < 8; ++bit) {
            if (data[byte] & (1 << (7 - bit))) {
                uint8_t pidValue = baseRange + (byte * 8) + bit + 1;
                pids.push_back(static_cast<OBDPID>(pidValue));
            }
        }
    }
    
    return pids;
}

} // namespace diagnostics
} // namespace fmus
