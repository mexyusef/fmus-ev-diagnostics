#include <fmus/scripting/lua_engine.h>
#include <fmus/logger.h>
#include <fmus/utils.h>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <mutex>

// Mock Lua implementation (in real implementation, would use actual Lua)
namespace fmus {
namespace scripting {

// Mock Lua state structure
struct lua_State {
    std::map<std::string, LuaValue> globals;
    std::map<std::string, std::function<LuaResult(const std::vector<LuaValue>&)>> functions;
    std::string lastError;
    std::string currentScript;
    ScriptContext context;
};

// LuaResult implementation
std::string LuaResult::toString() const {
    std::ostringstream ss;
    ss << "LuaResult[Success:" << (success ? "Yes" : "No");
    if (!error.empty()) {
        ss << ", Error:" << error;
    }
    ss << ", Values:" << values.size() << "]";
    return ss.str();
}

// ScriptInfo implementation
std::string ScriptInfo::toString() const {
    std::ostringstream ss;
    ss << "ScriptInfo[Name:" << name
       << ", Version:" << version
       << ", Author:" << author
       << ", ECUs:" << requiredECUs.size()
       << ", Protocols:" << supportedProtocols.size() << "]";
    return ss.str();
}

// LuaEngine implementation
class LuaEngine::Impl {
public:
    std::unique_ptr<lua_State> L;
    bool initialized = false;
    ScriptInfo scriptInfo;
    std::string lastError;
    mutable std::mutex luaMutex;
    
    Impl() = default;
    
    ~Impl() {
        if (L) {
            L.reset();
        }
    }
    
    bool initializeLua() {
        L = std::make_unique<lua_State>();
        
        // Register built-in functions
        registerBuiltinFunctions();
        
        initialized = true;
        return true;
    }
    
    void registerBuiltinFunctions() {
        L->functions["log"] = lua_functions::log;
        L->functions["sleep"] = lua_functions::sleep;
        L->functions["uds_request"] = lua_functions::uds_request;
        L->functions["obd_read"] = lua_functions::obd_read;
        L->functions["ecu_identify"] = lua_functions::ecu_identify;
        L->functions["read_dtcs"] = lua_functions::read_dtcs;
        L->functions["clear_dtcs"] = lua_functions::clear_dtcs;
        L->functions["bytes_to_hex"] = lua_functions::bytes_to_hex;
        L->functions["hex_to_bytes"] = lua_functions::hex_to_bytes;
        L->functions["calculate_checksum"] = lua_functions::calculate_checksum;
        L->functions["get_timestamp"] = lua_functions::get_timestamp;
    }
    
    LuaResult executeString(const std::string& script) {
        std::lock_guard<std::mutex> lock(luaMutex);
        
        if (!L) {
            return LuaResult(false, "Lua engine not initialized");
        }
        
        L->currentScript = script;
        
        // Mock script execution - in real implementation would use lua_dostring
        try {
            // Simple script parsing for demonstration
            if (script.find("function") != std::string::npos) {
                // Function definition - just store it
                return LuaResult(true);
            }
            
            // Look for function calls
            size_t pos = script.find('(');
            if (pos != std::string::npos) {
                std::string funcName = script.substr(0, pos);
                funcName.erase(0, funcName.find_first_not_of(" \t"));
                funcName.erase(funcName.find_last_not_of(" \t") + 1);
                
                // Extract arguments (simplified)
                std::vector<LuaValue> args;
                
                auto it = L->functions.find(funcName);
                if (it != L->functions.end()) {
                    return it->second(args);
                } else {
                    return LuaResult(false, "Function not found: " + funcName);
                }
            }
            
            return LuaResult(true);
            
        } catch (const std::exception& e) {
            return LuaResult(false, "Script execution error: " + std::string(e.what()));
        }
    }
    
