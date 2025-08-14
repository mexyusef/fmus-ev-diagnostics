#include <fmus/utils.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <fstream>

namespace fmus {
namespace utils {

std::string bytesToHex(const std::vector<uint8_t>& bytes, bool uppercase) {
    return bytesToHex(bytes.data(), bytes.size(), uppercase);
}

std::string bytesToHex(const uint8_t* data, size_t length, bool uppercase) {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    if (uppercase) {
        ss << std::uppercase;
    }
    
    for (size_t i = 0; i < length; ++i) {
        ss << std::setw(2) << static_cast<int>(data[i]);
    }
    
    return ss.str();
}

std::vector<uint8_t> hexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    
    // Remove spaces and convert to uppercase
    std::string cleanHex;
    for (char c : hex) {
        if (!std::isspace(c)) {
            cleanHex += std::toupper(c);
        }
    }
    
    // Must have even number of characters
    if (cleanHex.length() % 2 != 0) {
        return bytes; // Return empty vector on error
    }
    
    // Convert pairs of hex characters to bytes
    for (size_t i = 0; i < cleanHex.length(); i += 2) {
        std::string byteStr = cleanHex.substr(i, 2);
        try {
            uint8_t byte = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
            bytes.push_back(byte);
        } catch (...) {
            return std::vector<uint8_t>(); // Return empty vector on error
        }
    }
    
    return bytes;
}

bool isValidHex(const std::string& hex) {
    if (hex.empty()) {
        return false;
    }
    
    // Count non-space characters
    size_t hexChars = 0;
    for (char c : hex) {
        if (!std::isspace(c)) {
            if (!std::isxdigit(c)) {
                return false;
            }
            hexChars++;
        }
    }
    
    // Must have even number of hex characters
    return hexChars % 2 == 0;
}

std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(start, end - start + 1);
}

std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string join(const std::vector<std::string>& strings, const std::string& delimiter) {
    if (strings.empty()) {
        return "";
    }
    
    std::ostringstream ss;
    ss << strings[0];
    
    for (size_t i = 1; i < strings.size(); ++i) {
        ss << delimiter << strings[i];
    }
    
    return ss.str();
}

uint8_t calculateXOR(const std::vector<uint8_t>& data) {
    uint8_t result = 0;
    for (uint8_t byte : data) {
        result ^= byte;
    }
    return result;
}

uint8_t calculateSum(const std::vector<uint8_t>& data) {
    uint8_t result = 0;
    for (uint8_t byte : data) {
        result += byte;
    }
    return result;
}

uint8_t calculateChecksum8(const std::vector<uint8_t>& data) {
    return calculateSum(data);
}

uint16_t calculateChecksum16(const std::vector<uint8_t>& data) {
    uint16_t result = 0;
    for (uint8_t byte : data) {
        result += byte;
    }
    return result;
}

bool verifyChecksum8(const std::vector<uint8_t>& data, uint8_t expectedChecksum) {
    return calculateChecksum8(data) == expectedChecksum;
}

bool verifyChecksum16(const std::vector<uint8_t>& data, uint16_t expectedChecksum) {
    return calculateChecksum16(data) == expectedChecksum;
}

uint16_t bytesToUint16(const std::vector<uint8_t>& bytes, size_t offset, bool bigEndian) {
    if (offset + 1 >= bytes.size()) {
        return 0;
    }
    
    if (bigEndian) {
        return (static_cast<uint16_t>(bytes[offset]) << 8) | bytes[offset + 1];
    } else {
        return bytes[offset] | (static_cast<uint16_t>(bytes[offset + 1]) << 8);
    }
}

uint32_t bytesToUint32(const std::vector<uint8_t>& bytes, size_t offset, bool bigEndian) {
    if (offset + 3 >= bytes.size()) {
        return 0;
    }
    
    if (bigEndian) {
        return (static_cast<uint32_t>(bytes[offset]) << 24) |
               (static_cast<uint32_t>(bytes[offset + 1]) << 16) |
               (static_cast<uint32_t>(bytes[offset + 2]) << 8) |
               bytes[offset + 3];
    } else {
        return bytes[offset] |
               (static_cast<uint32_t>(bytes[offset + 1]) << 8) |
               (static_cast<uint32_t>(bytes[offset + 2]) << 16) |
               (static_cast<uint32_t>(bytes[offset + 3]) << 24);
    }
}

std::vector<uint8_t> uint16ToBytes(uint16_t value, bool bigEndian) {
    std::vector<uint8_t> bytes(2);
    
    if (bigEndian) {
        bytes[0] = static_cast<uint8_t>(value >> 8);
        bytes[1] = static_cast<uint8_t>(value & 0xFF);
    } else {
        bytes[0] = static_cast<uint8_t>(value & 0xFF);
        bytes[1] = static_cast<uint8_t>(value >> 8);
    }
    
    return bytes;
}

