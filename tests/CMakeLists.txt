cmake_minimum_required(VERSION 3.10)

# Unit tests
add_executable(test_unit test_unit.c)
target_link_libraries(test_unit PRIVATE cexpress)
target_include_directories(test_unit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Integration tests
add_executable(test_integration test_integration.c)
target_link_libraries(test_integration PRIVATE cexpress)
target_include_directories(test_integration PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Compiler warnings
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_unit PRIVATE
        -Wall -Wextra -Wpedantic
    )
    target_compile_options(test_integration PRIVATE
        -Wall -Wextra -Wpedantic
    )
endif()

# Add tests to CTest (enable_testing() called from root CMakeLists.txt)
add_test(NAME unit_tests COMMAND test_unit)
add_test(NAME integration_tests COMMAND test_integration)
