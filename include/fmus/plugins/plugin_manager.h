#ifndef FMUS_PLUGINS_PLUGIN_MANAGER_H
#define FMUS_PLUGINS_PLUGIN_MANAGER_H

/**
 * @file plugin_manager.h
 * @brief Plugin System for Extensible Diagnostics
 */

#include <fmus/auto.h>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <functional>

// Define exports macro
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
namespace plugins {

/**
 * @brief Plugin interface version
 */
constexpr int PLUGIN_INTERFACE_VERSION = 1;

/**
 * @brief Plugin types
 */
enum class PluginType {
    DIAGNOSTIC,         ///< Diagnostic protocol plugin
    VEHICLE_SPECIFIC,   ///< Vehicle-specific plugin
    TOOL,              ///< Diagnostic tool plugin
    EXPORT,            ///< Data export plugin
    VISUALIZATION,     ///< Data visualization plugin
    CUSTOM             ///< Custom plugin type
};

/**
 * @brief Plugin information
 */
struct PluginInfo {
    std::string name;
    std::string description;
    std::string version;
    std::string author;
    std::string website;
    PluginType type = PluginType::CUSTOM;
    int interfaceVersion = PLUGIN_INTERFACE_VERSION;
    std::vector<std::string> dependencies;
    std::map<std::string, std::string> metadata;
    
    std::string toString() const;
};

/**
 * @brief Plugin interface base class
 */
class FMUS_AUTO_API IPlugin {
public:
    virtual ~IPlugin() = default;
    
    /**
     * @brief Get plugin information
     */
    virtual PluginInfo getInfo() const = 0;
    
    /**
     * @brief Initialize plugin
     */
    virtual bool initialize(std::shared_ptr<Auto> autoInstance) = 0;
    
    /**
     * @brief Shutdown plugin
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief Check if plugin is initialized
     */
    virtual bool isInitialized() const = 0;
    
    /**
     * @brief Execute plugin command
     */
    virtual std::string executeCommand(const std::string& command, 
                                      const std::map<std::string, std::string>& parameters) = 0;
    
    /**
     * @brief Get plugin capabilities
     */
    virtual std::vector<std::string> getCapabilities() const = 0;
};

/**
 * @brief Diagnostic Plugin interface
 */
class FMUS_AUTO_API IDiagnosticPlugin : public IPlugin {
public:
    /**
     * @brief Get supported protocols
     */
    virtual std::vector<std::string> getSupportedProtocols() const = 0;
    
    /**
     * @brief Get supported ECU types
     */
    virtual std::vector<ECUType> getSupportedECUTypes() const = 0;
    
    /**
     * @brief Perform diagnostic operation
     */
    virtual std::map<std::string, std::string> performDiagnostic(
        const std::string& operation,
        const std::map<std::string, std::string>& parameters) = 0;
};

/**
 * @brief Vehicle-Specific Plugin interface
 */
class FMUS_AUTO_API IVehiclePlugin : public IPlugin {
public:
    /**
     * @brief Get supported vehicle makes
     */
    virtual std::vector<std::string> getSupportedMakes() const = 0;
    
    /**
     * @brief Get supported models for make
     */
    virtual std::vector<std::string> getSupportedModels(const std::string& make) const = 0;
    
    /**
     * @brief Get vehicle-specific diagnostic procedures
     */
    virtual std::vector<std::string> getDiagnosticProcedures(
        const std::string& make, const std::string& model, int year) const = 0;
};

/**
 * @brief Plugin loading result
 */
struct PluginLoadResult {
    bool success = false;
    std::string errorMessage;
    std::shared_ptr<IPlugin> plugin;
    
    PluginLoadResult() = default;
    PluginLoadResult(bool s, const std::string& error = "") : success(s), errorMessage(error) {}
    
    std::string toString() const;
};

/**
 * @brief Plugin Manager
 */
class FMUS_AUTO_API PluginManager {
public:
    /**
     * @brief Get singleton instance
     */
    static PluginManager* getInstance();
    
    /**
     * @brief Initialize plugin manager
     */
    bool initialize(std::shared_ptr<Auto> autoInstance);
    
    /**
     * @brief Shutdown plugin manager
     */
    void shutdown();
    
    /**
     * @brief Load plugin from file
     */
    PluginLoadResult loadPlugin(const std::string& filePath);
    
