#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "cexpress/cexpress.h"

/* Global app reference for signal handler */
cexpress_app *app = NULL;

/* Signal handler for graceful shutdown */
void handle_signal(int sig) {
    (void)sig;
    printf("\nShutting down server gracefully...\n");
    if (app) {
        cexpress_stop(app);
    }
}

/* Route handlers */
void handle_root(cexpress_req *req, cexpress_res *res) {
    (void)req;
    cexpress_send(res, 200, "Hello from C-Express HTTPS Server!");
}

void handle_secure_data(cexpress_req *req, cexpress_res *res) {
    (void)req;
    cexpress_json(res, "{\"message\": \"This data is encrypted!\", \"secure\": true}");
}

void server_started(void) {
    printf("HTTPS Server is running on https://localhost:8443\n");
    printf("Try these endpoints:\n");
    printf("  GET  https://localhost:8443/\n");
    printf("  GET  https://localhost:8443/secure\n");
    printf("\nNOTE: You need valid SSL certificate and key files:\n");
    printf("  - server.crt (certificate)\n");
    printf("  - server.key (private key)\n");
    printf("\nTo generate self-signed certificates for testing:\n");
    printf("  openssl req -x509 -newkey rsa:4096 -keyout server.key -out server.crt -days 365 -nodes\n");
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
    cexpress_get(app, "/secure", handle_secure_data);
    
    /* Configure TLS */
    cexpress_tls_config tls_config = {
        .cert_file = "server.crt",
        .key_file = "server.key",
        .ca_file = NULL,
        .verify_client = false
    };
    
    /* Start HTTPS server */
    printf("Starting HTTPS server...\n");
    int result = cexpress_listen_https(app, 8443, &tls_config, server_started);
    
    /* Cleanup */
    cexpress_destroy(app);
    
    return result;
}
