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

static void debug_block(struct block *node, struct astnode_debug *details);

static void debug_stmt(struct base_stmt *node, struct astnode_debug *details) {
    switch (node->type) {
        case STMT_VAR:
        {
            printf("%svar decl: isConstant: %s, type: %s\n", details->prefix, node->as.var->constant ? "true" : "false", node->as.var->type.val);
            debug_expr(node->as.var->val, details);
            break;
        }
        case STMT_IF:
        {
            printf("%sif_stmt:\n", details->prefix);
            prefix_add(details);
            for (size_t i = 0 ; i < node->as.is->body_count ; i++) {
                if (i < node->as.is->cond_count) {
                    printf("%scondition %ld:\n", details->prefix, i);
                    debug_expr(node->as.is->cond[i], details);
                }
                printf("%sbody %ld:\n", details->prefix, i);
                debug_block(node->as.is->blocks[i], details);
            }
            prefix_remove(details);
            break;
        }
        case STMT_FOR:
        {
            printf("%sfor_stmt:\n", details->prefix);
            if (node->as.fs->left != NULL) {
                printf("%sleft:\n", details->prefix);
                if (node->as.fs->left_stmt) {
                    debug_stmt(node->as.fs->left, details);
                } else {
                    debug_expr(node->as.fs->left, details);
                }
            }
            if (node->as.fs->med != NULL) {
                printf("%smed:\n", details->prefix);
                debug_expr(node->as.fs->med, details);
            }
            if (node->as.fs->right != NULL) {
                printf("%sright:\n", details->prefix);
                debug_expr(node->as.fs->right, details);
            }
            printf("%sbody:\n", details->prefix);
            debug_block(node->as.fs->body, details);
            break;
        }
        case STMT_WHILE:
        {
            printf("%swhile_stmt:\n", details->prefix);

            printf("%scond:\n", details->prefix);
            debug_expr(node->as.ws->cond, details);

            printf("%sbody:\n", details->prefix);
            debug_block(node->as.ws->body, details);
            break;
        }
        case STMT_CONTINUE:
            printf("%scontinue_stmt\n", details->prefix);
            break;
        case STMT_BREAK:
            printf("%sbreak_stmt\n", details->prefix);
            break;
        case STMT_RETURN:
        {
            printf("%sreturn:\n", details->prefix);
            if (node->as.rs->val) {
                debug_expr(node->as.rs->val, details);
            }
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

static char *print_member_type(enum member_type type) {
    if (type == FUNCTION_MEMBER) {
        return "function";
    } else if (type == PROPERTY_MEMBER) {
        return "property";
    }   
    return "";
}

static char *print_access_modifier(enum access_modifier access) {
    if (access == PRIVATE_MOD) {
        return "private";
    } else if (access == PUBLIC_MOD) {
        return "public";
    } else if (access == PROTECTED_MOD) {
        return "protected";
    }   
    return "";
}

static void debug_class_member(struct class_member *node, struct astnode_debug *details) {
    printf("%sclass_member: access: %s, member_type: %s\n", details->prefix, print_access_modifier(node->access), print_member_type(node->type));
    if (node->type == FUNCTION_MEMBER) {
        debug_function(node->as.method, details);
    } else {
        debug_stmt(node->as.property, details);
    }
}

static void debug_class(struct class_decl *node, struct astnode_debug *details) {
    printf("%sclass %s\n", details->prefix, node->name.val);
    for (size_t i = 0 ; i < node->member_count ; i++) {
        printf("%smember %ld:\n", details->prefix, i);
        prefix_add(details);
        debug_class_member(node->members[i], details);
        prefix_remove(details);
    }
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

    printf("%sclasses:\n", details->prefix);

    for (size_t i = 0 ; i < node->class_cnt ; i++) {
        printf("%sclass %ld:\n", details->prefix, i);
        prefix_add(details);
        debug_class(node->classes[i], details);
        prefix_remove(details);
    }

    prefix_remove(details);
}