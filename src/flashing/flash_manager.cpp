#include <fmus/flashing/flash_manager.h>
#include <fmus/logger.h>
#include <fmus/utils.h>
#include <fmus/thread_pool.h>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>

namespace fmus {
namespace flashing {

// FlashRegion implementation
std::string FlashRegion::toString() const {
    std::ostringstream ss;
    ss << "FlashRegion[" << name 
       << ", 0x" << std::hex << startAddress 
       << "-0x" << endAddress
       << ", Size:" << std::dec << getSize() 
       << ", Block:" << blockSize
       << ", Protected:" << (isProtected ? "Yes" : "No") << "]";
    return ss.str();
}

// FlashBlock implementation
std::string FlashBlock::toString() const {
    std::ostringstream ss;
    ss << "FlashBlock[Addr:0x" << std::hex << address 
       << ", Size:" << std::dec << data.size()
       << ", Checksum:0x" << std::hex << checksum
       << ", Verified:" << (isVerified ? "Yes" : "No") << "]";
    return ss.str();
}

// FlashFile implementation
bool FlashFile::loadFromFile(const std::string& filePath) {
    auto logger = Logger::getInstance();
    logger->info("Loading flash file: " + filePath);
    
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        logger->error("Failed to open flash file: " + filePath);
        return false;
    }
    
    // Read file data
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
    file.close();
    
    // Determine format from file extension
    FlashFileFormat detectedFormat = FlashFileFormat::BINARY;
    std::string extension = filePath.substr(filePath.find_last_of('.') + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension == "hex") {
        detectedFormat = FlashFileFormat::INTEL_HEX;
    } else if (extension == "s19" || extension == "s28" || extension == "s37" || extension == "srec") {
        detectedFormat = FlashFileFormat::MOTOROLA_S_RECORD;
    } else if (extension == "bin") {
        detectedFormat = FlashFileFormat::BINARY;
    } else if (extension == "elf") {
        detectedFormat = FlashFileFormat::ELF;
    }
    
    metadata["filename"] = filePath;
    metadata["size"] = std::to_string(data.size());
    
