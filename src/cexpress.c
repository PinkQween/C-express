#include "cexpress/cexpress.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

#define MAX_ROUTES 100
#define MAX_MIDDLEWARE 50
#define MAX_WS_ROUTES 50
#define BUFFER_SIZE 4096
#define WS_MAGIC_STRING "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

/* Internal route structure */
struct cexpress_route {
    cexpress_method method;
    char *path;
    cexpress_handler handler;
};

/* Internal middleware structure */
struct cexpress_middleware {
    cexpress_middleware_fn fn;
};

/* WebSocket route structure */
typedef struct {
    char *path;
    cexpress_ws_handler on_connect;
    cexpress_ws_message_handler on_message;
} cexpress_ws_route;

/* Application structure */
struct cexpress_app {
    cexpress_route routes[MAX_ROUTES];
    int route_count;
    cexpress_middleware middlewares[MAX_MIDDLEWARE];
    int middleware_count;
    cexpress_ws_route ws_routes[MAX_WS_ROUTES];
    int ws_route_count;
    int server_fd;
    bool running;
    SSL_CTX *ssl_ctx;  /* TLS context */
    bool use_tls;      /* Whether TLS is enabled */
};

/* Forward declarations for static functions */
static int ws_send_frame(cexpress_ws *ws, const void *data, size_t len, cexpress_ws_opcode opcode);
static ssize_t ws_read_frame(cexpress_ws *ws, unsigned char *buffer, size_t buffer_size, 
                              cexpress_ws_opcode *opcode);
static void handle_websocket(cexpress_app *app, int client_fd, SSL *ssl, 
                             const char *path, cexpress_ws_route *ws_route);

/* Helper function to parse HTTP method */
static cexpress_method parse_method(const char *method_str) {
    if (strcmp(method_str, "GET") == 0) return CEXPRESS_GET;
    if (strcmp(method_str, "POST") == 0) return CEXPRESS_POST;
    if (strcmp(method_str, "PUT") == 0) return CEXPRESS_PUT;
    if (strcmp(method_str, "DELETE") == 0) return CEXPRESS_DELETE;
    if (strcmp(method_str, "PATCH") == 0) return CEXPRESS_PATCH;
    if (strcmp(method_str, "HEAD") == 0) return CEXPRESS_HEAD;
    if (strcmp(method_str, "OPTIONS") == 0) return CEXPRESS_OPTIONS;
    return CEXPRESS_GET;
}

/* Helper function to match route path */
static bool match_path(const char *route_path, const char *request_path) {
    /* Simple exact match for now - can be extended for parameters */
    return strcmp(route_path, request_path) == 0;
}

/* Create a new express application */
cexpress_app* cexpress_create(void) {
    cexpress_app *app = (cexpress_app *)malloc(sizeof(cexpress_app));
    if (!app) return NULL;
    
    app->route_count = 0;
    app->middleware_count = 0;
    app->ws_route_count = 0;
    app->server_fd = -1;
    app->running = false;
    app->ssl_ctx = NULL;
    app->use_tls = false;
    
    /* Initialize OpenSSL */
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    
    return app;
}

/* Register a route with specific method */
static void register_route(cexpress_app *app, const char *path, 
                          cexpress_method method, cexpress_handler handler) {
    if (app->route_count >= MAX_ROUTES) {
        fprintf(stderr, "Maximum number of routes reached\n");
        return;
    }
    
    cexpress_route *route = &app->routes[app->route_count];
    route->path = strdup(path);
    if (!route->path) {
        fprintf(stderr, "Failed to allocate memory for route path\n");
        return;
    }
    route->method = method;
    route->handler = handler;
    app->route_count++;
}

/* HTTP method registration functions */
void cexpress_get(cexpress_app *app, const char *path, cexpress_handler handler) {
    register_route(app, path, CEXPRESS_GET, handler);
}

void cexpress_post(cexpress_app *app, const char *path, cexpress_handler handler) {
    register_route(app, path, CEXPRESS_POST, handler);
}

void cexpress_put(cexpress_app *app, const char *path, cexpress_handler handler) {
    register_route(app, path, CEXPRESS_PUT, handler);
}

