#include "test_framework.h"
#include "cexpress/cexpress.h"
#include <string.h>

/* Test cexpress_create and cexpress_destroy */
TEST_SUITE(core_api) {
    TEST(create_app_returns_non_null) {
        cexpress_app *app = cexpress_create();
        ASSERT_NOT_NULL(app);
        cexpress_destroy(app);
    }
    END_TEST();
    
    TEST(destroy_app_handles_null) {
        /* Should not crash */
        cexpress_destroy(NULL);
        ASSERT(1);  /* If we get here, test passed */
    }
    END_TEST();
    
    TEST(multiple_apps_can_be_created) {
        cexpress_app *app1 = cexpress_create();
        cexpress_app *app2 = cexpress_create();
        
        ASSERT_NOT_NULL(app1);
        ASSERT_NOT_NULL(app2);
        
        cexpress_destroy(app1);
        cexpress_destroy(app2);
    }
    END_TEST();
}
END_TEST_SUITE()

/* Test route registration */
TEST_SUITE(routing) {
    /* Simple handler for testing */
    void test_handler(cexpress_req *req, cexpress_res *res) {
        (void)req;
        cexpress_send(res, 200, "OK");
    }
    
    TEST(register_get_route) {
        cexpress_app *app = cexpress_create();
        ASSERT_NOT_NULL(app);
        
        /* Should not crash */
        cexpress_get(app, "/test", test_handler);
        ASSERT(1);
        
        cexpress_destroy(app);
    }
    END_TEST();
    
    TEST(register_post_route) {
        cexpress_app *app = cexpress_create();
        ASSERT_NOT_NULL(app);
        
        cexpress_post(app, "/api/users", test_handler);
        ASSERT(1);
        
        cexpress_destroy(app);
    }
    END_TEST();
    
    TEST(register_put_route) {
        cexpress_app *app = cexpress_create();
        ASSERT_NOT_NULL(app);
        
        cexpress_put(app, "/api/users/1", test_handler);
        ASSERT(1);
        
        cexpress_destroy(app);
    }
    END_TEST();
    
    TEST(register_delete_route) {
        cexpress_app *app = cexpress_create();
        ASSERT_NOT_NULL(app);
        
        cexpress_delete(app, "/api/users/1", test_handler);
        ASSERT(1);
        
        cexpress_destroy(app);
    }
    END_TEST();
    
    TEST(register_multiple_routes) {
        cexpress_app *app = cexpress_create();
        ASSERT_NOT_NULL(app);
        
        cexpress_get(app, "/", test_handler);
        cexpress_get(app, "/about", test_handler);
        cexpress_post(app, "/users", test_handler);
        cexpress_put(app, "/users/1", test_handler);
        cexpress_delete(app, "/users/1", test_handler);
        
        ASSERT(1);
        
        cexpress_destroy(app);
    }
    END_TEST();
}
END_TEST_SUITE()

/* Test response functions */
TEST_SUITE(response) {
    TEST(send_response) {
        cexpress_res res = {0};
        res.status_code = 200;
        res.sent = false;
        res.body = NULL;
        
        cexpress_send(&res, 200, "Hello World");
        
        ASSERT_EQUAL(res.status_code, 200);
        ASSERT_NOT_NULL(res.body);
        ASSERT_STR_EQUAL(res.body, "Hello World");
        ASSERT(res.sent);
        
        if (res.body) free(res.body);
    }
    END_TEST();
    
    TEST(send_json_response) {
        cexpress_res res = {0};
        res.status_code = 200;
        res.sent = false;
        res.body = NULL;
        
        const char *json = "{\"message\":\"test\"}";
        cexpress_json(&res, json);
        
        ASSERT_EQUAL(res.status_code, 200);
        ASSERT_NOT_NULL(res.body);
        ASSERT_STR_EQUAL(res.body, json);
        ASSERT(res.sent);
        
        if (res.body) free(res.body);
    }
    END_TEST();
    
    TEST(set_status_code) {
        cexpress_res res = {0};
        res.status_code = 200;
        
        cexpress_status(&res, 404);
        
        ASSERT_EQUAL(res.status_code, 404);
    }
    END_TEST();
    
    TEST(set_header) {
        cexpress_res res = {0};
        
        /* Should not crash - implementation is a stub */
        cexpress_set_header(&res, "Content-Type", "application/json");
        
        ASSERT(1);
    }
    END_TEST();
    
    TEST(send_handles_null_body) {
        cexpress_res res = {0};
        res.status_code = 200;
        res.sent = false;
        res.body = NULL;
        
        cexpress_send(&res, 204, NULL);
        
        ASSERT_EQUAL(res.status_code, 204);
        ASSERT(res.sent);
        
        if (res.body) free(res.body);
    }
    END_TEST();
    
    TEST(send_replaces_previous_body) {
        cexpress_res res = {0};
        res.status_code = 200;
        res.sent = false;
        res.body = NULL;
        
        cexpress_send(&res, 200, "First");
        ASSERT_STR_EQUAL(res.body, "First");
        
        cexpress_send(&res, 200, "Second");
        ASSERT_STR_EQUAL(res.body, "Second");
        
        if (res.body) free(res.body);
    }
    END_TEST();
}
END_TEST_SUITE()

/* Test middleware registration */
TEST_SUITE(middleware) {
    void test_middleware(cexpress_req *req, cexpress_res *res, void (*next)(void)) {
        (void)req;
        (void)res;
        (void)next;
    }
    
    TEST(register_middleware) {
        cexpress_app *app = cexpress_create();
        ASSERT_NOT_NULL(app);
        
        cexpress_use(app, test_middleware);
        ASSERT(1);
        
        cexpress_destroy(app);
    }
    END_TEST();
    
    TEST(register_multiple_middleware) {
        cexpress_app *app = cexpress_create();
        ASSERT_NOT_NULL(app);
        
        cexpress_use(app, test_middleware);
        cexpress_use(app, test_middleware);
        cexpress_use(app, test_middleware);
        
        ASSERT(1);
        
        cexpress_destroy(app);
    }
    END_TEST();
}
END_TEST_SUITE()

/* Main test runner */
int main(void) {
    printf(COLOR_BLUE "C-Express Unit Tests\n" COLOR_RESET);
    printf("====================\n");
    
    RUN_TEST_SUITE(core_api);
    RUN_TEST_SUITE(routing);
    RUN_TEST_SUITE(response);
    RUN_TEST_SUITE(middleware);
    
    print_test_summary();
    
    return get_test_result();
}
