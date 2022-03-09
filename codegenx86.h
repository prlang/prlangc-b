#ifndef CODEGENX86_H
#define CODEGENX86_H

#include <stdio.h>

#include "sema_analysis.h"
#include "token.h"

struct prlang_codegen {
    struct semantic_context *ctx;
    bool import;
    int depth;
    int act_label;
    bool if_cond;
    bool loop_cond;
    FILE *file;
};

struct prlang_codegen init_codegen(char *dir, char *filename, struct semantic_context *ctx);
void free_codegen(struct prlang_codegen gen);

void prlang_codegen(struct prlang_file *node, struct prlang_codegen *gen);

#endif