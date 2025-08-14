#include <fmus/ecu.h>
#include <fmus/auto.h>
#include <fmus/diagnostics/uds.h>
#include <fmus/diagnostics/obdii.h>
#include <fmus/logger.h>
#include <fmus/utils.h>
#include <sstream>
#include <thread>
#include <mutex>
#include <atomic>

namespace fmus {

// ECU Implementation using PIMPL pattern
class ECU::Impl {
public:
    const Auto& parent;
    ECUType type;
    uint32_t address;
    
    // UDS and OBD clients
    std::unique_ptr<diagnostics::UDSClient> udsClient;
    std::unique_ptr<diagnostics::OBDClient> obdClient;
    
    // State
    std::atomic<bool> responsive{false};
    std::atomic<bool> monitoring{false};
    
    // Live data monitoring
    std::thread monitoringThread;
    std::vector<uint16_t> monitoringPIDs;
    std::function<void(const std::vector<LiveDataParameter>&)> monitoringCallback;
    std::chrono::milliseconds monitoringInterval{1000};
    
    // Cached data
    ECUIdentification cachedIdentification;
    bool identificationCached = false;
    mutable std::mutex cacheMutex;
    
    Impl(const Auto& p, ECUType t, uint32_t addr) 
        : parent(p), type(t), address(addr) {}
    
    ~Impl() {
        if (monitoring) {
            stopLiveDataMonitoring();
        }
    }
    
    void stopLiveDataMonitoring() {
        monitoring = false;
        if (monitoringThread.joinable()) {
            monitoringThread.join();
        }
    }
    
    void monitoringLoop() {
        auto logger = Logger::getInstance();
        logger->debug("ECU live data monitoring started for address 0x" + 
                     utils::bytesToHex(utils::uint32ToBytes(address, true)));
        
        while (monitoring) {
            try {
                std::vector<LiveDataParameter> parameters;
                
                for (uint16_t pid : monitoringPIDs) {
                    LiveDataParameter param = readLiveDataParameter(pid);
                    if (!param.name.empty()) {
                        parameters.push_back(param);
                    }
                }
                
                if (!parameters.empty() && monitoringCallback) {
                    monitoringCallback(parameters);
                }
                
                std::this_thread::sleep_for(monitoringInterval);
                
            } catch (const std::exception& e) {
                logger->error("ECU monitoring error: " + std::string(e.what()));
            }
        }
        
        logger->debug("ECU live data monitoring stopped");
    }
    
