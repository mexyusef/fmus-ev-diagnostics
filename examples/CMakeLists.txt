# Examples for FMUS-AUTO

# Basic diagnostics example
add_executable(basic_diagnostics basic_diagnostics.cpp)
target_link_libraries(basic_diagnostics PRIVATE fmus_auto)
set_target_properties(basic_diagnostics PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)

# Comprehensive test example
add_executable(comprehensive_test comprehensive_test.cpp)
target_link_libraries(comprehensive_test PRIVATE fmus_auto)
set_target_properties(comprehensive_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)

# Phase 3 complete test example
add_executable(phase3_complete_test phase3_complete_test.cpp)
target_link_libraries(phase3_complete_test PRIVATE fmus_auto)
set_target_properties(phase3_complete_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)

# Phase 5 complete application example
add_executable(phase5_complete_application phase5_complete_application.cpp)
target_link_libraries(phase5_complete_application PRIVATE fmus_auto)
set_target_properties(phase5_complete_application PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)

# Copy shared library to examples directory on Windows
if(WIN32 AND NOT FMUS_AUTO_STATIC_BUILD)
    add_custom_command(
        TARGET basic_diagnostics POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:fmus_auto>
            $<TARGET_FILE_DIR:basic_diagnostics>
        COMMENT "Copying fmus_auto library to examples directory"
    )

    add_custom_command(
        TARGET comprehensive_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:fmus_auto>
            $<TARGET_FILE_DIR:comprehensive_test>
        COMMENT "Copying fmus_auto library to examples directory"
    )

    add_custom_command(
        TARGET phase3_complete_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:fmus_auto>
            $<TARGET_FILE_DIR:phase3_complete_test>
        COMMENT "Copying fmus_auto library to examples directory"
    )
endif()

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