#include <fmus/j2534.h>
#include <sstream>

namespace fmus {
namespace j2534 {

// Implementasi default constructor
ConnectionOptions::ConnectionOptions()
    : vendorName(""), protocol(0), baudRate(0), flags(0), timeout(5000) {
}

// Implementasi constructor dengan parameter
ConnectionOptions::ConnectionOptions(const std::string& vendorName,
                                   unsigned long protocol,
                                   unsigned long baudRate,
                                   unsigned long flags,
                                   unsigned long timeout)
    : vendorName(vendorName), protocol(protocol), baudRate(baudRate),
      flags(flags), timeout(timeout) {
}

// Implementasi copy constructor
ConnectionOptions::ConnectionOptions(const ConnectionOptions& other)
    : vendorName(other.vendorName),
      protocol(other.protocol),
      baudRate(other.baudRate),
      flags(other.flags),
      timeout(other.timeout) {
}

// Implementasi move constructor
ConnectionOptions::ConnectionOptions(ConnectionOptions&& other) noexcept
    : vendorName(std::move(other.vendorName)),
      protocol(other.protocol),
      baudRate(other.baudRate),
      flags(other.flags),
      timeout(other.timeout) {
}

// Implementasi copy assignment
ConnectionOptions& ConnectionOptions::operator=(const ConnectionOptions& other) {
    if (this != &other) {
        vendorName = other.vendorName;
        protocol = other.protocol;
        baudRate = other.baudRate;
        flags = other.flags;
        timeout = other.timeout;
    }
    return *this;
}

// Implementasi move assignment
ConnectionOptions& ConnectionOptions::operator=(ConnectionOptions&& other) noexcept {
    if (this != &other) {
        vendorName = std::move(other.vendorName);
        protocol = other.protocol;
        baudRate = other.baudRate;
        flags = other.flags;
        timeout = other.timeout;
    }
    return *this;
}

// Implementasi getVendorName
const std::string& ConnectionOptions::getVendorName() const {
    return vendorName;
}

// Implementasi getProtocol
unsigned long ConnectionOptions::getProtocol() const {
    return protocol;
}

// Implementasi getBaudRate
unsigned long ConnectionOptions::getBaudRate() const {
    return baudRate;
}

// Implementasi getFlags
unsigned long ConnectionOptions::getFlags() const {
    return flags;
}

// Implementasi getTimeout
unsigned long ConnectionOptions::getTimeout() const {
    return timeout;
}

// Implementasi setVendorName
void ConnectionOptions::setVendorName(const std::string& name) {
    vendorName = name;
}

// Implementasi setProtocol
void ConnectionOptions::setProtocol(unsigned long protocolId) {
    protocol = protocolId;
}

// Implementasi setBaudRate
void ConnectionOptions::setBaudRate(unsigned long rate) {
    baudRate = rate;
}

// Implementasi setFlags
void ConnectionOptions::setFlags(unsigned long flagsValue) {
    flags = flagsValue;
}

// Implementasi setTimeout
void ConnectionOptions::setTimeout(unsigned long timeoutValue) {
    timeout = timeoutValue;
}

// Implementasi toString
std::string ConnectionOptions::toString() const {
    std::stringstream ss;
    ss << "ConnectionOptions [Vendor: " << vendorName
       << ", Protocol: 0x" << std::hex << protocol
       << ", BaudRate: " << std::dec << baudRate
       << ", Flags: 0x" << std::hex << flags
       << ", Timeout: " << std::dec << timeout << "ms]";
    return ss.str();
}

// Operator perbandingan ==
bool ConnectionOptions::operator==(const ConnectionOptions& other) const {
    return vendorName == other.vendorName &&
           protocol == other.protocol &&
           baudRate == other.baudRate &&
           flags == other.flags &&
           timeout == other.timeout;
}

// Operator perbandingan !=
bool ConnectionOptions::operator!=(const ConnectionOptions& other) const {
    return !(*this == other);
}

} // namespace j2534
} // namespace fmus