void cexpress_delete(cexpress_app *app, const char *path, cexpress_handler handler) {
    register_route(app, path, CEXPRESS_DELETE, handler);
}

/* Register middleware */
void cexpress_use(cexpress_app *app, cexpress_middleware_fn middleware) {
    if (app->middleware_count >= MAX_MIDDLEWARE) {
        fprintf(stderr, "Maximum number of middleware reached\n");
        return;
    }
    
    app->middlewares[app->middleware_count++].fn = middleware;
}

/* Response functions */
void cexpress_send(cexpress_res *res, int status_code, const char *body) {
    res->status_code = status_code;
    if (res->body) {
        free(res->body);
        res->body = NULL;
    }
    if (body) {
        res->body = strdup(body);
        if (!res->body) {
            /* Allocation failed - use empty string */
            res->body = strdup("");
        }
    }
    res->sent = true;
}

void cexpress_json(cexpress_res *res, const char *json) {
    cexpress_set_header(res, "Content-Type", "application/json");
    /* Use existing status code if set, otherwise default to 200 */
    int status = (res->status_code > 0) ? res->status_code : 200;
    cexpress_send(res, status, json);
}

cexpress_res* cexpress_status(cexpress_res *res, int status_code) {
    res->status_code = status_code;
    return res;
}

void cexpress_set_header(cexpress_res *res, const char *key, const char *value) {
    /* Simple implementation - in production would use a hash map */
    (void)res;
    (void)key;
    (void)value;
    /* Header storage would be implemented here */
}

const char* cexpress_get_header(cexpress_req *req, const char *key) {
    /* Simple implementation - in production would use a hash map */
    (void)req;
    (void)key;
    return NULL;
}

const char* cexpress_get_param(cexpress_req *req, const char *key) {
    /* Simple implementation - in production would parse route parameters */
    (void)req;
    (void)key;
    return NULL;
}

/* TLS-aware read/write functions */
static ssize_t tls_read(SSL *ssl, int fd, void *buf, size_t count) {
    if (ssl) {
        return SSL_read(ssl, buf, count);
    }
    return read(fd, buf, count);
}

static ssize_t tls_write(SSL *ssl, int fd, const void *buf, size_t count) {
    if (ssl) {
        return SSL_write(ssl, buf, count);
    }
    return write(fd, buf, count);
}

/* Base64 encoding for WebSocket handshake */
static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char* base64_encode(const unsigned char *data, size_t len) {
    size_t output_len = 4 * ((len + 2) / 3);
    char *encoded = malloc(output_len + 1);
    if (!encoded) return NULL;
    
    size_t i, j;
    for (i = 0, j = 0; i < len;) {
        uint32_t octet_a = i < len ? data[i++] : 0;
        uint32_t octet_b = i < len ? data[i++] : 0;
        uint32_t octet_c = i < len ? data[i++] : 0;
        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;
        
        encoded[j++] = base64_chars[(triple >> 18) & 0x3F];
        encoded[j++] = base64_chars[(triple >> 12) & 0x3F];
        encoded[j++] = (i > len + 1) ? '=' : base64_chars[(triple >> 6) & 0x3F];
        encoded[j++] = (i > len) ? '=' : base64_chars[triple & 0x3F];
    }
    encoded[output_len] = '\0';
    return encoded;
}

/* Extract WebSocket key from request */
static char* extract_ws_key(const char *request) {
    const char *key_header = "Sec-WebSocket-Key: ";
    char *key_start = strstr(request, key_header);
    if (!key_start) return NULL;
    
    key_start += strlen(key_header);
    char *key_end = strstr(key_start, "\r\n");
    if (!key_end) return NULL;
    
    size_t key_len = key_end - key_start;
    char *key = malloc(key_len + 1);
    if (!key) return NULL;
    
    memcpy(key, key_start, key_len);
    key[key_len] = '\0';
    return key;
}

/* Generate WebSocket accept key */
static char* generate_ws_accept_key(const char *client_key) {
    char combined[256];
    snprintf(combined, sizeof(combined), "%s%s", client_key, WS_MAGIC_STRING);
    
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)combined, strlen(combined), hash);
    
    return base64_encode(hash, SHA_DIGEST_LENGTH);
}

