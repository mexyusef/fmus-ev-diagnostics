# Testing for FMUS-AUTO
include(CTest)
find_package(GTest REQUIRED)

# Add test executable
add_executable(test_j2534_device test_j2534_device.cpp)

# Link with our library and GTest
target_link_libraries(test_j2534_device
    PRIVATE
        fmus_auto
        ${GTEST_LIBRARY}
        ${GTEST_MAIN_LIBRARY}
)

# Set C++ standard and output directory
set_target_properties(test_j2534_device PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
)

# Make sure the DLLs get copied to the test output directory
add_custom_command(
    TARGET test_j2534_device
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/tests/${CMAKE_BUILD_TYPE}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/fmus_auto.dll"
        "C:/libuv-install/bin/uv.dll"
        "C:/usockets-install/bin/uSockets.dll"
        "C:/vcpkg/packages/zlib_x64-windows/bin/zlib1.dll"
        "${GTEST_DLL}"
        "${GTEST_MAIN_DLL}"
        "${GMOCK_DLL}"
        "${GMOCK_MAIN_DLL}"
        "${CMAKE_BINARY_DIR}/bin/tests/${CMAKE_BUILD_TYPE}/"
    COMMENT "Copying DLLs to test output directory"
)

# Add the test to CTest
add_test(
    NAME test_j2534_device
    COMMAND test_j2534_device
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
)

# Optional: Create a target to run tests with verbose output
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS test_j2534_device
    COMMENT "Running tests with verbose output"
)