std::vector<uint8_t> uint32ToBytes(uint32_t value, bool bigEndian) {
    std::vector<uint8_t> bytes(4);
    
    if (bigEndian) {
        bytes[0] = static_cast<uint8_t>(value >> 24);
        bytes[1] = static_cast<uint8_t>((value >> 16) & 0xFF);
        bytes[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
        bytes[3] = static_cast<uint8_t>(value & 0xFF);
    } else {
        bytes[0] = static_cast<uint8_t>(value & 0xFF);
        bytes[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
        bytes[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
        bytes[3] = static_cast<uint8_t>(value >> 24);
    }
    
    return bytes;
}

// CRC32 implementation
uint32_t calculateCRC32(const std::vector<uint8_t>& data) {
    static const uint32_t crc32_table[256] = {
        0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
        0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
        0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
        0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
        0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
        0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
        0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
        0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
        0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
        0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
        0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
        0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
        0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
        0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
        0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
        0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
        0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
        0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
        0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
        0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
        0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
        0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
        0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
        0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
        0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
        0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
        0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
        0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
        0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
        0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
        0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
        0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
        0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
        0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
        0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
        0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
        0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
        0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
        0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
        0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
        0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
        0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
        0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
    };

    uint32_t crc = 0xFFFFFFFF;
    for (uint8_t byte : data) {
        crc = crc32_table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

bool verifyCRC32(const std::vector<uint8_t>& data, uint32_t expectedCRC) {
    return calculateCRC32(data) == expectedCRC;
}

// Platform utilities
std::string getPlatformName() {
#ifdef _WIN32
    return "Windows";
#elif defined(__linux__)
    return "Linux";
#elif defined(__APPLE__)
    return "macOS";
#else
    return "Unknown";
#endif
}

bool isWindows() {
#ifdef _WIN32
    return true;
#else
    return false;
#endif
}

bool isLinux() {
#ifdef __linux__
    return true;
#else
    return false;
#endif
}

bool isMacOS() {
#ifdef __APPLE__
    return true;
#else
    return false;
#endif
}

// Validation utilities
bool isValidVIN(const std::string& vin) {
    if (vin.length() != 17) {
        return false;
    }

    // Check for invalid characters (I, O, Q are not allowed)
    for (char c : vin) {
        if (!std::isalnum(c) || c == 'I' || c == 'O' || c == 'Q') {
            return false;
        }
    }

    return true;
}

bool isValidCANId(uint32_t canId, bool extended) {
    if (extended) {
        return canId <= 0x1FFFFFFF; // 29-bit max
    } else {
        return canId <= 0x7FF; // 11-bit max
    }
}

bool isValidBaudRate(uint32_t baudRate) {
    std::vector<uint32_t> validRates = {
        9600, 10400, 19200, 38400, 57600, 115200,
        125000, 250000, 500000, 1000000
    };
    return std::find(validRates.begin(), validRates.end(), baudRate) != validRates.end();
}

// Math utilities
bool isPowerOfTwo(uint32_t value) {
    return value > 0 && (value & (value - 1)) == 0;
}

uint32_t nextPowerOfTwo(uint32_t value) {
    if (value == 0) return 1;
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    return value + 1;
}

// Debug utilities
void hexDump(const std::vector<uint8_t>& data, const std::string& title) {
    hexDump(data.data(), data.size(), title);
}

void hexDump(const uint8_t* data, size_t length, const std::string& title) {
    if (!title.empty()) {
        std::cout << "=== " << title << " ===" << std::endl;
    }

    for (size_t i = 0; i < length; i += 16) {
        // Print offset
        std::cout << std::hex << std::setw(8) << std::setfill('0') << i << ": ";

        // Print hex bytes
        for (size_t j = 0; j < 16; ++j) {
            if (i + j < length) {
                std::cout << std::hex << std::setw(2) << std::setfill('0')
                         << static_cast<int>(data[i + j]) << " ";
            } else {
                std::cout << "   ";
            }

            if (j == 7) std::cout << " ";
        }

        std::cout << " |";

        // Print ASCII representation
        for (size_t j = 0; j < 16 && i + j < length; ++j) {
            char c = static_cast<char>(data[i + j]);
            std::cout << (std::isprint(c) ? c : '.');
        }

        std::cout << "|" << std::endl;
    }

    std::cout << std::dec; // Reset to decimal
}

std::string formatBytes(const std::vector<uint8_t>& data, size_t maxBytes) {
    std::ostringstream ss;
    size_t count = std::min(data.size(), maxBytes);

    for (size_t i = 0; i < count; ++i) {
        if (i > 0) ss << " ";
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    }

    if (data.size() > maxBytes) {
        ss << "... (" << data.size() << " bytes total)";
    }

    return ss.str();
}

// Additional utility functions for Phase 2 & 3

bool fileExists(const std::string& filePath) {
    std::ifstream file(filePath);
    return file.good();
}

} // namespace utils
} // namespace fmus