/* Check if request is WebSocket upgrade */
static bool is_websocket_upgrade(const char *request) {
    return strstr(request, "Upgrade: websocket") != NULL &&
           strstr(request, "Connection: Upgrade") != NULL;
}

/* Parse HTTP request */
static void parse_request(const char *raw_request, cexpress_req *req) {
    char method[16] = {0};
    char path[1024] = {0};  /* Increased buffer size */
    char version[16] = {0};
    
    /* Parse request line - field width limits prevent buffer overflow */
    if (sscanf(raw_request, "%15s %1023s %15s", method, path, version) == 3) {
        req->method = parse_method(method);
        
        /* Find query string before duplicating */
        char *query_start = strchr(path, '?');
        if (query_start) {
            *query_start = '\0';
            req->path = strdup(path);
            req->query = strdup(query_start + 1);
            if (!req->query) {
                req->query = NULL;  /* Handle strdup failure */
            }
        } else {
            req->path = strdup(path);
            req->query = NULL;
        }
        
        if (!req->path) {
            req->path = strdup("/");  /* Fallback to root if allocation fails */
        }
    } else {
        /* Failed to parse - use defaults */
        req->method = CEXPRESS_GET;
        req->path = strdup("/");
        req->query = NULL;
    }
    
    /* Parse body if present */
    const char *body_start = strstr(raw_request, "\r\n\r\n");
    if (body_start) {
        body_start += 4;
        if (*body_start) {
            req->body = strdup(body_start);
            if (!req->body) {
                req->body = NULL;  /* Handle strdup failure */
            }
        } else {
            req->body = NULL;
        }
    } else {
        req->body = NULL;
    }
    
    req->params = NULL;
    req->headers = NULL;
    req->user_data = NULL;
}

/* Build HTTP response */
static char* build_response(cexpress_res *res) {
    char *response = (char *)malloc(BUFFER_SIZE);
    if (!response) return NULL;
    
    const char *status_text = "OK";
    switch (res->status_code) {
        case 200: status_text = "OK"; break;
        case 201: status_text = "Created"; break;
        case 204: status_text = "No Content"; break;
        case 400: status_text = "Bad Request"; break;
        case 401: status_text = "Unauthorized"; break;
        case 403: status_text = "Forbidden"; break;
        case 404: status_text = "Not Found"; break;
        case 500: status_text = "Internal Server Error"; break;
        case 502: status_text = "Bad Gateway"; break;
        case 503: status_text = "Service Unavailable"; break;
        default: status_text = "OK"; break;
    }
    
    snprintf(response, BUFFER_SIZE,
        "HTTP/1.1 %d %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        res->status_code,
        status_text,
        res->body ? strlen(res->body) : 0,
        res->body ? res->body : ""
    );
    
    return response;
}

