#include "cexpress/cexpress.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_ROUTES 100
#define MAX_MIDDLEWARE 50
#define BUFFER_SIZE 4096

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

/* Application structure */
struct cexpress_app {
    cexpress_route routes[MAX_ROUTES];
    int route_count;
    cexpress_middleware middlewares[MAX_MIDDLEWARE];
    int middleware_count;
    int server_fd;
    bool running;
};

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
    app->server_fd = -1;
    app->running = false;
    
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
    cexpress_send(res, 200, json);
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
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read <= 0) {
        close(client_fd);
        return;
    }
    
    buffer[bytes_read] = '\0';
    
    /* Create request and response objects */
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
        write(client_fd, response, strlen(response));
        free(response);
    }
    
    /* Cleanup */
    if (req.path) free(req.path);
    if (req.query) free(req.query);
    if (req.body) free(req.body);
    if (res.body) free(res.body);
    
    close(client_fd);
}

/* Server thread function */
static void* server_thread(void *arg) {
    cexpress_app *app = (cexpress_app *)arg;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    while (app->running) {
        int client_fd = accept(app->server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
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

/* Destroy express app and free resources */
void cexpress_destroy(cexpress_app *app) {
    if (!app) return;
    
    app->running = false;
    
    if (app->server_fd >= 0) {
        close(app->server_fd);
    }
    
    /* Free routes */
    for (int i = 0; i < app->route_count; i++) {
        if (app->routes[i].path) {
            free(app->routes[i].path);
        }
    }
    
    free(app);
}
