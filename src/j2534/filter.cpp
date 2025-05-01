#include <fmus/j2534.h>
#include <sstream>
#include <iomanip>

namespace fmus {
namespace j2534 {

// Implementasi FilterBuilder constructor
FilterBuilder::FilterBuilder() {
    reset();
}

// Reset semua field ke default values
void FilterBuilder::reset() {
    mProtocol = 0;  // CAN by default
    mFilterType = 0; // PASS_FILTER by default
    mMaskId = 0;
    mPatternId = 0;
    mMaskData.clear();
    mPatternData.clear();
    mFlowControlData.clear();
    mFlags = 0;
}

// Set protocol filter (CAN, ISO15765, dll)
FilterBuilder& FilterBuilder::protocol(unsigned long protocol) {
    mProtocol = protocol;
    return *this;
}

// Set filter type (PASS_FILTER, BLOCK_FILTER, FLOW_CONTROL_FILTER)
FilterBuilder& FilterBuilder::filterType(unsigned long type) {
    mFilterType = type;
    return *this;
}

// Set mask ID untuk filter
FilterBuilder& FilterBuilder::maskId(unsigned long id) {
    mMaskId = id;
    return *this;
}

// Set pattern ID untuk filter
FilterBuilder& FilterBuilder::patternId(unsigned long id) {
    mPatternId = id;
    return *this;
}

// Set mask data dari byte array
FilterBuilder& FilterBuilder::maskData(const std::vector<uint8_t>& data) {
    mMaskData = data;
    return *this;
}

// Set mask data dari pointer dan size
FilterBuilder& FilterBuilder::maskData(const uint8_t* data, size_t size) {
    mMaskData.assign(data, data + size);
    return *this;
}

// Set pattern data dari byte array
FilterBuilder& FilterBuilder::patternData(const std::vector<uint8_t>& data) {
    mPatternData = data;
    return *this;
}

// Set pattern data dari pointer dan size
FilterBuilder& FilterBuilder::patternData(const uint8_t* data, size_t size) {
    mPatternData.assign(data, data + size);
    return *this;
}

// Set flow control data dari byte array
FilterBuilder& FilterBuilder::flowControlData(const std::vector<uint8_t>& data) {
    mFlowControlData = data;
    return *this;
}

// Set flow control data dari pointer dan size
FilterBuilder& FilterBuilder::flowControlData(const uint8_t* data, size_t size) {
    mFlowControlData.assign(data, data + size);
    return *this;
}

// Set flag filter
FilterBuilder& FilterBuilder::flags(unsigned long flags) {
    mFlags = flags;
    return *this;
}

// Build filter object final
Filter FilterBuilder::build() const {
    return Filter(*this);
}

// Implementasi constructor Filter dari Builder
Filter::Filter(const FilterBuilder& builder)
    : protocol(builder.mProtocol),
      filterType(builder.mFilterType),
      maskId(builder.mMaskId),
      patternId(builder.mPatternId),
      maskData(builder.mMaskData),
      patternData(builder.mPatternData),
      flowControlData(builder.mFlowControlData),
      flags(builder.mFlags) {
}

// Implementasi default constructor Filter
Filter::Filter()
    : protocol(0),
      filterType(0),
      maskId(0),
      patternId(0),
      maskData(),
      patternData(),
      flowControlData(),
      flags(0) {
}

// Implementasi copy constructor
Filter::Filter(const Filter& other)
    : protocol(other.protocol),
      filterType(other.filterType),
      maskId(other.maskId),
      patternId(other.patternId),
      maskData(other.maskData),
      patternData(other.patternData),
      flowControlData(other.flowControlData),
      flags(other.flags) {
}

// Implementasi move constructor
Filter::Filter(Filter&& other) noexcept
    : protocol(other.protocol),
      filterType(other.filterType),
      maskId(other.maskId),
      patternId(other.patternId),
      maskData(std::move(other.maskData)),
      patternData(std::move(other.patternData)),
      flowControlData(std::move(other.flowControlData)),
      flags(other.flags) {
}

// Implementasi copy assignment
Filter& Filter::operator=(const Filter& other) {
    if (this != &other) {
        protocol = other.protocol;
        filterType = other.filterType;
        maskId = other.maskId;
        patternId = other.patternId;
        maskData = other.maskData;
        patternData = other.patternData;
        flowControlData = other.flowControlData;
        flags = other.flags;
    }
    return *this;
}

// Implementasi move assignment
Filter& Filter::operator=(Filter&& other) noexcept {
    if (this != &other) {
        protocol = other.protocol;
        filterType = other.filterType;
        maskId = other.maskId;
        patternId = other.patternId;
        maskData = std::move(other.maskData);
        patternData = std::move(other.patternData);
        flowControlData = std::move(other.flowControlData);
        flags = other.flags;
    }
    return *this;
}

// Implementasi getProtocol
unsigned long Filter::getProtocol() const {
    return protocol;
}

// Implementasi getFilterType
unsigned long Filter::getFilterType() const {
    return filterType;
}

// Implementasi getMaskId
unsigned long Filter::getMaskId() const {
    return maskId;
}

// Implementasi getPatternId
unsigned long Filter::getPatternId() const {
    return patternId;
}

// Implementasi getMaskData
const std::vector<uint8_t>& Filter::getMaskData() const {
    return maskData;
}

// Implementasi getPatternData
const std::vector<uint8_t>& Filter::getPatternData() const {
    return patternData;
}

// Implementasi getFlowControlData
const std::vector<uint8_t>& Filter::getFlowControlData() const {
    return flowControlData;
}

// Implementasi getFlags
unsigned long Filter::getFlags() const {
    return flags;
}

// Utility function untuk memformat data bytes
std::string formatDataBytes(const std::vector<uint8_t>& data) {
    std::stringstream ss;

    for (const auto& byte : data) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(byte) << " ";
    }

    return ss.str();
}

// Implementasi toString
std::string Filter::toString() const {
    std::stringstream ss;
    ss << "Filter [Type: ";

    // Konversi jenis filter menjadi string
    switch (filterType) {
        case 0:
            ss << "PASS_FILTER";
            break;
        case 1:
            ss << "BLOCK_FILTER";
            break;
        case 2:
            ss << "FLOW_CONTROL_FILTER";
            break;
        default:
            ss << "UNKNOWN(" << filterType << ")";
    }

    ss << ", Protocol: 0x" << std::hex << protocol
       << ", Mask ID: 0x" << std::setw(8) << std::setfill('0') << maskId
       << ", Pattern ID: 0x" << std::setw(8) << std::setfill('0') << patternId
       << ", Flags: 0x" << flags;

    if (!maskData.empty()) {
        ss << ", Mask Data: " << formatDataBytes(maskData);
    }

    if (!patternData.empty()) {
        ss << ", Pattern Data: " << formatDataBytes(patternData);
    }

    if (!flowControlData.empty()) {
        ss << ", FlowControl Data: " << formatDataBytes(flowControlData);
    }

    ss << "]";
    return ss.str();
}

// Operator perbandingan ==
bool Filter::operator==(const Filter& other) const {
    return protocol == other.protocol &&
           filterType == other.filterType &&
           maskId == other.maskId &&
           patternId == other.patternId &&
           maskData == other.maskData &&
           patternData == other.patternData &&
           flowControlData == other.flowControlData &&
           flags == other.flags;
}

// Operator perbandingan !=
bool Filter::operator!=(const Filter& other) const {
    return !(*this == other);
}

} // namespace j2534
} // namespace fmus