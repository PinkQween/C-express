#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <ctype.h>

#define MAX_PATH 1024
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"

/* Validate path to prevent command injection */
int is_valid_path(const char *path) {
    if (!path || strlen(path) == 0) {
        return 0;
    }
    
    /* Check for dangerous characters */
    const char *dangerous = ";|&$`<>(){}[]'\"\\!\n\r";
    for (const char *p = path; *p; p++) {
        if (strchr(dangerous, *p)) {
            return 0;
        }
    }
    
    /* Path should start with / or . or alphanumeric */
    if (path[0] != '/' && path[0] != '.' && !isalnum(path[0])) {
        return 0;
    }
    
    return 1;
}

void print_header(void) {
    printf("\n");
    printf(COLOR_BLUE "╔══════════════════════════════════════╗\n");
    printf("║   C-Express Installation Script      ║\n");
    printf("╚══════════════════════════════════════╝\n" COLOR_RESET);
    printf("\n");
}

void print_step(const char *step) {
    printf(COLOR_BLUE "==>" COLOR_RESET " %s\n", step);
}

void print_success(const char *msg) {
    printf(COLOR_GREEN "✓" COLOR_RESET " %s\n", msg);
}

void print_error(const char *msg) {
    printf(COLOR_RED "✗ Error:" COLOR_RESET " %s\n", msg);
}

void print_warning(const char *msg) {
    printf(COLOR_YELLOW "⚠ Warning:" COLOR_RESET " %s\n", msg);
}

int run_command(const char *cmd) {
    printf("  Running: %s\n", cmd);
    int status = system(cmd);
    
    if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        if (exit_status != 0) {
            print_error("Command failed");
            return -1;
        }
    } else {
        print_error("Command terminated abnormally");
        return -1;
    }
    
    return 0;
}

int directory_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

int check_cmake(void) {
    print_step("Checking for CMake...");
    
    if (system("cmake --version > /dev/null 2>&1") != 0) {
        print_error("CMake not found. Please install CMake 3.10 or higher.");
        return -1;
    }
    
    print_success("CMake is installed");
    return 0;
}

int check_compiler(void) {
    print_step("Checking for C compiler...");
    
    int has_gcc = (system("gcc --version > /dev/null 2>&1") == 0);
    int has_clang = (system("clang --version > /dev/null 2>&1") == 0);
    
    if (!has_gcc && !has_clang) {
        print_error("No C compiler found. Please install GCC or Clang.");
        return -1;
    }
    
    if (has_gcc) {
        print_success("GCC compiler found");
    } else if (has_clang) {
        print_success("Clang compiler found");
    }
    
    return 0;
}

int build_library(const char *build_dir, const char *install_prefix) {
    print_step("Building C-Express library...");
    
    /* Create build directory */
    if (!directory_exists(build_dir)) {
        if (mkdir(build_dir, 0755) != 0) {
            print_error("Failed to create build directory");
            return -1;
        }
    }
    
    /* Save current directory */
    char cwd[MAX_PATH];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        print_error("Failed to get current directory");
        return -1;
    }
    
    /* Change to build directory */
    if (chdir(build_dir) != 0) {
        print_error("Failed to change to build directory");
        return -1;
    }
    
    /* Run cmake */
    char cmake_cmd[MAX_PATH * 2];
    if (install_prefix) {
        snprintf(cmake_cmd, sizeof(cmake_cmd), "cmake -DCMAKE_INSTALL_PREFIX=%s ..", install_prefix);
    } else {
        snprintf(cmake_cmd, sizeof(cmake_cmd), "cmake ..");
    }
    
    if (run_command(cmake_cmd) != 0) {
        chdir(cwd);
        return -1;
    }
    
    /* Run make */
    if (run_command("make -j$(nproc 2>/dev/null || echo 2)") != 0) {
        chdir(cwd);
        return -1;
    }
    
    print_success("Build completed successfully");
    
    /* Return to original directory */
    chdir(cwd);
    return 0;
}