    return loadFromData(data, detectedFormat);
}

bool FlashFile::loadFromData(const std::vector<uint8_t>& data, FlashFileFormat fmt) {
    auto logger = Logger::getInstance();
    
    format = fmt;
    blocks.clear();
    
    bool success = false;
    switch (format) {
        case FlashFileFormat::INTEL_HEX:
            success = parseIntelHex(data);
            break;
        case FlashFileFormat::MOTOROLA_S_RECORD:
            success = parseMotorolaS(data);
            break;
        case FlashFileFormat::BINARY:
            success = parseBinary(data);
            break;
        case FlashFileFormat::ELF:
            success = parseELF(data);
            break;
        default:
            logger->error("Unsupported flash file format");
            return false;
    }
    
    if (success) {
        logger->info("Flash file loaded successfully: " + std::to_string(blocks.size()) + " blocks");
        metadata["blocks"] = std::to_string(blocks.size());
        metadata["total_size"] = std::to_string(getTotalSize());
        
        auto range = getAddressRange();
        metadata["start_address"] = "0x" + utils::bytesToHex(utils::uint32ToBytes(range.first, true));
        metadata["end_address"] = "0x" + utils::bytesToHex(utils::uint32ToBytes(range.second, true));
    }
    
    return success;
}

std::vector<FlashBlock> FlashFile::getBlocksForRegion(const FlashRegion& region) const {
    std::vector<FlashBlock> regionBlocks;
    
    for (const auto& block : blocks) {
        if (region.contains(block.address)) {
            regionBlocks.push_back(block);
        }
    }
    
    return regionBlocks;
}

size_t FlashFile::getTotalSize() const {
    size_t totalSize = 0;
    for (const auto& block : blocks) {
        totalSize += block.data.size();
    }
    return totalSize;
}

std::pair<uint32_t, uint32_t> FlashFile::getAddressRange() const {
    if (blocks.empty()) {
        return {0, 0};
    }
    
    uint32_t minAddr = blocks[0].address;
    uint32_t maxAddr = blocks[0].address + blocks[0].data.size() - 1;
    
    for (const auto& block : blocks) {
        minAddr = std::min(minAddr, block.address);
        maxAddr = std::max(maxAddr, block.address + static_cast<uint32_t>(block.data.size()) - 1);
    }
    
    return {minAddr, maxAddr};
}

bool FlashFile::validate() const {
    if (blocks.empty()) {
        return false;
    }
    
    // Check for overlapping blocks
    for (size_t i = 0; i < blocks.size(); ++i) {
        for (size_t j = i + 1; j < blocks.size(); ++j) {
            uint32_t end1 = blocks[i].address + blocks[i].data.size() - 1;
            uint32_t start2 = blocks[j].address;
            uint32_t end2 = blocks[j].address + blocks[j].data.size() - 1;
            uint32_t start1 = blocks[i].address;
            
            if ((start1 <= end2) && (start2 <= end1)) {
                return false; // Overlapping blocks
            }
        }
    }
    
    return true;
}

std::string FlashFile::toString() const {
    std::ostringstream ss;
    ss << "FlashFile[Format:" << flashFileFormatToString(format)
       << ", Blocks:" << blocks.size()
       << ", Size:" << getTotalSize() << " bytes";
    
    auto range = getAddressRange();
    ss << ", Range:0x" << std::hex << range.first << "-0x" << range.second << "]";
    
    return ss.str();
}

bool FlashFile::parseIntelHex(const std::vector<uint8_t>& data) {
    auto logger = Logger::getInstance();
    
    std::string content(data.begin(), data.end());
    std::istringstream stream(content);
    std::string line;
    
    uint32_t baseAddress = 0;
    uint32_t currentAddress = 0;
    std::vector<uint8_t> currentData;
    
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] != ':') {
            continue;
        }
        
        if (line.length() < 11) {
            logger->error("Invalid Intel HEX line: " + line);
            return false;
        }
        
        try {
            uint8_t byteCount = std::stoul(line.substr(1, 2), nullptr, 16);
            uint16_t address = std::stoul(line.substr(3, 4), nullptr, 16);
            uint8_t recordType = std::stoul(line.substr(7, 2), nullptr, 16);
            
            switch (recordType) {
                case 0x00: // Data record
                {
                    uint32_t fullAddress = baseAddress + address;
                    
                    // If address is not continuous, create new block
                    if (currentData.empty() || fullAddress != currentAddress + currentData.size()) {
                        if (!currentData.empty()) {
                            FlashBlock block;
                            block.address = currentAddress;
                            block.data = currentData;
                            block.checksum = calculateChecksum(currentData);
                            blocks.push_back(block);
                        }
                        currentAddress = fullAddress;
                        currentData.clear();
                    }
                    
                    // Add data bytes
                    for (int i = 0; i < byteCount; ++i) {
                        uint8_t dataByte = std::stoul(line.substr(9 + i * 2, 2), nullptr, 16);
                        currentData.push_back(dataByte);
                    }
                    break;
                }
                case 0x01: // End of file
                    if (!currentData.empty()) {
                        FlashBlock block;
                        block.address = currentAddress;
                        block.data = currentData;
                        block.checksum = calculateChecksum(currentData);
                        blocks.push_back(block);
                    }
                    return true;
                    
                case 0x04: // Extended linear address
                    if (byteCount == 2) {
                        uint16_t upperAddress = std::stoul(line.substr(9, 4), nullptr, 16);
                        baseAddress = static_cast<uint32_t>(upperAddress) << 16;
                    }
                    break;
                    
                default:
                    // Ignore other record types
                    break;
            }
        } catch (const std::exception& e) {
            logger->error("Error parsing Intel HEX line: " + std::string(e.what()));
            return false;
        }
    }
    
    return !blocks.empty();
}

