#ifndef COMPILER_H
#define COMPILER_H

#include <stdbool.h>

struct compiler_opt {
    bool debug;
    int max_errors; // default 99
};

void compile_dir_recursive(char *src_dir, char* src_name, struct compiler_opt options);
void compile(char* src_dir, char* src_name, struct compiler_opt options);

#endif
