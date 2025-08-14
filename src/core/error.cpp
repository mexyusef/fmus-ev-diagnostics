#include <fmus/j2534.h>
#include <sstream>
#include <unordered_map>

namespace fmus {
namespace j2534 {

// Error code descriptions
// Ini adalah deskripsi untuk semua kode error yang didefinisikan dalam J2534
static const std::unordered_map<int, std::string> ERROR_DESCRIPTIONS = {
    { 0x00,     "Status OK" },
    { 0x01,     "ERR_NOT_SUPPORTED - Function tidak didukung" },
    { 0x02,     "ERR_INVALID_CHANNEL_ID - Channel ID tidak valid" },
    { 0x03,     "ERR_INVALID_PROTOCOL_ID - Protocol tidak didukung atau tidak valid" },
    { 0x04,     "ERR_NULL_PARAMETER - Parameter bernilai NULL" },
    { 0x05,     "ERR_INVALID_IOCTL_VALUE - IOCTL value tidak valid" },
    { 0x06,     "ERR_INVALID_FLAGS - Flag tidak valid atau tidak cocok dengan request" },
    { 0x07,     "ERR_FAILED - Operasi gagal" },
    { 0x08,     "ERR_DEVICE_NOT_CONNECTED - Device tidak terhubung" },
    { 0x09,     "ERR_TIMEOUT - Operasi timeout" },
    { 0x0A,     "ERR_INVALID_MSG - Format message tidak valid" },
    { 0x0B,     "ERR_INVALID_TIME_INTERVAL - Interval waktu tidak valid" },
    { 0x0C,     "ERR_EXCEEDED_LIMIT - Operasi melebihi batas device (buffer penuh)" },
    { 0x0D,     "ERR_INVALID_MSG_ID - Message ID tidak valid" },
    { 0x0E,     "ERR_DEVICE_IN_USE - Device sedang digunakan oleh aplikasi lain" },
    { 0x0F,     "ERR_INVALID_IOCTL_ID - IOCTL ID tidak valid" },
    { 0x10,     "ERR_BUFFER_EMPTY - Tidak ada data yang tersedia" },
    { 0x11,     "ERR_BUFFER_FULL - Buffer penuh, data tidak dapat ditambahkan" },
    { 0x12,     "ERR_BUFFER_OVERFLOW - Buffer overflow, data hilang" },
    { 0x13,     "ERR_PIN_INVALID - PIN tidak valid" },
    { 0x14,     "ERR_CHANNEL_IN_USE - Channel sedang digunakan" },
    { 0x15,     "ERR_MSG_PROTOCOL_ID - Protocol ID dalam message tidak sesuai dengan channel" },
    { 0x16,     "ERR_INVALID_FILTER_ID - Filter ID tidak valid" },
    { 0x17,     "ERR_NO_FLOW_CONTROL - Tidak dapat menambahkan flow control filter" },
    { 0x18,     "ERR_NOT_UNIQUE - Parameter harus unik" },
    { 0x19,     "ERR_INVALID_BAUDRATE - Baud rate tidak valid atau tidak didukung" },
    { 0x1A,     "ERR_INVALID_DEVICE_ID - Device ID tidak valid" },
    { 0x1B,     "ERR_INVALID_DEVICE_ID - Device ID tidak valid" },
    { 0x20,     "ERR_FAILED - Operasi gagal dengan alasan tidak spesifik" }
};

// Implementation of J2534Error constructor
J2534Error::J2534Error(ErrorCode code, const std::string& message)
    : std::runtime_error(message),
      errorCode(code) {
}

J2534Error::J2534Error(int code, const std::string& message)
    : std::runtime_error(message),
      errorCode(static_cast<ErrorCode>(code)) {
}

// Utility functions
std::string errorCodeToString(ErrorCode code) {
    auto it = ERROR_DESCRIPTIONS.find(static_cast<int>(code));
    if (it != ERROR_DESCRIPTIONS.end()) {
        return it->second;
    }
    return "Unknown error";
}

std::string formatErrorMessage(ErrorCode code, const std::string& operation) {
    std::stringstream ss;
    ss << "J2534 error during " << operation << ": ";
    ss << errorCodeToString(code);
    ss << " (code: 0x" << std::hex << static_cast<int>(code) << ")";
    return ss.str();
}

// Legacy function for backward compatibility
std::string formatErrorMessage(int errorCode, const std::string& operation) {
    std::stringstream ss;
    ss << "J2534 error during " << operation << ": ";

    auto it = ERROR_DESCRIPTIONS.find(errorCode);
    if (it != ERROR_DESCRIPTIONS.end()) {
        ss << it->second;
    } else {
        ss << "Unknown error";
    }

    ss << " (code: 0x" << std::hex << errorCode << ")";
    return ss.str();
}

// Protocol utility functions
std::string protocolToString(Protocol protocol) {
    switch (protocol) {
        case Protocol::J1850VPW: return "J1850VPW";
        case Protocol::J1850PWM: return "J1850PWM";
        case Protocol::ISO9141: return "ISO9141";
        case Protocol::ISO14230_4: return "ISO14230-4";
        case Protocol::CAN: return "CAN";
        case Protocol::ISO15765: return "ISO15765";
        case Protocol::SCI_A_ENGINE: return "SCI_A_ENGINE";
        case Protocol::SCI_A_TRANS: return "SCI_A_TRANS";
        case Protocol::SCI_B_ENGINE: return "SCI_B_ENGINE";
        case Protocol::SCI_B_TRANS: return "SCI_B_TRANS";
        default: return "Unknown";
    }
}

Protocol stringToProtocol(const std::string& str) {
    if (str == "J1850VPW") return Protocol::J1850VPW;
    if (str == "J1850PWM") return Protocol::J1850PWM;
    if (str == "ISO9141") return Protocol::ISO9141;
    if (str == "ISO14230-4") return Protocol::ISO14230_4;
    if (str == "CAN") return Protocol::CAN;
    if (str == "ISO15765") return Protocol::ISO15765;
    if (str == "SCI_A_ENGINE") return Protocol::SCI_A_ENGINE;
    if (str == "SCI_A_TRANS") return Protocol::SCI_A_TRANS;
    if (str == "SCI_B_ENGINE") return Protocol::SCI_B_ENGINE;
    if (str == "SCI_B_TRANS") return Protocol::SCI_B_TRANS;
    return Protocol::CAN; // Default
}

} // namespace j2534
} // namespace fmus