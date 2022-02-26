#include <stdlib.h>
#include <stdio.h>

#include "ast.h"

struct base_expr *init_literal(struct token_val data) {
    struct base_expr* base = malloc(sizeof(struct base_expr));
    if (!base) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }

    base->type = EXPR_LITERAL;
    base->as.val = data;
    return base;
}

struct base_expr *init_unary_expr(struct token_val type, struct base_expr *val, enum unary_type ut) {
    struct base_expr* base = malloc(sizeof(struct base_expr));
    if (!base) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }

    struct unary_expr* node = malloc(sizeof(struct unary_expr));
    if (!node) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }
    node->op = type;
    node->val = val;
    node->ut = ut;

    base->type = EXPR_UNARY;
    base->as.unary = node;
    return base;
}

struct base_expr *init_binary_expr(struct token_val type, struct base_expr *left, struct base_expr *right) {
    struct base_expr* base = malloc(sizeof(struct base_expr));
    if (!base) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }

    struct binary_expr* node = malloc(sizeof(struct binary_expr));
    if (!node) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }
    node->op = type;
    node->left = left;
    node->right = right;

    base->type = EXPR_BINARY;
    base->as.binary = node;
    return base;
}

struct base_expr *init_call_expr(struct base_expr *expr, struct base_expr **arguments, int arguments_cnt) {
    struct base_expr* base = malloc(sizeof(struct base_expr));
    if (!base) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }

    struct call_expr* node = malloc(sizeof(struct call_expr));
    if (!node) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }
    node->val = expr;
    node->arguments = arguments;
    node->arg_count = arguments_cnt;

    base->type = EXPR_CALL;
    base->as.call = node;
    return base;
}

struct block *init_block(struct base_stmt **statements, size_t stmts_count) {
    struct block* node = malloc(sizeof(struct block));
    if (!node) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }
    node->statements = statements;
    node->stmts_count = stmts_count;
    return node;
}

struct base_stmt *init_while(struct base_expr *cond, struct block *block_node) {
    struct base_stmt* base = malloc(sizeof(struct base_stmt));
    if (!base) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }

    struct while_stmt* node = malloc(sizeof(struct while_stmt));
    if (!node) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }
    node->cond = cond;
    node->body = block_node;

    base->type = STMT_WHILE;
    base->as.ws = node;
    return base;
}

struct base_stmt *init_for(struct base_expr *left, bool left_stmt, struct base_expr *med, struct base_expr *right, struct block* block_node) {
    struct base_stmt* base = malloc(sizeof(struct base_stmt));
    if (!base) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }

    struct for_stmt* node = malloc(sizeof(struct for_stmt));
    if (!node) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }
    node->left = left;
    node->med = med;
    node->right = right;
    node->body = block_node;

    base->type = STMT_FOR;
    base->as.fs = node;
    return base;
}

struct base_stmt *init_if(struct base_expr **cond, int cond_cnt, struct block **blocks, int block_cnt) {
    struct base_stmt* base = malloc(sizeof(struct base_stmt));
    if (!base) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }

    struct if_stmt* node = malloc(sizeof(struct if_stmt));
    if (!node) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }
    node->cond = cond;
    node->cond_count = cond_cnt;
    node->blocks = blocks;
    node->body_count = block_cnt;

    base->type = STMT_IF;
    base->as.is = node;
    return base;
}

struct base_stmt *init_var_decl(struct base_expr *val, bool constant, struct token_val type) {
    struct base_stmt* base = malloc(sizeof(struct base_stmt));
    if (!base) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }

    struct var_decl* node = malloc(sizeof(struct var_decl));
    if (!node) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }
    node->val = val;
    node->constant = constant;
    node->type = type;

    base->type = STMT_VAR;
    base->as.var = node;
    return base;
}

struct base_stmt *init_return_stmt(struct base_expr *val) {
    struct base_stmt* base = malloc(sizeof(struct base_stmt));
    if (!base) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }

    struct return_stmt* node = malloc(sizeof(struct return_stmt));
    if (!node) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }
    node->val = val;

    base->type = STMT_RETURN;
    base->as.var = node;
    return base;
}

struct function *init_function(struct block *block, struct token_val name, struct token_val type, struct function_arg *arguments, int argument_cnt) {
    struct function* node = malloc(sizeof(struct function));
    if (!node) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }
    node->block = block;
    node->name = name;
    node->type = type;
    node->arguments = arguments;
    node->arg_count = argument_cnt;
    return node;
}

struct class_member *init_class_method(enum access_modifier access, enum member_type type, struct function *method) {
    struct class_member* node = malloc(sizeof(struct class_member));
    if (!node) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }
    node->access = access;
    node->type = type;
    node->as.method = method;
    return node;
}

struct class_member *init_class_property(enum access_modifier access, enum member_type type, struct var_decl *property) {
    struct class_member* node = malloc(sizeof(struct class_member));
    if (!node) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }
    node->access = access;
    node->type = type;
    node->as.property = property;
    return node;
}

struct class_decl *init_class(struct token_val name, size_t members_cnt, struct class_member **members) {
    struct class_decl* node = malloc(sizeof(struct class_decl));
    if (!node) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }
    node->name = name;
    node->member_count = members_cnt;
    node->members = members;
    return node;
}

struct prlang_file *init_prlang_file(struct var_decl **globals, int global_cnt, 
                                struct function **functions, int function_cnt,
                                struct class_decl **classes, int class_cnt) {

    struct prlang_file* node = malloc(sizeof(struct prlang_file));
    if (!node) {
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
    // TODO: do free
}
