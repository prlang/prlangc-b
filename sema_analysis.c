#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sema_analysis.h"

#define BUILTIN_TYPES_SIZE 6
#define ARG_REGISTER_LIMIT 6

#define END_NODE()                                                                    \
    do {                                                                                   \
        ctx->accessing = false;                                                            \
        ctx->access_type = NULL;                                                           \
    } while (0)                                                                            \

#define CLOSE_ACCESS()                                                                     \
    do {                                                                                   \
        if (!ctx->accessing) {                                                             \
            ctx->access_type = NULL;                                                       \
        }                                                                                  \
    } while (0)                                                                            \

struct semantic_context init_semantic(struct symbol_table *table, struct errors *err) {
    struct semantic_context ctx;
    ctx.table = table;
    ctx.err = err;
    ctx.actual_offset = 0;
    ctx.actual_depth = 0;
    ctx.loop = false;
    ctx.accessing = false;
    ctx.call = NULL;
    ctx.actual_fun = NULL;
    ctx.access_type = NULL;
    ctx.return_type = NULL;
    ctx.type = NULL;
    return ctx;
}

void free_semantic(struct semantic_context *ctx) {
    free_table(ctx->table);
}

static int get_offset(char *type) {
    if (strcmp(type, "int") == 0) {
        return INT_OFFSET;
    }
    else if (strcmp(type, "double") == 0) {
        return DOUBLE_OFFSET;
    } 
    else if (strcmp(type, "char") == 0) {
        return CHAR_OFFSET;
    }
    return POINTER_OFFSET;
}

const char *builtin_types[] = {
    "function",
    "int",
    "string",
    "double",
    "bool",
    "void",
    "char"
};

static bool check_sym_type(struct symbol_table *table, char *type) {
    for (int i = 0 ; i < BUILTIN_TYPES_SIZE ; i++) {
        if (strcmp(builtin_types[i], type) == 0) {
            return true;
        }
    }
    return get(&table->classes, type, 0, 0) != NULL;
}

// get type given an identifier, search on different places depending on the context
static char *get_from_ctx(struct semantic_context *ctx, char *name, int depth) {
    void *val;
    // search on global variables
    if ((val = get(&ctx->table->global, name, 0, 0)) != NULL) {
        struct symbol_val *tmp = (struct symbol_val*) val;
        return tmp->type;
    }
    // see if it is a function
    if ((val = get(&ctx->table->funcs, name, 0, 0)) != NULL) {
        return "function";
    }
    if (ctx->actual) {
        if ((val = get(&ctx->actual->properties, name, 0, 0)) != NULL) {
            struct symbol_val *tmp = (struct symbol_val*) val;
            return tmp->type;
        }
        if ((val = get(&ctx->actual->methods, name, 0, 0)) != NULL) {
            return "function";
        }
    }
    // if is inside any function search it
    if (ctx->actual_fun) {
        if ((val = get(&ctx->actual_fun->symbols, name, depth, 0)) != NULL) {
            struct symbol_val *tmp = (struct symbol_val*) val;
            return tmp->type;
        }
    }
    return NULL;
}

static void chk_type_replace(struct semantic_context *ctx, char *val, int line, int char_pos) {
    if (!check_sym_type(ctx->table, val)) {
        add_error(ctx->err, CREATE_ERROR("this type is not defined in this scope", line, char_pos));
    }
    ctx->actual_type = val;
    if (!ctx->access_type && ctx->accessing) {
        ctx->access_type = val;
    }
}

static void chk_type_replace_class(struct semantic_context *ctx, char *val, int line, int char_pos, int access, int expected_access) {
    if (!check_sym_type(ctx->table, val)) {
        add_error(ctx->err, CREATE_ERROR("this type is not defined in this scope", line, char_pos));
    }
    if (access != expected_access) {
        add_error(ctx->err, CREATE_ERROR("can't access this member", line, char_pos));
    }
    ctx->actual_type = val;
    ctx->access_type = val;

    CLOSE_ACCESS();
}

