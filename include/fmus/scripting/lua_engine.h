#ifndef FMUS_SCRIPTING_LUA_ENGINE_H
#define FMUS_SCRIPTING_LUA_ENGINE_H

/**
 * @file lua_engine.h
 * @brief Lua Scripting Engine for Custom Diagnostics
 */

#include <fmus/auto.h>
#include <fmus/ecu.h>
#include <fmus/diagnostics/uds.h>
#include <fmus/diagnostics/obdii.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <variant>

// Forward declare Lua state
struct lua_State;

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
namespace scripting {

/**
 * @brief Lua value types
 */
using LuaValue = std::variant<
    std::nullptr_t,
    bool,
    int64_t,
    double,
    std::string,
    std::vector<uint8_t>
>;

/**
 * @brief Lua function result
 */
struct LuaResult {
    bool success = false;
    std::string error;
    std::vector<LuaValue> values;
    
    LuaResult() = default;
    LuaResult(bool s, const std::string& e = "") : success(s), error(e) {}
    
    template<typename T>
    T getValue(size_t index = 0) const {
        if (index >= values.size()) {
            return T{};
        }
        
        if (std::holds_alternative<T>(values[index])) {
            return std::get<T>(values[index]);
        }
        
        return T{};
    }
    
    std::string toString() const;
};

/**
 * @brief Lua script execution context
 */
struct ScriptContext {
    std::shared_ptr<Auto> autoInstance;
    std::shared_ptr<ECU> currentECU;
    std::shared_ptr<diagnostics::UDSClient> udsClient;
    std::shared_ptr<diagnostics::OBDClient> obdClient;
    std::map<std::string, LuaValue> variables;
    
    ScriptContext() = default;
    ScriptContext(std::shared_ptr<Auto> autoInst) : autoInstance(autoInst) {}
};

/**
 * @brief Lua script information
 */
struct ScriptInfo {
    std::string name;
    std::string description;
    std::string author;
    std::string version;
    std::vector<std::string> requiredECUs;
    std::vector<std::string> supportedProtocols;
    std::map<std::string, std::string> metadata;
    
    std::string toString() const;
};

/**
 * @brief Lua Engine for custom diagnostic scripts
 */
class FMUS_AUTO_API LuaEngine {
public:
    /**
     * @brief Constructor
     */
    LuaEngine();
    
    /**
     * @brief Destructor
     */
    ~LuaEngine();
    
    /**
     * @brief Initialize Lua engine
     */
    bool initialize();
    
    /**
     * @brief Shutdown Lua engine
     */
    void shutdown();
    
    /**
     * @brief Check if engine is initialized
     */
    bool isInitialized() const;
    
    /**
     * @brief Set script execution context
     */
    void setContext(const ScriptContext& context);
    
    /**
     * @brief Get current context
     */
    ScriptContext getContext() const;
    
    /**
     * @brief Load script from file
     */
    bool loadScript(const std::string& filePath);
    
    /**
     * @brief Load script from string
     */
    bool loadScriptFromString(const std::string& script, const std::string& name = "inline");
    
    /**
     * @brief Execute loaded script
     */
    LuaResult executeScript();
    
    /**
     * @brief Execute script function
     */
    LuaResult callFunction(const std::string& functionName, const std::vector<LuaValue>& args = {});
    
    /**
     * @brief Execute script string directly
     */
    LuaResult execute(const std::string& script);
    
    /**
     * @brief Get script information
     */
    ScriptInfo getScriptInfo() const;
    
    /**
     * @brief Set global variable
     */
    void setGlobal(const std::string& name, const LuaValue& value);
    
    /**
     * @brief Get global variable
     */
    LuaValue getGlobal(const std::string& name) const;
    
    /**
     * @brief Register custom function
     */
    void registerFunction(const std::string& name, std::function<LuaResult(const std::vector<LuaValue>&)> func);
    
    /**
     * @brief Get available functions
     */
    std::vector<std::string> getAvailableFunctions() const;
    
    /**
     * @brief Get last error
     */
    std::string getLastError() const;
    
