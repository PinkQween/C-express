# C-Express

A lightweight, Express.js-inspired web framework for C.

## Overview

C-Express brings the simplicity and elegance of Express.js to C programming. It provides an intuitive API for building HTTP servers with routing, middleware support, and request/response handling.

## Features

- 🚀 **Simple API** - Familiar Express.js-like syntax
- 🛣️ **Routing** - Support for GET, POST, PUT, DELETE, and more HTTP methods
- 🔌 **Middleware** - Middleware registration API (framework for future implementation)
- 📦 **Lightweight** - Minimal dependencies, pure C implementation
- 🔧 **CMake Build System** - Easy integration into your projects
- 🌐 **HTTP/1.1 & HTTP/2** - Modern HTTP protocol support with nghttp2
- 🔒 **TLS/SSL Support** - HTTPS server with OpenSSL
- 🔌 **WebSocket** - Full WebSocket protocol support for real-time communication
- 🛠️ **Project Generator** - CLI tool to scaffold new C-Express projects
- 📝 **Automated Installer** - Easy installation script with dependency checking
- ✅ **Professional Tests** - Comprehensive unit and integration test suite

## Installation

### Prerequisites

- C compiler (GCC, Clang, or compatible)
- CMake 3.10 or higher
- POSIX-compliant system (Linux, macOS, BSD)
- OpenSSL 1.1+ (for HTTPS/TLS support)
- libnghttp2 (optional, for HTTP/2 support)

### Quick Install (Automated)

The easiest way to install C-Express is using the installation script:

```bash
# Clone the repository
git clone https://github.com/PinkQween/C-express.git
cd C-express

# Run the installation script
./install.sh

# Or with custom options
./install.sh --prefix /usr/local --skip-tests
```

Installer options:
- `--prefix <path>` - Installation prefix (default: /usr/local)
- `--no-sudo` - Don't use sudo for installation
- `--skip-tests` - Skip running tests
- `--build-dir <path>` - Build directory (default: ./build)

### Manual Installation

```bash
# Clone the repository
git clone https://github.com/PinkQween/C-express.git
cd C-express

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make

# Run tests (optional)
ctest --output-on-failure

# Install system-wide
sudo make install
```

### Verify Installation

After installation, verify that the CLI tool is available:

```bash
c-express --version
c-express --help
```

## Quick Start

### Creating a New Project

The fastest way to get started is using the c-express project generator:

```bash
# Create a new project
c-express myWebApp
cd myWebApp

# Build and run
mkdir build && cd build
cmake ..
make
./myWebApp
```

Or initialize a project in the current directory:

```bash
mkdir my-project && cd my-project
c-express .

# Build and run
mkdir build && cd build
cmake ..
make
./my-project
```

### Manual Setup

Here's a simple "Hello World" server:

```c
#include <stdio.h>
#include "cexpress/cexpress.h"

void handle_root(cexpress_req *req, cexpress_res *res) {
    cexpress_send(res, 200, "Hello from C-Express!");
}

void server_started(void) {
    printf("Server running on http://localhost:3000\n");
}

int main(void) {
    cexpress_app *app = cexpress_create();
    
    cexpress_get(app, "/", handle_root);
    
    cexpress_listen(app, 3000, server_started);
    cexpress_destroy(app);
    
    return 0;
}
```

### Building Your Application

```bash
# Using pkg-config (if installed system-wide)
gcc myapp.c -o myapp $(pkg-config --cflags --libs cexpress)

# Or with CMake
# CMakeLists.txt:
find_package(CExpress REQUIRED)
add_executable(myapp myapp.c)
target_link_libraries(myapp PRIVATE CExpress::cexpress)
```

## API Reference

### Core Functions

#### `cexpress_create()`
Creates a new Express application instance.

```c
cexpress_app* app = cexpress_create();
```

#### `cexpress_listen(app, port, callback)`
Starts the server on the specified port.

