#include <fmus/diagnostics/uds.h>
#include <fmus/logger.h>
#include <fmus/utils.h>
#include <fmus/thread_pool.h>
#include <sstream>
#include <iomanip>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace fmus {
namespace diagnostics {

// UDSMessage implementation
protocols::CANMessage UDSMessage::toCANMessage(uint32_t requestId) const {
    protocols::CANMessage canMsg;
    canMsg.id = requestId;
    canMsg.data.push_back(static_cast<uint8_t>(service));
    canMsg.data.insert(canMsg.data.end(), data.begin(), data.end());
    canMsg.timestamp = timestamp;
    return canMsg;
}

UDSMessage UDSMessage::fromCANMessage(const protocols::CANMessage& canMsg) {
    UDSMessage udsMsg;
    
    if (canMsg.data.empty()) {
        return udsMsg;
    }
    
    uint8_t serviceId = canMsg.data[0];
    
    // Check for negative response
    if (serviceId == 0x7F) {
        udsMsg.isNegativeResponse = true;
        if (canMsg.data.size() >= 3) {
            udsMsg.service = static_cast<UDSService>(canMsg.data[1]);
            udsMsg.negativeResponseCode = static_cast<UDSNegativeResponse>(canMsg.data[2]);
        }
        if (canMsg.data.size() > 3) {
            udsMsg.data.assign(canMsg.data.begin() + 3, canMsg.data.end());
        }
    }
    // Check for positive response
    else if (serviceId >= 0x40 && serviceId <= 0x7E) {
        udsMsg.isResponse = true;
        udsMsg.service = static_cast<UDSService>(serviceId - 0x40);
        if (canMsg.data.size() > 1) {
            udsMsg.data.assign(canMsg.data.begin() + 1, canMsg.data.end());
        }
    }
    // Request message
    else {
        udsMsg.service = static_cast<UDSService>(serviceId);
        if (canMsg.data.size() > 1) {
            udsMsg.data.assign(canMsg.data.begin() + 1, canMsg.data.end());
        }
    }
    
    udsMsg.timestamp = canMsg.timestamp;
    return udsMsg;
}

std::string UDSMessage::toString() const {
    std::ostringstream ss;
    ss << "UDS[";
    
    if (isNegativeResponse) {
        ss << "NRC:" << udsNegativeResponseToString(negativeResponseCode);
    } else if (isResponse) {
        ss << "RSP:" << udsServiceToString(service);
    } else {
        ss << "REQ:" << udsServiceToString(service);
    }
    
    if (!data.empty()) {
        ss << " DATA:" << utils::bytesToHex(data);
    }
    
    ss << "]";
    return ss.str();
}

bool UDSMessage::isValid() const {
    // Basic validation
    if (isNegativeResponse && negativeResponseCode == UDSNegativeResponse::GENERAL_REJECT) {
        return false;
    }
    
    return true;
}

// UDSConfig implementation
std::string UDSConfig::toString() const {
    std::ostringstream ss;
    ss << "UDSConfig[ReqID:0x" << std::hex << requestId
       << ", RspID:0x" << responseId
       << ", Timeout:" << std::dec << timeout << "ms"
       << ", P2:" << p2ClientMax << "ms"
       << ", P2*:" << p2StarClientMax << "ms"
       << ", ExtAddr:" << (extendedAddressing ? "Yes" : "No") << "]";
    return ss.str();
}

// UDSClient implementation
class UDSClient::Impl {
public:
    UDSConfig config;
    std::shared_ptr<protocols::CANProtocol> canProtocol;
    UDSSession currentSession = UDSSession::DEFAULT;
    std::atomic<bool> initialized{false};
    
    // Statistics
    Statistics stats;
    mutable std::mutex statsMutex;
    
    // Error handling
    ErrorInfo lastError;
    mutable std::mutex errorMutex;
    
    // Request/Response handling
    std::mutex requestMutex;
    std::condition_variable responseCondition;
    UDSMessage pendingResponse;
    bool responseReceived = false;
    
    Impl() {
        stats.startTime = std::chrono::system_clock::now();
    }
    
    void updateStats(bool isRequest, bool isNegative = false, bool isTimeout = false) {
        std::lock_guard<std::mutex> lock(statsMutex);
        if (isRequest) {
            stats.requestsSent++;
        } else {
            stats.responsesReceived++;
            if (isNegative) {
                stats.negativeResponses++;
            }
        }
        if (isTimeout) {
            stats.timeouts++;
        }
    }
    