    /**
     * @brief Unload plugin
     */
    bool unloadPlugin(const std::string& pluginName);
    
    /**
     * @brief Load plugins from directory
     */
    std::vector<PluginLoadResult> loadPluginsFromDirectory(const std::string& directory);
    
    /**
     * @brief Get loaded plugins
     */
    std::vector<std::shared_ptr<IPlugin>> getLoadedPlugins() const;
    
    /**
     * @brief Get plugin by name
     */
    std::shared_ptr<IPlugin> getPlugin(const std::string& name) const;
    
    /**
     * @brief Get plugins by type
     */
    std::vector<std::shared_ptr<IPlugin>> getPluginsByType(PluginType type) const;
    
    /**
     * @brief Check if plugin is loaded
     */
    bool isPluginLoaded(const std::string& name) const;
    
    /**
     * @brief Get plugin information
     */
    std::vector<PluginInfo> getPluginInfos() const;
    
    /**
     * @brief Register plugin event handler
     */
    void registerEventHandler(const std::string& event, 
                             std::function<void(const std::string&, const std::map<std::string, std::string>&)> handler);
    
    /**
     * @brief Trigger plugin event
     */
    void triggerEvent(const std::string& event, const std::map<std::string, std::string>& data);
    
    /**
     * @brief Get plugin statistics
     */
    struct Statistics {
        uint32_t pluginsLoaded = 0;
        uint32_t pluginsFailed = 0;
        uint32_t commandsExecuted = 0;
        uint32_t eventsTriggered = 0;
        std::chrono::system_clock::time_point startTime;
    };
    
    Statistics getStatistics() const;
    void resetStatistics();

private:
    PluginManager() = default;
    ~PluginManager() = default;
    
    class Impl;
    std::unique_ptr<Impl> pImpl;
    static PluginManager* instance;
};

/**
 * @brief Plugin factory function type
 */
using PluginFactoryFunction = std::function<std::shared_ptr<IPlugin>()>;

/**
 * @brief Plugin registration helper
 */
class FMUS_AUTO_API PluginRegistry {
public:
    /**
     * @brief Register plugin factory
     */
    static void registerPlugin(const std::string& name, PluginFactoryFunction factory);
    
    /**
     * @brief Create plugin instance
     */
    static std::shared_ptr<IPlugin> createPlugin(const std::string& name);
    
    /**
     * @brief Get registered plugin names
     */
    static std::vector<std::string> getRegisteredPlugins();

private:
    static std::map<std::string, PluginFactoryFunction> factories;
};

/**
 * @brief Plugin registration macro
 */
#define REGISTER_PLUGIN(name, className) \
    static bool registered_##className = []() { \
        PluginRegistry::registerPlugin(name, []() -> std::shared_ptr<IPlugin> { \
            return std::make_shared<className>(); \
        }); \
        return true; \
    }();

/**
 * @brief Plugin export functions (for dynamic loading)
 */
extern "C" {
    /**
     * @brief Get plugin interface version
     */
    FMUS_AUTO_API int getPluginInterfaceVersion();
    
    /**
     * @brief Create plugin instance
     */
    FMUS_AUTO_API IPlugin* createPlugin();
    
    /**
     * @brief Destroy plugin instance
     */
    FMUS_AUTO_API void destroyPlugin(IPlugin* plugin);
    
    /**
     * @brief Get plugin information
     */
    FMUS_AUTO_API PluginInfo getPluginInfo();
}

/**
 * @brief Plugin Error
 */
class FMUS_AUTO_API PluginError : public std::runtime_error {
public:
    enum class ErrorCode {
        LOAD_FAILED,
        INITIALIZATION_FAILED,
        DEPENDENCY_MISSING,
        VERSION_MISMATCH,
        EXECUTION_FAILED
    };
    
    PluginError(ErrorCode code, const std::string& message)
        : std::runtime_error(message), errorCode(code) {}
    
    ErrorCode getErrorCode() const { return errorCode; }

private:
    ErrorCode errorCode;
};

// Utility functions
FMUS_AUTO_API std::string pluginTypeToString(PluginType type);
FMUS_AUTO_API PluginType stringToPluginType(const std::string& str);
FMUS_AUTO_API bool isValidPluginFile(const std::string& filePath);
FMUS_AUTO_API std::string getPluginDirectory();

} // namespace plugins
} // namespace fmus

#endif // FMUS_PLUGINS_PLUGIN_MANAGER_H
