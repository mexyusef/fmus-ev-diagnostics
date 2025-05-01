# Examples for FMUS-AUTO

# Basic diagnostics example
add_executable(basic_diagnostics basic_diagnostics.cpp)
target_link_libraries(basic_diagnostics PRIVATE fmus_auto)
set_target_properties(basic_diagnostics PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)

# Make the examples depend on the copy_dlls target
add_dependencies(basic_diagnostics copy_dlls)

# Copy DLLs to the examples output directory
add_custom_command(
    TARGET basic_diagnostics POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/examples/${CMAKE_BUILD_TYPE}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/fmus_auto.dll"
        "C:/libuv-install/bin/uv.dll"
        "C:/usockets-install/bin/uSockets.dll"
        "C:/vcpkg/packages/zlib_x64-windows/bin/zlib1.dll"
        "${CMAKE_BINARY_DIR}/bin/examples/${CMAKE_BUILD_TYPE}/"
    COMMENT "Copying DLLs to examples directory"
)

# Install examples
install(TARGETS basic_diagnostics
    RUNTIME DESTINATION bin/examples
)

# Add more examples here as needed
# Example for ECU flashing:
# add_executable(ecu_flashing ecu_flashing.cpp)
# target_link_libraries(ecu_flashing PRIVATE fmus_auto)
# set_target_properties(ecu_flashing PROPERTIES
#     CXX_STANDARD 17
#     CXX_STANDARD_REQUIRED ON
#     RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
# )
# add_dependencies(ecu_flashing copy_dlls)