    void parseScriptInfo(const std::string& script) {
        // Parse script metadata from comments
        std::istringstream stream(script);
        std::string line;
        
        while (std::getline(stream, line)) {
            if (line.find("-- @name") == 0) {
                scriptInfo.name = line.substr(8);
            } else if (line.find("-- @description") == 0) {
                scriptInfo.description = line.substr(15);
            } else if (line.find("-- @author") == 0) {
                scriptInfo.author = line.substr(10);
            } else if (line.find("-- @version") == 0) {
                scriptInfo.version = line.substr(11);
            } else if (line.find("-- @ecu") == 0) {
                scriptInfo.requiredECUs.push_back(line.substr(7));
            } else if (line.find("-- @protocol") == 0) {
                scriptInfo.supportedProtocols.push_back(line.substr(12));
            }
        }
        
        // Set defaults
        if (scriptInfo.name.empty()) scriptInfo.name = "Unnamed Script";
        if (scriptInfo.version.empty()) scriptInfo.version = "1.0";
        if (scriptInfo.author.empty()) scriptInfo.author = "Unknown";
    }
};

LuaEngine::LuaEngine() : pImpl(std::make_unique<Impl>()) {}

LuaEngine::~LuaEngine() = default;

bool LuaEngine::initialize() {
    auto logger = Logger::getInstance();
    logger->info("Initializing Lua scripting engine");
    
    if (!pImpl->initializeLua()) {
        logger->error("Failed to initialize Lua engine");
        return false;
    }
    
    logger->info("Lua scripting engine initialized successfully");
    return true;
}

void LuaEngine::shutdown() {
    pImpl->initialized = false;
    pImpl->L.reset();
    
    auto logger = Logger::getInstance();
    logger->info("Lua scripting engine shutdown");
}

bool LuaEngine::isInitialized() const {
    return pImpl->initialized;
}

void LuaEngine::setContext(const ScriptContext& context) {
    if (pImpl->L) {
        pImpl->L->context = context;
    }
}

ScriptContext LuaEngine::getContext() const {
    if (pImpl->L) {
        return pImpl->L->context;
    }
    return ScriptContext{};
}

bool LuaEngine::loadScript(const std::string& filePath) {
    auto logger = Logger::getInstance();
    logger->info("Loading Lua script: " + filePath);
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        pImpl->lastError = "Failed to open script file: " + filePath;
        logger->error(pImpl->lastError);
        return false;
    }
    
