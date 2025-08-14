#include <fmus/protocols/can.h>
#include <fmus/logger.h>
#include <fmus/utils.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <mutex>
#include <atomic>

namespace fmus {
namespace protocols {

// CANMessage implementation
bool CANMessage::isValid() const {
    // Check ID range
    if (extended) {
        if (id > 0x1FFFFFFF) return false; // 29-bit max
    } else {
        if (id > 0x7FF) return false; // 11-bit max
    }
    
    // Check data length (CAN 2.0 allows 0-8 bytes)
    if (data.size() > 8) return false;
    
    return true;
}

std::string CANMessage::toString() const {
    std::ostringstream ss;
    ss << "CAN[";
    
    if (extended) {
        ss << "EXT:0x" << std::hex << std::setw(8) << std::setfill('0') << id;
    } else {
        ss << "STD:0x" << std::hex << std::setw(3) << std::setfill('0') << id;
    }
    
    if (rtr) {
        ss << " RTR";
    } else {
        ss << " DATA:" << utils::bytesToHex(data);
    }
    
    ss << "]";
    return ss.str();
}

j2534::Message CANMessage::toJ2534Message() const {
    j2534::Message msg;
    msg.protocol = j2534::Protocol::CAN;
    msg.id = id;
    msg.data = data;
    msg.flags = 0;
    
    if (extended) {
        msg.flags |= 0x04; // CAN_29BIT_ID flag
    }
    
    if (rtr) {
        msg.flags |= 0x02; // RTR flag
    }
    
    return msg;
}

CANMessage CANMessage::fromJ2534Message(const j2534::Message& msg) {
    CANMessage canMsg;
    canMsg.id = msg.id;
    canMsg.data = msg.data;
    canMsg.extended = (msg.flags & 0x04) != 0;
    canMsg.rtr = (msg.flags & 0x02) != 0;
    canMsg.timestamp = std::chrono::system_clock::now();
    
    return canMsg;
}

// CANConfig implementation
std::string CANConfig::toString() const {
    std::ostringstream ss;
    ss << "CANConfig[BaudRate:" << baudRate
       << ", ListenOnly:" << (listenOnly ? "Yes" : "No")
       << ", Loopback:" << (loopback ? "Yes" : "No")
       << ", ExtendedFrames:" << (extendedFrames ? "Yes" : "No")
       << ", TxTimeout:" << txTimeout << "ms"
       << ", RxTimeout:" << rxTimeout << "ms]";
    return ss.str();
}

// CANFilter implementation
bool CANFilter::matches(const CANMessage& message) const {
    // Check frame type compatibility
    if (extended != message.extended) {
        return false;
    }
    
    // Apply mask and compare
    uint32_t maskedId = message.id & mask;
    uint32_t filterId = id & mask;
    
    bool matches = (maskedId == filterId);
    
    // Return based on pass-through setting
    return passThrough ? matches : !matches;
}

j2534::Filter CANFilter::toJ2534Filter() const {
    j2534::Filter filter;
    filter.protocol = j2534::Protocol::CAN;
    filter.filterType = passThrough ? j2534::FilterType::PASS_FILTER : j2534::FilterType::BLOCK_FILTER;
    filter.maskId = mask;
    filter.patternId = id;
    filter.flags = extended ? 0x04 : 0x00;
    
    return filter;
}

std::string CANFilter::toString() const {
    std::ostringstream ss;
    ss << "CANFilter[";
    
    if (extended) {
        ss << "EXT:0x" << std::hex << std::setw(8) << std::setfill('0') << id;
        ss << "/0x" << std::setw(8) << mask;
    } else {
        ss << "STD:0x" << std::hex << std::setw(3) << std::setfill('0') << id;
        ss << "/0x" << std::setw(3) << mask;
    }
    
    ss << ", " << (passThrough ? "PASS" : "BLOCK") << "]";
    return ss.str();
}

// CANProtocol implementation
class CANProtocol::Impl {
public:
    CANConfig config;
    std::vector<CANFilter> filters;
    Statistics stats;
    std::atomic<bool> initialized{false};
    std::atomic<bool> monitoring{false};
    std::function<void(const CANMessage&)> monitorCallback;
    std::thread monitorThread;
    mutable std::mutex filtersMutex;
    mutable std::mutex statsMutex;
    
