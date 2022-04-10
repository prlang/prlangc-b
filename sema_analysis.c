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

static void new_semantic_type(struct semantic_context *ctx, char *type, int offset) {
    struct semantic_type *typo = malloc(sizeof(struct semantic_type));
    if (!typo) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }
    typo->id = ctx->type_id++;
    typo->offset = offset;
    if (contains(&ctx->types, type, 0, 0)) {
        printf("[ERROR] UNEXPECTED ERROR\n");
        return;
    }
    put(&ctx->types, type, typo, 0, 0);
}

struct semantic_context init_semantic(struct symbol_table *table, struct errors *err) {
    struct semantic_context ctx;
    ctx.types = init_symbol_hash();
    ctx.type_id = 0;
    ctx.table = table;
    ctx.err = err;
    ctx.actual_offset = 0;
    ctx.actual_depth = 0;
    ctx.return_depth = -1;
    ctx.loop = false;
    ctx.accessing = false;
    ctx.call = NULL;
    ctx.actual_fun = NULL;
    ctx.access_type = NULL;
    ctx.return_type = NULL;
    ctx.type = NULL;

    // add types
    new_semantic_type(&ctx, "int", 4);

    return ctx;
}

void free_semantic(struct semantic_context *ctx) {
    free_table(ctx->table);
}

int get_offset(struct semantic_context *ctx, char *type) {
    if (!contains(&ctx->types, type, 0, 0)) {
        return 0;
    }
    struct semantic_type *typo = get(&ctx->types, type, 0, 0);
    return typo->offset;
}

static bool check_sym_type(struct semantic_context *ctx, char *type) {
    return contains(&ctx->types, type, 0, 0);
}

// just for return value from get_from_ctx
struct return_get_ctx {
    char *type;
    int array_dim;
};

// get type given an identifier, search on different places depending on the context
static struct return_get_ctx get_from_ctx(struct semantic_context *ctx, char *name, int depth) {
    void *val;
    // if is inside any function search it
    if (ctx->actual_fun) {
        if ((val = get(&ctx->actual_fun->symbols, name, depth, 0)) != NULL) {
            struct symbol_val *tmp = (struct symbol_val*) val;
            return (struct return_get_ctx) { tmp->type, tmp->array };
        }
    }
    // search on global variables
    if ((val = get(&ctx->table->global, name, 0, 0)) != NULL) {
        struct symbol_val *tmp = (struct symbol_val*) val;
        return (struct return_get_ctx) { tmp->type, tmp->array };
    }
    // see if it is a function
    if ((val = get(&ctx->table->funcs, name, 0, 0)) != NULL) {
        return (struct return_get_ctx) { "function", 0 };
    }
    if (ctx->actual) {
        if ((val = get(&ctx->actual->properties, name, 0, 0)) != NULL) {
            struct symbol_val *tmp = (struct symbol_val*) val;
            return (struct return_get_ctx) { tmp->type, tmp->array };
        }
        if ((val = get(&ctx->actual->methods, name, 0, 0)) != NULL) {
            return (struct return_get_ctx) { "function", 0 };
        }
    }
    return (struct return_get_ctx) { NULL, 0 };
}

static void chk_type_replace(struct semantic_context *ctx, char *val, int array_dim, int line, int char_pos) {
    if (!check_sym_type(ctx, val) && strcmp("void", val)) {
        add_error(ctx->err, CREATE_ERROR("this type is not defined in this scope", line, char_pos));
    }
    ctx->actual_type = val;
    ctx->array_dim = array_dim;
    if (!ctx->access_type && ctx->accessing) {
        ctx->access_type = val;
    }
}

