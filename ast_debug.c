#include <stdio.h>
#include <string.h>

#include "ast.h"

struct astnode_debug init_debug(char *prefix) {
    struct astnode_debug dg = { prefix, 0 };
    return dg;
}

void prefix_add(struct astnode_debug *dg) {
    if (dg->prefix_length + DEBUG_SPACING >= DEBUG_LIMIT) {
        printf("[INFO] AST debug limit reached\n");
        return;
    }
    dg->prefix_length += DEBUG_SPACING;
    strcat(dg->prefix, "   ");
}

void prefix_remove(struct astnode_debug *dg) {
    if (dg->prefix_length - DEBUG_SPACING < 0) {
        return;
    }
    dg->prefix[dg->prefix_length - DEBUG_SPACING] = '\0';
    dg->prefix_length -= DEBUG_SPACING;
}

// NODES
static char *print_type(struct unary_expr *node) {
    if (node->ut == UT_POSTFIX) {
        return "postfix";
    } else {
        return "prefix";
    }
}

static void debug_expr(struct base_expr *node, struct astnode_debug *details) {
    switch (node->type) {
        case EXPR_LITERAL:
        {
            printf("%s%s\n", details->prefix, (char*) node->as.val.val);
            break;
        }
        case EXPR_BINARY:
        {
            printf("%s%s\n", details->prefix, tok_to_str(node->as.binary->op.type));
            prefix_add(details);
            debug_expr(node->as.binary->left, details);
            debug_expr(node->as.binary->right, details);
            prefix_remove(details);
            break;
        }
        case EXPR_CALL:
        {
            printf("%s(call)\n", details->prefix);
            for (int i = 0 ; i < node->as.call->arg_count ; i++) {
                printf("%sargument %d:\n", details->prefix, i);
                debug_expr(node->as.call->arguments[i], details);
            }
            printf("%sexpr:\n", details->prefix);

            prefix_add(details);
            debug_expr(node->as.call->val, details);
            prefix_remove(details);
            break;
        }
        case EXPR_UNARY:
        {
            printf("%s%s (unary %s)\n", details->prefix, tok_to_str(node->as.unary->op.type), print_type(node->as.unary));

            prefix_add(details);
            debug_expr(node->as.unary->val, details);
            prefix_remove(details);
            break;
        }
    }
}

static void debug_stmt(struct base_stmt *node, struct astnode_debug *details) {
    switch (node->type) {
        case STMT_VAR:
        {
            printf("%svar decl: isConstant: %s, type: %s\n", details->prefix, node->as.var->constant ? "true" : "false", node->as.var->type.val);
            debug_expr(node->as.var->val, details);
            break;
        }
        case STMT_EXPR:
        {
            debug_expr(node->as.expr, details);
            break;
        }
    }
}

static void debug_block(struct block *node, struct astnode_debug *details) {
    printf("%sblock:\n", details->prefix);
    prefix_add(details);
    for (size_t i = 0 ; i < node->stmts_count ; i++) {
        printf("%sstatement %ld:\n", details->prefix, i);
        debug_stmt(node->statements[i], details);
    }
    prefix_remove(details);
}

static void debug_function(struct function *node, struct astnode_debug *details) {
    printf("%sfunction: %s return: %s\n", details->prefix, node->name.val, node->type.val);
    printf("%sarguments:\n", details->prefix);

    prefix_add(details);
    for (size_t i = 0 ; i < node->arg_count ; i++) {
        printf("%stype: %s, name: %s\n", details->prefix, node->arguments[i].type.val, node->arguments[i].argument.val);
    }
    prefix_remove(details);
    
    debug_block(node->block, details);
}

void debug_prlang(struct prlang_file *node, struct astnode_debug *details) {
    printf("%splang_file:\n", details->prefix);

    prefix_add(details);
    printf("%sglobals:\n", details->prefix);

    for (size_t i = 0 ; i < node->global_cnt ; i++) {
        printf("%sglobal %ld:\n", details->prefix, i);
        prefix_add(details);
        debug_stmt(node->globals[i], details);
        prefix_remove(details);
    }

    printf("%sfunctions:\n", details->prefix);

    for (size_t i = 0 ; i < node->function_cnt ; i++) {
        printf("%sfunction %ld:\n", details->prefix, i);
        prefix_add(details);
        debug_function(node->functions[i], details);
        prefix_remove(details);
    }

    prefix_remove(details);
}