/* Handle client connection */
static void handle_client(cexpress_app *app, int client_fd) {
    SSL *ssl = NULL;
    
    /* TLS handshake if enabled */
    if (app->use_tls && app->ssl_ctx) {
        ssl = SSL_new(app->ssl_ctx);
        if (!ssl) {
            close(client_fd);
            return;
        }
        SSL_set_fd(ssl, client_fd);
        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(client_fd);
            return;
        }
    }
    
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = tls_read(ssl, client_fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read <= 0) {
        if (ssl) SSL_free(ssl);
        close(client_fd);
        return;
    }
    
    buffer[bytes_read] = '\0';
    
    /* Check for WebSocket upgrade */
    if (is_websocket_upgrade(buffer)) {
        /* Create request object to get path */
        cexpress_req req = {0};
        parse_request(buffer, &req);
        
        /* Find matching WebSocket route */
        for (int i = 0; i < app->ws_route_count; i++) {
            cexpress_ws_route *ws_route = &app->ws_routes[i];
            if (match_path(ws_route->path, req.path)) {
                /* Perform WebSocket handshake */
                char *client_key = extract_ws_key(buffer);
                if (client_key) {
                    char *accept_key = generate_ws_accept_key(client_key);
                    if (accept_key) {
                        char handshake[512];
                        snprintf(handshake, sizeof(handshake),
                            "HTTP/1.1 101 Switching Protocols\r\n"
                            "Upgrade: websocket\r\n"
                            "Connection: Upgrade\r\n"
                            "Sec-WebSocket-Accept: %s\r\n"
                            "\r\n", accept_key);
                        
                        tls_write(ssl, client_fd, handshake, strlen(handshake));
                        free(accept_key);
                        
                        /* Handle WebSocket connection */
                        handle_websocket(app, client_fd, ssl, req.path, ws_route);
                    }
                    free(client_key);
                }
                
                /* Cleanup and return */
                if (req.path) free(req.path);
                if (req.query) free(req.query);
                if (req.body) free(req.body);
                if (ssl) SSL_free(ssl);
                close(client_fd);
                return;
            }
        }
        
        /* No WebSocket route found - cleanup */
        if (req.path) free(req.path);
        if (req.query) free(req.query);
        if (req.body) free(req.body);
    }
    
    /* Regular HTTP request */
    cexpress_req req = {0};
    cexpress_res res = {0};
    res.status_code = 200;
    res.sent = false;
    
    parse_request(buffer, &req);
    
    /* Find matching route */
    bool route_found = false;
    for (int i = 0; i < app->route_count; i++) {
        cexpress_route *route = &app->routes[i];
        if (route->method == req.method && match_path(route->path, req.path)) {
            route->handler(&req, &res);
            route_found = true;
            break;
        }
    }
    
    /* Send 404 if no route found */
    if (!route_found) {
        cexpress_send(&res, 404, "Not Found");
    }
    
    /* Build and send response */
    char *response = build_response(&res);
    if (response) {
        tls_write(ssl, client_fd, response, strlen(response));
        free(response);
    }
    
    /* Cleanup */
    if (req.path) free(req.path);
    if (req.query) free(req.query);
    if (req.body) free(req.body);
    if (res.body) free(res.body);
    
    if (ssl) SSL_free(ssl);
    close(client_fd);
}

/* WebSocket frame parsing and sending */
static int ws_send_frame(cexpress_ws *ws, const void *data, size_t len, cexpress_ws_opcode opcode) {
    unsigned char header[14];
    size_t header_len = 2;
    
    header[0] = 0x80 | opcode; /* FIN bit + opcode */
    
    if (len < 126) {
        header[1] = len;
    } else if (len < 65536) {
        header[1] = 126;
        header[2] = (len >> 8) & 0xFF;
        header[3] = len & 0xFF;
        header_len = 4;
    } else {
        header[1] = 127;
        for (int i = 0; i < 8; i++) {
            header[9 - i] = (len >> (i * 8)) & 0xFF;
        }
        header_len = 10;
    }
    
    SSL *ssl = (SSL*)ws->ssl;
    if (tls_write(ssl, ws->fd, header, header_len) < 0) return -1;
    if (len > 0 && tls_write(ssl, ws->fd, data, len) < 0) return -1;
    
    return 0;
}

static ssize_t ws_read_frame(cexpress_ws *ws, unsigned char *buffer, size_t buffer_size, 
                              cexpress_ws_opcode *opcode) {
    unsigned char header[14];
    SSL *ssl = (SSL*)ws->ssl;
    
    if (tls_read(ssl, ws->fd, header, 2) < 2) return -1;
    
    *opcode = header[0] & 0x0F;
    bool masked = (header[1] & 0x80) != 0;
    uint64_t payload_len = header[1] & 0x7F;
    
    if (payload_len == 126) {
        if (tls_read(ssl, ws->fd, header + 2, 2) < 2) return -1;
        payload_len = (header[2] << 8) | header[3];
    } else if (payload_len == 127) {
        if (tls_read(ssl, ws->fd, header + 2, 8) < 8) return -1;
        payload_len = 0;
        for (int i = 0; i < 8; i++) {
            payload_len = (payload_len << 8) | header[2 + i];
        }
    }
    
    unsigned char mask[4] = {0};
    if (masked) {
        if (tls_read(ssl, ws->fd, mask, 4) < 4) return -1;
    }
    
    if (payload_len > buffer_size) return -1;
    
    ssize_t bytes_read = tls_read(ssl, ws->fd, buffer, payload_len);
    if (bytes_read < 0) return -1;
    
    if (masked) {
        for (size_t i = 0; i < payload_len; i++) {
            buffer[i] ^= mask[i % 4];
        }
    }
    
    return payload_len;
}

