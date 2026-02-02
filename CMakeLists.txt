cmake_minimum_required(VERSION 3.10)
project(CExpress VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(LIBNGHTTP2 libnghttp2)
endif()

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
target_link_libraries(cexpress PUBLIC Threads::Threads OpenSSL::SSL OpenSSL::Crypto)

# Optional HTTP/2 support via nghttp2
if(LIBNGHTTP2_FOUND)
    target_compile_definitions(cexpress PRIVATE CEXPRESS_HTTP2_SUPPORT)
    target_include_directories(cexpress PRIVATE ${LIBNGHTTP2_INCLUDE_DIRS})
    target_link_libraries(cexpress PUBLIC ${LIBNGHTTP2_LIBRARIES})
    message(STATUS "HTTP/2 support enabled via nghttp2")
else()
    message(STATUS "HTTP/2 support disabled (nghttp2 not found)")
endif()

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

# Build CLI tool
option(BUILD_CLI "Build c-express CLI tool" ON)
if(BUILD_CLI)
    add_subdirectory(cli)
endif()

# Build tests
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Build installer
option(BUILD_INSTALLER "Build installer" ON)
if(BUILD_INSTALLER)
    add_subdirectory(installer)
endif()

# Doxygen documentation
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
    message(STATUS "Doxygen found - 'make docs' target available")
else()
    message(STATUS "Doxygen not found - documentation target disabled")
endif()
