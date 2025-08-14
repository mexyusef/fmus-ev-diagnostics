#include <fmus/config.h>
#include <fmus/logger.h>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace fmus {

// Static member definitions
std::shared_ptr<Config> Config::instance = nullptr;
std::mutex Config::instanceMutex;

Config::Config() {
    loadDefaults();
}

Config::~Config() = default;

std::shared_ptr<Config> Config::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance) {
        instance = std::shared_ptr<Config>(new Config());
    }
    return instance;
}

bool Config::loadFromFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(valuesMutex);
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        auto logger = Logger::getInstance();
        logger->warning("Could not open config file: " + filename);
        return false;
    }
    
    configFile = filename;
    std::string line;
    int lineNumber = 0;
    
    while (std::getline(file, line)) {
        lineNumber++;
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // Find the '=' separator
        size_t equalPos = line.find('=');
        if (equalPos == std::string::npos) {
            auto logger = Logger::getInstance();
            logger->warning("Invalid config line " + std::to_string(lineNumber) + ": " + line);
            continue;
        }
        
        std::string key = line.substr(0, equalPos);
        std::string value = line.substr(equalPos + 1);
        
        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        // Parse value type
        ConfigValue configValue;
        
        // Check for boolean
        if (value == "true" || value == "false") {
            configValue = (value == "true");
        }
        // Check for integer
        else if (value.find_first_not_of("0123456789-") == std::string::npos) {
            try {
                configValue = std::stoi(value);
            } catch (...) {
                configValue = value; // Fallback to string
            }
        }
        // Check for double
        else if (value.find_first_not_of("0123456789.-") == std::string::npos && 
                 value.find('.') != std::string::npos) {
            try {
                configValue = std::stod(value);
            } catch (...) {
                configValue = value; // Fallback to string
            }
        }
        // Default to string
        else {
            configValue = value;
        }
        
        values[key] = configValue;
    }
    
    auto logger = Logger::getInstance();
    logger->info("Loaded configuration from: " + filename);
    return true;
}

bool Config::saveToFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(valuesMutex);
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        auto logger = Logger::getInstance();
        logger->error("Could not open config file for writing: " + filename);
        return false;
    }
    
    file << "# FMUS-AUTO Configuration File\n";
    file << "# Generated automatically\n\n";
    
    for (const auto& [key, value] : values) {
        file << key << "=";
        
        std::visit([&file](const auto& v) {
            if constexpr (std::is_same_v<std::decay_t<decltype(v)>, bool>) {
                file << (v ? "true" : "false");
            } else {
                file << v;
            }
        }, value);
        
        file << "\n";
    }
    
    configFile = filename;
    auto logger = Logger::getInstance();
    logger->info("Saved configuration to: " + filename);
    return true;
}

void Config::setValue(const std::string& key, const ConfigValue& value) {
    std::lock_guard<std::mutex> lock(valuesMutex);
    values[key] = value;
}

ConfigValue Config::getValue(const std::string& key, const ConfigValue& defaultValue) {
    std::lock_guard<std::mutex> lock(valuesMutex);
    auto it = values.find(key);
    return (it != values.end()) ? it->second : defaultValue;
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) {
    auto value = getValue(key, defaultValue);
    if (std::holds_alternative<std::string>(value)) {
        return std::get<std::string>(value);
    }
    return defaultValue;
}

int Config::getInt(const std::string& key, int defaultValue) {
    auto value = getValue(key, defaultValue);
    if (std::holds_alternative<int>(value)) {
        return std::get<int>(value);
    }
    return defaultValue;
}

double Config::getDouble(const std::string& key, double defaultValue) {
    auto value = getValue(key, defaultValue);
    if (std::holds_alternative<double>(value)) {
        return std::get<double>(value);
    }
    return defaultValue;
}

bool Config::getBool(const std::string& key, bool defaultValue) {
    auto value = getValue(key, defaultValue);
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value);
    }
    return defaultValue;
}

bool Config::hasKey(const std::string& key) const {
    std::lock_guard<std::mutex> lock(valuesMutex);
    return values.find(key) != values.end();
}

void Config::removeKey(const std::string& key) {
    std::lock_guard<std::mutex> lock(valuesMutex);
    values.erase(key);
}

void Config::clear() {
    std::lock_guard<std::mutex> lock(valuesMutex);
    values.clear();
}

std::vector<std::string> Config::getAllKeys() const {
    std::lock_guard<std::mutex> lock(valuesMutex);
    std::vector<std::string> keys;
    keys.reserve(values.size());
    
    for (const auto& [key, value] : values) {
        keys.push_back(key);
    }
    
    return keys;
}

void Config::loadDefaults() {
    // J2534 defaults
    values[ConfigKeys::J2534_TIMEOUT] = ConfigDefaults::J2534_TIMEOUT;

    // CAN defaults
    values[ConfigKeys::CAN_BAUDRATE] = ConfigDefaults::CAN_BAUDRATE;
    values[ConfigKeys::CAN_EXTENDED_ADDRESSING] = ConfigDefaults::CAN_EXTENDED_ADDRESSING;

    // UDS defaults
    values[ConfigKeys::UDS_REQUEST_ID] = ConfigDefaults::UDS_REQUEST_ID;
    values[ConfigKeys::UDS_RESPONSE_ID] = ConfigDefaults::UDS_RESPONSE_ID;
    values[ConfigKeys::UDS_TIMEOUT] = ConfigDefaults::UDS_TIMEOUT;

    // OBD-II defaults
    values[ConfigKeys::OBDII_PROTOCOL] = std::string(ConfigDefaults::OBDII_PROTOCOL);
    values[ConfigKeys::OBDII_BAUDRATE] = ConfigDefaults::OBDII_BAUDRATE;

    // Logging defaults
    values[ConfigKeys::LOG_LEVEL] = std::string(ConfigDefaults::LOG_LEVEL);
    values[ConfigKeys::LOG_CONSOLE] = ConfigDefaults::LOG_CONSOLE;

    // Security defaults
    values[ConfigKeys::SECURITY_LEVEL] = ConfigDefaults::SECURITY_LEVEL;
}

} // namespace fmus