static void sema_pass_literal(struct token_val node, struct semantic_context *ctx) {
    if (node.type == TOK_IDENTIFIER) {
        void *val;
        // is being accessed
        if (ctx->access_type) {
            struct class_table *table = get(&ctx->table->classes, ctx->access_type, 0, 0);
            if (!table) {
                add_error(ctx->err, CREATE_ERROR("can't access a primitive value", node.line, node.char_pos));
                ctx->accessing = false;
                return;
            }
            // if is a function call then search for a method
            if (ctx->call) {
                if ((val = get(&table->methods, node.val, 0, ctx->call->arg_count)) != NULL) {
                    struct class_member *tmp_sym = (struct class_member*) val;
                    struct function *tmp_fun = (struct function*) tmp_sym->as.method;
                    chk_type_replace_class(ctx, tmp_fun->type.val, node.line, node.char_pos, tmp_sym->access, PUBLIC_MOD);
                    return;
                }
                add_error(ctx->err, CREATE_ERROR("this method doesn't exist", node.line, node.char_pos));
                CLOSE_ACCESS();
                return;
            }
            // is not a call so search as property
            if ((val = get(&table->properties, node.val, 0, 0)) != NULL) {
                struct symbol_val *tmp = (struct symbol_val*) val;
                chk_type_replace_class(ctx, tmp->type, node.line, node.char_pos, tmp->modifier, 1);
                return;
            }
            add_error(ctx->err, CREATE_ERROR("this property doesn't exist", node.line, node.char_pos));
            CLOSE_ACCESS();
            return;
        }
        // just a function call
        if (ctx->call) {
            if ((val = get(&ctx->table->funcs, node.val, 0, ctx->call->arg_count)) != NULL) {
                struct function *tmp_fun = (struct function*) val;
                chk_type_replace(ctx, tmp_fun->type.val, node.line, node.char_pos);
                return;
            }
            add_error(ctx->err, CREATE_ERROR("this function doesn't exist", node.line, node.char_pos));
            return;
        }
        // nothing worked just search with the actual context
        if ((val = get_from_ctx(ctx, node.val, ctx->actual_depth)) != NULL) {
            chk_type_replace(ctx, val, node.line, node.char_pos);
            return;
        }

        if (ctx->accessing) {
            add_error(ctx->err, CREATE_ERROR("can't find the specified member", node.line, node.char_pos));
            return;
        }
        add_error(ctx->err, CREATE_ERROR("can't find the identifier", node.line, node.char_pos));
    // TODO: change this to something else
    } else if (node.type == TOK_NUMBER || node.type == TOK_FLOAT) {
        if (ctx->call) {
            add_error(ctx->err, CREATE_ERROR("call value isn't valid", node.line, node.char_pos));
            return;
        }
        ctx->actual_type = "int";
    } else if (node.type == TOK_STRING) {
        ctx->actual_type = "string";
    }
}

static void sema_pass_expr(struct base_expr *node, struct semantic_context *ctx);

static void access_binary_sema_pass(struct binary_expr *node, struct semantic_context *ctx) {
    ctx->accessing = true;
    sema_pass_expr(node->left, ctx);
    sema_pass_expr(node->right, ctx);
    ctx->accessing = false;
}

static char *child_binary_sema_pass(struct base_expr *node, struct semantic_context *ctx) {
    sema_pass_expr(node, ctx);
    char *left_type = ctx->actual_type;
    END_NODE();
    return left_type;
}

static void sema_pass_binary(struct binary_expr *node, struct semantic_context *ctx) {
    // if accessing
    if (node->op.type == TOK_DOT) {
        access_binary_sema_pass(node, ctx);
        return;
    }

    char *left = child_binary_sema_pass(node->left, ctx);
    char *right = child_binary_sema_pass(node->right, ctx);

    if (strcmp(left, right) != 0) {
        add_error(ctx->err, CREATE_ERROR("types doesn't match in expression", node->op.line, node->op.char_pos));
    }
}

