# FMUS-AUTO: Modern C++ Automotive Diagnostics Library

FMUS-AUTO is a C++ library for vehicle diagnostics, ECU programming, etc.

## Quick Start

### Installation

#### Prerequisites
- C++17 compatible compiler (Visual Studio 2019/2022, GCC 8+, Clang 7+)
- CMake 3.20 or higher
- A compatible J2534 interface device

```bash
# Clone the repository
git clone https://github.com/mexyusef/fmus-ev-diagnostics.git
cd fmus-ev-diagnostics

# Configure the build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build the library
cmake --build . --config Release

# Install (optional)
cmake --install .
```

### Basic Usage

```cpp
#include <fmus/auto.h>
#include <iostream>

int main() {
    // Connect to the first available adapter
    auto adapter = fmus::Auto::connect();

    // Get the engine ECU
    auto engine = adapter.getECU(fmus::ECUType::Engine);

    // Read DTCs
    auto dtcs = engine.readDTCs();
    for (const auto& dtc : dtcs) {
        std::cout << dtc.code << ": " << dtc.description << std::endl;
    }

    // Clear DTCs
    engine.clearDTCs();

    // Read live data
    auto liveData = engine.startLiveData(
        { 0x0C, 0x0D, 0x05 },  // RPM, Speed, Engine temp
        [](const auto& params) {
            // This callback is called with updated parameter values
            for (const auto& param : params) {
                std::cout << param.name << ": " << param.getFormattedValue() << std::endl;
            }
        }
    );

    // Let data stream for 10 seconds
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // Stop the data stream
    liveData.stop();

    return 0;
}
```

### ECU Flashing Example

```cpp
#include <fmus/auto.h>
#include <iostream>

int main() {
    auto adapter = fmus::Auto::connect();
    auto ecu = adapter.getECU(fmus::ECUType::Engine);

    // Flash with progress reporting
    bool success = ecu.flashFirmware(
        "firmware.bin",
        [](int progress) {
            std::cout << "Progress: " << progress << "%" << std::endl;
        }
    );

    if (success) {
        std::cout << "Flashing completed successfully!" << std::endl;
    } else {
        std::cerr << "Flashing failed!" << std::endl;
    }

    return 0;
}
```

## Supported Hardware

fmus-ev-diagnostics works with all SAE J2534-compatible interfaces, including:

- Drew Technologies CarDAQ-Plus 2/3, MongoosePro
- Tactrix OpenPort 2.0/3.0
- Kvaser PassThru RP1210
- MacchiatoLink
- OBDLink SX/MX/EX
- And many more commercial and DIY J2534 interfaces

## Architecture

FMUS-AUTO is designed with a layered, modular architecture:

1. **Foundation Layer**: Direct J2534 API wrappers, low-level message framing
2. **Protocol Layer**: Protocol-specific implementations (OBD-II, UDS, CAN)
3. **Diagnostic Layer**: High-level diagnostic services (DTC, sensor data, programming)
4. **Application Layer**: User-friendly, vehicle-specific functionality

Each layer provides progressively more abstraction and ease-of-use while maintaining the ability to access lower-level functionality when needed.

## Documentation

WIP.

##  Building and Development

### Building from Source

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release

# Test
cd build && ctest -C Release
```

### Options

- `FMUS_AUTO_BUILD_EXAMPLES`: Build example applications (ON by default)
- `FMUS_AUTO_BUILD_TESTS`: Build test suite (ON by default)
- `FMUS_AUTO_ENABLE_PYTHON`: Build Python bindings (OFF by default)
- `FMUS_AUTO_ENABLE_LUA`: Build Lua bindings (OFF by default)