    std::string script((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();
    
    return loadScriptFromString(script, filePath);
}

bool LuaEngine::loadScriptFromString(const std::string& script, const std::string& name) {
    if (!pImpl->initialized) {
        pImpl->lastError = "Lua engine not initialized";
        return false;
    }
    
    // Parse script metadata
    pImpl->parseScriptInfo(script);
    if (pImpl->scriptInfo.name == "Unnamed Script" && !name.empty()) {
        pImpl->scriptInfo.name = name;
    }
    
    // Load script (in real implementation would use luaL_loadstring)
    pImpl->L->currentScript = script;
    
    auto logger = Logger::getInstance();
    logger->info("Lua script loaded: " + pImpl->scriptInfo.name);
    
    return true;
}

LuaResult LuaEngine::executeScript() {
    if (!pImpl->initialized || !pImpl->L) {
        return LuaResult(false, "Lua engine not initialized");
    }
    
    return pImpl->executeString(pImpl->L->currentScript);
}

LuaResult LuaEngine::callFunction(const std::string& functionName, const std::vector<LuaValue>& args) {
    if (!pImpl->initialized || !pImpl->L) {
        return LuaResult(false, "Lua engine not initialized");
    }
    
    std::lock_guard<std::mutex> lock(pImpl->luaMutex);
    
    auto it = pImpl->L->functions.find(functionName);
    if (it != pImpl->L->functions.end()) {
        return it->second(args);
    }
    
    return LuaResult(false, "Function not found: " + functionName);
}

LuaResult LuaEngine::execute(const std::string& script) {
    return pImpl->executeString(script);
}

ScriptInfo LuaEngine::getScriptInfo() const {
    return pImpl->scriptInfo;
}

void LuaEngine::setGlobal(const std::string& name, const LuaValue& value) {
    if (pImpl->L) {
        std::lock_guard<std::mutex> lock(pImpl->luaMutex);
        pImpl->L->globals[name] = value;
    }
}

LuaValue LuaEngine::getGlobal(const std::string& name) const {
    if (pImpl->L) {
        std::lock_guard<std::mutex> lock(pImpl->luaMutex);
        auto it = pImpl->L->globals.find(name);
        if (it != pImpl->L->globals.end()) {
            return it->second;
        }
    }
    return std::nullptr_t{};
}

void LuaEngine::registerFunction(const std::string& name, std::function<LuaResult(const std::vector<LuaValue>&)> func) {
    if (pImpl->L) {
        std::lock_guard<std::mutex> lock(pImpl->luaMutex);
        pImpl->L->functions[name] = func;
    }
}

std::vector<std::string> LuaEngine::getAvailableFunctions() const {
    std::vector<std::string> functions;
    
    if (pImpl->L) {
        std::lock_guard<std::mutex> lock(pImpl->luaMutex);
        for (const auto& pair : pImpl->L->functions) {
            functions.push_back(pair.first);
        }
    }
    
    return functions;
}

std::string LuaEngine::getLastError() const {
    return pImpl->lastError;
}

void LuaEngine::clearError() {
    pImpl->lastError.clear();
}

// ScriptLibrary implementation
class ScriptLibrary::Impl {
public:
    std::map<std::string, std::string> scripts; // name -> file path
    std::map<std::string, ScriptInfo> scriptInfos;
    
    bool loadScriptInfo(const std::string& name, const std::string& filePath) {
        LuaEngine engine;
        if (!engine.initialize()) {
            return false;
        }
        
        if (!engine.loadScript(filePath)) {
            return false;
        }
        
        scriptInfos[name] = engine.getScriptInfo();
        return true;
    }
};

ScriptLibrary::ScriptLibrary() : pImpl(std::make_unique<Impl>()) {}

ScriptLibrary::~ScriptLibrary() = default;

bool ScriptLibrary::loadScriptsFromDirectory(const std::string& directory) {
    auto logger = Logger::getInstance();
    logger->info("Loading scripts from directory: " + directory);
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string filePath = entry.path().string();
                std::string extension = entry.path().extension().string();
                
                if (extension == ".lua") {
                    std::string name = entry.path().stem().string();
                    addScript(name, filePath);
                }
            }
        }
        
        logger->info("Loaded " + std::to_string(pImpl->scripts.size()) + " scripts");
        return true;
        
    } catch (const std::exception& e) {
        logger->error("Failed to load scripts from directory: " + std::string(e.what()));
        return false;
    }
}

bool ScriptLibrary::addScript(const std::string& name, const std::string& filePath) {
    pImpl->scripts[name] = filePath;
    return pImpl->loadScriptInfo(name, filePath);
}

void ScriptLibrary::removeScript(const std::string& name) {
    pImpl->scripts.erase(name);
    pImpl->scriptInfos.erase(name);
}

std::vector<std::string> ScriptLibrary::getAvailableScripts() const {
    std::vector<std::string> scripts;
    for (const auto& pair : pImpl->scripts) {
        scripts.push_back(pair.first);
    }
    return scripts;
}

ScriptInfo ScriptLibrary::getScriptInfo(const std::string& name) const {
    auto it = pImpl->scriptInfos.find(name);
    if (it != pImpl->scriptInfos.end()) {
        return it->second;
    }
    return ScriptInfo{};
}