    void setLastError(UDSNegativeResponse code, const std::string& description) {
        std::lock_guard<std::mutex> lock(errorMutex);
        lastError.hasError = true;
        lastError.errorCode = code;
        lastError.description = description;
        lastError.timestamp = std::chrono::system_clock::now();
    }
    
    void clearLastError() {
        std::lock_guard<std::mutex> lock(errorMutex);
        lastError.hasError = false;
    }
    
    void onCANMessage(const protocols::CANMessage& canMsg) {
        if (canMsg.id != config.responseId) {
            return;
        }
        
        UDSMessage udsMsg = UDSMessage::fromCANMessage(canMsg);
        
        std::lock_guard<std::mutex> lock(requestMutex);
        pendingResponse = udsMsg;
        responseReceived = true;
        responseCondition.notify_one();
    }
};

UDSClient::UDSClient() : pImpl(std::make_unique<Impl>()) {}

UDSClient::~UDSClient() = default;

bool UDSClient::initialize(const UDSConfig& config, std::shared_ptr<protocols::CANProtocol> canProtocol) {
    auto logger = Logger::getInstance();
    logger->info("Initializing UDS client: " + config.toString());
    
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
        logger->error("Failed to start CAN monitoring for UDS");
        return false;
    }
    
    pImpl->initialized = true;
    logger->info("UDS client initialized successfully");
    return true;
}

void UDSClient::shutdown() {
    if (pImpl->canProtocol && pImpl->canProtocol->isMonitoring()) {
        pImpl->canProtocol->stopMonitoring();
    }
    
    pImpl->initialized = false;
    
    auto logger = Logger::getInstance();
    logger->info("UDS client shutdown");
}

bool UDSClient::isInitialized() const {
    return pImpl->initialized;
}

UDSMessage UDSClient::sendRequest(const UDSMessage& request) {
    if (!pImpl->initialized) {
        UDSMessage errorResponse;
        errorResponse.isNegativeResponse = true;
        errorResponse.negativeResponseCode = UDSNegativeResponse::CONDITIONS_NOT_CORRECT;
        return errorResponse;
    }
    
    auto logger = Logger::getInstance();
    logger->debug("Sending UDS request: " + request.toString());
    
    // Convert to CAN message and send
    auto canMsg = request.toCANMessage(pImpl->config.requestId);
    
    {
        std::unique_lock<std::mutex> lock(pImpl->requestMutex);
        pImpl->responseReceived = false;
    }
    
    if (!pImpl->canProtocol->sendMessage(canMsg)) {
        logger->error("Failed to send UDS request");
        pImpl->setLastError(UDSNegativeResponse::GENERAL_REJECT, "Failed to send CAN message");
        
        UDSMessage errorResponse;
        errorResponse.isNegativeResponse = true;
        errorResponse.negativeResponseCode = UDSNegativeResponse::GENERAL_REJECT;
        return errorResponse;
    }
    
    pImpl->updateStats(true);
    
    // Wait for response
    std::unique_lock<std::mutex> lock(pImpl->requestMutex);
    bool received = pImpl->responseCondition.wait_for(lock, 
        std::chrono::milliseconds(pImpl->config.timeout),
        [this] { return pImpl->responseReceived; });
    
    if (!received) {
        logger->warning("UDS request timeout");
        pImpl->updateStats(false, false, true);
        pImpl->setLastError(UDSNegativeResponse::GENERAL_REJECT, "Request timeout");
        
        UDSMessage timeoutResponse;
        timeoutResponse.isNegativeResponse = true;
        timeoutResponse.negativeResponseCode = UDSNegativeResponse::GENERAL_REJECT;
        return timeoutResponse;
    }
    
    UDSMessage response = pImpl->pendingResponse;
    pImpl->updateStats(false, response.isNegativeResponse);
    
    if (response.isNegativeResponse) {
        pImpl->setLastError(response.negativeResponseCode, 
                           udsNegativeResponseToString(response.negativeResponseCode));
    } else {
        pImpl->clearLastError();
    }
    
    logger->debug("Received UDS response: " + response.toString());
    return response;
}

void UDSClient::sendRequestAsync(const UDSMessage& request, 
                                std::function<void(const UDSMessage&)> callback) {
    auto threadPool = getGlobalThreadPool();
    threadPool->enqueue([this, request, callback]() {
        UDSMessage response = sendRequest(request);
        callback(response);
    });
}