/* Handle WebSocket connection */
static void handle_websocket(cexpress_app *app, int client_fd, SSL *ssl, 
                             const char *path, cexpress_ws_route *ws_route) {
    cexpress_ws ws = {0};
    ws.fd = client_fd;
    ws.ssl = ssl;
    ws.closed = false;
    
    if (ws_route->on_connect) {
        ws_route->on_connect(&ws);
    }
    
    unsigned char buffer[BUFFER_SIZE];
    while (!ws.closed && app->running) {
        cexpress_ws_opcode opcode;
        ssize_t len = ws_read_frame(&ws, buffer, sizeof(buffer), &opcode);
        
        if (len < 0) break;
        
        if (opcode == CEXPRESS_WS_CLOSE) {
            ws.closed = true;
            ws_send_frame(&ws, NULL, 0, CEXPRESS_WS_CLOSE);
            break;
        } else if (opcode == CEXPRESS_WS_PING) {
            ws_send_frame(&ws, buffer, len, CEXPRESS_WS_PONG);
        } else if (ws_route->on_message) {
            buffer[len] = '\0';
            ws_route->on_message(&ws, (char*)buffer, len, opcode);
        }
    }
}

/* Server thread function */
static void* server_thread(void *arg) {
    cexpress_app *app = (cexpress_app *)arg;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    /* Set socket to non-blocking mode */
    int flags = fcntl(app->server_fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(app->server_fd, F_SETFL, flags | O_NONBLOCK);
    }
    
    while (app->running) {
        /* Use poll to wait with timeout so we can check running flag */
        struct pollfd pfd = {
            .fd = app->server_fd,
            .events = POLLIN,
            .revents = 0
        };
        
        int poll_result = poll(&pfd, 1, 100); /* 100ms timeout */
        
        if (poll_result < 0) {
            if (errno == EINTR) {
                /* Interrupted by signal, check running flag */
                continue;
            }
            if (app->running) {
                perror("poll failed");
            }
            break;
        }
        
        if (poll_result == 0) {
            /* Timeout, check running flag and continue */
            continue;
        }
        
        /* Socket is ready for accept */
        int client_fd = accept(app->server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                /* No connection available right now */
                continue;
            }
            if (app->running) {
                perror("accept failed");
            }
            continue;
        }
        
        handle_client(app, client_fd);
    }
    
    return NULL;
}