static void sema_pass_literal_call(struct call_expr *node, struct token_val val, struct semantic_context *ctx) {
    ctx->call = node;
    sema_pass_literal(val, ctx);
    ctx->call = NULL;
}

static void sema_pass_access_call(struct call_expr *node, struct binary_expr *val, struct semantic_context *ctx) {
    ctx->accessing = true;
    sema_pass_expr(val->left, ctx);
    // right node is the one that holds the method name
    ctx->call = node;
    sema_pass_expr(val->right, ctx);
    ctx->call = NULL;
    ctx->accessing = false;
}

static void sema_pass_call(struct call_expr *node, struct semantic_context *ctx) {
    enum expr_type type = node->val->type;
    if (type == EXPR_LITERAL) {
        struct token_val tmp = node->val->as.val;
        sema_pass_literal_call(node, tmp, ctx);
    } else if (type == EXPR_BINARY) {
        struct binary_expr *bn = node->val->as.binary;
        sema_pass_access_call(node, bn, ctx);
    }
}

static void sema_pass_unary(struct unary_expr *node, struct semantic_context *ctx) {
    sema_pass_expr(node->val, ctx);
}

static void sema_pass_expr(struct base_expr *node, struct semantic_context *ctx) {
    switch (node->type) {
        case EXPR_BINARY:
            sema_pass_binary(node->as.binary, ctx);
            break;
        case EXPR_CALL:
            sema_pass_call(node->as.call, ctx);
            break;
        case EXPR_LITERAL:
            sema_pass_literal(node->as.val, ctx);
            break;
        case EXPR_UNARY:
            sema_pass_unary(node->as.unary, ctx);
            break;
        default:
            break;
    }
}

static void sema_pass_var(struct var_decl *node, struct semantic_context *ctx, bool global) {
    struct symbol_hash *symbols;
    if (global) {
        symbols = &ctx->table->global;
    } else {
        if (!ctx->actual_fun) {
            return;
        }
        symbols = &ctx->actual_fun->symbols;
    }
    enum expr_type glob_type = node->val->type;
    struct token_val name;
    if (!check_sym_type(ctx->table, node->type.val)) {
        add_error(ctx->err, CREATE_ERROR("this type is not defined in this scope", node->type.line, node->type.char_pos));
    }
    ctx->type = node->type.val;
    // var int number;
    if (glob_type == EXPR_LITERAL) {
        name = node->val->as.val;
        if (name.type != TOK_IDENTIFIER) {
            add_error(ctx->err, CREATE_ERROR("the token is not an identifier", name.line, name.char_pos));
        }
    // var int number = 3;
    // check if left node in binary expr is an identifier
    } else {
        name = node->val->as.binary->left->as.val;
        if (name.type != TOK_IDENTIFIER) {
            add_error(ctx->err, CREATE_ERROR("the token is not an identifier", name.line, name.char_pos));
        }
        sema_pass_expr(node->val->as.binary->right, ctx);
        if (strcmp(ctx->type, ctx->actual_type) != 0) {
            // TODO: memory leak
            char *buffer = malloc(sizeof(char) * 64);
            snprintf(buffer, 64, "%s %s", "mismatching types expected", ctx->type);
            add_error(ctx->err, CREATE_ERROR(buffer, node->type.line, node->type.char_pos));
        }
        ctx->type = NULL;
    }
    if (contains(symbols, name.val, ctx->actual_depth, 0)) {
        add_error(ctx->err, CREATE_ERROR("variable already defined in this scope", name.line, name.char_pos));
        return;
    }
    ctx->actual_offset += get_offset(node->type.val);
    struct symbol_val *sym = malloc(sizeof(struct symbol_val));
    sym->type = node->type.val;
    sym->offset = ctx->actual_offset;
    sym->arg = false;
    sym->arg_pos = 0;
    // no modifier
    sym->modifier = 0;
    put(symbols, name.val, sym, ctx->actual_depth, 0);
}