    LiveDataParameter readLiveDataParameter(uint16_t parameterId) {
        LiveDataParameter param;
        
        // Try OBD-II first for standard parameters
        if (obdClient && obdClient->isInitialized()) {
            try {
                auto obdParam = obdClient->readParameter(static_cast<diagnostics::OBDPID>(parameterId));
                param.name = obdParam.name;
                param.description = obdParam.description;
                param.value = obdParam.value;
                param.unit = obdParam.unit;
                param.timestamp = obdParam.timestamp;
                return param;
            } catch (...) {
                // Fall through to UDS
            }
        }
        
        // Try UDS for manufacturer-specific parameters
        if (udsClient && udsClient->isInitialized()) {
            try {
                auto data = udsClient->readDataByIdentifier(parameterId);
                if (!data.empty()) {
                    param.name = "Parameter_" + std::to_string(parameterId);
                    param.description = "UDS Data Identifier 0x" + utils::bytesToHex(utils::uint16ToBytes(parameterId, true));
                    
                    // Convert raw data to value (simplified)
                    if (data.size() >= 4) {
                        param.value = static_cast<double>(utils::bytesToUint32(data, 0, true));
                    } else if (data.size() >= 2) {
                        param.value = static_cast<double>(utils::bytesToUint16(data, 0, true));
                    } else if (data.size() >= 1) {
                        param.value = static_cast<double>(data[0]);
                    }
                    
                    param.unit = "raw";
                    param.timestamp = std::chrono::system_clock::now();
                }
            } catch (...) {
                // Parameter not available
            }
        }
        
        return param;
    }
};

// ECU Implementation
ECU::ECU(const Auto& parent, ECUType ecuType, uint32_t address)
    : pImpl(std::make_unique<Impl>(parent, ecuType, address)) {
    
    auto logger = Logger::getInstance();
    logger->debug("Creating ECU: " + ecuTypeToString(ecuType) + " at address 0x" + 
                 utils::bytesToHex(utils::uint32ToBytes(address, true)));
}

ECU::ECU(const ECU& other) 
    : pImpl(std::make_unique<Impl>(other.pImpl->parent, other.pImpl->type, other.pImpl->address)) {
    // Copy cached data
    std::lock_guard<std::mutex> lock(other.pImpl->cacheMutex);
    pImpl->cachedIdentification = other.pImpl->cachedIdentification;
    pImpl->identificationCached = other.pImpl->identificationCached;
}

ECU::ECU(ECU&& other) noexcept = default;

ECU& ECU::operator=(const ECU& other) {
    if (this != &other) {
        pImpl = std::make_unique<Impl>(other.pImpl->parent, other.pImpl->type, other.pImpl->address);
        std::lock_guard<std::mutex> lock(other.pImpl->cacheMutex);
        pImpl->cachedIdentification = other.pImpl->cachedIdentification;
        pImpl->identificationCached = other.pImpl->identificationCached;
    }
    return *this;
}

ECU& ECU::operator=(ECU&& other) noexcept = default;

ECU::~ECU() = default;

ECUType ECU::getType() const {
    return pImpl->type;
}

uint32_t ECU::getAddress() const {
    return pImpl->address;
}

bool ECU::isResponsive() const {
    // Try to ping the ECU with a simple UDS request
    if (pImpl->udsClient && pImpl->udsClient->isInitialized()) {
        try {
            bool result = pImpl->udsClient->sendTesterPresent(true); // Suppress response
            pImpl->responsive = result;
            return result;
        } catch (...) {
            pImpl->responsive = false;
            return false;
        }
    }
    
    // If no UDS client, assume responsive for now
    return true;
}

ECUIdentification ECU::readIdentification() {
    std::lock_guard<std::mutex> lock(pImpl->cacheMutex);
    
    if (pImpl->identificationCached) {
        return pImpl->cachedIdentification;
    }
    
    ECUIdentification id;
    
    // Try to read identification via UDS
    if (pImpl->udsClient && pImpl->udsClient->isInitialized()) {
        try {
            // Read VIN (DID 0xF190)
            auto vinData = pImpl->udsClient->readDataByIdentifier(0xF190);
            if (!vinData.empty()) {
                id.vin = std::string(vinData.begin(), vinData.end());
            }
            
            // Read ECU Serial Number (DID 0xF18C)
            auto serialData = pImpl->udsClient->readDataByIdentifier(0xF18C);
            if (!serialData.empty()) {
                id.ecuSerialNumber = utils::bytesToHex(serialData);
            }
            
            // Read Part Number (DID 0xF187)
            auto partData = pImpl->udsClient->readDataByIdentifier(0xF187);
            if (!partData.empty()) {
                id.partNumber = std::string(partData.begin(), partData.end());
            }
            
            // Read Software Version (DID 0xF195)
            auto swData = pImpl->udsClient->readDataByIdentifier(0xF195);
            if (!swData.empty()) {
                id.softwareVersion = std::string(swData.begin(), swData.end());
            }
            
            // Read Hardware Version (DID 0xF191)
            auto hwData = pImpl->udsClient->readDataByIdentifier(0xF191);
            if (!hwData.empty()) {
                id.hardwareVersion = std::string(hwData.begin(), hwData.end());
            }
            
        } catch (const std::exception& e) {
            auto logger = Logger::getInstance();
            logger->warning("Failed to read ECU identification via UDS: " + std::string(e.what()));
        }
    }
    
    // Try OBD-II for VIN if UDS failed
    if (id.vin.empty() && pImpl->obdClient && pImpl->obdClient->isInitialized()) {
        try {
            id.vin = pImpl->obdClient->getVIN();
        } catch (...) {
            // VIN not available via OBD-II
        }
    }
    
    // Set defaults if data not available
    if (id.vin.empty()) id.vin = "UNKNOWN";
    if (id.ecuSerialNumber.empty()) id.ecuSerialNumber = "UNKNOWN";
    if (id.partNumber.empty()) id.partNumber = "UNKNOWN";
    if (id.softwareVersion.empty()) id.softwareVersion = "1.0.0";
    if (id.hardwareVersion.empty()) id.hardwareVersion = "A";
    if (id.supplierName.empty()) id.supplierName = "Unknown";
    if (id.calibrationId.empty()) id.calibrationId = "CAL_UNKNOWN";
    if (id.repairShopCode.empty()) id.repairShopCode = "SHOP_UNKNOWN";
    if (id.programmingDate.empty()) id.programmingDate = "2023-01-01";
    
    pImpl->cachedIdentification = id;
    pImpl->identificationCached = true;
    
    return id;
}

std::vector<DiagnosticTroubleCode> ECU::readDTCs() {
    std::vector<DiagnosticTroubleCode> dtcs;
    
    // Try OBD-II first
    if (pImpl->obdClient && pImpl->obdClient->isInitialized()) {
        try {
            auto obdDTCs = pImpl->obdClient->readStoredDTCs();
            for (const auto& obdDTC : obdDTCs) {
                DiagnosticTroubleCode dtc;
                dtc.code = obdDTC.code;
                dtc.description = obdDTC.description;
                dtc.status = obdDTC.isConfirmed ? 0x08 : 0x00;
                dtc.isPending = obdDTC.isPending;
                dtc.isConfirmed = obdDTC.isConfirmed;
                dtc.isActive = obdDTC.isConfirmed;
                dtc.timestamp = obdDTC.timestamp;
                dtcs.push_back(dtc);
            }
        } catch (...) {
            // Fall through to UDS
        }
    }
    
    // Try UDS if OBD-II failed or no DTCs found
    if (dtcs.empty() && pImpl->udsClient && pImpl->udsClient->isInitialized()) {
        try {
            auto udsDTCs = pImpl->udsClient->readStoredDTCs();
            for (const auto& udsDTC : udsDTCs) {
                DiagnosticTroubleCode dtc;
                dtc.code = "P" + std::to_string(udsDTC.dtcNumber);
                dtc.description = "UDS DTC";
                dtc.status = udsDTC.statusMask;
                dtc.isPending = (udsDTC.statusMask & 0x04) != 0;
                dtc.isConfirmed = (udsDTC.statusMask & 0x08) != 0;
                dtc.isActive = (udsDTC.statusMask & 0x01) != 0;
                dtc.timestamp = std::chrono::system_clock::now();
                dtcs.push_back(dtc);
            }
        } catch (...) {
            // No DTCs available
        }
    }
    
    return dtcs;
}

std::vector<DiagnosticTroubleCode> ECU::readPendingDTCs() {
    std::vector<DiagnosticTroubleCode> dtcs;
    
    // Try OBD-II first
    if (pImpl->obdClient && pImpl->obdClient->isInitialized()) {
        try {
            auto obdDTCs = pImpl->obdClient->readPendingDTCs();
            for (const auto& obdDTC : obdDTCs) {
                DiagnosticTroubleCode dtc;
                dtc.code = obdDTC.code;
                dtc.description = obdDTC.description;
                dtc.status = 0x04; // Pending
                dtc.isPending = true;
                dtc.isConfirmed = false;
                dtc.isActive = false;
                dtc.timestamp = obdDTC.timestamp;
                dtcs.push_back(dtc);
            }
        } catch (...) {
            // No pending DTCs
        }
    }
    
    return dtcs;
}

void ECU::clearDTCs() {
    bool cleared = false;
    
    // Try OBD-II first
    if (pImpl->obdClient && pImpl->obdClient->isInitialized()) {
        try {
            cleared = pImpl->obdClient->clearDTCs();
        } catch (...) {
            // Fall through to UDS
        }
    }
    
    // Try UDS if OBD-II failed
    if (!cleared && pImpl->udsClient && pImpl->udsClient->isInitialized()) {
        try {
            cleared = pImpl->udsClient->clearDiagnosticInformation();
        } catch (...) {
            // Clear failed
        }
    }
    
    if (!cleared) {
        throw ECUError("Failed to clear DTCs", pImpl->address);
    }
}

LiveDataParameter ECU::readLiveData(uint16_t parameterId) {
    return pImpl->readLiveDataParameter(parameterId);
}

std::vector<LiveDataParameter> ECU::readLiveData(const std::vector<uint16_t>& parameterIds) {
    std::vector<LiveDataParameter> parameters;
    
    for (uint16_t id : parameterIds) {
        auto param = readLiveData(id);
        if (!param.name.empty()) {
            parameters.push_back(param);
        }
    }
    
    return parameters;
}

void ECU::startLiveDataMonitoring(
    const std::vector<uint16_t>& parameterIds,
    std::function<void(const std::vector<LiveDataParameter>&)> callback,
    std::chrono::milliseconds interval) {

    if (pImpl->monitoring) {
        stopLiveDataMonitoring();
    }

    pImpl->monitoringPIDs = parameterIds;
    pImpl->monitoringCallback = callback;
    pImpl->monitoringInterval = interval;
    pImpl->monitoring = true;

    pImpl->monitoringThread = std::thread(&ECU::Impl::monitoringLoop, pImpl.get());
}

void ECU::stopLiveDataMonitoring() {
    pImpl->stopLiveDataMonitoring();
}

void ECU::performActuatorTest(uint16_t actuatorId, uint32_t testValue) {
    if (pImpl->udsClient && pImpl->udsClient->isInitialized()) {
        try {
            auto testData = utils::uint32ToBytes(testValue, true);
            bool success = pImpl->udsClient->inputOutputControl(actuatorId, 0x03, testData); // Short term adjustment
            if (!success) {
                throw ECUError("Actuator test failed", pImpl->address, 0x2F, 0x31);
            }
        } catch (const std::exception& e) {
            throw ECUError("Actuator test error: " + std::string(e.what()), pImpl->address);
        }
    } else {
        throw ECUError("UDS client not available for actuator test", pImpl->address);
    }
}

std::vector<uint8_t> ECU::readDataByIdentifier(uint16_t dataIdentifier) {
    if (pImpl->udsClient && pImpl->udsClient->isInitialized()) {
        try {
            return pImpl->udsClient->readDataByIdentifier(dataIdentifier);
        } catch (const std::exception& e) {
            throw ECUError("Read data by identifier failed: " + std::string(e.what()),
                          pImpl->address, 0x22, 0x31);
        }
    }

    throw ECUError("UDS client not available", pImpl->address);
}

void ECU::writeDataByIdentifier(uint16_t dataIdentifier, const std::vector<uint8_t>& data) {
    if (pImpl->udsClient && pImpl->udsClient->isInitialized()) {
        try {
            bool success = pImpl->udsClient->writeDataByIdentifier(dataIdentifier, data);
            if (!success) {
                throw ECUError("Write data by identifier failed", pImpl->address, 0x2E, 0x31);
            }
        } catch (const std::exception& e) {
            throw ECUError("Write data by identifier error: " + std::string(e.what()),
                          pImpl->address);
        }
    } else {
        throw ECUError("UDS client not available", pImpl->address);
    }
}

void ECU::startDiagnosticSession(uint8_t sessionType) {
    if (pImpl->udsClient && pImpl->udsClient->isInitialized()) {
        try {
            bool success = pImpl->udsClient->startDiagnosticSession(
                static_cast<diagnostics::UDSSession>(sessionType));
            if (!success) {
                throw ECUError("Start diagnostic session failed", pImpl->address, 0x10, 0x31);
            }
        } catch (const std::exception& e) {
            throw ECUError("Start diagnostic session error: " + std::string(e.what()),
                          pImpl->address);
        }
    } else {
        throw ECUError("UDS client not available", pImpl->address);
    }
}

bool ECU::requestSecurityAccess(uint8_t level, const std::vector<uint8_t>& key) {
    if (pImpl->udsClient && pImpl->udsClient->isInitialized()) {
        try {
            if (key.empty()) {
                // Request seed only
                auto seed = pImpl->udsClient->requestSeed(level);
                return !seed.empty();
            } else {
                // Send key
                return pImpl->udsClient->sendKey(level, key);
            }
        } catch (const std::exception& e) {
            auto logger = Logger::getInstance();
            logger->error("Security access error: " + std::string(e.what()));
            return false;
        }
    }

    return false;
}

std::vector<uint8_t> ECU::sendUDSRequest(uint8_t serviceId, const std::vector<uint8_t>& data) {
    if (pImpl->udsClient && pImpl->udsClient->isInitialized()) {
        try {
            diagnostics::UDSMessage request(static_cast<diagnostics::UDSService>(serviceId), data);
            diagnostics::UDSMessage response = pImpl->udsClient->sendRequest(request);

            if (response.isNegativeResponse) {
                throw ECUError("UDS negative response", pImpl->address, serviceId,
                              static_cast<uint8_t>(response.negativeResponseCode));
            }

            return response.data;
        } catch (const ECUError&) {
            throw; // Re-throw ECU errors
        } catch (const std::exception& e) {
            throw ECUError("UDS request error: " + std::string(e.what()), pImpl->address);
        }
    }

    throw ECUError("UDS client not available", pImpl->address);
}

std::string ECU::getTypeString() const {
    return ecuTypeToString(pImpl->type);
}

std::string ECU::toString() const {
    std::ostringstream ss;
    ss << "ECU[Type:" << getTypeString()
       << ", Address:0x" << std::hex << pImpl->address
       << ", Responsive:" << (isResponsive() ? "Yes" : "No") << "]";
    return ss.str();
}

// DiagnosticTroubleCode implementation
char DiagnosticTroubleCode::getCategory() const {
    if (code.empty()) return '?';
    return code[0];
}

bool DiagnosticTroubleCode::isEmissionsRelated() const {
    return getCategory() == 'P';
}

// LiveDataParameter implementation
std::string LiveDataParameter::getValueAsString() const {
    std::ostringstream ss;
    std::visit([&ss](const auto& v) {
        ss << v;
    }, value);
    return ss.str();
}

std::optional<double> LiveDataParameter::getValueAsNumber() const {
    if (std::holds_alternative<int32_t>(value)) {
        return static_cast<double>(std::get<int32_t>(value));
    } else if (std::holds_alternative<uint32_t>(value)) {
        return static_cast<double>(std::get<uint32_t>(value));
    } else if (std::holds_alternative<float>(value)) {
        return static_cast<double>(std::get<float>(value));
    } else if (std::holds_alternative<double>(value)) {
        return static_cast<double>(std::get<double>(value));
    }
    return std::nullopt;
}

// ECUError implementation
ECUError::ECUError(const std::string& message, uint32_t ecuAddress,
                   uint8_t serviceId, uint8_t errorCode)
    : std::runtime_error(message),
      ecuAddress(ecuAddress),
      serviceId(serviceId),
      errorCode(errorCode) {
}

uint32_t ECUError::getECUAddress() const {
    return ecuAddress;
}

uint8_t ECUError::getServiceId() const {
    return serviceId;
}

uint8_t ECUError::getErrorCode() const {
    return errorCode;
}

// Utility functions
std::string ecuTypeToString(ECUType type) {
    switch (type) {
        case ECUType::ENGINE: return "Engine";
        case ECUType::TRANSMISSION: return "Transmission";
        case ECUType::ABS: return "ABS";
        case ECUType::AIRBAG: return "Airbag";
        case ECUType::BODY: return "Body";
        case ECUType::INSTRUMENT: return "Instrument";
        case ECUType::HVAC: return "HVAC";
        case ECUType::GATEWAY: return "Gateway";
        case ECUType::INFOTAINMENT: return "Infotainment";
        case ECUType::TELEMATICS: return "Telematics";
        case ECUType::CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

ECUType stringToECUType(const std::string& str) {
    if (str == "Engine") return ECUType::ENGINE;
    if (str == "Transmission") return ECUType::TRANSMISSION;
    if (str == "ABS") return ECUType::ABS;
    if (str == "Airbag") return ECUType::AIRBAG;
    if (str == "Body") return ECUType::BODY;
    if (str == "Instrument") return ECUType::INSTRUMENT;
    if (str == "HVAC") return ECUType::HVAC;
    if (str == "Gateway") return ECUType::GATEWAY;
    if (str == "Infotainment") return ECUType::INFOTAINMENT;
    if (str == "Telematics") return ECUType::TELEMATICS;
    return ECUType::CUSTOM;
}

} // namespace fmus
