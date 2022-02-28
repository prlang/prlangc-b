#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include <stddef.h>

#include "token.h"

#define DEBUG_LIMIT 4096
#define DEBUG_SPACING 3

// DEBUGGING
struct astnode_debug {
    char *prefix;
    int prefix_length;
};

struct astnode_debug init_debug(char *prefix);

void prefix_add(struct astnode_debug *dg);
void prefix_remove(struct astnode_debug *dg);

enum expr_type {
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_CALL,
    EXPR_LITERAL
};

enum stmt_type {
    STMT_VAR,
    STMT_IF,
    STMT_WHILE,
    STMT_RETURN,
    STMT_FOR,
    STMT_EXPR,
    STMT_BREAK,
    STMT_CONTINUE
};

enum access_modifier {
    PUBLIC_MOD,
    PRIVATE_MOD,
    PROTECTED_MOD
};

// used at bytecode generation
enum unary_type {
    UT_PREFIX,
    UT_POSTFIX
};

enum member_type {
    FUNCTION_MEMBER,
    PROPERTY_MEMBER
};

struct base_stmt;
struct base_expr;

struct binary_expr {
    struct token_val op;
    struct base_expr *left;
    struct base_expr *right;
};

struct unary_expr {
    struct token_val op;
    struct base_expr *val;
    enum unary_type ut;
};

struct call_expr {
    struct base_expr *val;
    struct base_expr **arguments;
    size_t arg_count;
};

struct base_expr {
    enum expr_type type;
    union {
        struct binary_expr *binary;
        struct unary_expr *unary;
        struct token_val val;
        struct call_expr *call;
    } as;
};

struct var_decl {
    struct token_val type;
    struct base_expr *val;
    bool constant;
};

struct block {
    struct base_stmt **statements;
    size_t stmts_count;
};

struct while_stmt {
    struct base_expr *cond;
    struct block *body;
};

struct for_stmt {
    void *left;
    bool left_stmt;
    struct base_expr *med;
    struct base_expr *right;
    struct block* body;
};

struct if_stmt {
    // if / else if conditions
    struct base_expr **cond;
    size_t cond_count;

    struct block **blocks;
    size_t body_count;
};

struct return_stmt {
    struct base_expr *val;
};

struct base_stmt {
    enum stmt_type type;
    union {
        struct var_decl *var;
        struct base_expr *expr;
        struct if_stmt *is;
        struct while_stmt *ws;
        struct for_stmt *fs;
        struct return_stmt *rs;
    } as;
};

struct function_arg {
    struct token_val type;
    struct token_val argument;
};

struct function {
    struct block *block;
    struct token_val name;
    struct token_val type;
    struct function_arg *arguments;
    size_t arg_count;
};

struct class_member {
    enum access_modifier access;
    enum member_type type;
    union {
        struct function *method;
        struct var_decl *property;
    } as;
};

struct class_decl {
    struct token_val name;

    size_t member_count;
    struct class_member **members;
};

struct prlang_file {
    size_t global_cnt;
    struct var_decl **globals;

    size_t function_cnt;
    struct function **functions;

    size_t class_cnt;
    struct class_decl **classes;
};

struct base_expr *init_literal(struct token_val data);
struct base_expr *init_unary_expr(struct token_val type, struct base_expr *val, enum unary_type ut);
struct base_expr *init_binary_expr(struct token_val type, struct base_expr *left, struct base_expr *right);
struct base_expr *init_call_expr(struct base_expr *expr, struct base_expr **arguments, int arguments_cnt);
struct block *init_block(struct base_stmt **statements, size_t stmts_count);
struct base_stmt *init_while(struct base_expr *cond, struct block *block_node);
struct base_stmt *init_for(void *left, bool left_stmt, struct base_expr *med, struct base_expr *right, struct block* block_node);
struct base_stmt *init_if(struct base_expr **cond, int cond_cnt, struct block **blocks, int block_cnt);
struct base_stmt *init_var_decl(struct base_expr *val, bool constant, struct token_val type);
struct base_stmt *init_return_stmt(struct base_expr *val);
struct function *init_function(struct block *block, struct token_val name, struct token_val type, struct function_arg *arguments, int argument_cnt);
struct class_member *init_class_method(enum access_modifier access, enum member_type type, struct function *method);
struct class_member *init_class_property(enum access_modifier access, enum member_type type, struct var_decl *property);
struct class_decl *init_class(struct token_val name, size_t members_cnt, struct class_member **members);
struct prlang_file *init_prlang_file(struct var_decl **globals, int global_cnt, 
                                struct function **functions, int function_cnt,
                                struct class_decl **classes, int class_cnt);

void free_prlang(struct prlang_file *node);

void debug_prlang(struct prlang_file *node, struct astnode_debug *details);

#endif