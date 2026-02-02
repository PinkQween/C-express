#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
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

/* HTTP route handlers */
void handle_root(cexpress_req *req, cexpress_res *res) {
    (void)req;
    const char *html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head><title>WebSocket Test</title></head>"
        "<body>"
        "<h1>WebSocket Test Page</h1>"
        "<div id='status'>Connecting...</div>"
        "<input type='text' id='message' placeholder='Type a message'>"
        "<button onclick='sendMessage()'>Send</button>"
        "<div id='messages'></div>"
        "<script>"
        "const ws = new WebSocket('ws://localhost:3000/ws');"
        "ws.onopen = () => { document.getElementById('status').textContent = 'Connected!'; };"
        "ws.onmessage = (e) => {"
        "  const div = document.createElement('div');"
        "  div.textContent = 'Server: ' + e.data;"
        "  document.getElementById('messages').appendChild(div);"
        "};"
        "ws.onclose = () => { document.getElementById('status').textContent = 'Disconnected'; };"
        "function sendMessage() {"
        "  const msg = document.getElementById('message').value;"
        "  ws.send(msg);"
        "  const div = document.createElement('div');"
        "  div.textContent = 'You: ' + msg;"
        "  document.getElementById('messages').appendChild(div);"
        "  document.getElementById('message').value = '';"
        "}"
        "</script>"
        "</body>"
        "</html>";
    cexpress_send(res, 200, html);
}

/* WebSocket handlers */
void ws_on_connect(cexpress_ws *ws) {
    printf("WebSocket client connected!\n");
    cexpress_ws_send_text(ws, "Welcome to C-Express WebSocket server!");
}

void ws_on_message(cexpress_ws *ws, const char *message, size_t len, cexpress_ws_opcode opcode) {
    printf("Received message (len=%zu, opcode=%d): %s\n", len, opcode, message);
    
    if (opcode == CEXPRESS_WS_TEXT) {
        /* Echo the message back */
        char response[256];
        snprintf(response, sizeof(response), "Echo: %s", message);
        cexpress_ws_send_text(ws, response);
        
        /* If message is "close", close the connection */
        if (strcmp(message, "close") == 0) {
            cexpress_ws_close(ws, 1000, "Client requested close");
        }
    }
}

void server_started(void) {
    printf("WebSocket Server is running on http://localhost:3000\n");
    printf("Try these endpoints:\n");
    printf("  HTTP: http://localhost:3000/ (test page)\n");
    printf("  WS:   ws://localhost:3000/ws (WebSocket endpoint)\n");
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
    
    /* Register HTTP routes */
    cexpress_get(app, "/", handle_root);
    
    /* Register WebSocket route */
    cexpress_websocket(app, "/ws", ws_on_connect, ws_on_message);
    
    /* Start server */
    int result = cexpress_listen(app, 3000, server_started);
    
    /* Cleanup */
    cexpress_destroy(app);
    
    return result;
}