static void chk_type_replace_class(struct semantic_context *ctx, char *val, int line, int char_pos, int access, int expected_access) {
    if (!check_sym_type(ctx, val) && strcmp("void", val)) {
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
                ctx->access_type = NULL;
                return;
            }
            // if is a function call then search for a method
            if (ctx->call) {
                if ((val = get(&table->methods, node.val, 0, ctx->call->arg_count)) != NULL) {
                    struct class_member *tmp_sym = (struct class_member*) val;
                    struct function *tmp_fun = (struct function*) tmp_sym->as.method;
                    chk_type_replace_class(ctx, tmp_fun->type->type.val, node.line, node.char_pos, tmp_sym->access, PUBLIC_MOD);
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
                chk_type_replace(ctx, tmp_fun->type->type.val, tmp_fun->type->array_dim, node.line, node.char_pos);
                return;
            }
            add_error(ctx->err, CREATE_ERROR("this function doesn't exist", node.line, node.char_pos));
            return;
        }
        // nothing worked just search with the actual context
        struct return_get_ctx ret_ctx;
        if ((ret_ctx = get_from_ctx(ctx, node.val, ctx->actual_depth)).type != NULL) {
            chk_type_replace(ctx, ret_ctx.type, ret_ctx.array_dim, node.line, node.char_pos);
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
        ctx->array_dim = 0;
    } else if (node.type == TOK_STRING) {
        ctx->actual_type = "string";
    }
}

static void sema_pass_expr(struct base_expr *node, struct semantic_context *ctx);

static void sema_pass_cons(struct base_expr *node, struct semantic_context *ctx) {
    if (node->type == EXPR_ARRAY) {
        struct array_access *arr = node->as.arr;
        if (!check_sym_type(ctx, arr->name->as.val.val)) {
            add_error(ctx->err, CREATE_ERROR("this type is not defined in this scope", arr->name->as.val.line, arr->name->as.val.char_pos));
        }
        for (size_t i = 0 ; i < arr->arg_count ; i++) {
            if (!arr->arguments[i]) {
                continue;
            }
            sema_pass_expr(arr->arguments[i], ctx);
            if (strcmp("int", ctx->actual_type) != 0) {
                add_error(ctx->err, CREATE_ERROR("array access value needs to be an int", 0, 0));
            }
        }
        ctx->actual_type = arr->name->as.val.val;
        ctx->array_dim = arr->arg_count;
    }
}

static void sema_pass_array(struct array_access *node, struct semantic_context *ctx) {
    sema_pass_expr(node->name, ctx);
    if (node->arg_count > ctx->array_dim) {
        add_error(ctx->err, CREATE_ERROR("can't access specified index", 0, 0));
    }
    int tmp = ctx->array_dim;
    char *tmp_type = ctx->actual_type;
    for (size_t i = 0 ; i < node->arg_count ; i++) {
        sema_pass_expr(node->arguments[i], ctx);
        if (strcmp("int", ctx->actual_type) != 0) {
            add_error(ctx->err, CREATE_ERROR("array access value needs to be an int", 0, 0));
        }
    }
    ctx->array_dim = tmp - node->arg_count;
    ctx->actual_type = tmp_type;
}

static void access_binary_sema_pass(struct binary_expr *node, struct semantic_context *ctx) {
    ctx->accessing = true;
    sema_pass_expr(node->left, ctx);
    if (ctx->array_dim > 0 && node->right->type == EXPR_LITERAL) {
        char *rightVal = node->right->as.val.val;
        if (strcmp(rightVal, "length") != 0) {
            add_error(ctx->err, CREATE_ERROR("can't access value in array", node->op.line, node->op.char_pos));
            ctx->accessing = false;
            ctx->access_type = NULL;
            ctx->array_dim = 0;
            return;
        }
        ctx->actual_type = "int";
        ctx->array_dim = 0;
        ctx->accessing = false;
        ctx->access_type = NULL;
        return;
    }
    sema_pass_expr(node->right, ctx);
    ctx->accessing = false;
}

static struct return_get_ctx child_binary_sema_pass(struct base_expr *node, struct semantic_context *ctx) {
    sema_pass_expr(node, ctx);
    END_NODE();
    return (struct return_get_ctx) { ctx->actual_type, ctx->array_dim };
}