    /**
     * @brief Clear error state
     */
    void clearError();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Script Library Manager
 */
class FMUS_AUTO_API ScriptLibrary {
public:
    /**
     * @brief Constructor
     */
    ScriptLibrary();
    
    /**
     * @brief Destructor
     */
    ~ScriptLibrary();
    
    /**
     * @brief Load scripts from directory
     */
    bool loadScriptsFromDirectory(const std::string& directory);
    
    /**
     * @brief Add script to library
     */
    bool addScript(const std::string& name, const std::string& filePath);
    
    /**
     * @brief Remove script from library
     */
    void removeScript(const std::string& name);
    
    /**
     * @brief Get available scripts
     */
    std::vector<std::string> getAvailableScripts() const;
    
    /**
     * @brief Get script information
     */
    ScriptInfo getScriptInfo(const std::string& name) const;
    
    /**
     * @brief Execute script by name
     */
    LuaResult executeScript(const std::string& name, const ScriptContext& context);
    
    /**
     * @brief Find scripts for ECU type
     */
    std::vector<std::string> findScriptsForECU(ECUType ecuType) const;
    
    /**
     * @brief Find scripts for protocol
     */
    std::vector<std::string> findScriptsForProtocol(const std::string& protocol) const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Lua scripting error
 */
class FMUS_AUTO_API LuaError : public std::runtime_error {
public:
    enum class ErrorType {
        INITIALIZATION_FAILED,
        SCRIPT_LOAD_FAILED,
        EXECUTION_FAILED,
        FUNCTION_NOT_FOUND,
        TYPE_ERROR,
        RUNTIME_ERROR
    };
    
    LuaError(ErrorType type, const std::string& message, int line = -1)
        : std::runtime_error(message), errorType(type), lineNumber(line) {}
    
    ErrorType getErrorType() const { return errorType; }
    int getLineNumber() const { return lineNumber; }

private:
    ErrorType errorType;
    int lineNumber;
};

// Utility functions
FMUS_AUTO_API std::string luaValueToString(const LuaValue& value);
FMUS_AUTO_API LuaValue stringToLuaValue(const std::string& str, const std::string& type = "string");
FMUS_AUTO_API std::string luaErrorTypeToString(LuaError::ErrorType type);

// Built-in Lua functions for FMUS
namespace lua_functions {

/**
 * @brief Log message from Lua script
 */
FMUS_AUTO_API LuaResult log(const std::vector<LuaValue>& args);

/**
 * @brief Sleep for specified milliseconds
 */
FMUS_AUTO_API LuaResult sleep(const std::vector<LuaValue>& args);

/**
 * @brief Send UDS request
 */
FMUS_AUTO_API LuaResult uds_request(const std::vector<LuaValue>& args);

/**
 * @brief Read OBD parameter
 */
FMUS_AUTO_API LuaResult obd_read(const std::vector<LuaValue>& args);

/**
 * @brief Read ECU identification
 */
FMUS_AUTO_API LuaResult ecu_identify(const std::vector<LuaValue>& args);

/**
 * @brief Read DTCs
 */
FMUS_AUTO_API LuaResult read_dtcs(const std::vector<LuaValue>& args);

/**
 * @brief Clear DTCs
 */
FMUS_AUTO_API LuaResult clear_dtcs(const std::vector<LuaValue>& args);

/**
 * @brief Convert bytes to hex string
 */
FMUS_AUTO_API LuaResult bytes_to_hex(const std::vector<LuaValue>& args);

/**
 * @brief Convert hex string to bytes
 */
FMUS_AUTO_API LuaResult hex_to_bytes(const std::vector<LuaValue>& args);

/**
 * @brief Calculate checksum
 */
FMUS_AUTO_API LuaResult calculate_checksum(const std::vector<LuaValue>& args);

/**
 * @brief Get current timestamp
 */
FMUS_AUTO_API LuaResult get_timestamp(const std::vector<LuaValue>& args);

} // namespace lua_functions

} // namespace scripting
} // namespace fmus

#endif // FMUS_SCRIPTING_LUA_ENGINE_H
