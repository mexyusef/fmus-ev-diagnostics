#ifndef FMUS_UTILS_H
#define FMUS_UTILS_H

/**
 * @file utils.h
 * @brief Utility functions for FMUS-AUTO
 */

#include <string>
#include <vector>
#include <cstdint>
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
namespace utils {

// Hex utilities
FMUS_AUTO_API std::string bytesToHex(const std::vector<uint8_t>& bytes, bool uppercase = true);
FMUS_AUTO_API std::string bytesToHex(const uint8_t* data, size_t length, bool uppercase = true);
FMUS_AUTO_API std::vector<uint8_t> hexToBytes(const std::string& hex);
FMUS_AUTO_API bool isValidHex(const std::string& hex);

// String utilities
FMUS_AUTO_API std::string trim(const std::string& str);
FMUS_AUTO_API std::string toLower(const std::string& str);
FMUS_AUTO_API std::string toUpper(const std::string& str);
FMUS_AUTO_API std::vector<std::string> split(const std::string& str, char delimiter);
FMUS_AUTO_API std::string join(const std::vector<std::string>& strings, const std::string& delimiter);

// Checksum utilities
FMUS_AUTO_API uint8_t calculateChecksum8(const std::vector<uint8_t>& data);
FMUS_AUTO_API uint16_t calculateChecksum16(const std::vector<uint8_t>& data);
FMUS_AUTO_API uint32_t calculateCRC32(const std::vector<uint8_t>& data);
FMUS_AUTO_API bool verifyChecksum8(const std::vector<uint8_t>& data, uint8_t expectedChecksum);
FMUS_AUTO_API bool verifyChecksum16(const std::vector<uint8_t>& data, uint16_t expectedChecksum);
FMUS_AUTO_API bool verifyCRC32(const std::vector<uint8_t>& data, uint32_t expectedCRC);

// File utilities
FMUS_AUTO_API bool fileExists(const std::string& filename);
FMUS_AUTO_API std::vector<uint8_t> readBinaryFile(const std::string& filename);
FMUS_AUTO_API bool writeBinaryFile(const std::string& filename, const std::vector<uint8_t>& data);
FMUS_AUTO_API std::string readTextFile(const std::string& filename);
FMUS_AUTO_API bool writeTextFile(const std::string& filename, const std::string& content);
FMUS_AUTO_API std::string getFileExtension(const std::string& filename);
FMUS_AUTO_API std::string getFileName(const std::string& path);
FMUS_AUTO_API std::string getDirectoryPath(const std::string& path);

// Time utilities
FMUS_AUTO_API std::string getCurrentTimestamp();
FMUS_AUTO_API std::string formatTimestamp(const std::chrono::system_clock::time_point& timePoint);
FMUS_AUTO_API uint64_t getTimestampMs();
FMUS_AUTO_API void sleepMs(uint32_t milliseconds);

// Byte manipulation utilities
FMUS_AUTO_API uint16_t bytesToUint16(const std::vector<uint8_t>& bytes, size_t offset = 0, bool bigEndian = true);
FMUS_AUTO_API uint32_t bytesToUint32(const std::vector<uint8_t>& bytes, size_t offset = 0, bool bigEndian = true);
FMUS_AUTO_API std::vector<uint8_t> uint16ToBytes(uint16_t value, bool bigEndian = true);
FMUS_AUTO_API std::vector<uint8_t> uint32ToBytes(uint32_t value, bool bigEndian = true);

// Validation utilities
FMUS_AUTO_API bool isValidVIN(const std::string& vin);
FMUS_AUTO_API bool isValidCANId(uint32_t canId, bool extended = false);
FMUS_AUTO_API bool isValidBaudRate(uint32_t baudRate);

// Math utilities
FMUS_AUTO_API uint8_t calculateXOR(const std::vector<uint8_t>& data);
FMUS_AUTO_API uint8_t calculateSum(const std::vector<uint8_t>& data);
FMUS_AUTO_API bool isPowerOfTwo(uint32_t value);
FMUS_AUTO_API uint32_t nextPowerOfTwo(uint32_t value);

// Platform utilities
FMUS_AUTO_API std::string getPlatformName();
FMUS_AUTO_API bool isWindows();
FMUS_AUTO_API bool isLinux();
FMUS_AUTO_API bool isMacOS();

// Debug utilities
FMUS_AUTO_API void hexDump(const std::vector<uint8_t>& data, const std::string& title = "");
FMUS_AUTO_API void hexDump(const uint8_t* data, size_t length, const std::string& title = "");
FMUS_AUTO_API std::string formatBytes(const std::vector<uint8_t>& data, size_t maxBytes = 16);

} // namespace utils
} // namespace fmus

#endif // FMUS_UTILS_H