bool FlashFile::parseMotorolaS(const std::vector<uint8_t>& data) {
    auto logger = Logger::getInstance();
    
    std::string content(data.begin(), data.end());
    std::istringstream stream(content);
    std::string line;
    
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] != 'S') {
            continue;
        }
        
        if (line.length() < 4) {
            continue;
        }
        
        try {
            char recordType = line[1];
            uint8_t byteCount = std::stoul(line.substr(2, 2), nullptr, 16);
            
            if (recordType == '1' || recordType == '2' || recordType == '3') {
                // Data records
                int addressBytes = (recordType == '1') ? 2 : (recordType == '2') ? 3 : 4;
                
                if (line.length() < 4 + addressBytes * 2) {
                    continue;
                }
                
                uint32_t address = 0;
                for (int i = 0; i < addressBytes; ++i) {
                    uint8_t addrByte = std::stoul(line.substr(4 + i * 2, 2), nullptr, 16);
                    address = (address << 8) | addrByte;
                }
                
                int dataBytes = byteCount - addressBytes - 1; // -1 for checksum
                std::vector<uint8_t> blockData;
                
                for (int i = 0; i < dataBytes; ++i) {
                    uint8_t dataByte = std::stoul(line.substr(4 + addressBytes * 2 + i * 2, 2), nullptr, 16);
                    blockData.push_back(dataByte);
                }
                
                if (!blockData.empty()) {
                    FlashBlock block;
                    block.address = address;
                    block.data = blockData;
                    block.checksum = calculateChecksum(blockData);
                    blocks.push_back(block);
                }
            }
        } catch (const std::exception& e) {
            logger->error("Error parsing Motorola S-Record line: " + std::string(e.what()));
            return false;
        }
    }
    
    return !blocks.empty();
}

bool FlashFile::parseBinary(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return false;
    }
    
    // For binary files, create one block starting at address 0
    FlashBlock block;
    block.address = 0;
    block.data = data;
    block.checksum = calculateChecksum(data);
    blocks.push_back(block);
    
    return true;
}

bool FlashFile::parseELF(const std::vector<uint8_t>& data) {
    // Simplified ELF parsing - in real implementation would use proper ELF library
    auto logger = Logger::getInstance();
    logger->warning("ELF parsing not fully implemented - treating as binary");
    return parseBinary(data);
}

// FlashConfig implementation
std::string FlashConfig::toString() const {
    std::ostringstream ss;
    ss << "FlashConfig[BlockSize:" << blockSize
       << ", Timeout:" << timeout << "ms"
       << ", Verify:" << (verifyAfterWrite ? "Yes" : "No")
       << ", Erase:" << (eraseBeforeWrite ? "Yes" : "No")
       << ", SecurityLevel:" << static_cast<int>(securityLevel)
       << ", Regions:" << regions.size() << "]";
    return ss.str();
}

// FlashStatistics implementation
std::chrono::milliseconds FlashStatistics::getDuration() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
}

double FlashStatistics::getAverageSpeed() const {
    auto duration = getDuration();
    if (duration.count() == 0) return 0.0;
    return static_cast<double>(bytesWritten) / (duration.count() / 1000.0);
}

std::string FlashStatistics::toString() const {
    std::ostringstream ss;
    ss << "FlashStats[Duration:" << getDuration().count() << "ms"
       << ", Blocks:" << blocksWritten << "/" << totalBlocks
       << ", Bytes:" << bytesWritten << "/" << totalBytes
       << ", Speed:" << std::fixed << std::setprecision(2) << getAverageSpeed() << " B/s"
       << ", Errors:" << blocksFailed << "]";
    return ss.str();
}

// FlashManager implementation
class FlashManager::Impl {
public:
    std::shared_ptr<diagnostics::UDSClient> udsClient;
    FlashConfig config;
    std::atomic<bool> initialized{false};
    
