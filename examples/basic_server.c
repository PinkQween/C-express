#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "cexpress/cexpress.h"

/* Global app reference for signal handler */
cexpress_app *app = NULL;
volatile sig_atomic_t should_exit = 0;

/* Signal handler for graceful shutdown */
void handle_signal(int sig) {
    (void)sig;
    should_exit = 1;
}

/* Route handlers */
void handle_root(cexpress_req *req, cexpress_res *res) {
    (void)req;
    cexpress_send(res, 200, "Hello from C-Express!");
}

void handle_json(cexpress_req *req, cexpress_res *res) {
    (void)req;
    cexpress_json(res, "{\"message\": \"Hello, JSON!\", \"status\": \"success\"}");
}

void handle_user(cexpress_req *req, cexpress_res *res) {
    (void)req;
    cexpress_send(res, 200, "User endpoint");
}

void handle_create_user(cexpress_req *req, cexpress_res *res) {
    (void)req;
    cexpress_status(res, 201);
    cexpress_json(res, "{\"message\": \"User created\", \"id\": 123}");
}

void handle_about(cexpress_req *req, cexpress_res *res) {
    (void)req;
    const char *html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head><title>About C-Express</title></head>"
        "<body>"
        "<h1>About C-Express</h1>"
        "<p>A lightweight Express.js-like framework for C</p>"
        "<ul>"
        "<li><a href='/'>Home</a></li>"
        "<li><a href='/api/json'>JSON API</a></li>"
        "<li><a href='/users'>Users</a></li>"
        "</ul>"
        "</body>"
        "</html>";
    cexpress_send(res, 200, html);
}

void server_started(void) {
    printf("Server is running on http://localhost:3000\n");
    printf("Try these endpoints:\n");
    printf("  GET  http://localhost:3000/\n");
    printf("  GET  http://localhost:3000/about\n");
    printf("  GET  http://localhost:3000/api/json\n");
    printf("  GET  http://localhost:3000/users\n");
    printf("  POST http://localhost:3000/users\n");
    printf("\nPress Ctrl+C to stop the server\n");
}

int main(void) {
    /* Setup signal handler */
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    /* Create express app */
    app = cexpress_create();
    if (!app) {
        fprintf(stderr, "Failed to create express app\n");
        return 1;
    }
    
    /* Register routes */
    cexpress_get(app, "/", handle_root);
    cexpress_get(app, "/about", handle_about);
    cexpress_get(app, "/api/json", handle_json);
    cexpress_get(app, "/users", handle_user);
    cexpress_post(app, "/users", handle_create_user);
    
    /* Start server */
    int result = cexpress_listen(app, 3000, server_started);
    
    /* Cleanup */
    cexpress_destroy(app);
    
    return result;
}
