#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stddef.h>

#include "compiler.h"

#ifdef _WIN32
    #include <direct.h>
    #define GetCurrentDir _getcwd
#elif defined __unix__
    #include <unistd.h>
    #define GetCurrentDir getcwd
#elif defined __APPLE__
#endif

#define DIRECTORY_SIZE 4096

enum command_arg {
    COMPILE,
    UNDEFINED
};

enum parse_details {
    DIR,
    ARGS,
    SRC
};

struct command_opt {
    enum command_arg arg;
    int index_src;
    int index_dir; // optional
    bool debug;
};

struct command_opt parse(int argc, char *argv[]) {
    if (argc < 2) {
        printf("[ERROR] Can't parse arguments\n");
        exit(1);
    }

    struct command_opt options = { UNDEFINED, 0, 0, false };
    enum parse_details details = SRC;

    for (size_t i = 1 ; i < argc ; i++) {
        char *arg = argv[i];
        if (strcmp(arg, "-c") == 0) {
            options.arg = COMPILE;
            details = SRC;
        }
        else if (strcmp(arg, "-d") == 0) {
            options.debug = true;
        }
        else if (strcmp(arg, "-dir") == 0) {
            details = DIR;
        }
        else if (strcmp(arg, "--help") == 0) {
            // help
            exit(0);
        }
        else if (strcmp(arg, "--version") == 0) {
            // version
            exit(0);
        }
        else {
            if (options.arg == UNDEFINED) {
                options.arg = COMPILE;
            } 
            if (details == SRC) {
                options.index_src = i;
                // simple command: plang hello.p <args>
                details = ARGS;
            }
            else if (details == ARGS) {

            }
            else if (details == DIR) {
                options.index_dir = i;
            }
        }
    }

    if (options.arg == UNDEFINED) {
        printf("[ERROR] Can't parse arguments\n");
        exit(1);
    }
    return options;
}

int main(int argc, char *argv[]) {
    struct command_opt cmd = parse(argc, argv);

    char directory[DIRECTORY_SIZE];

    // is directory specified in command options
    if (cmd.index_dir != 0) {
        strcpy(directory, argv[cmd.index_dir]);
    }
    else if (!getcwd(directory, DIRECTORY_SIZE)) {
        printf("[ERROR] Invalid current directory\n");
        exit(1);
    }

    struct compiler_opt opt = { 
        cmd.debug, 99 
    };

    if (cmd.arg == COMPILE) {
        compile(directory, argv[cmd.index_src], opt);
    }

    return 0;
}