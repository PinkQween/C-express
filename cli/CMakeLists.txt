cmake_minimum_required(VERSION 3.10)

# Build c-express CLI tool
add_executable(c-express-cli main.c)

# Set output name to just "c-express"
set_target_properties(c-express-cli PROPERTIES OUTPUT_NAME c-express)

# Compiler warnings
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(c-express-cli PRIVATE
        -Wall -Wextra -Wpedantic
    )
endif()

# Install c-express CLI tool
install(TARGETS c-express-cli
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