    // Statistics
    FlashStatistics stats;
    mutable std::mutex statsMutex;
    
    Impl() {
        stats.startTime = std::chrono::system_clock::now();
    }
    
    void updateStats(size_t blocksWritten, size_t bytesWritten, bool failed = false) {
        std::lock_guard<std::mutex> lock(statsMutex);
        stats.blocksWritten += blocksWritten;
        stats.bytesWritten += bytesWritten;
        if (failed) {
            stats.blocksFailed++;
        }
    }
    
    bool requestDownload(uint32_t address, uint32_t size) {
        try {
            // UDS Request Download service (0x34)
            std::vector<uint8_t> requestData;
            requestData.push_back(0x00); // dataFormatIdentifier
            requestData.push_back(0x44); // addressAndLengthFormatIdentifier (4 bytes each)
            
            // Add address (4 bytes, big endian)
            auto addrBytes = utils::uint32ToBytes(address, true);
            requestData.insert(requestData.end(), addrBytes.begin(), addrBytes.end());
            
            // Add size (4 bytes, big endian)
            auto sizeBytes = utils::uint32ToBytes(size, true);
            requestData.insert(requestData.end(), sizeBytes.begin(), sizeBytes.end());
            
            diagnostics::UDSMessage request(diagnostics::UDSService::REQUEST_DOWNLOAD, requestData);
            diagnostics::UDSMessage response = udsClient->sendRequest(request);
            
            return !response.isNegativeResponse;
        } catch (...) {
            return false;
        }
    }
    
    bool transferData(uint8_t blockSequence, const std::vector<uint8_t>& data) {
        try {
            std::vector<uint8_t> requestData;
            requestData.push_back(blockSequence);
            requestData.insert(requestData.end(), data.begin(), data.end());
            
            diagnostics::UDSMessage request(diagnostics::UDSService::TRANSFER_DATA, requestData);
            diagnostics::UDSMessage response = udsClient->sendRequest(request);
            
            return !response.isNegativeResponse;
        } catch (...) {
            return false;
        }
    }
    
    bool requestTransferExit() {
        try {
            diagnostics::UDSMessage request(diagnostics::UDSService::REQUEST_TRANSFER_EXIT, {});
            diagnostics::UDSMessage response = udsClient->sendRequest(request);
            
            return !response.isNegativeResponse;
        } catch (...) {
            return false;
        }
    }
};

FlashManager::FlashManager() : pImpl(std::make_unique<Impl>()) {}

FlashManager::~FlashManager() = default;

bool FlashManager::initialize(std::shared_ptr<diagnostics::UDSClient> udsClient, const FlashConfig& config) {
    auto logger = Logger::getInstance();
    logger->info("Initializing flash manager: " + config.toString());
    
    if (!udsClient || !udsClient->isInitialized()) {
        logger->error("UDS client not initialized");
        return false;
    }
    
    pImpl->udsClient = udsClient;
    pImpl->config = config;
    pImpl->initialized = true;
    
    logger->info("Flash manager initialized successfully");
    return true;
}

void FlashManager::shutdown() {
    pImpl->initialized = false;
    
    auto logger = Logger::getInstance();
    logger->info("Flash manager shutdown");
}

bool FlashManager::isInitialized() const {
    return pImpl->initialized;
}

