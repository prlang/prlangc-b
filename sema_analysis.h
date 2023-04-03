#ifndef SEMA_ANALYSIS_H
#define SEMA_ANALYSIS_H

#include "error.h"
#include "symbol_table.h"
#include "ast.h"

#define POINTER_OFFSET 8
#define DOUBLE_OFFSET 8
#define INT_OFFSET 4
#define CHAR_OFFSET 1

struct semantic_type {
    int id;
    int offset;
};

// TODO: check main function
// global context
struct semantic_context {
    // used for runtime type checking generation <string, semantic_type>
    struct symbol_hash types;
    int type_id;
    struct symbol_table *table;
    struct errors *err;
    // used to check break and continue
    bool loop;
    // used to check actual type being processed
    char *type;
    char *actual_type;
    int array_dim;
    // the next two values are used for checking class member access
    // example: value.getFoo().getBar(); or value.foo.bar
    bool accessing;
    char *access_type;
    struct call_expr *call;

    // check code after return statement
    int return_depth;

    struct fun_table *actual_fun;
    char *return_type;
    int return_dim;
    // for assembly generation
    int actual_offset;
    int actual_depth;
    // used to check the code inside methods
    struct class_table *actual_class;
};

struct semantic_context init_semantic(struct symbol_table *table, struct errors *err);
void free_semantic(struct semantic_context *ctx);

int get_offset(struct semantic_context *ctx, char *type);

void prlang_semapass(struct prlang_file *node, struct semantic_context *ctx);

#endif