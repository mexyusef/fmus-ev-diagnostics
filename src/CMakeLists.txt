# Source files for FMUS-AUTO library

# Core component sources
set(FMUS_CORE_SOURCES
    core/error.cpp
    core/config.cpp
    core/thread_pool.cpp
)

# J2534 component sources
set(FMUS_J2534_SOURCES
    j2534/device.cpp
    j2534/adapter_info.cpp
    j2534/message.cpp
    j2534/filter.cpp
    j2534/connection_options.cpp
    j2534/channel_config.cpp
    j2534/library_loader.cpp
)

# Protocol component sources
set(FMUS_PROTOCOL_SOURCES
    protocols/can.cpp
)

# Diagnostics component sources
set(FMUS_DIAGNOSTICS_SOURCES
    diagnostics/uds.cpp
    diagnostics/obdii.cpp
)

# Utils component sources
set(FMUS_UTILS_SOURCES
    utils/logger.cpp
    utils/hex_utils.cpp
)

# ECU component sources
set(FMUS_ECU_SOURCES
    ecu/ecu.cpp
)

# Flashing component sources
set(FMUS_FLASHING_SOURCES
    flashing/flash_manager.cpp
)

# Scripting component sources
set(FMUS_SCRIPTING_SOURCES
    scripting/lua_engine.cpp
)

# Collect all sources
set(FMUS_AUTO_SOURCES
    ${FMUS_CORE_SOURCES}
    ${FMUS_J2534_SOURCES}
    ${FMUS_PROTOCOL_SOURCES}
    ${FMUS_DIAGNOSTICS_SOURCES}
    ${FMUS_ECU_SOURCES}
    ${FMUS_FLASHING_SOURCES}
    ${FMUS_SCRIPTING_SOURCES}
    ${FMUS_UTILS_SOURCES}
    PARENT_SCOPE
)

# Add include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)