bool FlashManager::programFlash(const FlashFile& flashFile, FlashProgressCallback callback) {
    if (!pImpl->initialized) {
        throw FlashError(FlashError::ErrorCode::INITIALIZATION_FAILED, "Flash manager not initialized");
    }
    
    auto logger = Logger::getInstance();
    logger->info("Starting flash programming: " + flashFile.toString());
    
    // Reset statistics
    {
        std::lock_guard<std::mutex> lock(pImpl->statsMutex);
        pImpl->stats = FlashStatistics{};
        pImpl->stats.startTime = std::chrono::system_clock::now();
        pImpl->stats.totalBlocks = flashFile.getBlocks().size();
        pImpl->stats.totalBytes = flashFile.getTotalSize();
    }
    
    try {
        // Enter programming session
        if (!pImpl->udsClient->startDiagnosticSession(diagnostics::UDSSession::PROGRAMMING)) {
            throw FlashError(FlashError::ErrorCode::BOOTLOADER_ENTRY_FAILED, "Failed to enter programming session");
        }
        
        // Security access if required
        if (!pImpl->config.securityKey.empty()) {
            if (!pImpl->udsClient->unlockSecurityAccess(pImpl->config.securityLevel, pImpl->config.securityKey)) {
                throw FlashError(FlashError::ErrorCode::SECURITY_ACCESS_DENIED, "Security access denied");
            }
        }
        
        const auto& blocks = flashFile.getBlocks();
        uint8_t blockSequence = 1;
        
        for (size_t i = 0; i < blocks.size(); ++i) {
            const auto& block = blocks[i];
            
            if (callback) {
                callback("Programming", i, blocks.size(), "Block " + std::to_string(i + 1));
            }
            
            // Request download for this block
            if (!pImpl->requestDownload(block.address, block.data.size())) {
                throw FlashError(FlashError::ErrorCode::PROGRAMMING_FAILED, 
                               "Request download failed", block.address);
            }
            
            // Transfer data in chunks
            size_t offset = 0;
            while (offset < block.data.size()) {
                size_t chunkSize = std::min(static_cast<size_t>(pImpl->config.blockSize), 
                                          block.data.size() - offset);
                
                std::vector<uint8_t> chunk(block.data.begin() + offset, 
                                         block.data.begin() + offset + chunkSize);
                
                if (!pImpl->transferData(blockSequence++, chunk)) {
                    throw FlashError(FlashError::ErrorCode::PROGRAMMING_FAILED, 
                                   "Transfer data failed", block.address + offset);
                }
                
                offset += chunkSize;
                pImpl->updateStats(0, chunkSize);
            }
            
            // Request transfer exit
            if (!pImpl->requestTransferExit()) {
                throw FlashError(FlashError::ErrorCode::PROGRAMMING_FAILED, 
                               "Request transfer exit failed", block.address);
            }
            
            pImpl->updateStats(1, 0);
        }
        
        // Verify if requested
        if (pImpl->config.verifyAfterWrite) {
            if (callback) {
                callback("Verifying", 0, 1, "Verifying flash data");
            }
            
            if (!verifyFlash(flashFile, callback)) {
                throw FlashError(FlashError::ErrorCode::VERIFICATION_FAILED, "Flash verification failed");
            }
        }
        
        {
            std::lock_guard<std::mutex> lock(pImpl->statsMutex);
            pImpl->stats.endTime = std::chrono::system_clock::now();
        }
        
        logger->info("Flash programming completed successfully: " + pImpl->stats.toString());
        
        if (callback) {
            callback("Complete", 1, 1, "Programming completed successfully");
        }
        
        return true;
        
    } catch (const FlashError&) {
        throw; // Re-throw flash errors
    } catch (const std::exception& e) {
        throw FlashError(FlashError::ErrorCode::PROGRAMMING_FAILED, 
                        "Programming error: " + std::string(e.what()));
    }
}

bool FlashManager::verifyFlash(const FlashFile& flashFile, FlashProgressCallback callback) {
    if (!pImpl->initialized) {
        return false;
    }
    
    auto logger = Logger::getInstance();
    logger->info("Verifying flash data");
    
    const auto& blocks = flashFile.getBlocks();
    
    for (size_t i = 0; i < blocks.size(); ++i) {
        const auto& block = blocks[i];
        
        if (callback) {
            callback("Verifying", i, blocks.size(), "Block " + std::to_string(i + 1));
        }
        
        try {
            // Read data from ECU
            auto readData = pImpl->udsClient->readDataByIdentifier(0x1000 + i); // Simplified
            
            // Compare with expected data
            if (readData != block.data) {
                logger->error("Verification failed at address 0x" + 
                            utils::bytesToHex(utils::uint32ToBytes(block.address, true)));
                return false;
            }
            
            pImpl->updateStats(0, 0); // Update verification stats
            
        } catch (const std::exception& e) {
            logger->error("Verification error: " + std::string(e.what()));
            return false;
        }
    }
    
    logger->info("Flash verification completed successfully");
    return true;
}