```c
cexpress_listen(app, 3000, server_started);
```

#### `cexpress_destroy(app)`
Cleans up and destroys the application instance.

```c
cexpress_destroy(app);
```

### Routing

#### GET Route
```c
void handle_get(cexpress_req *req, cexpress_res *res) {
    cexpress_send(res, 200, "GET request handled");
}

cexpress_get(app, "/path", handle_get);
```

#### POST Route
```c
void handle_post(cexpress_req *req, cexpress_res *res) {
    cexpress_send(res, 201, "Created");
}

cexpress_post(app, "/users", handle_post);
```

#### Other HTTP Methods
- `cexpress_put(app, path, handler)` - Handle PUT requests
- `cexpress_delete(app, path, handler)` - Handle DELETE requests

### Response Methods

#### Send Response
```c
cexpress_send(res, 200, "Response body");
```

#### Send JSON
```c
cexpress_json(res, "{\"message\": \"Hello, JSON!\"}");
```

#### Set Status
```c
cexpress_status(res, 404);
cexpress_send(res, 404, "Not Found");
```

#### Set Headers
```c
cexpress_set_header(res, "Content-Type", "text/html");
```

### Request Properties

- `req->method` - HTTP method (CEXPRESS_GET, CEXPRESS_POST, etc.)
- `req->path` - Request path
- `req->query` - Query string
- `req->body` - Request body
- `req->params` - Route parameters
- `req->headers` - Request headers

### Middleware

**Note:** Middleware registration is available via `cexpress_use()`, but middleware execution is not yet implemented in the current version. This API is provided for future compatibility.

```c
void logger_middleware(cexpress_req *req, cexpress_res *res, void (*next)(void)) {
    printf("[%s] %s\n", "LOG", req->path);
    next();
}

cexpress_use(app, logger_middleware);  /* Registered but not yet executed */
```

### HTTPS/TLS Support

C-Express supports secure HTTPS connections using OpenSSL.

#### Start HTTPS Server
```c
cexpress_tls_config tls_config = {
    .cert_file = "server.crt",       /* Path to SSL certificate */
    .key_file = "server.key",        /* Path to private key */
    .ca_file = NULL,                 /* Optional: CA certificate for client verification */
    .verify_client = false           /* Whether to verify client certificates */
};

cexpress_listen_https(app, 8443, &tls_config, callback);
```

#### Generate Self-Signed Certificate (for testing)
```bash
openssl req -x509 -newkey rsa:4096 -keyout server.key -out server.crt -days 365 -nodes
```

### WebSocket Support

C-Express provides full WebSocket protocol support for real-time bidirectional communication.

#### Register WebSocket Route
```c
void ws_on_connect(cexpress_ws *ws) {
    printf("Client connected!\n");
    cexpress_ws_send_text(ws, "Welcome!");
}

void ws_on_message(cexpress_ws *ws, const char *message, size_t len, cexpress_ws_opcode opcode) {
    printf("Received: %s\n", message);
    
    /* Echo message back */
    cexpress_ws_send_text(ws, message);
    
    /* Or send binary data */
    cexpress_ws_send(ws, data, data_len, CEXPRESS_WS_BINARY);
    
    /* Close connection */
    if (strcmp(message, "bye") == 0) {
        cexpress_ws_close(ws, 1000, "Normal closure");
    }
}

cexpress_websocket(app, "/ws", ws_on_connect, ws_on_message);
```

#### WebSocket Functions
- `cexpress_websocket(app, path, on_connect, on_message)` - Register WebSocket route
- `cexpress_ws_send(ws, data, len, opcode)` - Send WebSocket frame
- `cexpress_ws_send_text(ws, message)` - Send text message
- `cexpress_ws_close(ws, code, reason)` - Close WebSocket connection

#### WebSocket Opcodes
- `CEXPRESS_WS_TEXT` - Text message
- `CEXPRESS_WS_BINARY` - Binary message
- `CEXPRESS_WS_CLOSE` - Connection close
- `CEXPRESS_WS_PING` - Ping frame
- `CEXPRESS_WS_PONG` - Pong frame

