#ifndef FMUS_J2534_LIBRARY_LOADER_H
#define FMUS_J2534_LIBRARY_LOADER_H

/**
 * @file library_loader.h
 * @brief J2534 library dynamic loading and function binding
 */

#include <fmus/j2534.h>
#include <string>
#include <memory>
#include <functional>

// Platform-specific includes
#ifdef _WIN32
    #include <windows.h>
    #define LIBRARY_HANDLE HMODULE
    #define CALL_CONV WINAPI
#else
    #include <dlfcn.h>
    #define LIBRARY_HANDLE void*
    #define CALL_CONV
#endif

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
namespace j2534 {

// J2534 Function Prototypes
typedef long (CALL_CONV *PassThruOpen)(void* pName, unsigned long* pDeviceID);
typedef long (CALL_CONV *PassThruClose)(unsigned long DeviceID);
typedef long (CALL_CONV *PassThruConnect)(unsigned long DeviceID, unsigned long ProtocolID, 
                                          unsigned long Flags, unsigned long Baudrate, 
                                          unsigned long* pChannelID);
typedef long (CALL_CONV *PassThruDisconnect)(unsigned long ChannelID);
typedef long (CALL_CONV *PassThruReadMsgs)(unsigned long ChannelID, void* pMsg, 
                                           unsigned long* pNumMsgs, unsigned long Timeout);
typedef long (CALL_CONV *PassThruWriteMsgs)(unsigned long ChannelID, void* pMsg, 
                                            unsigned long* pNumMsgs, unsigned long Timeout);
typedef long (CALL_CONV *PassThruStartPeriodicMsg)(unsigned long ChannelID, void* pMsg, 
                                                   unsigned long* pMsgID, unsigned long TimeInterval);
typedef long (CALL_CONV *PassThruStopPeriodicMsg)(unsigned long ChannelID, unsigned long MsgID);
typedef long (CALL_CONV *PassThruStartMsgFilter)(unsigned long ChannelID, unsigned long FilterType, 
                                                 void* pMaskMsg, void* pPatternMsg, 
                                                 void* pFlowControlMsg, unsigned long* pFilterID);
typedef long (CALL_CONV *PassThruStopMsgFilter)(unsigned long ChannelID, unsigned long FilterID);
typedef long (CALL_CONV *PassThruSetProgrammingVoltage)(unsigned long DeviceID, unsigned long PinNumber, 
                                                        unsigned long Voltage);
typedef long (CALL_CONV *PassThruReadVersion)(unsigned long DeviceID, char* pFirmwareVersion, 
                                              char* pDllVersion, char* pApiVersion);
typedef long (CALL_CONV *PassThruGetLastError)(char* pErrorDescription);
typedef long (CALL_CONV *PassThruIoctl)(unsigned long ChannelID, unsigned long IoctlID, 
                                        void* pInput, void* pOutput);

/**
 * @brief J2534 Library Loader and Function Binder
 */
class FMUS_AUTO_API LibraryLoader {
public:
    /**
     * @brief Constructor
     */
    LibraryLoader();
    
    /**
     * @brief Destructor
     */
    ~LibraryLoader();
    
    /**
     * @brief Load J2534 library from path
     */
    bool loadLibrary(const std::string& libraryPath);
    
    /**
     * @brief Unload current library
     */
    void unloadLibrary();
    
    /**
     * @brief Check if library is loaded
     */
    bool isLoaded() const;
    
    /**
     * @brief Get library path
     */
    std::string getLibraryPath() const;
    
    /**
     * @brief Get last error message
     */
    std::string getLastError() const;
    
    // J2534 Function Pointers
    PassThruOpen passThruOpen = nullptr;
    PassThruClose passThruClose = nullptr;
    PassThruConnect passThruConnect = nullptr;
    PassThruDisconnect passThruDisconnect = nullptr;
    PassThruReadMsgs passThruReadMsgs = nullptr;
    PassThruWriteMsgs passThruWriteMsgs = nullptr;
    PassThruStartPeriodicMsg passThruStartPeriodicMsg = nullptr;
    PassThruStopPeriodicMsg passThruStopPeriodicMsg = nullptr;
    PassThruStartMsgFilter passThruStartMsgFilter = nullptr;
    PassThruStopMsgFilter passThruStopMsgFilter = nullptr;
    PassThruSetProgrammingVoltage passThruSetProgrammingVoltage = nullptr;
    PassThruReadVersion passThruReadVersion = nullptr;
    PassThruGetLastError passThruGetLastError = nullptr;
    PassThruIoctl passThruIoctl = nullptr;

private:
    LIBRARY_HANDLE libraryHandle;
    std::string libraryPath;
    std::string lastError;
    