int install_library(const char *build_dir, int use_sudo) {
    print_step("Installing C-Express...");
    
    /* Save current directory */
    char cwd[MAX_PATH];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        print_error("Failed to get current directory");
        return -1;
    }
    
    /* Change to build directory */
    if (chdir(build_dir) != 0) {
        print_error("Failed to change to build directory");
        chdir(cwd);
        return -1;
    }
    
    /* Run make install */
    const char *install_cmd = use_sudo ? "sudo make install" : "make install";
    if (run_command(install_cmd) != 0) {
        chdir(cwd);
        return -1;
    }
    
    print_success("Installation completed successfully");
    
    /* Return to original directory */
    chdir(cwd);
    return 0;
}

int run_tests(const char *build_dir) {
    print_step("Running tests...");
    
    /* Save current directory */
    char cwd[MAX_PATH];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        print_error("Failed to get current directory");
        return -1;
    }
    
    /* Change to build directory */
    if (chdir(build_dir) != 0) {
        print_error("Failed to change to build directory");
        chdir(cwd);
        return -1;
    }
    
    /* Run ctest */
    if (run_command("ctest --output-on-failure") != 0) {
        print_warning("Some tests failed, but installation can continue");
        chdir(cwd);
        return 0;  /* Don't fail installation on test failure */
    }
    
    print_success("All tests passed");
    
    /* Return to original directory */
    chdir(cwd);
    return 0;
}

void print_usage(const char *prog) {
    printf("Usage: %s [OPTIONS]\n\n", prog);
    printf("Options:\n");
    printf("  --prefix <path>    Installation prefix (default: /usr/local)\n");
    printf("  --no-sudo          Don't use sudo for installation\n");
    printf("  --skip-tests       Skip running tests\n");
    printf("  --build-dir <path> Build directory (default: ./build)\n");
    printf("  -h, --help         Show this help message\n");
    printf("\n");
}

int main(int argc, char *argv[]) {
    const char *install_prefix = NULL;
    const char *build_dir = "build";
    int use_sudo = 1;
    int run_tests_flag = 1;
    
    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--prefix") == 0 && i + 1 < argc) {
            install_prefix = argv[++i];
            if (!is_valid_path(install_prefix)) {
                fprintf(stderr, "Error: Invalid installation prefix path\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--no-sudo") == 0) {
            use_sudo = 0;
        } else if (strcmp(argv[i], "--skip-tests") == 0) {
            run_tests_flag = 0;
        } else if (strcmp(argv[i], "--build-dir") == 0 && i + 1 < argc) {
            build_dir = argv[++i];
            if (!is_valid_path(build_dir)) {
                fprintf(stderr, "Error: Invalid build directory path\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            printf("Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    print_header();
    
    /* Check prerequisites */
    if (check_cmake() != 0) {
        return 1;
    }
    
    if (check_compiler() != 0) {
        return 1;
    }
    
    printf("\n");
    
    /* Build library */
    if (build_library(build_dir, install_prefix) != 0) {
        return 1;
    }
    
    printf("\n");
    
    /* Run tests if requested */
    if (run_tests_flag) {
        run_tests(build_dir);
        printf("\n");
    }
    
    /* Install library */
    if (install_library(build_dir, use_sudo) != 0) {
        return 1;
    }
    
    printf("\n");
    printf(COLOR_GREEN "╔══════════════════════════════════════╗\n");
    printf("║  Installation completed successfully! ║\n");
    printf("╚══════════════════════════════════════╝\n" COLOR_RESET);
    printf("\n");
    printf("You can now:\n");
    printf("  - Create a new project: " COLOR_BLUE "c-express myapp" COLOR_RESET "\n");
    printf("  - Initialize in current dir: " COLOR_BLUE "c-express ." COLOR_RESET "\n");
    printf("  - Check the examples in: " COLOR_BLUE "%s/examples/" COLOR_RESET "\n", build_dir);
    printf("\n");
    
    return 0;
}