## Testing

C-Express includes a comprehensive test suite with both unit and integration tests.

### Running Tests

```bash
# From the build directory
cd build
ctest --output-on-failure

# Or run tests directly
./tests/test_unit
./tests/test_integration
```

### Test Structure

The test framework provides:
- **Unit Tests** - Test individual functions and components
- **Integration Tests** - Test HTTP server functionality end-to-end
- **Assertion Macros** - Professional testing utilities
  - `ASSERT(condition)` - Basic assertion
  - `ASSERT_EQUAL(a, b)` - Equality check
  - `ASSERT_NOT_NULL(ptr)` - Null pointer check
  - `ASSERT_STR_EQUAL(a, b)` - String comparison
  - `ASSERT_STR_CONTAINS(str, substr)` - Substring check

### Writing Tests

Tests use a simple macro-based framework:

```c
#include "test_framework.h"

TEST_SUITE(my_feature) {
    TEST(feature_works) {
        int result = my_function();
        ASSERT_EQUAL(result, 42);
    }
    END_TEST();
}
END_TEST_SUITE()

int main(void) {
    RUN_TEST_SUITE(my_feature);
    print_test_summary();
    return get_test_result();
}
```

## Examples

### JSON API Server

```c
#include "cexpress/cexpress.h"

void get_users(cexpress_req *req, cexpress_res *res) {
    cexpress_json(res, 
        "[{\"id\":1,\"name\":\"Alice\"},{\"id\":2,\"name\":\"Bob\"}]");
}

void create_user(cexpress_req *req, cexpress_res *res) {
    cexpress_status(res, 201);
    cexpress_json(res, "{\"id\":3,\"name\":\"Charlie\"}");
}

int main(void) {
    cexpress_app *app = cexpress_create();
    
    cexpress_get(app, "/api/users", get_users);
    cexpress_post(app, "/api/users", create_user);
    
    cexpress_listen(app, 8080, NULL);
    cexpress_destroy(app);
    return 0;
}
```

### HTTPS Server Example

```c
#include "cexpress/cexpress.h"

void handle_secure(cexpress_req *req, cexpress_res *res) {
    cexpress_json(res, "{\"message\": \"Secure connection!\"}");
}

int main(void) {
    cexpress_app *app = cexpress_create();
    cexpress_get(app, "/secure", handle_secure);
    
    cexpress_tls_config tls = {
        .cert_file = "server.crt",
        .key_file = "server.key"
    };
    
    cexpress_listen_https(app, 8443, &tls, NULL);
    cexpress_destroy(app);
    return 0;
}
```

### WebSocket Server Example

```c
#include "cexpress/cexpress.h"

void ws_connect(cexpress_ws *ws) {
    cexpress_ws_send_text(ws, "Welcome!");
}

void ws_message(cexpress_ws *ws, const char *msg, size_t len, cexpress_ws_opcode op) {
    cexpress_ws_send_text(ws, msg);  /* Echo */
}

int main(void) {
    cexpress_app *app = cexpress_create();
    cexpress_websocket(app, "/ws", ws_connect, ws_message);
    
    cexpress_listen(app, 3000, NULL);
    cexpress_destroy(app);
    return 0;
}
```

### Running Examples

The repository includes several example applications:

```bash
cd build

# Basic HTTP server
./examples/basic_server

# HTTPS/TLS server (requires certificate files)
./examples/https_server

# WebSocket server
./examples/websocket_server
```

**Basic Server** - http://localhost:3000/
- http://localhost:3000/ - Simple text response
- http://localhost:3000/about - HTML page
- http://localhost:3000/api/json - JSON response

**HTTPS Server** - https://localhost:8443/
- Demonstrates TLS/SSL encryption
- Requires server.crt and server.key files