// Diagnostic Session Control (0x10)
bool UDSClient::startDiagnosticSession(UDSSession session) {
    UDSMessage request(UDSService::DIAGNOSTIC_SESSION_CONTROL, {static_cast<uint8_t>(session)});
    UDSMessage response = sendRequest(request);
    
    if (!response.isNegativeResponse) {
        pImpl->currentSession = session;
        return true;
    }
    
    return false;
}

UDSSession UDSClient::getCurrentSession() const {
    return pImpl->currentSession;
}

// ECU Reset (0x11)
bool UDSClient::resetECU(uint8_t resetType) {
    UDSMessage request(UDSService::ECU_RESET, {resetType});
    UDSMessage response = sendRequest(request);
    
    return !response.isNegativeResponse;
}

// Security Access (0x27)
std::vector<uint8_t> UDSClient::requestSeed(uint8_t level) {
    UDSMessage request(UDSService::SECURITY_ACCESS, {level});
    UDSMessage response = sendRequest(request);
    
    if (!response.isNegativeResponse && response.data.size() > 1) {
        // Return seed (skip the sub-function byte)
        return std::vector<uint8_t>(response.data.begin() + 1, response.data.end());
    }
    
    return {};
}

bool UDSClient::sendKey(uint8_t level, const std::vector<uint8_t>& key) {
    std::vector<uint8_t> requestData = {static_cast<uint8_t>(level + 1)};
    requestData.insert(requestData.end(), key.begin(), key.end());
    
    UDSMessage request(UDSService::SECURITY_ACCESS, requestData);
    UDSMessage response = sendRequest(request);
    
    return !response.isNegativeResponse;
}

bool UDSClient::unlockSecurityAccess(uint8_t level, const std::vector<uint8_t>& key) {
    // First request seed
    auto seed = requestSeed(level);
    if (seed.empty()) {
        return false;
    }
    
    // Then send key
    return sendKey(level, key);
}

// Tester Present (0x3E)
bool UDSClient::sendTesterPresent(bool suppressResponse) {
    uint8_t subFunction = suppressResponse ? 0x80 : 0x00;
    UDSMessage request(UDSService::TESTER_PRESENT, {subFunction});
    UDSMessage response = sendRequest(request);
    
    return !response.isNegativeResponse;
}

// Read Data by Identifier (0x22)
std::vector<uint8_t> UDSClient::readDataByIdentifier(uint16_t dataIdentifier) {
    auto didBytes = utils::uint16ToBytes(dataIdentifier, true); // Big endian
    UDSMessage request(UDSService::READ_DATA_BY_IDENTIFIER, didBytes);
    UDSMessage response = sendRequest(request);
    
    if (!response.isNegativeResponse && response.data.size() > 2) {
        // Return data (skip the DID echo)
        return std::vector<uint8_t>(response.data.begin() + 2, response.data.end());
    }
    
    return {};
}

std::map<uint16_t, std::vector<uint8_t>> UDSClient::readMultipleDataByIdentifier(
    const std::vector<uint16_t>& identifiers) {
    
    std::map<uint16_t, std::vector<uint8_t>> results;
    
    for (uint16_t did : identifiers) {
        auto data = readDataByIdentifier(did);
        if (!data.empty()) {
            results[did] = data;
        }
    }
    
    return results;
}

// Write Data by Identifier (0x2E)
bool UDSClient::writeDataByIdentifier(uint16_t dataIdentifier, const std::vector<uint8_t>& data) {
    auto didBytes = utils::uint16ToBytes(dataIdentifier, true);
    std::vector<uint8_t> requestData = didBytes;
    requestData.insert(requestData.end(), data.begin(), data.end());
    
    UDSMessage request(UDSService::WRITE_DATA_BY_IDENTIFIER, requestData);
    UDSMessage response = sendRequest(request);
    
    return !response.isNegativeResponse;
}

// Clear Diagnostic Information (0x14)
bool UDSClient::clearDiagnosticInformation(uint32_t groupOfDTC) {
    auto dtcBytes = utils::uint32ToBytes(groupOfDTC, true);
    // Only use the first 3 bytes for DTC group
    std::vector<uint8_t> requestData(dtcBytes.begin(), dtcBytes.begin() + 3);
    
    UDSMessage request(UDSService::CLEAR_DIAGNOSTIC_INFORMATION, requestData);
    UDSMessage response = sendRequest(request);
    
    return !response.isNegativeResponse;
}

