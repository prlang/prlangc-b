#include <stdlib.h>
#include <stdio.h>

#include "ast.h"

struct base_expr *init_literal(struct token_val data) {

}

struct base_expr *init_unary_expr(enum token_type type, struct base_expr *val, enum unary_type ut) {

}

struct base_expr *init_binary_expr(struct token_val type, struct base_expr *left, struct base_expr *right) {

}

struct base_expr *init_call_expr(struct base_expr *expr, struct base_expr **arguments, int arguments_cnt) {

}

struct block *init_block(struct base_stmt **statements) {

}

struct base_stmt *init_while(struct base_expr *cond, struct block *block_node) {

}

struct base_stmt *init_for(struct base_expr *left, struct base_expr *med, struct base_expr *right, struct block* block_node) {

}

struct base_stmt *init_if(struct base_expr **cond, int cond_cnt, struct block **blocks, int block_cnt) {

}

struct base_stmt *init_var_decl(struct base_expr *val, bool constant, struct token_val type) {

}

struct base_stmt *init_return_stmt(struct base_expr *val) {

}

struct function *init_function(struct block *block, struct token_val name, struct token_val type, struct function_arg *arguments, int argument_cnt) {

}

struct class_member *init_class_method(enum access_modifier access, enum member_type type, struct function *method) {

}

struct class_member *init_class_property(enum access_modifier access, enum member_type type, struct var_decl *property) {

}

struct class_decl *init_class(struct token_val name, int method_cnt, struct class_member **methods, int properties_cnt, struct class_member **properties) {

}

struct prlang_file *init_prlang_file(struct var_decl **globals, int global_cnt, 
                                struct function **functions, int function_cnt,
                                struct class_decl **classes, int class_cnt) {

    struct prlang_file* node = malloc(sizeof(struct prlang_file));
    if (node == NULL) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }
    node->global_cnt = global_cnt;
    node->globals = globals;
    node->function_cnt = function_cnt;
    node->functions = functions;
    node->class_cnt = class_cnt;
    node->classes = classes;
    return node;
}

void free_prlang(struct prlang_file *node) {
    
}