**WebSocket Server** - http://localhost:3000/
- http://localhost:3000/ - Test page with WebSocket client
- ws://localhost:3000/ws - WebSocket endpoint

## Project Structure

```
C-express/
├── include/
│   └── cexpress/
│       └── cexpress.h         # Public API header
├── src/
│   └── cexpress.c             # Implementation
├── cli/
│   ├── main.c                 # c-express CLI tool
│   └── CMakeLists.txt
├── tests/
│   ├── test_framework.h       # Test framework
│   ├── test_unit.c            # Unit tests
│   ├── test_integration.c     # Integration tests
│   └── CMakeLists.txt
├── installer/
│   ├── install.c              # Automated installer
│   └── CMakeLists.txt
├── examples/
│   ├── basic_server.c         # Example application
│   └── CMakeLists.txt
├── cmake/
│   └── CExpressConfig.cmake.in
├── CMakeLists.txt             # Build configuration
└── README.md
```

## Comparison with Express.js

| Express.js | C-Express |
|------------|-----------|
| `const app = express()` | `cexpress_app *app = cexpress_create()` |
| `app.get('/path', handler)` | `cexpress_get(app, "/path", handler)` |
| `app.post('/path', handler)` | `cexpress_post(app, "/path", handler)` |
| `res.send('text')` | `cexpress_send(res, 200, "text")` |
| `res.json({...})` | `cexpress_json(res, "{...}")` |
| `res.status(404)` | `cexpress_status(res, 404)` |
| `app.listen(3000, callback)` | `cexpress_listen(app, 3000, callback)` |
| `app.use(middleware)` | `cexpress_use(app, middleware)` |
| `https.createServer(options, app)` | `cexpress_listen_https(app, 8443, &tls_config, callback)` |
| `app.ws('/path', handler)` (via express-ws) | `cexpress_websocket(app, "/ws", on_connect, on_message)` |
| `ws.send(data)` | `cexpress_ws_send_text(ws, message)` |

## Limitations

This is a basic implementation intended for educational purposes and small projects. Current limitations and areas for improvement:

- **Sequential request handling** - Requests are handled one at a time, not concurrently
- **Middleware execution** - Middleware registration API exists but execution is not yet implemented
- **Route parameters** - API exists but parameter parsing (e.g., `/users/:id`) is not implemented
- **Header storage** - Header get/set functions are stubs; full implementation needed
- **HTTP parsing** - Simplified parser; production use requires proper HTTP/1.1 compliance
- **Security** - Not hardened for production use; needs input validation and security auditing
- **HTTP/2** - nghttp2 library detected but protocol handling not yet implemented

For production use, consider:

- Implement concurrent request handling with thread pool or async I/O
- Complete middleware execution pipeline
- Add support for route parameters (e.g., `/users/:id`)
- Implement proper header storage and retrieval with hash maps
- ✅ ~~Add HTTPS/TLS support~~ **IMPLEMENTED**
- ✅ ~~Add WebSocket support~~ **IMPLEMENTED**
- ✅ ~~Add proper signal handling for graceful shutdown~~ **IMPLEMENTED**
- Complete HTTP/2 protocol implementation
- Implement connection pooling and keep-alive
- Add support for static file serving
- Improve error handling and edge cases
- Add request size limits and timeouts
- Security hardening and input validation

## Documentation

C-Express uses [Doxygen](https://www.doxygen.nl/) for API documentation generation.

### Generate Documentation

```bash
# Using the build script (easiest)
./build_docs.sh

# Using CMake
cd build
make docs

# Or directly with Doxygen
doxygen Doxyfile
```

The generated documentation will be in the `docs/` directory. Open `docs/index.html` in your browser to view the full API reference.

### Prerequisites

- Doxygen 1.8+ (install with `apt install doxygen` on Debian/Ubuntu)

## Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.

## License

This project is open source and available under the MIT License.

## Acknowledgments

Inspired by Express.js - the de facto standard web framework for Node.js.
