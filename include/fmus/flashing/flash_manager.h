#ifndef FMUS_FLASHING_FLASH_MANAGER_H
#define FMUS_FLASHING_FLASH_MANAGER_H

/**
 * @file flash_manager.h
 * @brief ECU Flash Programming Manager
 */

#include <fmus/diagnostics/uds.h>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <string>
#include <map>

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
namespace flashing {

/**
 * @brief Flash file formats
 */
enum class FlashFileFormat {
    INTEL_HEX,          ///< Intel HEX format
    MOTOROLA_S_RECORD,  ///< Motorola S-Record format
    BINARY,             ///< Raw binary format
    ELF,                ///< ELF executable format
    ODX_F,              ///< ODX-F flash container
    CUSTOM              ///< Custom format
};

/**
 * @brief Flash memory region
 */
struct FlashRegion {
    uint32_t startAddress;      ///< Start address
    uint32_t endAddress;        ///< End address
    uint32_t blockSize;         ///< Block size for programming
    bool isProtected = false;   ///< Whether region is write-protected
    std::string name;           ///< Region name (e.g., "Application", "Bootloader")
    
    uint32_t getSize() const { return endAddress - startAddress + 1; }
    bool contains(uint32_t address) const { 
        return address >= startAddress && address <= endAddress; 
    }
    
    std::string toString() const;
};

/**
 * @brief Flash data block
 */
struct FlashBlock {
    uint32_t address;           ///< Block address
    std::vector<uint8_t> data;  ///< Block data
    uint32_t checksum = 0;      ///< Block checksum
    bool isVerified = false;    ///< Whether block has been verified
    
    std::string toString() const;
};

/**
 * @brief Flash file container
 */
class FMUS_AUTO_API FlashFile {
public:
    /**
     * @brief Constructor
     */
    FlashFile() = default;
    
    /**
     * @brief Load flash file from path
     */
    bool loadFromFile(const std::string& filePath);
    
    /**
     * @brief Load flash data from memory
     */
    bool loadFromData(const std::vector<uint8_t>& data, FlashFileFormat format);
    
    /**
     * @brief Get file format
     */
    FlashFileFormat getFormat() const { return format; }
    
    /**
     * @brief Get all flash blocks
     */
    const std::vector<FlashBlock>& getBlocks() const { return blocks; }
    
    /**
     * @brief Get blocks for specific region
     */
    std::vector<FlashBlock> getBlocksForRegion(const FlashRegion& region) const;
    
    /**
     * @brief Get total data size
     */
    size_t getTotalSize() const;
    
    /**
     * @brief Get address range
     */
    std::pair<uint32_t, uint32_t> getAddressRange() const;
    
    /**
     * @brief Validate file integrity
     */
    bool validate() const;
    
    /**
     * @brief Get file metadata
     */
    std::map<std::string, std::string> getMetadata() const { return metadata; }
    
    std::string toString() const;

private:
    FlashFileFormat format = FlashFileFormat::BINARY;
    std::vector<FlashBlock> blocks;
    std::map<std::string, std::string> metadata;
    
    bool parseIntelHex(const std::vector<uint8_t>& data);
    bool parseMotorolaS(const std::vector<uint8_t>& data);
    bool parseBinary(const std::vector<uint8_t>& data);
    bool parseELF(const std::vector<uint8_t>& data);
};

/**
 * @brief Flash programming progress callback
 */
using FlashProgressCallback = std::function<void(
    const std::string& operation,   ///< Current operation
    size_t current,                 ///< Current progress
    size_t total,                   ///< Total progress
    const std::string& message     ///< Status message
)>;

/**
 * @brief Flash programming configuration
 */
struct FlashConfig {
    uint32_t blockSize = 256;           ///< Programming block size
    uint32_t timeout = 5000;            ///< Operation timeout (ms)
    bool verifyAfterWrite = true;       ///< Verify after each block
    bool eraseBeforeWrite = true;       ///< Erase before programming
    uint8_t securityLevel = 1;          ///< Security access level
    std::vector<uint8_t> securityKey;   ///< Security key
    std::vector<FlashRegion> regions;   ///< Flash memory regions
    
