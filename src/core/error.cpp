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

// Implementasi constructor DeviceError
DeviceError::DeviceError(const std::string& message, int errorCode)
    : std::runtime_error(message),
      errorCode(errorCode) {
}

// Implementasi method getErrorCode
int DeviceError::getErrorCode() const {
    return errorCode;
}

// Utility function untuk membuat pesan error standar berdasarkan kode error
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

} // namespace j2534
} // namespace fmus