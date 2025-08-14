#include <fmus/j2534.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace fmus {
namespace j2534 {

// Message implementation
std::string Message::toString() const {
    std::ostringstream ss;
    ss << "Message [Protocol: " << static_cast<uint32_t>(protocol)
       << ", ID: 0x" << std::hex << std::setw(8) << std::setfill('0') << id
       << ", Flags: 0x" << flags
       << ", Data: ";
    
    for (size_t i = 0; i < data.size(); ++i) {
        if (i > 0) ss << " ";
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    }
    
    ss << "]";
    return ss.str();
}

std::string Message::toHexString() const {
    std::ostringstream ss;
    for (size_t i = 0; i < data.size(); ++i) {
        if (i > 0) ss << " ";
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    }
    return ss.str();
}

Message Message::fromHexString(const std::string& hexStr, Protocol proto) {
    Message msg;
    msg.protocol = proto;
    
    std::string cleanHex = hexStr;
    // Remove spaces and convert to uppercase
    cleanHex.erase(std::remove_if(cleanHex.begin(), cleanHex.end(), ::isspace), cleanHex.end());
    std::transform(cleanHex.begin(), cleanHex.end(), cleanHex.begin(), ::toupper);
    
    // Parse hex string into bytes
    for (size_t i = 0; i < cleanHex.length(); i += 2) {
        if (i + 1 < cleanHex.length()) {
            std::string byteStr = cleanHex.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
            msg.data.push_back(byte);
        }
    }
    
    return msg;
}

// Filter implementation
std::string Filter::toString() const {
    std::ostringstream ss;
    ss << "Filter [Protocol: " << static_cast<uint32_t>(protocol)
       << ", Type: " << static_cast<uint32_t>(filterType)
       << ", MaskID: 0x" << std::hex << maskId
       << ", PatternID: 0x" << patternId
       << ", Flags: 0x" << flags << "]";
    return ss.str();
}

// AdapterInfo implementation
std::string AdapterInfo::toString() const {
    std::ostringstream ss;
    ss << "AdapterInfo [Vendor: " << vendorName
       << ", Device: " << deviceName
       << ", Path: " << libraryPath
       << ", ID: " << deviceId
       << ", Connected: " << (connected ? "Yes" : "No")
       << ", Protocols: ";
    
    for (size_t i = 0; i < supportedProtocols.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << static_cast<uint32_t>(supportedProtocols[i]);
    }
    
    ss << "]";
    return ss.str();
}

bool AdapterInfo::supportsProtocol(Protocol protocol) const {
    return std::find(supportedProtocols.begin(), supportedProtocols.end(), protocol) 
           != supportedProtocols.end();
}

// ConnectionOptions implementation
std::string ConnectionOptions::toString() const {
    std::ostringstream ss;
    ss << "ConnectionOptions [Vendor: " << vendorName
       << ", DeviceID: " << deviceId
       << ", Protocol: " << static_cast<uint32_t>(protocol)
       << ", BaudRate: " << static_cast<uint32_t>(baudRate)
       << ", Flags: 0x" << std::hex << flags << "]";
    return ss.str();
}

// ChannelConfig implementation
std::string ChannelConfig::toString() const {
    std::ostringstream ss;
    ss << "ChannelConfig [Protocol: " << static_cast<uint32_t>(protocol)
       << ", BaudRate: " << static_cast<uint32_t>(baudRate)
       << ", Flags: 0x" << std::hex << flags
       << ", Parameters: " << parameters.size() << "]";
    return ss.str();
}

} // namespace j2534
} // namespace fmus
