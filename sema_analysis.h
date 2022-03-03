#ifndef SEMA_ANALYSIS_H
#define SEMA_ANALYSIS_H

#include "error.h"
#include "symbol_table.h"
#include "ast.h"

#define POINTER_OFFSET 8
#define DOUBLE_OFFSET 8
#define INT_OFFSET 4
#define CHAR_OFFSET 1

// global context
struct semantic_context {
    struct symbol_table *table;
    struct errors *err;
    // used to check break and continue
    bool loop;
    // used to check actual type being processed
    char *type;
    char *actual_type;
    // used to check the code inside methods
    struct class_table *actual;
    // the next two values are used for checking class member access
    // example: value.getFoo().getBar(); or value.foo.bar
    bool accessing;
    char *access_type;
    struct call_expr *call;

    // check code after return statement
    int return_depth;

    struct fun_table *actual_fun;
    char *return_type;
    // for assembly generation
    int actual_offset;
    int actual_depth;
};

struct semantic_context init_semantic(struct symbol_table *table, struct errors *err);
void free_semantic(struct semantic_context *ctx);

void prlang_semapass(struct prlang_file *node, struct semantic_context *ctx);

#endif