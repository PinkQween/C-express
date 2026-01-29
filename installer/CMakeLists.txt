cmake_minimum_required(VERSION 3.10)

# Build installer
add_executable(cexpress-installer install.c)

# Compiler warnings
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cexpress-installer PRIVATE
        -Wall -Wextra -Wpedantic
    )
endif()

# Note: Installer is for building from source, not typically installed
