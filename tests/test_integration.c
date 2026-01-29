#include "test_framework.h"
#include "cexpress/cexpress.h"
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define TEST_PORT 8888
#define BUFFER_SIZE 4096

/* Global app for integration tests */
static cexpress_app *test_app = NULL;
static pthread_t server_thread;
static volatile int server_ready = 0;

/* Test handlers */
void handle_test_root(cexpress_req *req, cexpress_res *res) {
    (void)req;
    cexpress_send(res, 200, "Test Root");
}

void handle_test_json(cexpress_req *req, cexpress_res *res) {
    (void)req;
    cexpress_json(res, "{\"test\":\"json\"}");
}

void handle_test_404(cexpress_req *req, cexpress_res *res) {
    (void)req;
    cexpress_send(res, 404, "Not Found");
}

void handle_test_post(cexpress_req *req, cexpress_res *res) {
    (void)req;
    cexpress_status(res, 201);
    cexpress_json(res, "{\"created\":true}");
}

/* Server thread function */
void* run_test_server(void *arg) {
    (void)arg;
    server_ready = 1;
    cexpress_listen(test_app, TEST_PORT, NULL);
    return NULL;
}

/* HTTP client helper */
char* send_http_request(const char *method, const char *path, const char *body) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return NULL;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TEST_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sock);
        return NULL;
    }
    
    /* Build HTTP request */
    char request[BUFFER_SIZE];
    if (body) {
        snprintf(request, sizeof(request),
            "%s %s HTTP/1.1\r\n"
            "Host: localhost:%d\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            method, path, TEST_PORT, strlen(body), body);
    } else {
        snprintf(request, sizeof(request),
            "%s %s HTTP/1.1\r\n"
            "Host: localhost:%d\r\n"
            "Connection: close\r\n"
            "\r\n",
            method, path, TEST_PORT);
    }
    
    /* Send request */
    if (send(sock, request, strlen(request), 0) < 0) {
        close(sock);
        return NULL;
    }
    
    /* Receive response */
    char *response = (char*)malloc(BUFFER_SIZE);
    if (!response) {
        close(sock);
        return NULL;
    }
    
    ssize_t total = 0;
    ssize_t n;
    while ((n = recv(sock, response + total, BUFFER_SIZE - total - 1, 0)) > 0) {
        total += n;
        if (total >= BUFFER_SIZE - 1) {
            break;
        }
    }
    response[total] = '\0';
    
    close(sock);
    return response;
}

/* Setup and teardown */
void setup_integration_tests(void) {
    test_app = cexpress_create();
    
    /* Register test routes */
    cexpress_get(test_app, "/", handle_test_root);
    cexpress_get(test_app, "/json", handle_test_json);
    cexpress_get(test_app, "/404", handle_test_404);
    cexpress_post(test_app, "/create", handle_test_post);
    
    /* Start server in background thread */
    server_ready = 0;
    pthread_create(&server_thread, NULL, run_test_server, NULL);
    
    /* Wait for server to be ready */
    while (!server_ready) {
        usleep(10000);  /* 10ms */
    }
    
    /* Give server time to start listening */
    usleep(100000);  /* 100ms */
}

void teardown_integration_tests(void) {
    if (test_app) {
        cexpress_destroy(test_app);
        test_app = NULL;
    }
}

/* Integration tests */
TEST_SUITE(http_integration) {
    TEST(get_root_endpoint) {
        char *response = send_http_request("GET", "/", NULL);
        ASSERT_NOT_NULL(response);
        
        ASSERT_STR_CONTAINS(response, "200 OK");
        ASSERT_STR_CONTAINS(response, "Test Root");
        
        free(response);
    }
    END_TEST();
    
    TEST(get_json_endpoint) {
        char *response = send_http_request("GET", "/json", NULL);
        ASSERT_NOT_NULL(response);
        
        ASSERT_STR_CONTAINS(response, "200 OK");
        ASSERT_STR_CONTAINS(response, "{\"test\":\"json\"}");
        
        free(response);
    }
    END_TEST();
    
    TEST(get_404_endpoint) {
        char *response = send_http_request("GET", "/404", NULL);
        ASSERT_NOT_NULL(response);
        
        ASSERT_STR_CONTAINS(response, "404");
        ASSERT_STR_CONTAINS(response, "Not Found");
        
        free(response);
    }
    END_TEST();
    
    TEST(get_nonexistent_endpoint) {
        char *response = send_http_request("GET", "/nonexistent", NULL);
        ASSERT_NOT_NULL(response);
        
        ASSERT_STR_CONTAINS(response, "404");
        
        free(response);
    }
    END_TEST();
    
    TEST(post_create_endpoint) {
        char *response = send_http_request("POST", "/create", "test data");
        ASSERT_NOT_NULL(response);
        
        ASSERT_STR_CONTAINS(response, "201");
        ASSERT_STR_CONTAINS(response, "{\"created\":true}");
        
        free(response);
    }
    END_TEST();
}
END_TEST_SUITE()

/* Main test runner */
int main(void) {
    printf(COLOR_BLUE "C-Express Integration Tests\n" COLOR_RESET);
    printf("============================\n");
    
    printf("\nSetting up test server...\n");
    setup_integration_tests();
    
    RUN_TEST_SUITE(http_integration);
    
    printf("\nTearing down test server...\n");
    teardown_integration_tests();
    
    print_test_summary();
    
    return get_test_result();
}