/* Start listening on specified port */
int cexpress_listen(cexpress_app *app, int port, void (*callback)(void)) {
    struct sockaddr_in server_addr;
    
    /* Create socket */
    app->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (app->server_fd < 0) {
        perror("socket creation failed");
        return -1;
    }
    
    /* Set socket options */
    int opt = 1;
    if (setsockopt(app->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(app->server_fd);
        return -1;
    }
    
    /* Setup server address */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    /* Bind socket */
    if (bind(app->server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(app->server_fd);
        return -1;
    }
    
    /* Listen for connections */
    if (listen(app->server_fd, 10) < 0) {
        perror("listen failed");
        close(app->server_fd);
        return -1;
    }
    
    app->running = true;
    
    /* Call callback if provided */
    if (callback) {
        callback();
    }
    
    /* Start server thread */
    pthread_t thread;
    if (pthread_create(&thread, NULL, server_thread, app) != 0) {
        perror("pthread_create failed");
        close(app->server_fd);
        app->running = false;
        return -1;
    }
    pthread_join(thread, NULL);
    
    return 0;
}

/* Start listening with HTTPS/TLS */
int cexpress_listen_https(cexpress_app *app, int port, const cexpress_tls_config *tls_config, 
                          void (*callback)(void)) {
    if (!tls_config || !tls_config->cert_file || !tls_config->key_file) {
        fprintf(stderr, "TLS config, cert_file, and key_file are required\n");
        return -1;
    }
    
    /* Create SSL context */
    app->ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (!app->ssl_ctx) {
        ERR_print_errors_fp(stderr);
        return -1;
    }
    
    /* Load certificate */
    if (SSL_CTX_use_certificate_file(app->ssl_ctx, tls_config->cert_file, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(app->ssl_ctx);
        app->ssl_ctx = NULL;
        return -1;
    }
    
    /* Load private key */
    if (SSL_CTX_use_PrivateKey_file(app->ssl_ctx, tls_config->key_file, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(app->ssl_ctx);
        app->ssl_ctx = NULL;
        return -1;
    }
    
    /* Verify private key */
    if (!SSL_CTX_check_private_key(app->ssl_ctx)) {
        fprintf(stderr, "Private key does not match certificate\n");
        SSL_CTX_free(app->ssl_ctx);
        app->ssl_ctx = NULL;
        return -1;
    }
    
    /* Optional: Load CA cert for client verification */
    if (tls_config->ca_file) {
        if (SSL_CTX_load_verify_locations(app->ssl_ctx, tls_config->ca_file, NULL) != 1) {
            ERR_print_errors_fp(stderr);
        }
        if (tls_config->verify_client) {
            SSL_CTX_set_verify(app->ssl_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
        }
    }
    
    app->use_tls = true;
    
    /* Call regular listen */
    return cexpress_listen(app, port, callback);
}

/* WebSocket route registration */
void cexpress_websocket(cexpress_app *app, const char *path,
                        cexpress_ws_handler on_connect,
                        cexpress_ws_message_handler on_message) {
    if (app->ws_route_count >= MAX_WS_ROUTES) {
        fprintf(stderr, "Maximum number of WebSocket routes reached\n");
        return;
    }
    
    cexpress_ws_route *ws_route = &app->ws_routes[app->ws_route_count];
    ws_route->path = strdup(path);
    if (!ws_route->path) {
        fprintf(stderr, "Failed to allocate memory for WebSocket route path\n");
        return;
    }
    ws_route->on_connect = on_connect;
    ws_route->on_message = on_message;
    app->ws_route_count++;
}

/* WebSocket send functions */
int cexpress_ws_send(cexpress_ws *ws, const void *message, size_t len, cexpress_ws_opcode opcode) {
    if (!ws || ws->closed) return -1;
    return ws_send_frame(ws, message, len, opcode);
}

int cexpress_ws_send_text(cexpress_ws *ws, const char *message) {
    if (!ws || !message) return -1;
    return cexpress_ws_send(ws, message, strlen(message), CEXPRESS_WS_TEXT);
}

void cexpress_ws_close(cexpress_ws *ws, int code, const char *reason) {
    if (!ws || ws->closed) return;
    
    unsigned char close_frame[125];
    close_frame[0] = (code >> 8) & 0xFF;
    close_frame[1] = code & 0xFF;
    
    size_t reason_len = 0;
    if (reason) {
        reason_len = strlen(reason);
        if (reason_len > 123) reason_len = 123;
        memcpy(close_frame + 2, reason, reason_len);
    }
    
    ws_send_frame(ws, close_frame, 2 + reason_len, CEXPRESS_WS_CLOSE);
    ws->closed = true;
}

/* Stop the server gracefully */
void cexpress_stop(cexpress_app *app) {
    if (!app) return;
    
    app->running = false;
    
    /* Close server socket to wake up accept() if it's blocking */
    if (app->server_fd >= 0) {
        shutdown(app->server_fd, SHUT_RDWR);
    }
}

/* Destroy express app and free resources */
void cexpress_destroy(cexpress_app *app) {
    if (!app) return;
    
    app->running = false;
    
    if (app->server_fd >= 0) {
        close(app->server_fd);
    }
    
    /* Free SSL context */
    if (app->ssl_ctx) {
        SSL_CTX_free(app->ssl_ctx);
    }
    
    /* Free routes */
    for (int i = 0; i < app->route_count; i++) {
        if (app->routes[i].path) {
            free(app->routes[i].path);
        }
    }
    
    /* Free WebSocket routes */
    for (int i = 0; i < app->ws_route_count; i++) {
        if (app->ws_routes[i].path) {
            free(app->ws_routes[i].path);
        }
    }
    
    free(app);
}