static void sema_pass_return(struct return_stmt *node, struct semantic_context *ctx) {
    if (strcmp(ctx->return_type, "void") == 0) {
        if (node->val) {
            add_error(ctx->err, CREATE_ERROR("return type doesn't match", 0, 0));
        }
        return;
    }
    if (!node->val) {
        add_error(ctx->err, CREATE_ERROR("return type doesn't matchb", 0, 0));
        return;
    }

    sema_pass_expr(node->val, ctx);

    if (strcmp(ctx->return_type, ctx->actual_type) != 0) {
        add_error(ctx->err, CREATE_ERROR("return type doesn't matchc", 0, 0));
        return;
    }
}

static void sema_pass_stmt(struct base_stmt *node, struct semantic_context *ctx) {
    switch (node->type) {
        case STMT_VAR:
            sema_pass_var(node->as.var, ctx, false);
            break;
        case STMT_RETURN:
            sema_pass_return(node->as.rs, ctx);
            break;
        case STMT_EXPR:
            sema_pass_expr(node->as.expr, ctx);
            break;
        default:
            break;
    }
}

static void sema_pass_block(struct block *node, struct semantic_context *ctx) {
    ctx->actual_depth++;
    for (int i = 0 ; i < node->stmts_count ; i++) {
        sema_pass_stmt(node->statements[i], ctx);
    }
    ctx->actual_depth--;
}

static void sema_pass_fun(struct function *node, struct semantic_context *ctx) {
    int arg_offset = 0;
    for (int i = 0 ; i < node->arg_count ; i++) {
        if (!check_sym_type(ctx->table, node->arguments[i].type.val)) {
            add_error(ctx->err, CREATE_ERROR("this type is not defined in this scope", node->arguments[i].type.line, node->arguments[i].type.char_pos));
            continue;
        }
        struct symbol_val *sym = malloc(sizeof(struct symbol_val));
        sym->arg = true;
        sym->arg_pos = ctx->actual_fun->symbols.count;
        sym->modifier = 0;
        sym->type = node->arguments[i].type.val;
        if (i < ARG_REGISTER_LIMIT) {
            sym->offset = 0;
        } else {
            sym->offset = arg_offset;
            arg_offset += get_offset(node->arguments[i].type.val);
        }
        if (contains(&ctx->actual_fun->symbols, node->arguments[i].argument.val, 0, 0)) {
            add_error(ctx->err, CREATE_ERROR("argument already defined in this scope", node->arguments[i].argument.line, node->arguments[i].argument.char_pos));
            continue;
        }
        put(&ctx->actual_fun->symbols, node->arguments[i].argument.val, sym, 0, 0);
    }
    sema_pass_block(node->block, ctx);
}

static void sema_pass_fun_args(struct function *fun, struct semantic_context *ctx) {
    if (!check_sym_type(ctx->table, fun->type.val)) {
        add_error(ctx->err, CREATE_ERROR("this type is not defined in this scope", fun->type.line, fun->type.char_pos));
    }
    for (int i = 0 ; i < fun->arg_count ; i++) {
        if (!check_sym_type(ctx->table, fun->arguments[i].type.val)) {
            add_error(ctx->err, CREATE_ERROR("this type is not defined in this scope", fun->type.line, fun->type.char_pos));
        }
    }
}

static struct token_val property_get_name(struct var_decl *node, struct semantic_context *ctx) {
    enum expr_type glob_type = node->val->type;
    struct token_val name;
    if (glob_type == EXPR_LITERAL) {
        name = node->val->as.val;
        if (name.type != TOK_IDENTIFIER) {
            add_error(ctx->err, CREATE_ERROR("the token is not an identifier", name.line, name.char_pos));
        }
    // var int number = 3;
    // check if left node in binary expr is an identifier
    } else {
        name = node->val->as.binary->left->as.val;
        if (name.type != TOK_IDENTIFIER) {
            add_error(ctx->err, CREATE_ERROR("the token is not an identifier", name.line, name.char_pos));
        }
    }
    return name;
}

