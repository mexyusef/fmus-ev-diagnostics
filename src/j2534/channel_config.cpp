#include <fmus/j2534.h>
#include <sstream>
#include <unordered_map>

namespace fmus {
namespace j2534 {

// Implementasi default constructor untuk ChannelConfig
ChannelConfig::ChannelConfig()
    : baudRate(500000), flags(0), parameters() {
}

// Implementasi constructor dengan parameter
ChannelConfig::ChannelConfig(unsigned long baudRate, unsigned long flags)
    : baudRate(baudRate), flags(flags), parameters() {
}

// Implementasi copy constructor
ChannelConfig::ChannelConfig(const ChannelConfig& other)
    : baudRate(other.baudRate),
      flags(other.flags),
      parameters(other.parameters) {
}

// Implementasi move constructor
ChannelConfig::ChannelConfig(ChannelConfig&& other) noexcept
    : baudRate(other.baudRate),
      flags(other.flags),
      parameters(std::move(other.parameters)) {
}

// Implementasi copy assignment
ChannelConfig& ChannelConfig::operator=(const ChannelConfig& other) {
    if (this != &other) {
        baudRate = other.baudRate;
        flags = other.flags;
        parameters = other.parameters;
    }
    return *this;
}

// Implementasi move assignment
ChannelConfig& ChannelConfig::operator=(ChannelConfig&& other) noexcept {
    if (this != &other) {
        baudRate = other.baudRate;
        flags = other.flags;
        parameters = std::move(other.parameters);
    }
    return *this;
}

// Implementasi getBaudRate
unsigned long ChannelConfig::getBaudRate() const {
    return baudRate;
}

// Implementasi getFlags
unsigned long ChannelConfig::getFlags() const {
    return flags;
}

// Implementasi setBaudRate
void ChannelConfig::setBaudRate(unsigned long rate) {
    baudRate = rate;
}

// Implementasi setFlags
void ChannelConfig::setFlags(unsigned long flagsValue) {
    flags = flagsValue;
}

// Implementasi setParameter
void ChannelConfig::setParameter(unsigned long parameter, unsigned long value) {
    parameters[parameter] = value;
}

// Implementasi getParameter
unsigned long ChannelConfig::getParameter(unsigned long parameter) const {
    auto it = parameters.find(parameter);
    if (it != parameters.end()) {
        return it->second;
    }
    return 0; // Default value jika parameter tidak ditemukan
}

// Implementasi hasParameter
bool ChannelConfig::hasParameter(unsigned long parameter) const {
    return parameters.find(parameter) != parameters.end();
}

// Implementasi clearParameters
void ChannelConfig::clearParameters() {
    parameters.clear();
}

// Helper untuk mendapatkan nama parameter yang bisa dibaca
std::string getParameterName(unsigned long parameter) {
    static const std::unordered_map<unsigned long, std::string> PARAMETER_NAMES = {
        { 0x01, "DATA_RATE" },
        { 0x02, "LOOPBACK" },
        { 0x03, "NODE_ADDRESS" },
        { 0x04, "NETWORK_LINE" },
        { 0x05, "P1_MIN" },
        { 0x06, "P1_MAX" },
        { 0x07, "P2_MIN" },
        { 0x08, "P2_MAX" },
        { 0x09, "P3_MIN" },
        { 0x0A, "P3_MAX" },
        { 0x0B, "P4_MIN" },
        { 0x0C, "P4_MAX" },
        { 0x0D, "W1" },
        { 0x0E, "W2" },
        { 0x0F, "W3" },
        { 0x10, "W4" },
        { 0x11, "W5" },
        { 0x12, "TIDLE" },
        { 0x13, "TINIL" },
        { 0x14, "TWUP" },
        { 0x15, "PARITY" },
        { 0x16, "BIT_SAMPLE_POINT" },
        { 0x17, "SYNC_JUMP_WIDTH" },
        { 0x18, "W0" },
        { 0x19, "T1_MAX" },
        { 0x1A, "T2_MAX" },
        { 0x1B, "T3_MAX" },
        { 0x1C, "T4_MAX" },
        { 0x1D, "T5_MAX" },
        { 0x1E, "ISO15765_BS" },
        { 0x1F, "ISO15765_STMIN" },
        { 0x20, "ISO15765_BS_TX" },
        { 0x21, "ISO15765_STMIN_TX" },
        { 0x22, "DATA_BITS" },
        { 0x23, "FIVE_BAUD_MOD" },
        { 0x24, "BS_TX" },
        { 0x25, "STMIN_TX" },
        { 0x26, "T3_MAX_MULTIPLIER" },
        { 0x27, "ISO15765_WFT_MAX" },
        { 0x28, "CAN_MIXED_FORMAT" },
        { 0x29, "J1962_PINS" }
    };

    auto it = PARAMETER_NAMES.find(parameter);
    if (it != PARAMETER_NAMES.end()) {
        return it->second;
    }

    std::stringstream ss;
    ss << "PARAM_0x" << std::hex << parameter;
    return ss.str();
}

// Implementasi toString
std::string ChannelConfig::toString() const {
    std::stringstream ss;
    ss << "ChannelConfig [BaudRate: " << baudRate
       << ", Flags: 0x" << std::hex << flags
       << ", Parameters: {";

    bool first = true;
    for (const auto& param : parameters) {
        if (!first) {
            ss << ", ";
        }
        ss << getParameterName(param.first) << "=0x" << std::hex << param.second;
        first = false;
    }

    ss << "}]";
    return ss.str();
}

// Implementasi getParameters
const std::unordered_map<unsigned long, unsigned long>& ChannelConfig::getParameters() const {
    return parameters;
}

// Implementasi static builder untuk konfigurasi CAN standard
ChannelConfig ChannelConfig::forCAN(unsigned long baudRate) {
    ChannelConfig config;
    config.setBaudRate(baudRate);
    // Tidak ada flags khusus untuk CAN standard
    return config;
}

// Implementasi static builder untuk konfigurasi CAN Extended
ChannelConfig ChannelConfig::forCANExtended(unsigned long baudRate) {
    ChannelConfig config;
    config.setBaudRate(baudRate);
    config.setFlags(0x00000100); // CAN_29BIT_ID
    return config;
}

// Implementasi static builder untuk konfigurasi ISO15765 standard
ChannelConfig ChannelConfig::forISO15765(unsigned long baudRate) {
    ChannelConfig config;
    config.setBaudRate(baudRate);
    // Set parameter default untuk ISO15765
    config.setParameter(0x1E, 8);   // ISO15765_BS - Block Size
    config.setParameter(0x1F, 0);   // ISO15765_STMIN - Separation Time
    return config;
}

// Implementasi static builder untuk konfigurasi ISO15765 Extended
ChannelConfig ChannelConfig::forISO15765Extended(unsigned long baudRate) {
    ChannelConfig config;
    config.setBaudRate(baudRate);
    config.setFlags(0x00000100); // CAN_29BIT_ID
    // Set parameter default untuk ISO15765
    config.setParameter(0x1E, 8);   // ISO15765_BS - Block Size
    config.setParameter(0x1F, 0);   // ISO15765_STMIN - Separation Time
    return config;
}

// Operator perbandingan ==
bool ChannelConfig::operator==(const ChannelConfig& other) const {
    return baudRate == other.baudRate &&
           flags == other.flags &&
           parameters == other.parameters;
}

// Operator perbandingan !=
bool ChannelConfig::operator!=(const ChannelConfig& other) const {
    return !(*this == other);
}

} // namespace j2534
} // namespace fmus