// Read DTC Information (0x19)
std::vector<UDSClient::DTCInfo> UDSClient::readDTCInformation(uint8_t subFunction, uint8_t statusMask) {
    std::vector<uint8_t> requestData = {subFunction, statusMask};
    UDSMessage request(UDSService::READ_DTC_INFORMATION, requestData);
    UDSMessage response = sendRequest(request);
    
    std::vector<DTCInfo> dtcs;
    
    if (!response.isNegativeResponse && response.data.size() > 2) {
        // Parse DTC data (simplified - real implementation would be more complex)
        size_t offset = 2; // Skip sub-function and status availability mask
        
        while (offset + 4 <= response.data.size()) {
            DTCInfo dtc;
            dtc.dtcNumber = utils::bytesToUint32(response.data, offset, true) >> 8; // 24-bit DTC
            dtc.statusMask = response.data[offset + 3];
            dtcs.push_back(dtc);
            offset += 4;
        }
    }
    
    return dtcs;
}

std::vector<UDSClient::DTCInfo> UDSClient::readStoredDTCs() {
    return readDTCInformation(0x02, 0x08); // reportDTCByStatusMask, confirmedDTC
}

std::vector<UDSClient::DTCInfo> UDSClient::readPendingDTCs() {
    return readDTCInformation(0x02, 0x04); // reportDTCByStatusMask, pendingDTC
}

std::vector<UDSClient::DTCInfo> UDSClient::readConfirmedDTCs() {
    return readDTCInformation(0x02, 0x08); // reportDTCByStatusMask, confirmedDTC
}

// Routine Control (0x31)
std::vector<uint8_t> UDSClient::routineControl(RoutineControlType controlType, 
                                              uint16_t routineIdentifier,
                                              const std::vector<uint8_t>& parameters) {
    auto ridBytes = utils::uint16ToBytes(routineIdentifier, true);
    std::vector<uint8_t> requestData = {static_cast<uint8_t>(controlType)};
    requestData.insert(requestData.end(), ridBytes.begin(), ridBytes.end());
    requestData.insert(requestData.end(), parameters.begin(), parameters.end());
    
    UDSMessage request(UDSService::ROUTINE_CONTROL, requestData);
    UDSMessage response = sendRequest(request);
    
    if (!response.isNegativeResponse && response.data.size() > 3) {
        // Return routine results (skip control type and RID echo)
        return std::vector<uint8_t>(response.data.begin() + 3, response.data.end());
    }
    
    return {};
}

// Input/Output Control (0x2F)
bool UDSClient::inputOutputControl(uint16_t dataIdentifier, uint8_t controlParameter,
                                  const std::vector<uint8_t>& controlState) {
    auto didBytes = utils::uint16ToBytes(dataIdentifier, true);
    std::vector<uint8_t> requestData = didBytes;
    requestData.push_back(controlParameter);
    requestData.insert(requestData.end(), controlState.begin(), controlState.end());
    
    UDSMessage request(UDSService::INPUT_OUTPUT_CONTROL_BY_IDENTIFIER, requestData);
    UDSMessage response = sendRequest(request);
    
    return !response.isNegativeResponse;
}

UDSClient::ErrorInfo UDSClient::getLastError() const {
    std::lock_guard<std::mutex> lock(pImpl->errorMutex);
    return pImpl->lastError;
}

UDSClient::Statistics UDSClient::getStatistics() const {
    std::lock_guard<std::mutex> lock(pImpl->statsMutex);
    return pImpl->stats;
}

void UDSClient::resetStatistics() {
    std::lock_guard<std::mutex> lock(pImpl->statsMutex);
    pImpl->stats = Statistics{};
    pImpl->stats.startTime = std::chrono::system_clock::now();
}

UDSConfig UDSClient::getConfiguration() const {
    return pImpl->config;
}

// Utility functions
std::string udsServiceToString(UDSService service) {
    switch (service) {
        case UDSService::DIAGNOSTIC_SESSION_CONTROL: return "DiagnosticSessionControl";
        case UDSService::ECU_RESET: return "ECUReset";
        case UDSService::SECURITY_ACCESS: return "SecurityAccess";
        case UDSService::COMMUNICATION_CONTROL: return "CommunicationControl";
        case UDSService::TESTER_PRESENT: return "TesterPresent";
        case UDSService::READ_DATA_BY_IDENTIFIER: return "ReadDataByIdentifier";
        case UDSService::WRITE_DATA_BY_IDENTIFIER: return "WriteDataByIdentifier";
        case UDSService::CLEAR_DIAGNOSTIC_INFORMATION: return "ClearDiagnosticInformation";
        case UDSService::READ_DTC_INFORMATION: return "ReadDTCInformation";
        case UDSService::INPUT_OUTPUT_CONTROL_BY_IDENTIFIER: return "InputOutputControlByIdentifier";
        case UDSService::ROUTINE_CONTROL: return "RoutineControl";
        case UDSService::REQUEST_DOWNLOAD: return "RequestDownload";
        case UDSService::REQUEST_UPLOAD: return "RequestUpload";
        case UDSService::TRANSFER_DATA: return "TransferData";
        case UDSService::REQUEST_TRANSFER_EXIT: return "RequestTransferExit";
        default: return "Unknown";
    }
}

