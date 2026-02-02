#ifndef CEXPRESS_H
#define CEXPRESS_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct cexpress_app cexpress_app;
typedef struct cexpress_req cexpress_req;
typedef struct cexpress_res cexpress_res;
typedef struct cexpress_route cexpress_route;
typedef struct cexpress_middleware cexpress_middleware;
typedef struct cexpress_ws cexpress_ws;

/* HTTP Methods */
typedef enum {
    CEXPRESS_GET,
    CEXPRESS_POST,
    CEXPRESS_PUT,
    CEXPRESS_DELETE,
    CEXPRESS_PATCH,
    CEXPRESS_HEAD,
    CEXPRESS_OPTIONS
} cexpress_method;

/* WebSocket Opcodes */
typedef enum {
    CEXPRESS_WS_TEXT = 0x1,
    CEXPRESS_WS_BINARY = 0x2,
    CEXPRESS_WS_CLOSE = 0x8,
    CEXPRESS_WS_PING = 0x9,
    CEXPRESS_WS_PONG = 0xA
} cexpress_ws_opcode;

/* TLS Configuration */
typedef struct {
    const char *cert_file;      /* Path to certificate file (PEM format) */
    const char *key_file;       /* Path to private key file (PEM format) */
    const char *ca_file;        /* Optional: CA certificate file for client verification */
    bool verify_client;         /* Whether to verify client certificates */
} cexpress_tls_config;

/* Request structure */
struct cexpress_req {
    cexpress_method method;
    char *path;
    char *query;
    char *body;
    void *params;      /* Route parameters */
    void *headers;     /* Request headers */
    void *user_data;   /* User-defined data */
    bool is_websocket; /* Whether this is a WebSocket upgrade request */
};

/* Response structure */
struct cexpress_res {
    int status_code;
    char *body;
    void *headers;     /* Response headers */
    bool sent;         /* Whether response has been sent */
    void *ws;          /* WebSocket connection (if upgraded) */
};

/* WebSocket connection structure */
struct cexpress_ws {
    int fd;            /* Socket file descriptor */
    bool closed;       /* Whether connection is closed */
    void *ssl;         /* SSL connection (if TLS enabled) */
    void *user_data;   /* User-defined data */
};

/* Handler function types */
typedef void (*cexpress_handler)(cexpress_req *req, cexpress_res *res);
typedef void (*cexpress_middleware_fn)(cexpress_req *req, cexpress_res *res, void (*next)(void));
typedef void (*cexpress_ws_handler)(cexpress_ws *ws);
typedef void (*cexpress_ws_message_handler)(cexpress_ws *ws, const char *message, size_t len, cexpress_ws_opcode opcode);

/* Core API functions */

/**
 * Create a new express application
 * @return New express app instance
 */
cexpress_app* cexpress_create(void);

/**
 * Start listening on specified port
 * @param app Express app instance
 * @param port Port number to listen on
 * @param callback Optional callback when server starts
 * @return 0 on success, -1 on error
 */
int cexpress_listen(cexpress_app *app, int port, void (*callback)(void));

/**
 * Start listening on specified port with HTTPS/TLS
 * @param app Express app instance
 * @param port Port number to listen on
 * @param tls_config TLS configuration (cert, key, etc.)
 * @param callback Optional callback when server starts
 * @return 0 on success, -1 on error
 */
int cexpress_listen_https(cexpress_app *app, int port, const cexpress_tls_config *tls_config, void (*callback)(void));

/**
 * Register a GET route handler
 * @param app Express app instance
 * @param path Route path
 * @param handler Handler function
 */
void cexpress_get(cexpress_app *app, const char *path, cexpress_handler handler);

/**
 * Register a POST route handler
 * @param app Express app instance
 * @param path Route path
 * @param handler Handler function
 */
void cexpress_post(cexpress_app *app, const char *path, cexpress_handler handler);

/**
 * Register a PUT route handler
 * @param app Express app instance
 * @param path Route path
 * @param handler Handler function
 */
void cexpress_put(cexpress_app *app, const char *path, cexpress_handler handler);

/**
 * Register a DELETE route handler
 * @param app Express app instance
 * @param path Route path
 * @param handler Handler function
 */
void cexpress_delete(cexpress_app *app, const char *path, cexpress_handler handler);

/**
 * Register middleware
 * @param app Express app instance
 * @param middleware Middleware function
 */
void cexpress_use(cexpress_app *app, cexpress_middleware_fn middleware);

/**
 * Send response with status code and body
 * @param res Response object
 * @param status_code HTTP status code
 * @param body Response body
 */
void cexpress_send(cexpress_res *res, int status_code, const char *body);

/**
 * Send JSON response
 * @param res Response object
 * @param json JSON string
 */
void cexpress_json(cexpress_res *res, const char *json);

/**
 * Set response status code
 * @param res Response object
 * @param status_code HTTP status code
 * @return Response object for chaining
 */
cexpress_res* cexpress_status(cexpress_res *res, int status_code);

/**
 * Set response header
 * @param res Response object
 * @param key Header name
 * @param value Header value
 */
void cexpress_set_header(cexpress_res *res, const char *key, const char *value);

/**
 * Get request header
 * @param req Request object
 * @param key Header name
 * @return Header value or NULL if not found
 */
const char* cexpress_get_header(cexpress_req *req, const char *key);

/**
 * Get route parameter
 * @param req Request object
 * @param key Parameter name
 * @return Parameter value or NULL if not found
 */
const char* cexpress_get_param(cexpress_req *req, const char *key);

/**
 * Register a WebSocket route handler
 * @param app Express app instance
 * @param path Route path for WebSocket upgrade
 * @param on_connect Handler called when WebSocket connects
 * @param on_message Handler called when WebSocket receives message
 */
void cexpress_websocket(cexpress_app *app, const char *path, 
                        cexpress_ws_handler on_connect,
                        cexpress_ws_message_handler on_message);

/**
 * Send WebSocket message
 * @param ws WebSocket connection
 * @param message Message to send
 * @param len Message length
 * @param opcode Message type (text, binary, etc.)
 * @return 0 on success, -1 on error
 */
int cexpress_ws_send(cexpress_ws *ws, const void *message, size_t len, cexpress_ws_opcode opcode);

/**
 * Send WebSocket text message
 * @param ws WebSocket connection
 * @param message Null-terminated text message
 * @return 0 on success, -1 on error
 */
int cexpress_ws_send_text(cexpress_ws *ws, const char *message);

/**
 * Close WebSocket connection
 * @param ws WebSocket connection
 * @param code Close code
 * @param reason Close reason (optional)
 */
void cexpress_ws_close(cexpress_ws *ws, int code, const char *reason);

/**
 * Stop the server gracefully
 * @param app Express app instance
 */
void cexpress_stop(cexpress_app *app);

/**
 * Destroy express app and free resources
 * @param app Express app instance
 */
void cexpress_destroy(cexpress_app *app);

#ifdef __cplusplus
}
#endif

#endif /* CEXPRESS_H */
