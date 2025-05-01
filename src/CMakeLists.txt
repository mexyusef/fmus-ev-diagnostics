# Source files for FMUS-AUTO library

# Core component sources
set(FMUS_CORE_SOURCES
    core/error.cpp
)

# J2534 component sources
set(FMUS_J2534_SOURCES
    j2534/device.cpp
    j2534/adapter_info.cpp
    j2534/message.cpp
    j2534/filter.cpp
    j2534/connection_options.cpp
    j2534/channel_config.cpp
)

# Protocol component sources
set(FMUS_PROTOCOL_SOURCES
    # Will be added as development progresses
)

# Diagnostics component sources
set(FMUS_DIAGNOSTICS_SOURCES
    # Will be added as development progresses
)

# Utils component sources
set(FMUS_UTILS_SOURCES
    utils/logger.cpp
)

# Collect all sources
set(FMUS_AUTO_SOURCES
    ${FMUS_CORE_SOURCES}
    ${FMUS_J2534_SOURCES}
    ${FMUS_PROTOCOL_SOURCES}
    ${FMUS_DIAGNOSTICS_SOURCES}
    ${FMUS_UTILS_SOURCES}
    PARENT_SCOPE
)

# Add include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)