std::string udsSessionToString(UDSSession session) {
    switch (session) {
        case UDSSession::DEFAULT: return "Default";
        case UDSSession::PROGRAMMING: return "Programming";
        case UDSSession::EXTENDED_DIAGNOSTIC: return "ExtendedDiagnostic";
        case UDSSession::SAFETY_SYSTEM_DIAGNOSTIC: return "SafetySystemDiagnostic";
        default: return "Unknown";
    }
}

std::string udsNegativeResponseToString(UDSNegativeResponse nrc) {
    switch (nrc) {
        case UDSNegativeResponse::GENERAL_REJECT: return "GeneralReject";
        case UDSNegativeResponse::SERVICE_NOT_SUPPORTED: return "ServiceNotSupported";
        case UDSNegativeResponse::SUB_FUNCTION_NOT_SUPPORTED: return "SubFunctionNotSupported";
        case UDSNegativeResponse::INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT: return "IncorrectMessageLengthOrInvalidFormat";
        case UDSNegativeResponse::RESPONSE_TOO_LONG: return "ResponseTooLong";
        case UDSNegativeResponse::BUSY_REPEAT_REQUEST: return "BusyRepeatRequest";
        case UDSNegativeResponse::CONDITIONS_NOT_CORRECT: return "ConditionsNotCorrect";
        case UDSNegativeResponse::REQUEST_SEQUENCE_ERROR: return "RequestSequenceError";
        case UDSNegativeResponse::NO_RESPONSE_FROM_SUBNET_COMPONENT: return "NoResponseFromSubnetComponent";
        case UDSNegativeResponse::FAILURE_PREVENTS_EXECUTION_OF_REQUESTED_ACTION: return "FailurePreventsExecutionOfRequestedAction";
        case UDSNegativeResponse::REQUEST_OUT_OF_RANGE: return "RequestOutOfRange";
        case UDSNegativeResponse::SECURITY_ACCESS_DENIED: return "SecurityAccessDenied";
        case UDSNegativeResponse::INVALID_KEY: return "InvalidKey";
        case UDSNegativeResponse::EXCEED_NUMBER_OF_ATTEMPTS: return "ExceedNumberOfAttempts";
        case UDSNegativeResponse::REQUIRED_TIME_DELAY_NOT_EXPIRED: return "RequiredTimeDelayNotExpired";
        case UDSNegativeResponse::UPLOAD_DOWNLOAD_NOT_ACCEPTED: return "UploadDownloadNotAccepted";
        case UDSNegativeResponse::TRANSFER_DATA_SUSPENDED: return "TransferDataSuspended";
        case UDSNegativeResponse::GENERAL_PROGRAMMING_FAILURE: return "GeneralProgrammingFailure";
        case UDSNegativeResponse::WRONG_BLOCK_SEQUENCE_COUNTER: return "WrongBlockSequenceCounter";
        case UDSNegativeResponse::REQUEST_CORRECTLY_RECEIVED_RESPONSE_PENDING: return "RequestCorrectlyReceivedResponsePending";
        case UDSNegativeResponse::SUB_FUNCTION_NOT_SUPPORTED_IN_ACTIVE_SESSION: return "SubFunctionNotSupportedInActiveSession";
        case UDSNegativeResponse::SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION: return "ServiceNotSupportedInActiveSession";
        default: return "Unknown";
    }
}

bool isValidDataIdentifier(uint16_t did) {
    // Basic validation - real implementation would check against specific ranges
    return did != 0x0000;
}

std::vector<uint8_t> encodeDataIdentifier(uint16_t did) {
    return utils::uint16ToBytes(did, true);
}

uint16_t decodeDataIdentifier(const std::vector<uint8_t>& data, size_t offset) {
    return utils::bytesToUint16(data, offset, true);
}

} // namespace diagnostics
} // namespace fmus
