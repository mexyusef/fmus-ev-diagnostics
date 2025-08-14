#include <fmus/j2534/library_loader.h>
#include <fmus/logger.h>
#include <fmus/utils.h>

#ifdef _WIN32
    #include <windows.h>
    #include <winreg.h>
#else
    #include <dlfcn.h>
#endif

namespace fmus {
namespace j2534 {

LibraryLoader::LibraryLoader() : libraryHandle(nullptr) {}

LibraryLoader::~LibraryLoader() {
    unloadLibrary();
}

bool LibraryLoader::loadLibrary(const std::string& libraryPath) {
    auto logger = Logger::getInstance();
    
    if (isLoaded()) {
        unloadLibrary();
    }
    
    logger->info("Loading J2534 library: " + libraryPath);
    
#ifdef _WIN32
    libraryHandle = LoadLibraryA(libraryPath.c_str());
    if (!libraryHandle) {
        DWORD error = GetLastError();
        lastError = "Failed to load library: " + std::to_string(error);
        logger->error(lastError);
        return false;
    }
#else
    libraryHandle = dlopen(libraryPath.c_str(), RTLD_LAZY);
    if (!libraryHandle) {
        lastError = "Failed to load library: " + std::string(dlerror());
        logger->error(lastError);
        return false;
    }
#endif
    
    this->libraryPath = libraryPath;
    
    if (!bindFunctions()) {
        unloadLibrary();
        return false;
    }
    
    logger->info("J2534 library loaded successfully");
    return true;
}

void LibraryLoader::unloadLibrary() {
    if (libraryHandle) {
#ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(libraryHandle));
#else
        dlclose(libraryHandle);
#endif
        libraryHandle = nullptr;
        libraryPath.clear();
        
        // Clear function pointers
        passThruOpen = nullptr;
        passThruClose = nullptr;
        passThruConnect = nullptr;
        passThruDisconnect = nullptr;
        passThruReadMsgs = nullptr;
        passThruWriteMsgs = nullptr;
        passThruStartPeriodicMsg = nullptr;
        passThruStopPeriodicMsg = nullptr;
        passThruStartMsgFilter = nullptr;
        passThruStopMsgFilter = nullptr;
        passThruSetProgrammingVoltage = nullptr;
        passThruReadVersion = nullptr;
        passThruGetLastError = nullptr;
        passThruIoctl = nullptr;
    }
}

bool LibraryLoader::isLoaded() const {
    return libraryHandle != nullptr;
}

std::string LibraryLoader::getLibraryPath() const {
    return libraryPath;
}

std::string LibraryLoader::getLastError() const {
    return lastError;
}

bool LibraryLoader::bindFunctions() {
    auto logger = Logger::getInstance();
    
    // Bind all J2534 functions
    passThruOpen = reinterpret_cast<PassThruOpen>(getFunctionAddress("PassThruOpen"));
    passThruClose = reinterpret_cast<PassThruClose>(getFunctionAddress("PassThruClose"));
    passThruConnect = reinterpret_cast<PassThruConnect>(getFunctionAddress("PassThruConnect"));
    passThruDisconnect = reinterpret_cast<PassThruDisconnect>(getFunctionAddress("PassThruDisconnect"));
    passThruReadMsgs = reinterpret_cast<PassThruReadMsgs>(getFunctionAddress("PassThruReadMsgs"));
    passThruWriteMsgs = reinterpret_cast<PassThruWriteMsgs>(getFunctionAddress("PassThruWriteMsgs"));
    passThruStartPeriodicMsg = reinterpret_cast<PassThruStartPeriodicMsg>(getFunctionAddress("PassThruStartPeriodicMsg"));
    passThruStopPeriodicMsg = reinterpret_cast<PassThruStopPeriodicMsg>(getFunctionAddress("PassThruStopPeriodicMsg"));
    passThruStartMsgFilter = reinterpret_cast<PassThruStartMsgFilter>(getFunctionAddress("PassThruStartMsgFilter"));
    passThruStopMsgFilter = reinterpret_cast<PassThruStopMsgFilter>(getFunctionAddress("PassThruStopMsgFilter"));
    passThruSetProgrammingVoltage = reinterpret_cast<PassThruSetProgrammingVoltage>(getFunctionAddress("PassThruSetProgrammingVoltage"));
    passThruReadVersion = reinterpret_cast<PassThruReadVersion>(getFunctionAddress("PassThruReadVersion"));
    passThruGetLastError = reinterpret_cast<PassThruGetLastError>(getFunctionAddress("PassThruGetLastError"));
    passThruIoctl = reinterpret_cast<PassThruIoctl>(getFunctionAddress("PassThruIoctl"));
    
    // Check if all critical functions are bound
    if (!passThruOpen || !passThruClose || !passThruConnect || !passThruDisconnect ||
        !passThruReadMsgs || !passThruWriteMsgs) {
        lastError = "Failed to bind critical J2534 functions";
        logger->error(lastError);
        return false;
    }
    
    logger->debug("All J2534 functions bound successfully");
    return true;
}

void* LibraryLoader::getFunctionAddress(const std::string& functionName) {
#ifdef _WIN32
    return GetProcAddress(static_cast<HMODULE>(libraryHandle), functionName.c_str());
#else
    return dlsym(libraryHandle, functionName.c_str());
#endif
}

// DeviceRegistry implementation
std::vector<AdapterInfo> DeviceRegistry::scanRegistry() {
    std::vector<AdapterInfo> adapters;
    
#ifdef _WIN32
    auto logger = Logger::getInstance();
    logger->info("Scanning Windows registry for J2534 devices");
    
    const std::string j2534Key = "SOFTWARE\\PassThruSupport.04.04";
    std::vector<std::string> deviceKeys = enumerateSubKeys(HKEY_LOCAL_MACHINE, j2534Key);
    
    for (const auto& deviceKey : deviceKeys) {
        AdapterInfo adapter;
        std::string fullKey = j2534Key + "\\" + deviceKey;
        
        // Read device information
        readRegistryKey(HKEY_LOCAL_MACHINE, fullKey, "Name", adapter.deviceName);
        readRegistryKey(HKEY_LOCAL_MACHINE, fullKey, "Vendor", adapter.vendorName);
        readRegistryKey(HKEY_LOCAL_MACHINE, fullKey, "FunctionLibrary", adapter.libraryPath);
        
        if (!adapter.libraryPath.empty()) {
            // Set supported protocols (would need to be determined from device)
            adapter.supportedProtocols = {Protocol::CAN, Protocol::ISO15765};
            adapters.push_back(adapter);
            
            logger->info("Found J2534 device: " + adapter.toString());
        }
    }
#else
    // On Linux, we'll look for common J2534 library locations
    std::vector<std::string> commonPaths = {
        "/usr/lib/libj2534.so",
        "/usr/local/lib/libj2534.so",
        "/opt/j2534/lib/libj2534.so"
    };
    
    for (const auto& path : commonPaths) {
        if (utils::fileExists(path)) {
            AdapterInfo adapter;
            adapter.vendorName = "Generic";
            adapter.deviceName = "Linux J2534 Device";
            adapter.libraryPath = path;
            adapter.supportedProtocols = {Protocol::CAN, Protocol::ISO15765};
            adapters.push_back(adapter);
        }
    }
#endif
    
    return adapters;
}

std::vector<std::string> DeviceRegistry::getInstalledDevices() {
    std::vector<std::string> devices;
    
#ifdef _WIN32
    const std::string j2534Key = "SOFTWARE\\PassThruSupport.04.04";
    devices = enumerateSubKeys(HKEY_LOCAL_MACHINE, j2534Key);
#endif
    
    return devices;
}

#ifdef _WIN32
bool DeviceRegistry::readRegistryKey(HKEY hKey, const std::string& subKey, 
                                    const std::string& valueName, std::string& value) {
    HKEY hSubKey;
    LONG result = RegOpenKeyExA(hKey, subKey.c_str(), 0, KEY_READ, &hSubKey);
    
    if (result != ERROR_SUCCESS) {
        return false;
    }
    
    char buffer[1024];
    DWORD bufferSize = sizeof(buffer);
    DWORD type;
    
    result = RegQueryValueExA(hSubKey, valueName.c_str(), nullptr, &type, 
                             reinterpret_cast<LPBYTE>(buffer), &bufferSize);
    
    RegCloseKey(hSubKey);
    
    if (result == ERROR_SUCCESS && type == REG_SZ) {
        value = std::string(buffer);
        return true;
    }
    
    return false;
}

std::vector<std::string> DeviceRegistry::enumerateSubKeys(HKEY hKey, const std::string& subKey) {
    std::vector<std::string> subKeys;
    
    HKEY hSubKey;
    LONG result = RegOpenKeyExA(hKey, subKey.c_str(), 0, KEY_READ, &hSubKey);
    
    if (result != ERROR_SUCCESS) {
        return subKeys;
    }
    
    DWORD index = 0;
    char keyName[256];
    DWORD keyNameSize;
    
    while (true) {
        keyNameSize = sizeof(keyName);
        result = RegEnumKeyExA(hSubKey, index, keyName, &keyNameSize, 
                              nullptr, nullptr, nullptr, nullptr);
        
        if (result != ERROR_SUCCESS) {
            break;
        }
        
        subKeys.push_back(std::string(keyName));
        index++;
    }
    
    RegCloseKey(hSubKey);
    return subKeys;
}
#endif

} // namespace j2534
} // namespace fmus
