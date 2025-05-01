#include <fmus/j2534.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstring>

namespace fmus {
namespace j2534 {

// Implementasi kelas MessageBuilder
MessageBuilder::MessageBuilder() {
    reset();
}

// Reset semua field ke default values
void MessageBuilder::reset() {
    mProtocol = 0;  // CAN by default
    mId = 0;
    mData.clear();
    mFlags = 0;
    mTimestamp = 0;
}

// Set protokol message (CAN, ISO15765, dll)
MessageBuilder& MessageBuilder::protocol(unsigned long protocol) {
    mProtocol = protocol;
    return *this;
}

// Set message ID (CAN ID atau header untuk protokol lain)
MessageBuilder& MessageBuilder::id(unsigned long id) {
    mId = id;
    return *this;
}

// Set data message dari byte array
MessageBuilder& MessageBuilder::data(const std::vector<uint8_t>& data) {
    mData = data;
    return *this;
}

// Set data message dari pointer dan size
MessageBuilder& MessageBuilder::data(const uint8_t* data, size_t size) {
    mData.assign(data, data + size);
    return *this;
}

// Set flag message (extended ID, RTR, dll)
MessageBuilder& MessageBuilder::flags(unsigned long flags) {
    mFlags = flags;
    return *this;
}

// Set timestamp message
MessageBuilder& MessageBuilder::timestamp(unsigned long timestamp) {
    mTimestamp = timestamp;
    return *this;
}

// Build message object final
Message MessageBuilder::build() const {
    return Message(*this);
}

// Implementasi constructor Message dari Builder
Message::Message(const MessageBuilder& builder)
    : protocol(builder.mProtocol),
      id(builder.mId),
      data(builder.mData),
      flags(builder.mFlags),
      timestamp(builder.mTimestamp) {
}

// Implementasi default constructor Message
Message::Message()
    : protocol(0),
      id(0),
      data(),
      flags(0),
      timestamp(0) {
}

// Implementasi copy constructor
Message::Message(const Message& other)
    : protocol(other.protocol),
      id(other.id),
      data(other.data),
      flags(other.flags),
      timestamp(other.timestamp) {
}

// Implementasi move constructor
Message::Message(Message&& other) noexcept
    : protocol(other.protocol),
      id(other.id),
      data(std::move(other.data)),
      flags(other.flags),
      timestamp(other.timestamp) {
}

// Implementasi copy assignment
Message& Message::operator=(const Message& other) {
    if (this != &other) {
        protocol = other.protocol;
        id = other.id;
        data = other.data;
        flags = other.flags;
        timestamp = other.timestamp;
    }
    return *this;
}

// Implementasi move assignment
Message& Message::operator=(Message&& other) noexcept {
    if (this != &other) {
        protocol = other.protocol;
        id = other.id;
        data = std::move(other.data);
        flags = other.flags;
        timestamp = other.timestamp;
    }
    return *this;
}

// Implementasi getProtocol
unsigned long Message::getProtocol() const {
    return protocol;
}

// Implementasi getId
unsigned long Message::getId() const {
    return id;
}

// Implementasi getData
const std::vector<uint8_t>& Message::getData() const {
    return data;
}

// Implementasi getFlags
unsigned long Message::getFlags() const {
    return flags;
}

// Implementasi getTimestamp
unsigned long Message::getTimestamp() const {
    return timestamp;
}

// Implementasi getDataSize
size_t Message::getDataSize() const {
    return data.size();
}

// Implementasi toString
std::string Message::toString() const {
    std::ostringstream ss;
    ss << "Message [Protocol: 0x" << std::hex << protocol
       << ", ID: 0x" << std::setw(8) << std::setfill('0') << id
       << ", Flags: 0x" << flags
       << ", Timestamp: " << std::dec << timestamp
       << ", Data: ";

    for (const auto& byte : data) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(byte) << " ";
    }

    ss << "]";
    return ss.str();
}

// Implementasi toHexString
std::string Message::toHexString() const {
    std::ostringstream ss;

    // Format ID sesuai dengan jenis protokol
    if (protocol == 1) { // CAN
        // Format CAN ID
        if (flags & 0x00000100) { // Extended CAN ID
            ss << std::hex << std::setw(8) << std::setfill('0') << id;
        } else { // Standard CAN ID
            ss << std::hex << std::setw(3) << std::setfill('0') << id;
        }
    } else {
        // Format ID untuk protokol lain
        ss << std::hex << std::setw(8) << std::setfill('0') << id;
    }

    ss << "#";

    // Format data bytes
    for (const auto& byte : data) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(byte);
    }

    return ss.str();
}

// Helper statik untuk membuat Message dari format hex string
Message Message::fromHexString(const std::string& hexStr, unsigned long protocol) {
    MessageBuilder builder;
    builder.protocol(protocol);

    // Format: "ID#DATA" atau hanya "DATA"
    size_t hashPos = hexStr.find('#');

    if (hashPos != std::string::npos) {
        // Parse ID
        std::string idStr = hexStr.substr(0, hashPos);
        unsigned long id = std::stoul(idStr, nullptr, 16);
        builder.id(id);

        // Tentukan flag berdasarkan panjang string ID untuk CAN
        if (protocol == 1) { // CAN
            if (idStr.length() > 3) {
                builder.flags(0x00000100); // Extended CAN ID
            }
        }

        // Parse data
        std::string dataStr = hexStr.substr(hashPos + 1);
        std::vector<uint8_t> data;

        for (size_t i = 0; i < dataStr.length(); i += 2) {
            if (i + 1 < dataStr.length()) {
                std::string byteStr = dataStr.substr(i, 2);
                uint8_t byte = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
                data.push_back(byte);
            }
        }

        builder.data(data);
    } else {
        // Jika tidak ada ID, anggap seluruh string adalah data
        std::vector<uint8_t> data;

        for (size_t i = 0; i < hexStr.length(); i += 2) {
            if (i + 1 < hexStr.length()) {
                std::string byteStr = hexStr.substr(i, 2);
                uint8_t byte = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
                data.push_back(byte);
            }
        }

        builder.data(data);
    }

    return builder.build();
}

// Operator perbandingan ==
bool Message::operator==(const Message& other) const {
    return protocol == other.protocol &&
           id == other.id &&
           data == other.data &&
           flags == other.flags;
    // Timestamp biasanya tidak dibandingkan karena akan selalu berbeda
}

// Operator perbandingan !=
bool Message::operator!=(const Message& other) const {
    return !(*this == other);
}

} // namespace j2534
} // namespace fmus