    Impl() {
        stats.startTime = std::chrono::system_clock::now();
    }
    
    ~Impl() {
        if (monitoring) {
            stopMonitoring();
        }
    }
    
    void stopMonitoring() {
        monitoring = false;
        if (monitorThread.joinable()) {
            monitorThread.join();
        }
    }
    
    void monitoringLoop() {
        auto logger = Logger::getInstance();
        logger->debug("CAN monitoring thread started");
        
        while (monitoring) {
            try {
                // In a real implementation, this would read from the J2534 device
                // For now, we'll just sleep to simulate monitoring
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                
                // Simulate receiving a message occasionally
                if (monitorCallback && (rand() % 1000) == 0) {
                    CANMessage msg(0x7E8, {0x06, 0x41, 0x00, 0xBE, 0x3F, 0xB8, 0x13});
                    
                    // Apply filters
                    bool passFilter = filters.empty(); // Pass if no filters
                    for (const auto& filter : filters) {
                        if (filter.matches(msg)) {
                            passFilter = true;
                            break;
                        }
                    }
                    
                    if (passFilter) {
                        {
                            std::lock_guard<std::mutex> lock(statsMutex);
                            stats.messagesReceived++;
                            stats.filtersApplied++;
                        }
                        monitorCallback(msg);
                    }
                }
                
            } catch (const std::exception& e) {
                logger->error("CAN monitoring error: " + std::string(e.what()));
                std::lock_guard<std::mutex> lock(statsMutex);
                stats.errorsDetected++;
            }
        }
        
        logger->debug("CAN monitoring thread stopped");
    }
};

CANProtocol::CANProtocol() : pImpl(std::make_unique<Impl>()) {}

CANProtocol::~CANProtocol() = default;

bool CANProtocol::initialize(const CANConfig& config) {
    auto logger = Logger::getInstance();
    logger->info("Initializing CAN protocol: " + config.toString());
    
    if (!isValidCANBaudRate(config.baudRate)) {
        logger->error("Invalid CAN baud rate: " + std::to_string(config.baudRate));
        return false;
    }
    
    pImpl->config = config;
    pImpl->initialized = true;
    
    logger->info("CAN protocol initialized successfully");
    return true;
}

void CANProtocol::shutdown() {
    if (pImpl->monitoring) {
        stopMonitoring();
    }
    
    pImpl->initialized = false;
    
    auto logger = Logger::getInstance();
    logger->info("CAN protocol shutdown");
}

bool CANProtocol::isInitialized() const {
    return pImpl->initialized;
}

bool CANProtocol::sendMessage(const CANMessage& message) {
    if (!pImpl->initialized) {
        return false;
    }
    
    if (!message.isValid()) {
        auto logger = Logger::getInstance();
        logger->error("Invalid CAN message: " + message.toString());
        return false;
    }
    
    auto logger = Logger::getInstance();
    logger->debug("Sending CAN message: " + message.toString());
    
    // In a real implementation, this would send via J2534
    // For now, just simulate success
    
    {
        std::lock_guard<std::mutex> lock(pImpl->statsMutex);
        pImpl->stats.messagesSent++;
    }
    
    return true;
}

bool CANProtocol::sendMessages(const std::vector<CANMessage>& messages) {
    bool allSuccess = true;
    
    for (const auto& msg : messages) {
        if (!sendMessage(msg)) {
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

std::vector<CANMessage> CANProtocol::receiveMessages(uint32_t timeout) {
    std::vector<CANMessage> messages;
    
    if (!pImpl->initialized) {
        return messages;
    }
    
    // In a real implementation, this would receive from J2534
    // For now, return empty vector
    
    return messages;
}

bool CANProtocol::addFilter(const CANFilter& filter) {
    std::lock_guard<std::mutex> lock(pImpl->filtersMutex);
    
    auto logger = Logger::getInstance();
    logger->debug("Adding CAN filter: " + filter.toString());
    
    pImpl->filters.push_back(filter);
    return true;
}

bool CANProtocol::removeFilter(const CANFilter& filter) {
    std::lock_guard<std::mutex> lock(pImpl->filtersMutex);
    
    auto it = std::find_if(pImpl->filters.begin(), pImpl->filters.end(),
        [&filter](const CANFilter& f) {
            return f.id == filter.id && f.mask == filter.mask && f.extended == filter.extended;
        });
    
    if (it != pImpl->filters.end()) {
        pImpl->filters.erase(it);
        return true;
    }
    
    return false;
}

void CANProtocol::clearFilters() {
    std::lock_guard<std::mutex> lock(pImpl->filtersMutex);
    pImpl->filters.clear();
}

std::vector<CANFilter> CANProtocol::getFilters() const {
    std::lock_guard<std::mutex> lock(pImpl->filtersMutex);
    return pImpl->filters;
}

bool CANProtocol::startMonitoring(std::function<void(const CANMessage&)> callback) {
    if (!pImpl->initialized || pImpl->monitoring) {
        return false;
    }
    
    pImpl->monitorCallback = callback;
    pImpl->monitoring = true;
    pImpl->monitorThread = std::thread(&CANProtocol::Impl::monitoringLoop, pImpl.get());
    
    return true;
}

void CANProtocol::stopMonitoring() {
    pImpl->stopMonitoring();
}

bool CANProtocol::isMonitoring() const {
    return pImpl->monitoring;
}

CANProtocol::Statistics CANProtocol::getStatistics() const {
    std::lock_guard<std::mutex> lock(pImpl->statsMutex);
    return pImpl->stats;
}

void CANProtocol::resetStatistics() {
    std::lock_guard<std::mutex> lock(pImpl->statsMutex);
    pImpl->stats = Statistics{};
    pImpl->stats.startTime = std::chrono::system_clock::now();
}

CANConfig CANProtocol::getConfiguration() const {
    return pImpl->config;
}

bool CANProtocol::updateConfiguration(const CANConfig& config) {
    if (pImpl->monitoring) {
        return false; // Cannot update while monitoring
    }
    
    bool wasInitialized = pImpl->initialized;
    if (wasInitialized) {
        shutdown();
    }
    
    bool result = initialize(config);
    return result;
}

// Utility functions
bool isValidCANId(uint32_t id, bool extended) {
    if (extended) {
        return id <= 0x1FFFFFFF; // 29-bit max
    } else {
        return id <= 0x7FF; // 11-bit max
    }
}

bool isValidCANBaudRate(uint32_t baudRate) {
    std::vector<uint32_t> validRates = getStandardCANBaudRates();
    return std::find(validRates.begin(), validRates.end(), baudRate) != validRates.end();
}

std::vector<uint32_t> getStandardCANBaudRates() {
    return {
        10000,   // 10 kbps
        20000,   // 20 kbps
        50000,   // 50 kbps
        100000,  // 100 kbps
        125000,  // 125 kbps
        250000,  // 250 kbps
        500000,  // 500 kbps
        800000,  // 800 kbps
        1000000  // 1 Mbps
    };
}

std::string canIdToString(uint32_t id, bool extended) {
    std::ostringstream ss;
    ss << "0x" << std::hex << std::uppercase;
    
    if (extended) {
        ss << std::setw(8) << std::setfill('0') << id;
    } else {
        ss << std::setw(3) << std::setfill('0') << id;
    }
    
    return ss.str();
}

uint32_t stringToCANId(const std::string& str) {
    try {
        if (str.substr(0, 2) == "0x" || str.substr(0, 2) == "0X") {
            return std::stoul(str.substr(2), nullptr, 16);
        } else {
            return std::stoul(str, nullptr, 16);
        }
    } catch (...) {
        return 0;
    }
}

} // namespace protocols
} // namespace fmus