LuaResult ScriptLibrary::executeScript(const std::string& name, const ScriptContext& context) {
    auto it = pImpl->scripts.find(name);
    if (it == pImpl->scripts.end()) {
        return LuaResult(false, "Script not found: " + name);
    }
    
    LuaEngine engine;
    if (!engine.initialize()) {
        return LuaResult(false, "Failed to initialize Lua engine");
    }
    
    engine.setContext(context);
    
    if (!engine.loadScript(it->second)) {
        return LuaResult(false, "Failed to load script: " + engine.getLastError());
    }
    
    return engine.executeScript();
}

std::vector<std::string> ScriptLibrary::findScriptsForECU(ECUType ecuType) const {
    std::vector<std::string> matchingScripts;

    // Simple ECU type to string conversion
    std::string ecuTypeStr;
    switch (ecuType) {
        case ECUType::Engine: ecuTypeStr = "Engine"; break;
        case ECUType::Transmission: ecuTypeStr = "Transmission"; break;
        case ECUType::ABS: ecuTypeStr = "ABS"; break;
        case ECUType::SRS: ecuTypeStr = "SRS"; break;
        case ECUType::BCM: ecuTypeStr = "BCM"; break;
        default: ecuTypeStr = "Unknown"; break;
    }

    for (const auto& pair : pImpl->scriptInfos) {
        const auto& info = pair.second;
        for (const auto& requiredECU : info.requiredECUs) {
            if (requiredECU == ecuTypeStr || requiredECU == "Any") {
                matchingScripts.push_back(pair.first);
                break;
            }
        }
    }

    return matchingScripts;
}

std::vector<std::string> ScriptLibrary::findScriptsForProtocol(const std::string& protocol) const {
    std::vector<std::string> matchingScripts;
    
    for (const auto& pair : pImpl->scriptInfos) {
        const auto& info = pair.second;
        for (const auto& supportedProtocol : info.supportedProtocols) {
            if (supportedProtocol == protocol || supportedProtocol == "Any") {
                matchingScripts.push_back(pair.first);
                break;
            }
        }
    }
    
    return matchingScripts;
}

// Utility functions
std::string luaValueToString(const LuaValue& value) {
    return std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
            return "nil";
        } else if constexpr (std::is_same_v<T, bool>) {
            return v ? "true" : "false";
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return std::to_string(v);
        } else if constexpr (std::is_same_v<T, double>) {
            return std::to_string(v);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return v;
        } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
            return utils::bytesToHex(v);
        }
        return "";
    }, value);
}

LuaValue stringToLuaValue(const std::string& str, const std::string& type) {
    if (type == "nil") {
        return std::nullptr_t{};
    } else if (type == "boolean") {
        return str == "true";
    } else if (type == "integer") {
        return static_cast<int64_t>(std::stoll(str));
    } else if (type == "number") {
        return std::stod(str);
    } else if (type == "bytes") {
        return utils::hexToBytes(str);
    } else {
        return str;
    }
}

std::string luaErrorTypeToString(LuaError::ErrorType type) {
    switch (type) {
        case LuaError::ErrorType::INITIALIZATION_FAILED: return "Initialization Failed";
        case LuaError::ErrorType::SCRIPT_LOAD_FAILED: return "Script Load Failed";
        case LuaError::ErrorType::EXECUTION_FAILED: return "Execution Failed";
        case LuaError::ErrorType::FUNCTION_NOT_FOUND: return "Function Not Found";
        case LuaError::ErrorType::TYPE_ERROR: return "Type Error";
        case LuaError::ErrorType::RUNTIME_ERROR: return "Runtime Error";
        default: return "Unknown Error";
    }
}

