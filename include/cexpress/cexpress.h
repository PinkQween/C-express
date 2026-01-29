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

/* Request structure */
struct cexpress_req {
    cexpress_method method;
    char *path;
    char *query;
    char *body;
    void *params;      /* Route parameters */
    void *headers;     /* Request headers */
    void *user_data;   /* User-defined data */
};

/* Response structure */
struct cexpress_res {
    int status_code;
    char *body;
    void *headers;     /* Response headers */
    bool sent;         /* Whether response has been sent */
};

/* Handler function types */
typedef void (*cexpress_handler)(cexpress_req *req, cexpress_res *res);
typedef void (*cexpress_middleware_fn)(cexpress_req *req, cexpress_res *res, void (*next)(void));

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
 * Destroy express app and free resources
 * @param app Express app instance
 */
void cexpress_destroy(cexpress_app *app);

#ifdef __cplusplus
}
#endif

#endif /* CEXPRESS_H */