    std::string toString() const;
};

/**
 * @brief Flash programming statistics
 */
struct FlashStatistics {
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    size_t totalBlocks = 0;
    size_t blocksWritten = 0;
    size_t blocksVerified = 0;
    size_t blocksFailed = 0;
    size_t totalBytes = 0;
    size_t bytesWritten = 0;
    uint32_t checksumErrors = 0;
    uint32_t timeoutErrors = 0;
    
    std::chrono::milliseconds getDuration() const;
    double getAverageSpeed() const; // bytes per second
    std::string toString() const;
};

/**
 * @brief Flash Manager for ECU programming
 */
class FMUS_AUTO_API FlashManager {
public:
    /**
     * @brief Constructor
     */
    FlashManager();
    
    /**
     * @brief Destructor
     */
    ~FlashManager();
    
    /**
     * @brief Initialize flash manager
     */
    bool initialize(std::shared_ptr<diagnostics::UDSClient> udsClient, const FlashConfig& config);
    
    /**
     * @brief Shutdown flash manager
     */
    void shutdown();
    
    /**
     * @brief Check if manager is initialized
     */
    bool isInitialized() const;
    
    /**
     * @brief Program flash file to ECU
     */
    bool programFlash(const FlashFile& flashFile, FlashProgressCallback callback = nullptr);
    
    /**
     * @brief Read flash data from ECU
     */
    bool readFlash(const FlashRegion& region, std::vector<uint8_t>& data, 
                   FlashProgressCallback callback = nullptr);
    
    /**
     * @brief Verify flash data
     */
    bool verifyFlash(const FlashFile& flashFile, FlashProgressCallback callback = nullptr);
    
    /**
     * @brief Erase flash region
     */
    bool eraseFlash(const FlashRegion& region, FlashProgressCallback callback = nullptr);
    
    /**
     * @brief Get ECU flash information
     */
    std::vector<FlashRegion> getFlashRegions();
    
    /**
     * @brief Check if ECU is in bootloader mode
     */
    bool isInBootloaderMode();
    
    /**
     * @brief Enter bootloader mode
     */
    bool enterBootloaderMode();
    
    /**
     * @brief Exit bootloader mode
     */
    bool exitBootloaderMode();
    
    /**
     * @brief Get programming statistics
     */
    FlashStatistics getStatistics() const;
    
    /**
     * @brief Reset statistics
     */
    void resetStatistics();
    
    /**
     * @brief Get current configuration
     */
    FlashConfig getConfiguration() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Flash programming error
 */
class FMUS_AUTO_API FlashError : public std::runtime_error {
public:
    enum class ErrorCode {
        INITIALIZATION_FAILED,
        FILE_LOAD_FAILED,
        SECURITY_ACCESS_DENIED,
        BOOTLOADER_ENTRY_FAILED,
        PROGRAMMING_FAILED,
        VERIFICATION_FAILED,
        ERASE_FAILED,
        TIMEOUT,
        CHECKSUM_ERROR,
        INVALID_ADDRESS,
        REGION_PROTECTED
    };
    
    FlashError(ErrorCode code, const std::string& message, uint32_t address = 0)
        : std::runtime_error(message), errorCode(code), address(address) {}
    
    ErrorCode getErrorCode() const { return errorCode; }
    uint32_t getAddress() const { return address; }

private:
    ErrorCode errorCode;
    uint32_t address;
};

// Utility functions
FMUS_AUTO_API std::string flashFileFormatToString(FlashFileFormat format);
FMUS_AUTO_API FlashFileFormat stringToFlashFileFormat(const std::string& str);
FMUS_AUTO_API std::string flashErrorCodeToString(FlashError::ErrorCode code);
FMUS_AUTO_API uint32_t calculateChecksum(const std::vector<uint8_t>& data);
FMUS_AUTO_API bool validateAddress(uint32_t address, const std::vector<FlashRegion>& regions);
FMUS_AUTO_API FlashRegion findRegionForAddress(uint32_t address, const std::vector<FlashRegion>& regions);

} // namespace flashing
} // namespace fmus

#endif // FMUS_FLASHING_FLASH_MANAGER_H
