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
- 🌐 **HTTP Server** - Basic HTTP/1.1 server implementation
- 🛠️ **Project Generator** - CLI tool to scaffold new C-Express projects
- 📝 **Automated Installer** - Easy installation script with dependency checking
- ✅ **Professional Tests** - Comprehensive unit and integration test suite

## Installation

### Prerequisites

- C compiler (GCC, Clang, or compatible)
- CMake 3.10 or higher
- POSIX-compliant system (Linux, macOS, BSD)

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
c-express myHTTPSApp
cd myHTTPSApp

# Build and run
mkdir build && cd build
cmake ..
make
./myHTTPSApp
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

### Complete Example

See `examples/basic_server.c` for a complete working example with multiple routes and response types.

To run the example:

```bash
cd build
./examples/basic_server
```

Then visit:
- http://localhost:3000/ - Simple text response
- http://localhost:3000/about - HTML page
- http://localhost:3000/api/json - JSON response

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

## Limitations

This is a basic implementation intended for educational purposes and small projects. Current limitations and areas for improvement:

- **Sequential request handling** - Requests are handled one at a time, not concurrently
- **Middleware execution** - Middleware registration API exists but execution is not yet implemented
- **Route parameters** - API exists but parameter parsing (e.g., `/users/:id`) is not implemented
- **Header storage** - Header get/set functions are stubs; full implementation needed
- **HTTP parsing** - Simplified parser; production use requires proper HTTP/1.1 compliance
- **Security** - Not hardened for production use; needs input validation and security auditing

For production use, consider:

- Implement concurrent request handling with thread pool or async I/O
- Complete middleware execution pipeline
- Add support for route parameters (e.g., `/users/:id`)
- Implement proper header storage and retrieval with hash maps
- Add HTTPS/TLS support
- Implement connection pooling and keep-alive
- Add support for static file serving
- Improve error handling and edge cases
- Add request size limits and timeouts
- Security hardening and input validation

## Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.

## License

This project is open source and available under the MIT License.

## Acknowledgments

Inspired by Express.js - the de facto standard web framework for Node.js.
