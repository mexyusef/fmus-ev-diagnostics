#ifndef FMUS_CONFIG_H
#define FMUS_CONFIG_H

/**
 * @file config.h
 * @brief Configuration management system for FMUS-AUTO
 */

#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <variant>
#include <vector>

// Define exports macro for cross-platform compatibility
#ifdef _WIN32
    #ifdef FMUS_AUTO_EXPORTS
        #define FMUS_AUTO_API __declspec(dllexport)
    #else
        #define FMUS_AUTO_API __declspec(dllimport)
    #endif
#else
    #define FMUS_AUTO_API
#endif

namespace fmus {

/**
 * @brief Configuration value type
 */
using ConfigValue = std::variant<std::string, int, double, bool>;

/**
 * @brief Configuration management class
 */
class FMUS_AUTO_API Config {
public:
    /**
     * @brief Get the singleton instance
     */
    static std::shared_ptr<Config> getInstance();
    
    /**
     * @brief Load configuration from file
     */
    bool loadFromFile(const std::string& filename);
    
    /**
     * @brief Save configuration to file
     */
    bool saveToFile(const std::string& filename);
    
    /**
     * @brief Set a configuration value
     */
    void setValue(const std::string& key, const ConfigValue& value);
    
    /**
     * @brief Get a configuration value
     */
    ConfigValue getValue(const std::string& key, const ConfigValue& defaultValue = std::string(""));
    
    /**
     * @brief Get string value
     */
    std::string getString(const std::string& key, const std::string& defaultValue = "");
    
    /**
     * @brief Get integer value
     */
    int getInt(const std::string& key, int defaultValue = 0);
    
    /**
     * @brief Get double value
     */
    double getDouble(const std::string& key, double defaultValue = 0.0);
    
    /**
     * @brief Get boolean value
     */
    bool getBool(const std::string& key, bool defaultValue = false);
    
    /**
     * @brief Check if key exists
     */
    bool hasKey(const std::string& key) const;
    
    /**
     * @brief Remove a key
     */
    void removeKey(const std::string& key);
    
    /**
     * @brief Clear all configuration
     */
    void clear();
    
    /**
     * @brief Get all keys
     */
    std::vector<std::string> getAllKeys() const;
    
    /**
     * @brief Load default configuration
     */
    void loadDefaults();

    // Destructor needs to be public for shared_ptr
    ~Config();

private:
    Config();
    
    static std::shared_ptr<Config> instance;
    static std::mutex instanceMutex;
    
    std::map<std::string, ConfigValue> values;
    mutable std::mutex valuesMutex;
    
    std::string configFile;
};

// Default configuration keys
namespace ConfigKeys {
    // J2534 Settings
    constexpr const char* J2534_LIBRARY_PATH = "j2534.library_path";
    constexpr const char* J2534_DEVICE_ID = "j2534.device_id";
    constexpr const char* J2534_TIMEOUT = "j2534.timeout_ms";
    
    // CAN Settings
    constexpr const char* CAN_BAUDRATE = "can.baudrate";
    constexpr const char* CAN_EXTENDED_ADDRESSING = "can.extended_addressing";
    
    // UDS Settings
    constexpr const char* UDS_REQUEST_ID = "uds.request_id";
    constexpr const char* UDS_RESPONSE_ID = "uds.response_id";
    constexpr const char* UDS_TIMEOUT = "uds.timeout_ms";
    
    // OBD-II Settings
    constexpr const char* OBDII_PROTOCOL = "obdii.protocol";
    constexpr const char* OBDII_BAUDRATE = "obdii.baudrate";
    
    // Logging Settings
    constexpr const char* LOG_LEVEL = "logging.level";
    constexpr const char* LOG_FILE = "logging.file";
    constexpr const char* LOG_CONSOLE = "logging.console";
    
    // Security Settings
    constexpr const char* SECURITY_LEVEL = "security.level";
    constexpr const char* SECURITY_KEY_FILE = "security.key_file";
}

// Default configuration values
namespace ConfigDefaults {
    constexpr int J2534_TIMEOUT = 5000;
    constexpr int CAN_BAUDRATE = 500000;
    constexpr bool CAN_EXTENDED_ADDRESSING = false;
    constexpr int UDS_REQUEST_ID = 0x7E0;
    constexpr int UDS_RESPONSE_ID = 0x7E8;
    constexpr int UDS_TIMEOUT = 1000;
    constexpr const char* OBDII_PROTOCOL = "ISO15765";
    constexpr int OBDII_BAUDRATE = 500000;
    constexpr const char* LOG_LEVEL = "INFO";
    constexpr bool LOG_CONSOLE = true;
    constexpr int SECURITY_LEVEL = 1;
}

} // namespace fmus

#endif // FMUS_CONFIG_H