static void sema_pass_binary(struct binary_expr *node, struct semantic_context *ctx) {
    // if accessing
    if (node->op.type == TOK_DOT) {
        access_binary_sema_pass(node, ctx);
        return;
    }

    struct return_get_ctx left = child_binary_sema_pass(node->left, ctx);
    struct return_get_ctx right = child_binary_sema_pass(node->right, ctx);

    if (strcmp(left.type, right.type) != 0 || left.array_dim != right.array_dim) {
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
        case EXPR_CONS:
            sema_pass_cons(node->as.cons, ctx);
            break;
        case EXPR_ARRAY:
            sema_pass_array(node->as.arr, ctx);
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
    if (!check_sym_type(ctx, node->type->type.val)) {
        add_error(ctx->err, CREATE_ERROR("this type is not defined in this scope", node->type->type.line, node->type->type.char_pos));
    }
    ctx->type = node->type->type.val;
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
        if (strcmp(ctx->type, ctx->actual_type) != 0 || ctx->array_dim != node->type->array_dim) {
            // TODO: memory leak
            char *buffer = malloc(sizeof(char) * 64);
            snprintf(buffer, 64, "%s %s", "mismatching types expected", ctx->type);
            add_error(ctx->err, CREATE_ERROR(buffer, node->type->type.line, node->type->type.char_pos));
        }
        ctx->type = NULL;
    }
    if (contains(symbols, name.val, ctx->actual_depth, 0)) {
        add_error(ctx->err, CREATE_ERROR("variable already defined in this scope", name.line, name.char_pos));
        return;
    }
    if (node->type->array_dim == 0) {
        ctx->actual_offset += get_offset(ctx, node->type->type.val);
    } else {
        ctx->actual_offset += 8;
    }
    struct symbol_val *sym = malloc(sizeof(struct symbol_val));
    sym->type = node->type->type.val;
    sym->array = node->type->array_dim;
    sym->offset = ctx->actual_offset;
    sym->arg_pos = 0;
    sym->positive_offset = 0;
    // no modifier
    sym->modifier = 0;
    put(symbols, name.val, sym, ctx->actual_depth, 0);
}

static void sema_pass_return(struct return_stmt *node, struct semantic_context *ctx) {
    ctx->return_depth = ctx->actual_depth;

    if (strcmp(ctx->return_type, "void") == 0) {
        if (node->val) {
            add_error(ctx->err, CREATE_ERROR("return type doesn't match", 0, 0));
        }
        return;
    }
    if (!node->val) {
        add_error(ctx->err, CREATE_ERROR("return type doesn't match", 0, 0));
        return;
    }

    sema_pass_expr(node->val, ctx);

    if (strcmp(ctx->return_type, ctx->actual_type) != 0) {
        add_error(ctx->err, CREATE_ERROR("return type doesn't match", 0, 0));
        return;
    }
}

static void sema_pass_block(struct block *node, struct semantic_context *ctx);

static void sema_pass_if(struct if_stmt *node, struct semantic_context *ctx) {
    for (size_t i = 0 ; i < node->body_count ; i++) {
        if (i < node->cond_count) {
            sema_pass_expr(node->cond[i], ctx);
        }
        sema_pass_block(node->blocks[i], ctx);
    }
}

static void sema_pass_for(struct for_stmt *node, struct semantic_context *ctx) {
    if (node->left_stmt && node->left) {
        sema_pass_var(((struct base_stmt*) node->left)->as.var, ctx, false);
    } else {
        sema_pass_expr(node->left, ctx);
    }
    if (node->med) {
        sema_pass_expr(node->med, ctx);
    }
    if (node->right) {
        sema_pass_expr(node->right, ctx);
    }

    sema_pass_block(node->body, ctx);
}

static void sema_pass_while(struct while_stmt *node, struct semantic_context *ctx) {
    sema_pass_expr(node->cond, ctx);
    sema_pass_block(node->body, ctx);
}

