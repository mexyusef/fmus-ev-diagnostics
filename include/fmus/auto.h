#ifndef FMUS_AUTO_H
#define FMUS_AUTO_H

/**
 * @file auto.h
 * @brief Main header file for the FMUS-AUTO library
 *
 * This header includes the core functionality of the FMUS-AUTO library,
 * a modern, powerful, and comprehensive C++ library for vehicle diagnostics,
 * ECU programming, and automotive communication.
 */

#include <string>
#include <memory>
#include <vector>
#include <functional>

#include "j2534.h"
#include "ecu.h"

/**
 * @brief Version information for the FMUS-AUTO library
 */
#define FMUS_AUTO_VERSION_MAJOR 0
#define FMUS_AUTO_VERSION_MINOR 1
#define FMUS_AUTO_VERSION_PATCH 0
#define FMUS_AUTO_VERSION_STRING "0.1.0"

/**
 * @namespace fmus
 * @brief Main namespace for all FMUS libraries
 */
namespace fmus {

/**
 * @class Auto
 * @brief Main entry point for the FMUS-AUTO library
 *
 * This class provides a simple, high-level API for automotive diagnostics
 * and ECU programming using the J2534 PassThru standard.
 */
class Auto {
public:
    /**
     * @brief Connect to an available J2534 adapter
     *
     * This static method detects and connects to the first available
     * J2534 adapter. For more control over adapter selection, use the
     * connect(options) overload.
     *
     * @return An Auto instance connected to the detected adapter
     * @throws ConnectionError if no adapter can be detected or connected
     */
    static Auto connect();

    /**
     * @brief Connect to a J2534 adapter with specific options
     *
     * @param options Connection options including adapter selection criteria
     * @return An Auto instance connected to the selected adapter
     * @throws ConnectionError if the adapter cannot be connected
     */
    static Auto connect(const j2534::ConnectionOptions& options);

    /**
     * @brief Discover all available J2534 adapters
     *
     * @return A vector of available adapter information
     */
    static std::vector<j2534::AdapterInfo> discoverAdapters();

    /**
     * @brief Get an ECU interface for the default ECU
     *
     * @return An ECU object for the default ECU
     */
    ECU getECU() const;

    /**
     * @brief Get an ECU interface for a specific ECU
     *
     * @param ecuType The type of ECU to connect to
     * @return An ECU object for the specified ECU
     */
    ECU getECU(ECUType ecuType) const;

    /**
     * @brief Get an ECU interface by address
     *
     * @param address The CAN address of the ECU
     * @return An ECU object for the specified address
     */
    ECU getECUByAddress(uint32_t address) const;

    /**
     * @brief Close the connection to the adapter
     *
     * This method is called automatically when the Auto object is destroyed,
     * but can be called explicitly to release resources early.
     */
    void disconnect();

    /**
     * @brief Check if the connection to the adapter is active
     *
     * @return true if connected, false otherwise
     */
    bool isConnected() const;

    /**
     * @brief Set a callback for connection status changes
     *
     * @param callback Function to call when connection status changes
     */
    void setConnectionCallback(std::function<void(bool)> callback);

    // Constructors and assignment operators
    ~Auto();
    Auto(Auto&& other) noexcept;
    Auto& operator=(Auto&& other) noexcept;

    // Deleted copy constructor and assignment
    Auto(const Auto&) = delete;
    Auto& operator=(const Auto&) = delete;

private:
    // Private implementation
    class Impl;
    std::unique_ptr<Impl> pImpl;

    // Private constructor used by static factory methods
    explicit Auto(std::unique_ptr<Impl> impl);
};

// Forward declarations for types used in the API
class ConnectionOptions;
class AdapterInfo;
class ECU;
enum class ECUType;
class ConnectionError;

} // namespace fmus

#endif // FMUS_AUTO_H