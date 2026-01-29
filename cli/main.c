#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

#define MAX_PATH 1024

/* Template for main.c */
const char *main_template = 
"#include <stdio.h>\n"
"#include <signal.h>\n"
"#include \"cexpress/cexpress.h\"\n"
"\n"
"cexpress_app *app = NULL;\n"
"volatile sig_atomic_t should_exit = 0;\n"
"\n"
"void handle_signal(int sig) {\n"
"    (void)sig;\n"
"    should_exit = 1;\n"
"}\n"
"\n"
"void handle_root(cexpress_req *req, cexpress_res *res) {\n"
"    (void)req;\n"
"    cexpress_send(res, 200, \"Hello from C-Express!\");\n"
"}\n"
"\n"
"void server_started(void) {\n"
"    printf(\"Server running on http://localhost:3000\\n\");\n"
"    printf(\"Press Ctrl+C to stop\\n\");\n"
"}\n"
"\n"
"int main(void) {\n"
"    signal(SIGINT, handle_signal);\n"
"    signal(SIGTERM, handle_signal);\n"
"    \n"
"    app = cexpress_create();\n"
"    if (!app) {\n"
"        fprintf(stderr, \"Failed to create app\\n\");\n"
"        return 1;\n"
"    }\n"
"    \n"
"    cexpress_get(app, \"/\", handle_root);\n"
"    \n"
"    int result = cexpress_listen(app, 3000, server_started);\n"
"    cexpress_destroy(app);\n"
"    \n"
"    return result;\n"
"}\n";

/* Template for CMakeLists.txt */
const char *cmake_template = 
"cmake_minimum_required(VERSION 3.10)\n"
"project(%s VERSION 1.0.0 LANGUAGES C)\n"
"\n"
"set(CMAKE_C_STANDARD 99)\n"
"set(CMAKE_C_STANDARD_REQUIRED ON)\n"
"\n"
"# Find CExpress package\n"
"find_package(CExpress REQUIRED)\n"
"\n"
"# Add executable\n"
"add_executable(%s main.c)\n"
"\n"
"# Link CExpress library\n"
"target_link_libraries(%s PRIVATE CExpress::cexpress)\n"
"\n"
"# Compiler warnings\n"
"if(CMAKE_C_COMPILER_ID MATCHES \"GNU|Clang\")\n"
"    target_compile_options(%s PRIVATE\n"
"        -Wall -Wextra -Wpedantic\n"
"    )\n"
"endif()\n";

/* Template for README.md */
const char *readme_template = 
"# %s\n"
"\n"
"A C-Express application.\n"
"\n"
"## Building\n"
"\n"
"```bash\n"
"mkdir build && cd build\n"
"cmake ..\n"
"make\n"
"```\n"
"\n"
"## Running\n"
"\n"
"```bash\n"
"./build/%s\n"
"```\n"
"\n"
"Then visit http://localhost:3000\n";

/* Template for .gitignore */
const char *gitignore_template = 
"build/\n"
"*.o\n"
"*.a\n"
"*.so\n"
"*.exe\n"
".vscode/\n"
".idea/\n";

void print_usage(const char *prog_name) {
    printf("C-Express Project Generator\n\n");
    printf("Usage:\n");
    printf("  %s .                    Initialize C-Express project in current directory\n", prog_name);
    printf("  %s <project-name>       Create new C-Express project in <project-name>/\n", prog_name);
    printf("\n");
    printf("Options:\n");
    printf("  -h, --help              Show this help message\n");
    printf("  -v, --version           Show version information\n");
}

void print_version(void) {
    printf("c-express 1.0.0\n");
}

int create_directory(const char *path) {
    struct stat st = {0};
    
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0755) != 0) {
            fprintf(stderr, "Error: Failed to create directory '%s': %s\n", 
                    path, strerror(errno));
            return -1;
        }
    }
    
    return 0;
}

int write_file(const char *path, const char *content) {
    FILE *file = fopen(path, "w");
    if (!file) {
        fprintf(stderr, "Error: Failed to create file '%s': %s\n", 
                path, strerror(errno));
        return -1;
    }
    
    fprintf(file, "%s", content);
    fclose(file);
    
    printf("  Created: %s\n", path);
    return 0;
}

int create_project(const char *project_name, int in_current_dir) {
    char cmake_content[4096];
    char readme_content[2048];
    char saved_cwd[MAX_PATH];
    
    /* Save current directory for restoration */
    if (getcwd(saved_cwd, sizeof(saved_cwd)) == NULL) {
        fprintf(stderr, "Error: Failed to get current directory\n");
        return -1;
    }
    
    /* Normalize project name */
    const char *normalized_name = project_name;
    if (strcmp(project_name, ".") == 0) {
        /* Extract directory name (POSIX-only: uses '/' as path separator) */
        char *dir_name = strrchr(saved_cwd, '/');
        if (dir_name) {
            normalized_name = dir_name + 1;
        } else {
            normalized_name = "myapp";
        }
    }
    
    printf("Creating C-Express project: %s\n\n", normalized_name);
    
    /* Create project directory if not in current directory */
    if (!in_current_dir) {
        if (create_directory(project_name) != 0) {
            return -1;
        }
        if (chdir(project_name) != 0) {
            fprintf(stderr, "Error: Failed to change to directory '%s': %s\n", 
                    project_name, strerror(errno));
            return -1;
        }
    }
    
    /* Create main.c */
    if (write_file("main.c", main_template) != 0) {
        chdir(saved_cwd);
        return -1;
    }
    
    /* Create CMakeLists.txt */
    snprintf(cmake_content, sizeof(cmake_content), cmake_template, 
             normalized_name, normalized_name, normalized_name, normalized_name);
    if (write_file("CMakeLists.txt", cmake_content) != 0) {
        chdir(saved_cwd);
        return -1;
    }
    
    /* Create README.md */
    snprintf(readme_content, sizeof(readme_content), readme_template, 
             normalized_name, normalized_name);
    if (write_file("README.md", readme_content) != 0) {
        chdir(saved_cwd);
        return -1;
    }
    
    /* Create .gitignore */
    if (write_file(".gitignore", gitignore_template) != 0) {
        chdir(saved_cwd);
        return -1;
    }
    
    /* Restore original directory */
    chdir(saved_cwd);
    
    printf("\n✓ Project created successfully!\n\n");
    printf("Next steps:\n");
    
    if (!in_current_dir) {
        printf("  cd %s\n", project_name);
    }
    
    printf("  mkdir build && cd build\n");
    printf("  cmake ..\n");
    printf("  make\n");
    printf("  ./%s\n", normalized_name);
    
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    /* Parse arguments */
    const char *project_name = argv[1];
    
    if (strcmp(project_name, "-h") == 0 || strcmp(project_name, "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }
    
    if (strcmp(project_name, "-v") == 0 || strcmp(project_name, "--version") == 0) {
        print_version();
        return 0;
    }
    
    /* Create project */
    int in_current_dir = (strcmp(project_name, ".") == 0);
    
    if (create_project(project_name, in_current_dir) != 0) {
        return 1;
    }
    
    return 0;
}
