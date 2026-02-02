cmake_minimum_required(VERSION 3.10)

# Basic server example
add_executable(basic_server basic_server.c)
target_link_libraries(basic_server PRIVATE cexpress)

# HTTPS server example
add_executable(https_server https_server.c)
target_link_libraries(https_server PRIVATE cexpress)

# WebSocket server example
add_executable(websocket_server websocket_server.c)
target_link_libraries(websocket_server PRIVATE cexpress)

# Install examples (optional)
install(TARGETS basic_server https_server websocket_server
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
)