FlashStatistics FlashManager::getStatistics() const {
    std::lock_guard<std::mutex> lock(pImpl->statsMutex);
    return pImpl->stats;
}

void FlashManager::resetStatistics() {
    std::lock_guard<std::mutex> lock(pImpl->statsMutex);
    pImpl->stats = FlashStatistics{};
    pImpl->stats.startTime = std::chrono::system_clock::now();
}

FlashConfig FlashManager::getConfiguration() const {
    return pImpl->config;
}

// Utility functions
std::string flashFileFormatToString(FlashFileFormat format) {
    switch (format) {
        case FlashFileFormat::INTEL_HEX: return "Intel HEX";
        case FlashFileFormat::MOTOROLA_S_RECORD: return "Motorola S-Record";
        case FlashFileFormat::BINARY: return "Binary";
        case FlashFileFormat::ELF: return "ELF";
        case FlashFileFormat::ODX_F: return "ODX-F";
        case FlashFileFormat::CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

FlashFileFormat stringToFlashFileFormat(const std::string& str) {
    if (str == "Intel HEX") return FlashFileFormat::INTEL_HEX;
    if (str == "Motorola S-Record") return FlashFileFormat::MOTOROLA_S_RECORD;
    if (str == "Binary") return FlashFileFormat::BINARY;
    if (str == "ELF") return FlashFileFormat::ELF;
    if (str == "ODX-F") return FlashFileFormat::ODX_F;
    return FlashFileFormat::CUSTOM;
}

std::string flashErrorCodeToString(FlashError::ErrorCode code) {
    switch (code) {
        case FlashError::ErrorCode::INITIALIZATION_FAILED: return "Initialization Failed";
        case FlashError::ErrorCode::FILE_LOAD_FAILED: return "File Load Failed";
        case FlashError::ErrorCode::SECURITY_ACCESS_DENIED: return "Security Access Denied";
        case FlashError::ErrorCode::BOOTLOADER_ENTRY_FAILED: return "Bootloader Entry Failed";
        case FlashError::ErrorCode::PROGRAMMING_FAILED: return "Programming Failed";
        case FlashError::ErrorCode::VERIFICATION_FAILED: return "Verification Failed";
        case FlashError::ErrorCode::ERASE_FAILED: return "Erase Failed";
        case FlashError::ErrorCode::TIMEOUT: return "Timeout";
        case FlashError::ErrorCode::CHECKSUM_ERROR: return "Checksum Error";
        case FlashError::ErrorCode::INVALID_ADDRESS: return "Invalid Address";
        case FlashError::ErrorCode::REGION_PROTECTED: return "Region Protected";
        default: return "Unknown Error";
    }
}

uint32_t calculateChecksum(const std::vector<uint8_t>& data) {
    return utils::calculateCRC32(data);
}

bool validateAddress(uint32_t address, const std::vector<FlashRegion>& regions) {
    for (const auto& region : regions) {
        if (region.contains(address)) {
            return !region.isProtected;
        }
    }
    return false;
}

FlashRegion findRegionForAddress(uint32_t address, const std::vector<FlashRegion>& regions) {
    for (const auto& region : regions) {
        if (region.contains(address)) {
            return region;
        }
    }
    
    // Return empty region if not found
    FlashRegion empty;
    empty.name = "Unknown";
    return empty;
}

} // namespace flashing
} // namespace fmus
