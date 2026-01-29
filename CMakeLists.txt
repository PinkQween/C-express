cmake_minimum_required(VERSION 3.10)
project(CExpress VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Library source files
set(CEXPRESS_SOURCES
    src/cexpress.c
)

# Create library
add_library(cexpress STATIC ${CEXPRESS_SOURCES})

# Include directories
target_include_directories(cexpress PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link libraries
target_link_libraries(cexpress PUBLIC Threads::Threads)

# Compiler warnings
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cexpress PRIVATE
        -Wall -Wextra -Wpedantic
    )
endif()

# Installation rules
include(GNUInstallDirs)

install(TARGETS cexpress
    EXPORT CExpressTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export targets
install(EXPORT CExpressTargets
    FILE CExpressTargets.cmake
    NAMESPACE CExpress::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CExpress
)

# Create config file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CExpressConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/CExpressConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CExpress
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/CExpressConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/CExpressConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/CExpressConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CExpress
)

# Build examples
option(BUILD_EXAMPLES "Build example applications" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
