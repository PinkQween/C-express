/* Test framework for C-Express */
#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Test statistics */
typedef struct {
    int total;
    int passed;
    int failed;
} test_stats_t;

/* Global test statistics */
static test_stats_t global_stats = {0, 0, 0};

/* Color codes for output */
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"

/* Test macros */
#define TEST_SUITE(name) \
    void test_suite_##name(void); \
    void test_suite_##name(void) { \
        printf("\n" COLOR_BLUE "=== Test Suite: %s ===" COLOR_RESET "\n", #name);

#define END_TEST_SUITE() \
    }

#define TEST(name) \
    do { \
        printf("  Testing: %s ... ", #name); \
        fflush(stdout); \
        int test_passed = 1;

#define END_TEST() \
        if (test_passed) { \
            printf(COLOR_GREEN "PASS" COLOR_RESET "\n"); \
            global_stats.passed++; \
        } else { \
            printf(COLOR_RED "FAIL" COLOR_RESET "\n"); \
            global_stats.failed++; \
        } \
        global_stats.total++; \
    } while (0);

#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            printf("\n    " COLOR_RED "Assertion failed:" COLOR_RESET " %s\n", #condition); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            test_passed = 0; \
        } \
    } while (0)

#define ASSERT_EQUAL(a, b) \
    do { \
        if ((a) != (b)) { \
            printf("\n    " COLOR_RED "Assertion failed:" COLOR_RESET " %s == %s\n", #a, #b); \
            printf("    Expected: %d, Got: %d\n", (int)(b), (int)(a)); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            test_passed = 0; \
        } \
    } while (0)

#define ASSERT_NOT_EQUAL(a, b) \
    do { \
        if ((a) == (b)) { \
            printf("\n    " COLOR_RED "Assertion failed:" COLOR_RESET " %s != %s\n", #a, #b); \
            printf("    Both values: %d\n", (int)(a)); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            test_passed = 0; \
        } \
    } while (0)

#define ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            printf("\n    " COLOR_RED "Assertion failed:" COLOR_RESET " %s should be NULL\n", #ptr); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            test_passed = 0; \
        } \
    } while (0)

#define ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            printf("\n    " COLOR_RED "Assertion failed:" COLOR_RESET " %s should not be NULL\n", #ptr); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            test_passed = 0; \
        } \
    } while (0)

#define ASSERT_STR_EQUAL(a, b) \
    do { \
        if ((a) == NULL || (b) == NULL || strcmp((a), (b)) != 0) { \
            printf("\n    " COLOR_RED "Assertion failed:" COLOR_RESET " %s == %s\n", #a, #b); \
            printf("    Expected: \"%s\"\n", (b) ? (b) : "NULL"); \
            printf("    Got:      \"%s\"\n", (a) ? (a) : "NULL"); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            test_passed = 0; \
        } \
    } while (0)

#define ASSERT_STR_CONTAINS(str, substr) \
    do { \
        if ((str) == NULL || (substr) == NULL || strstr((str), (substr)) == NULL) { \
            printf("\n    " COLOR_RED "Assertion failed:" COLOR_RESET " %s should contain %s\n", #str, #substr); \
            printf("    String:    \"%s\"\n", (str) ? (str) : "NULL"); \
            printf("    Substring: \"%s\"\n", (substr) ? (substr) : "NULL"); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            test_passed = 0; \
        } \
    } while (0)

/* Run test suite */
#define RUN_TEST_SUITE(name) \
    test_suite_##name();

/* Print test summary */
static void print_test_summary(void) {
    printf("\n" COLOR_BLUE "=== Test Summary ===" COLOR_RESET "\n");
    printf("Total:  %d\n", global_stats.total);
    printf(COLOR_GREEN "Passed: %d" COLOR_RESET "\n", global_stats.passed);
    
    if (global_stats.failed > 0) {
        printf(COLOR_RED "Failed: %d" COLOR_RESET "\n", global_stats.failed);
    } else {
        printf("Failed: %d\n", global_stats.failed);
    }
    
    if (global_stats.failed == 0 && global_stats.total > 0) {
        printf("\n" COLOR_GREEN "All tests passed!" COLOR_RESET "\n\n");
    } else if (global_stats.failed > 0) {
        printf("\n" COLOR_RED "Some tests failed!" COLOR_RESET "\n\n");
    }
}

/* Get test result (for exit code) */
static int get_test_result(void) {
    return (global_stats.failed > 0) ? 1 : 0;
}

#endif /* TEST_FRAMEWORK_H */