static void sema_pass_stmt(struct base_stmt *node, struct semantic_context *ctx) {
    if (ctx->return_depth != -1 && ctx->return_depth >= ctx->actual_depth) {
        add_error(ctx->err, CREATE_ERROR("statement after return", 0, 0));
    }
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
        case STMT_BLOCK:
            sema_pass_block(node->as.bl, ctx);
            break;
        case STMT_IF:
            sema_pass_if(node->as.is, ctx);
            break;
        case STMT_WHILE:
            sema_pass_while(node->as.ws, ctx);
            break;
        case STMT_FOR:
            sema_pass_for(node->as.fs, ctx);
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
    ctx->return_depth = -1;
    ctx->actual_depth--;
}

static void sema_pass_fun(struct function *node, struct semantic_context *ctx) {
    int arg_offset = 0;
    int positive_offset = 16;
    int positive_total = 0;
    for (int i = 0 ; i < node->arg_count ; i++) {
        if (!check_sym_type(ctx, node->arguments[i].type.val)) {
            add_error(ctx->err, CREATE_ERROR("this type is not defined in this scope", node->arguments[i].type.line, node->arguments[i].type.char_pos));
            continue;
        }
        struct symbol_val *sym = malloc(sizeof(struct symbol_val));
        sym->arg_pos = ctx->actual_fun->symbols.count;
        sym->modifier = 0;
        sym->type = node->arguments[i].type.val;
        sym->array = node->arguments[i].array_dim;
        if (i < ARG_REGISTER_LIMIT) {
            if (node->arguments[i].array_dim > 0) {
                arg_offset += 8;
                sym->offset = arg_offset;
            } else {
                arg_offset += get_offset(ctx, node->arguments[i].type.val);
                sym->offset = arg_offset;
            }
        } else {
            sym->positive_offset = positive_offset;
            positive_offset += 8;
            positive_total += 8;
        }
        if (contains(&ctx->actual_fun->symbols, node->arguments[i].argument.val, 0, 0)) {
            add_error(ctx->err, CREATE_ERROR("argument already defined in this scope", node->arguments[i].argument.line, node->arguments[i].argument.char_pos));
            continue;
        }
        put(&ctx->actual_fun->symbols, node->arguments[i].argument.val, sym, 0, 0);
    }
    ctx->actual_offset = arg_offset;
    ctx->actual_fun->total_pos_offset = positive_total;
    sema_pass_block(node->block, ctx);
}

static void sema_pass_fun_args(struct function *fun, struct semantic_context *ctx) {
    if (!check_sym_type(ctx, fun->type->type.val) && strcmp("void", fun->type->type.val)) {
        add_error(ctx->err, CREATE_ERROR("this type is not defined in this scope", fun->type->type.line, fun->type->type.char_pos));
    }
    for (int i = 0 ; i < fun->arg_count ; i++) {
        if (!check_sym_type(ctx, fun->arguments[i].type.val)) {
            add_error(ctx->err, CREATE_ERROR("this type is not defined in this scope", fun->type->type.line, fun->type->type.char_pos));
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
            sym->type = var->type->type.val;
            // no modifier right now
            sym->modifier = klass->members[i]->access + 1;
            sym->array = var->type->array_dim;
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
        new_semantic_type(ctx, klass->name.val, 8);
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
        if (contains(&ctx->table->funcs_code, node->functions[i]->name.val, 0, node->functions[i]->arg_count)) {
            free(fn);
            continue;
        }
        fn->symbols = init_symbol_hash();
        ctx->actual_fun = fn;
        ctx->return_type = node->functions[i]->type->type.val;
        put(&ctx->table->funcs_code, node->functions[i]->name.val, fn, 0, node->functions[i]->arg_count);
        sema_pass_fun(node->functions[i], ctx);
        fn->total_offset = ctx->actual_offset;
        ctx->actual_offset = 0;
    }
    ctx->return_type = NULL;
    ctx->actual_fun = NULL;
}