static void prlang_members_definition_semapass(struct class_table *table, struct class_decl *klass, struct semantic_context *ctx) {
    for (int i = 0; i < klass->member_count ; i++) {
        enum member_type memt = klass->members[i]->type;
        if (memt == FUNCTION_MEMBER) {
            struct function *method = (struct function*) klass->members[i]->as.method;
            if (contains(&table->methods, method->name.val, 0, method->arg_count)) {
                add_error(ctx->err, CREATE_ERROR("method already defined", method->name.line, method->name.char_pos));
                continue;
            }
            sema_pass_fun_args(method, ctx);
            put(&table->methods, method->name.val, klass->members[i], 0, method->arg_count);
        } else if (memt == PROPERTY_MEMBER) {
            struct var_decl *var = klass->members[i]->as.property->as.var;
            struct token_val name = property_get_name(var, ctx);
            if (contains(&table->properties, name.val, 0, 0)) {
                add_error(ctx->err, CREATE_ERROR("variable already defined in this scope", name.line, name.char_pos));
                continue;
            }

            struct symbol_val *sym = malloc(sizeof(struct symbol_val));
            sym->type = var->type.val;
            // no modifier right now
            sym->modifier = klass->members[i]->access + 1;
            put(&table->properties, name.val, sym, 0, 0);
        }
    }
}

static void prlang_class_definitions_semapass(struct prlang_file *node, struct semantic_context *ctx) {
    for (int i = 0 ; i < node->class_cnt ; i++) {
        struct class_decl *klass = node->classes[i];
        struct class_table *table = malloc(sizeof(struct class_table));
        table->methods = init_symbol_hash();
        table->properties = init_symbol_hash();
        if (contains(&ctx->table->classes, klass->name.val, 0, 0)) {
            add_error(ctx->err, CREATE_ERROR("class already defined", klass->name.line, klass->name.char_pos));
        }
        put(&ctx->table->classes, klass->name.val, table, 0, 0);
        prlang_members_definition_semapass(table, klass, ctx);
    }
}

static void prlang_fun_definitions_semapass(struct prlang_file *node, struct semantic_context *ctx) {
    for (int i = 0 ; i < node->function_cnt ; i++) {
        struct function *fun = (struct function*) node->functions[i];
        if (contains(&ctx->table->funcs, fun->name.val, 0, fun->arg_count)) {
            add_error(ctx->err, CREATE_ERROR("function already defined", fun->name.line, fun->name.char_pos));
            continue;
        }
        sema_pass_fun_args(fun, ctx);
        put(&ctx->table->funcs, fun->name.val, fun, 0, fun->arg_count);
    }
}

void prlang_semapass(struct prlang_file *node, struct semantic_context *ctx) {
    // THIS JUST ADD TOP LEVEL DEFINITIONS
    // add classes
    prlang_class_definitions_semapass(node, ctx);
    // add functions
    prlang_fun_definitions_semapass(node, ctx);

    // INIT CHECKING ACTUAL CODE
    // check global variables
    for (int i = 0 ; i < node->global_cnt ; i++) {
        sema_pass_var(node->globals[i], ctx, true);
    }

    ctx->actual = NULL;

    // check functions
    for (int i = 0 ; i < node->function_cnt ; i++) {
        struct fun_table *fn = malloc(sizeof(struct fun_table));
        fn->symbols = init_symbol_hash();
        ctx->actual_fun = fn;
        ctx->return_type = node->functions[i]->type.val;
        sema_pass_fun(node->functions[i], ctx);
        ctx->actual_offset = 0;
    }
    ctx->return_type = NULL;
    ctx->actual_fun = NULL;
}