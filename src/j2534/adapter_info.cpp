#include <fmus/j2534.h>
#include <fmus/auto.h>
#include <sstream>

namespace fmus {
namespace j2534 {

// Implementasi constructor default
AdapterInfo::AdapterInfo()
    : vendorName("Unknown Vendor"), deviceName("Unknown Device"), libraryPath(""),
      deviceId(0), connected(false) {
}

// Implementasi copy constructor
AdapterInfo::AdapterInfo(const AdapterInfo& other)
    : vendorName(other.vendorName),
      deviceName(other.deviceName),
      libraryPath(other.libraryPath),
      deviceId(other.deviceId),
      connected(other.connected) {
}

// Implementasi move constructor
AdapterInfo::AdapterInfo(AdapterInfo&& other) noexcept
    : vendorName(std::move(other.vendorName)),
      deviceName(std::move(other.deviceName)),
      libraryPath(std::move(other.libraryPath)),
      deviceId(other.deviceId),
      connected(other.connected) {

    other.deviceId = 0;
    other.connected = false;
}

// Implementasi copy assignment
AdapterInfo& AdapterInfo::operator=(const AdapterInfo& other) {
    if (this != &other) {
        vendorName = other.vendorName;
        deviceName = other.deviceName;
        libraryPath = other.libraryPath;
        deviceId = other.deviceId;
        connected = other.connected;
    }
    return *this;
}

// Implementasi move assignment
AdapterInfo& AdapterInfo::operator=(AdapterInfo&& other) noexcept {
    if (this != &other) {
        vendorName = std::move(other.vendorName);
        deviceName = std::move(other.deviceName);
        libraryPath = std::move(other.libraryPath);
        deviceId = other.deviceId;
        connected = other.connected;

        other.deviceId = 0;
        other.connected = false;
    }
    return *this;
}

// Implementasi toString
std::string AdapterInfo::toString() const {
    std::stringstream ss;
    ss << "J2534 Adapter: " << vendorName << " - " << deviceName
       << " (Library: " << libraryPath << ")";
    if (connected) {
        ss << " [Connected, ID: " << deviceId << "]";
    }
    return ss.str();
}

// Implementasi getVendorName
const std::string& AdapterInfo::getVendorName() const {
    return vendorName;
}

// Implementasi getDeviceName
const std::string& AdapterInfo::getDeviceName() const {
    return deviceName;
}

// Implementasi getLibraryPath
const std::string& AdapterInfo::getLibraryPath() const {
    return libraryPath;
}

// Implementasi getDeviceId
unsigned long AdapterInfo::getDeviceId() const {
    return deviceId;
}

// Implementasi isConnected
bool AdapterInfo::isConnected() const {
    return connected;
}

// Implementasi setVendorName
void AdapterInfo::setVendorName(const std::string& name) {
    vendorName = name;
}

// Implementasi setDeviceName
void AdapterInfo::setDeviceName(const std::string& name) {
    deviceName = name;
}

// Implementasi setLibraryPath
void AdapterInfo::setLibraryPath(const std::string& path) {
    libraryPath = path;
}

// Implementasi setDeviceId
void AdapterInfo::setDeviceId(unsigned long id) {
    deviceId = id;
}

// Implementasi setConnected
void AdapterInfo::setConnected(bool isConnected) {
    connected = isConnected;
}

} // namespace j2534
} // namespace fmus