    /**
     * @brief Bind all J2534 functions
     */
    bool bindFunctions();
    
    /**
     * @brief Get function address from library
     */
    void* getFunctionAddress(const std::string& functionName);
};

/**
 * @brief J2534 Device Registry Scanner (Windows only)
 */
class FMUS_AUTO_API DeviceRegistry {
public:
    /**
     * @brief Scan Windows registry for J2534 devices
     */
    static std::vector<AdapterInfo> scanRegistry();
    
    /**
     * @brief Get installed J2534 devices from registry
     */
    static std::vector<std::string> getInstalledDevices();
    
private:
#ifdef _WIN32
    static bool readRegistryKey(HKEY hKey, const std::string& subKey, 
                               const std::string& valueName, std::string& value);
    static std::vector<std::string> enumerateSubKeys(HKEY hKey, const std::string& subKey);
#endif
};

/**
 * @brief J2534 Message Structure (matches J2534 spec)
 */
struct PASSTHRU_MSG {
    unsigned long ProtocolID;
    unsigned long RxStatus;
    unsigned long TxFlags;
    unsigned long Timestamp;
    unsigned long DataSize;
    unsigned long ExtraDataIndex;
    unsigned char Data[4128];
};

/**
 * @brief J2534 Configuration Structure
 */
struct SCONFIG {
    unsigned long Parameter;
    unsigned long Value;
};

/**
 * @brief J2534 Configuration List
 */
struct SCONFIG_LIST {
    unsigned long NumOfParams;
    SCONFIG* ConfigPtr;
};

// J2534 Constants
namespace J2534Constants {
    // Protocol IDs
    constexpr unsigned long J1850VPW = 1;
    constexpr unsigned long J1850PWM = 2;
    constexpr unsigned long ISO9141 = 3;
    constexpr unsigned long ISO14230_4 = 4;
    constexpr unsigned long CAN = 5;
    constexpr unsigned long ISO15765 = 6;
    constexpr unsigned long SCI_A_ENGINE = 7;
    constexpr unsigned long SCI_A_TRANS = 8;
    constexpr unsigned long SCI_B_ENGINE = 9;
    constexpr unsigned long SCI_B_TRANS = 10;
    
    // Filter Types
    constexpr unsigned long PASS_FILTER = 1;
    constexpr unsigned long BLOCK_FILTER = 2;
    constexpr unsigned long FLOW_CONTROL_FILTER = 3;
    
    // Flags
    constexpr unsigned long CAN_29BIT_ID = 0x00000100;
    constexpr unsigned long ISO15765_FRAME_PAD = 0x00000040;
    constexpr unsigned long ISO15765_ADDR_TYPE = 0x00000080;
    
    // IOCTL IDs
    constexpr unsigned long GET_CONFIG = 0x01;
    constexpr unsigned long SET_CONFIG = 0x02;
    constexpr unsigned long READ_VBATT = 0x03;
    constexpr unsigned long FIVE_BAUD_INIT = 0x04;
    constexpr unsigned long FAST_INIT = 0x05;
    
    // Configuration Parameters
    constexpr unsigned long DATA_RATE = 0x01;
    constexpr unsigned long LOOPBACK = 0x03;
    constexpr unsigned long NODE_ADDRESS = 0x04;
    constexpr unsigned long NETWORK_LINE = 0x05;
    constexpr unsigned long P1_MIN = 0x06;
    constexpr unsigned long P1_MAX = 0x07;
    constexpr unsigned long P2_MIN = 0x08;
    constexpr unsigned long P2_MAX = 0x09;
    constexpr unsigned long P3_MIN = 0x0A;
    constexpr unsigned long P3_MAX = 0x0B;
    constexpr unsigned long P4_MIN = 0x0C;
    constexpr unsigned long P4_MAX = 0x0D;
}

} // namespace j2534
} // namespace fmus

#endif // FMUS_J2534_LIBRARY_LOADER_H