// Built-in Lua functions
namespace lua_functions {

LuaResult log(const std::vector<LuaValue>& args) {
    if (args.empty()) {
        return LuaResult(false, "log() requires at least one argument");
    }
    
    auto logger = Logger::getInstance();
    std::string message = luaValueToString(args[0]);
    
    if (args.size() > 1) {
        std::string level = luaValueToString(args[1]);
        if (level == "debug") {
            logger->debug("[Lua] " + message);
        } else if (level == "warning") {
            logger->warning("[Lua] " + message);
        } else if (level == "error") {
            logger->error("[Lua] " + message);
        } else {
            logger->info("[Lua] " + message);
        }
    } else {
        logger->info("[Lua] " + message);
    }
    
    return LuaResult(true);
}

LuaResult sleep(const std::vector<LuaValue>& args) {
    if (args.empty()) {
        return LuaResult(false, "sleep() requires milliseconds argument");
    }
    
    try {
        int64_t ms = std::get<int64_t>(args[0]);
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        return LuaResult(true);
    } catch (...) {
        return LuaResult(false, "Invalid milliseconds value");
    }
}

LuaResult uds_request(const std::vector<LuaValue>& args) {
    // Mock UDS request - in real implementation would use actual UDS client
    if (args.size() < 2) {
        return LuaResult(false, "uds_request() requires service and data arguments");
    }
    
    LuaResult result(true);
    result.values.push_back(std::vector<uint8_t>{0x50, 0x01}); // Mock positive response
    return result;
}

LuaResult obd_read(const std::vector<LuaValue>& args) {
    // Mock OBD read - in real implementation would use actual OBD client
    if (args.empty()) {
        return LuaResult(false, "obd_read() requires PID argument");
    }
    
    LuaResult result(true);
    result.values.push_back(42.0); // Mock value
    result.values.push_back(std::string("RPM")); // Mock unit
    return result;
}

LuaResult ecu_identify(const std::vector<LuaValue>& args) {
    // Mock ECU identification
    LuaResult result(true);
    result.values.push_back(std::string("1HGBH41JXMN109186")); // VIN
    result.values.push_back(std::string("ECU123456789")); // Serial
    result.values.push_back(std::string("1.0.0")); // Software version
    return result;
}

LuaResult read_dtcs(const std::vector<LuaValue>& args) {
    // Mock DTC reading
    LuaResult result(true);
    result.values.push_back(std::string("P0171")); // DTC code
    result.values.push_back(std::string("System Too Lean (Bank 1)")); // Description
    return result;
}

LuaResult clear_dtcs(const std::vector<LuaValue>& args) {
    // Mock DTC clearing
    return LuaResult(true);
}

LuaResult bytes_to_hex(const std::vector<LuaValue>& args) {
    if (args.empty()) {
        return LuaResult(false, "bytes_to_hex() requires bytes argument");
    }
    
    try {
        auto bytes = std::get<std::vector<uint8_t>>(args[0]);
        LuaResult result(true);
        result.values.push_back(utils::bytesToHex(bytes));
        return result;
    } catch (...) {
        return LuaResult(false, "Invalid bytes argument");
    }
}

LuaResult hex_to_bytes(const std::vector<LuaValue>& args) {
    if (args.empty()) {
        return LuaResult(false, "hex_to_bytes() requires hex string argument");
    }
    
    try {
        std::string hex = std::get<std::string>(args[0]);
        LuaResult result(true);
        result.values.push_back(utils::hexToBytes(hex));
        return result;
    } catch (...) {
        return LuaResult(false, "Invalid hex string argument");
    }
}

LuaResult calculate_checksum(const std::vector<LuaValue>& args) {
    if (args.empty()) {
        return LuaResult(false, "calculate_checksum() requires bytes argument");
    }
    
    try {
        auto bytes = std::get<std::vector<uint8_t>>(args[0]);
        uint32_t checksum = utils::calculateCRC32(bytes);
        LuaResult result(true);
        result.values.push_back(static_cast<int64_t>(checksum));
        return result;
    } catch (...) {
        return LuaResult(false, "Invalid bytes argument");
    }
}

LuaResult get_timestamp(const std::vector<LuaValue>& args) {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    LuaResult result(true);
    result.values.push_back(static_cast<int64_t>(timestamp));
    return result;
}

} // namespace lua_functions

} // namespace scripting
} // namespace fmus
