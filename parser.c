#include <stdio.h>
#include <stdlib.h>

#include "parser.h" 

#define BINARY_INFIX(prec, fun, assoc) ((struct binary_infix) {{  prec, (struct base_expr * (*)(struct parse_infix *, struct parser_helper *, struct base_expr *, struct token_val)) &fun }, assoc })
#define INFIX(prec, fun) ((struct parse_infix) { prec , (struct base_expr * (*)(struct parse_infix *, struct parser_helper *, struct base_expr *, struct token_val)) &fun })
#define PREFIX(fun) ((struct parse_prefix) { &fun })

#define ARRAY_FACTOR 2

// INIT EXPRESSION PARSE

struct parse_prefix *prefixes[TOK_EXIT];
struct parse_infix *infixes[TOK_EXIT];

static struct base_expr *parse_expr(struct parser_helper *helper);

// TODO: change this to a list struct  
static void add_toarray(void ***statements, int *arr_size, int *count, void *node) {
    if (*count >= *arr_size) {
        *arr_size *= ARRAY_FACTOR;
        struct base_expr **tmp = realloc(*statements, sizeof(struct base_expr*) * *arr_size);
        if (tmp == NULL) {
            printf("[ERROR] Not enough memory\n");
            exit(1);
        }
        *statements = tmp;
    }
    (*statements)[(*count)++] = node;
}

static int get_precedence(struct parser_helper *helper) {
    struct parse_infix *prefix = infixes[ACTUAL_TOK().type];
    return prefix != NULL ? prefix->precedence : 0;
}

// PARSING CALL ARGUMENTS
static struct base_expr **parse_arguments(struct parser_helper *helper, int *argument_cnt) {
    if (ACTUAL_TOK().type != TOK_RIGHT_PAREN) {
        int arr_size = ARRAY_FACTOR;
        struct base_expr **arguments = malloc(sizeof(struct base_expr*) * arr_size);
        do {
            ENSURE_SIZE();
            struct base_expr *expr = parse_expr(helper);
            add_toarray(&arguments, &arr_size, argument_cnt, expr);
        } while (READ_TOK().type == TOK_COMMA);

        PREVIOUS_TOK();
        CONSUME_TOK(TOK_RIGHT_PAREN, "expected )");
        return arguments;
    }
    READ_TOK();
    return NULL;
}

// EXPRESSION
static struct base_expr *parse_expr_internal(struct parser_helper *helper, int precedence) {

    ENSURE_SIZE();
    struct token_val tok = READ_TOK();
    struct parse_prefix *prefix = prefixes[tok.type];

    if (prefix == NULL) {
        add_error(helper->errors, CREATE_ERROR("unexpected token at expression", tok.line, tok.char_pos));
        return NULL;
    }

    struct base_expr *left = prefix->parse(prefix, helper, tok);

    while (precedence < get_precedence(helper)) {
        tok = READ_TOK();
        struct parse_infix *infix = infixes[tok.type];
        left = infix->parse(infix, helper, left, tok);
    }
    return left;
}

// BINARY
static struct base_expr *parse_binary(struct binary_infix *infix, struct parser_helper *helper, struct base_expr *node, struct token_val val) {
    struct base_expr *right = parse_expr_internal(helper, infix->base.precedence - (infix->is_right ? 1 : 0));
    return init_binary_expr(val, node, right);
}

// ASSIGN
static struct base_expr *parse_assign(struct binary_infix *infix, struct parser_helper *helper, struct base_expr *node, struct token_val val) {

    struct base_expr *right = parse_expr_internal(helper, infix->base.precedence - (infix->is_right ? 1 : 0));
    enum expr_type left_type = node->type;

    if (left_type != EXPR_LITERAL && left_type != EXPR_BINARY) {
        add_error(helper->errors, CREATE_ERROR("invalid assign type", val.line, val.char_pos));
        return NULL;
    }

    if (left_type == EXPR_BINARY) {
        struct binary_expr *bin = (struct binary_expr*) node->as.binary;
        if (bin->op.type != TOK_DOT && bin->op.type != TOK_ASSIGN) {
            add_error(helper->errors, CREATE_ERROR("invalid assign type", val.line, val.char_pos));
            return NULL;
        }
    }

    return init_binary_expr(val, node, right);
}

// ACCESS
static struct base_expr *parse_access(struct parse_infix *infix, struct parser_helper *helper, struct base_expr *node, struct token_val val) {

    struct base_expr *right = parse_expr_internal(helper, PREC_ACCESS);

    if (right == NULL || node == NULL) {
        add_error(helper->errors, CREATE_ERROR("accessing invalid value", val.line, val.char_pos));
        return NULL;
    }

    enum expr_type left_type = node->type;
    enum expr_type right_type = right->type;

    if ( (left_type != EXPR_CALL && left_type != EXPR_LITERAL && left_type != EXPR_BINARY) || 
            (right_type != EXPR_CALL && right_type != EXPR_LITERAL && right_type != EXPR_BINARY) ) {
        add_error(helper->errors, CREATE_ERROR("accessing invalid value", val.line, val.char_pos));
        return NULL;
    }

    return init_binary_expr(val, node, right);
}

// UNARY POSTFIX
static struct base_expr *parse_unary_postfix(struct parse_infix *infix, struct parser_helper *helper, struct base_expr *node, struct token_val val) {
    return init_unary_expr(val, node, UT_POSTFIX);
}

// UNARY PREFIX
static struct base_expr *parse_unary(struct parse_prefix *prefix, struct parser_helper *helper, struct token_val val) {
    struct base_expr *child = parse_expr_internal(helper, PREC_PREFIX);
    return init_unary_expr(val, child, UT_PREFIX);
}

// GROUPING
static struct base_expr *parse_grouping(struct parse_prefix *prefix, struct parser_helper *helper, struct token_val val) {
    struct base_expr *expr = parse_expr(helper);
    CONSUME_TOK(TOK_RIGHT_PAREN, "expected )");
    return expr;
}

// CALL
static struct base_expr *parse_call(struct parse_infix *infix, struct parser_helper *helper, struct base_expr *node, struct token_val val) {
    int arg_cnt = 0;
    struct base_expr **arguments = parse_arguments(helper, &arg_cnt);
    return init_call_expr(node, arguments, arg_cnt);
}

// LITERAL
static struct base_expr *parse_literal(struct parse_prefix *prefix, struct parser_helper *helper, struct token_val val) {
    return init_literal(val);
}

// - | +
struct binary_infix term = BINARY_INFIX(PREC_TERM, parse_binary, false);

// * | / | %
struct binary_infix factor = BINARY_INFIX(PREC_FACTOR, parse_binary, false);

// &&
struct binary_infix and = BINARY_INFIX(PREC_AND, parse_binary, false);

// ||
struct binary_infix or = BINARY_INFIX(PREC_OR, parse_binary, false);

// == | !=
struct binary_infix equality = BINARY_INFIX(PREC_EQUALITY, parse_binary, false);

// >= | <= | < | >
struct binary_infix comparison = BINARY_INFIX(PREC_COMPARISON, parse_binary, false);

// =
struct binary_infix assign = BINARY_INFIX(PREC_ASSIGNMENT, parse_assign, false);


// ++ | --
struct parse_infix postfix = INFIX(PREC_POSTFIX, parse_unary_postfix);

// .
struct parse_infix access = INFIX(PREC_ACCESS, parse_access);

// call
struct parse_infix call = INFIX(PREC_CALL, parse_call);


// - | ! | ++ | -- | +
struct parse_prefix prefix = PREFIX(parse_unary);

// ( expr )
struct parse_prefix grouping = PREFIX(parse_grouping);

// number | identifier | string
struct parse_prefix literal = PREFIX(parse_literal);


struct parser_helper init_parser_helper(struct lexer *lex, struct errors *errors) {
    struct parser_helper helper;

    helper.errors = errors;
    helper.lex = lex;
    helper.index = 0;

    // init pratt parser
    prefixes[TOK_STRING] = &literal;
    prefixes[TOK_IDENTIFIER] = &literal;
    prefixes[TOK_NUMBER] = &literal;
    prefixes[TOK_FLOAT] = &literal;
    prefixes[TOK_TRUE] = &literal;
    prefixes[TOK_FALSE] = &literal;
    prefixes[TOK_NULLV] = &literal;
    prefixes[TOK_SUBSTRACT] = &prefix;
    prefixes[TOK_ADD] = &prefix;
    prefixes[TOK_NEGATION] = &prefix;
    prefixes[TOK_INCREMENT] = &prefix;
    prefixes[TOK_DECREMENT] = &prefix;
    prefixes[TOK_LEFT_PAREN] =  &grouping;

    infixes[TOK_SUBSTRACT] = (struct parse_infix*) &term;
    infixes[TOK_ADD] = (struct parse_infix*) &term;
    infixes[TOK_MULT] = (struct parse_infix*) &factor;
    infixes[TOK_DIV] = (struct parse_infix*) &factor;
    infixes[TOK_MOD] = (struct parse_infix*) &factor;
    infixes[TOK_AND] = (struct parse_infix*) &and;
    infixes[TOK_OR] = (struct parse_infix*) &or;

    infixes[TOK_EQUAL] = (struct parse_infix*) &equality;
    infixes[TOK_NOT_EQUAL] = (struct parse_infix*) &equality;

    infixes[TOK_GREAT_EQUAL] = (struct parse_infix*) &comparison;
    infixes[TOK_GREAT] = (struct parse_infix*) &comparison;
    infixes[TOK_LESS_EQUAL] = (struct parse_infix*) &comparison;
    infixes[TOK_LESS] = (struct parse_infix*) &comparison;

    infixes[TOK_ASSIGN] = (struct parse_infix*) &assign;
    infixes[TOK_DOT] = &access;
    infixes[TOK_INCREMENT] = &postfix;
    infixes[TOK_DECREMENT] = &postfix;
    infixes[TOK_LEFT_PAREN] = &call;

    return helper;
}

static struct base_expr *parse_expr(struct parser_helper *helper) {
    return parse_expr_internal(helper, 0);
}

static struct base_expr *parse_expr_stmt(struct parser_helper *helper) {
    struct base_expr *expr = parse_expr(helper);
    CONSUME_TOK(TOK_SEMICOLON, "expected ;");
    return expr;
}

// PARSE STATEMENTS

static struct block *parse_block(struct parser_helper *helper);

static struct base_stmt *parse_var(struct parser_helper *helper) {

    bool constant = false;

    ENSURE_SIZE();
    struct token_val var = READ_TOK();

    if (var.type == TOK_VAR) {
    } else if (var.type == TOK_CONSTANT) {
        constant = true;
    } else {
        add_error(helper->errors, CREATE_ERROR("expected var or const", var.line, var.char_pos));
        return NULL;
    }

    ENSURE_SIZE();
    struct token_val type_tok = READ_TOK();

    if (type_tok.type != TOK_IDENTIFIER) {
        add_error(helper->errors, CREATE_ERROR("expected type", type_tok.line, type_tok.char_pos));
        return NULL;
    }

    struct token_val tok = ACTUAL_TOK();
    struct base_expr *assign = parse_expr_stmt(helper);

    if (assign == NULL) {
        return NULL;
    }

    enum expr_type assign_type = assign->type;

    // case: var my_var;
    // skip all the checks
    if (assign_type == EXPR_LITERAL) {
        return init_var_decl(assign, constant, type_tok);
    }

    if (assign_type != EXPR_BINARY) {
        add_error(helper->errors, CREATE_ERROR("invalid expr", tok.line, tok.char_pos));
        return NULL;
    }

    struct binary_expr *bin = assign->as.binary;
    
    if (bin->op.type != TOK_ASSIGN) {
        add_error(helper->errors, CREATE_ERROR("invalid expr", tok.line, tok.char_pos));
        return NULL;
    }

    if (bin->left->type != EXPR_LITERAL) {
        add_error(helper->errors, CREATE_ERROR("invalid expr", tok.line, tok.char_pos));
        return NULL;
    }

    return init_var_decl(assign, constant, type_tok);
}

static struct base_stmt *parse_while(struct parser_helper *helper) {

    CONSUME_TOK(TOK_WHILE, "expected while");
    CONSUME_TOK(TOK_LEFT_PAREN, "expected (");

    struct base_expr *cond = parse_expr(helper);

    CONSUME_TOK(TOK_RIGHT_PAREN, "expected )");

    struct block *body = parse_block(helper);

    return init_while(cond, body);
}

static struct base_stmt *parse_for(struct parser_helper *helper) {

    void *left;
    bool left_stmt = false;

    struct base_expr *med;
    struct base_expr *right;

    CONSUME_TOK(TOK_FOR, "expected for");
    CONSUME_TOK(TOK_LEFT_PAREN, "expected (");

    struct token_val tok = ACTUAL_TOK();

    if (tok.type == TOK_VAR || tok.type == TOK_CONSTANT) {
        left = parse_var(helper);
        left_stmt = true;
    } else if (tok.type == TOK_SEMICOLON) {
        left = NULL;
        ENSURE_SIZE();
        READ_TOK();
    } else {
        left = parse_expr_stmt(helper);
    }

    tok = ACTUAL_TOK();

    if (tok.type != TOK_SEMICOLON) {
        med = parse_expr_stmt(helper);
    } else {
        med = NULL;
        ENSURE_SIZE();
        READ_TOK();
    }

    tok = ACTUAL_TOK();

    if (tok.type != TOK_RIGHT_PAREN) {
        right = parse_expr(helper);
        ENSURE_SIZE();
        READ_TOK();
    } else {
        right = NULL;
        ENSURE_SIZE();
        READ_TOK();
    }

    struct block *body = parse_block(helper);
    return init_for(left, left_stmt, med, right, body);
}

static struct base_stmt *parse_if(struct parser_helper *helper) {

    // condition array
    int cond_cnt = 0;
    int cond_arrsize = ARRAY_FACTOR;
    struct base_expr **conditions = malloc(cond_arrsize * sizeof(struct base_expr*));

    // block array
    int block_cnt = 0;
    int block_arrsize = ARRAY_FACTOR;
    struct block **blocks = malloc(block_arrsize * sizeof(struct block*));

    CONSUME_TOK(TOK_IF, "expected if");

    CONSUME_TOK(TOK_LEFT_PAREN, "expected (");
    struct base_expr *cond = parse_expr(helper);
    add_toarray(&conditions, &cond_arrsize, &cond_cnt, cond);
    CONSUME_TOK(TOK_RIGHT_PAREN, "expected )");

    struct block *if_block = parse_block(helper);
    add_toarray(&blocks, &block_arrsize, &block_cnt, if_block);

    if (READ_TOK().type == TOK_ELSE) {
        struct token_val next = ACTUAL_TOK();
        switch (next.type) {
            case TOK_IF:
            {
                struct if_stmt *elseif = parse_if(helper)->as.is;

                if (elseif == NULL) {
                    return NULL;
                }
                for (int i = 0 ; i < elseif->cond_count ; i++) {
                    add_toarray(&conditions, &cond_arrsize, &cond_cnt, elseif->cond[i]);
                }
                for (int i = 0 ; i < elseif->body_count ; i++) {
                    add_toarray(&blocks, &block_arrsize, &block_cnt, elseif->blocks[i]);
                }

                free(elseif->cond);
                free(elseif->blocks);
                free(elseif);
                break;
            }
            case TOK_LEFT_BRACES:
            {
                struct block *else_block = parse_block(helper);
                add_toarray(&blocks, &block_arrsize, &block_cnt, else_block);
                break;
            }
            default:
                add_error(helper->errors, CREATE_ERROR("expected { or if", next.line, next.char_pos));
                return NULL;
        }
    } else {
        PREVIOUS_TOK();
    }
    return init_if(conditions, cond_cnt, blocks, block_cnt);
}

static struct base_stmt *parse_return(struct parser_helper *helper) {
    CONSUME_TOK(TOK_RETURN, "expected return");
    if (ACTUAL_TOK().type == TOK_SEMICOLON) {
        READ_TOK();
        return init_return_stmt(NULL);
    }
    struct base_expr *val = parse_expr_stmt(helper);
    return init_return_stmt(val);
}

static struct base_stmt **parse_statements(struct parser_helper *helper, int *count) {
    struct token_val tok = ACTUAL_TOK();

    // ARRAY DETAILS
    int arr_size = ARRAY_FACTOR;
    struct base_stmt **statements = malloc(arr_size * sizeof(struct base_stmt*));

    while (tok.type != TOK_RIGHT_BRACES) {
        switch (tok.type) {
            case TOK_IF:
            {
                struct base_stmt *ifs = parse_if(helper);
                if (ifs == NULL) return NULL;
                add_toarray(&statements, &arr_size, count, ifs); 
                break;
            }
            case TOK_LEFT_BRACES:
            {
                struct block *bl = parse_block(helper);
                if (!bl) return NULL;
                struct base_stmt *stmt = malloc(sizeof(struct base_stmt*));
                if (!stmt) {
                    printf("[ERROR] Not enough memory\n");
                    exit(1);
                }
                stmt->type = STMT_BLOCK;
                stmt->as.bl = bl;
                add_toarray(&statements, &arr_size, count, stmt);
                break;
            }
            case TOK_CONSTANT:
            case TOK_VAR:
            {
                struct base_stmt *var = parse_var(helper);
                if (var == NULL) return NULL;
                add_toarray(&statements, &arr_size, count, var);
                break;
            }
            case TOK_WHILE:
            {
                struct base_stmt *whv = parse_while(helper);
                if (whv == NULL) return NULL;
                add_toarray(&statements, &arr_size, count, whv);
                break;
            }
            case TOK_FOR:
            {
                struct base_stmt *fr = parse_for(helper);
                if (fr == NULL) return NULL;
                add_toarray(&statements, &arr_size, count, fr);
                break;
            }
            case TOK_RETURN:
            {
                add_toarray(&statements, &arr_size, count, parse_return(helper));
                break;
            }
            case TOK_EXIT:
            default:
            {
                struct base_expr *expr = parse_expr_stmt(helper);
                if (!expr) return NULL;
                struct base_stmt *stmt = malloc(sizeof(struct base_stmt*));
                if (!stmt) {
                    printf("[ERROR] Not enough memory\n");
                    exit(1);
                }
                stmt->type = STMT_EXPR;
                stmt->as.expr = expr;
                add_toarray(&statements, &arr_size, count, stmt);
                break;
            }
        }
        tok = ACTUAL_TOK();
    }
    return statements;
}

static struct block *parse_block(struct parser_helper *helper) {
    CONSUME_TOK(TOK_LEFT_BRACES, "expected {");
    int count = 0;
    struct base_stmt **statements = parse_statements(helper, &count);
    CONSUME_TOK(TOK_RIGHT_BRACES, "expected }");
    return init_block(statements, count);
}

// PARSE FUNCTION

static struct function_arg *parse_fn_arguments(struct parser_helper *helper, int *argument_count) {
    if (ACTUAL_TOK().type != TOK_RIGHT_PAREN) {
        int arr_size = ARRAY_FACTOR;
        struct function_arg *arguments = malloc(sizeof(struct function_arg) * arr_size);
        do {
            if (*argument_count >= arr_size) {
                arr_size *= ARRAY_FACTOR;
                struct function_arg *tmp = realloc(arguments, sizeof(struct function_arg) * arr_size);
                if (tmp == NULL) {
                    printf("[ERROR] Not enough memory\n");
                    exit(1);
                }
                arguments = tmp;
            }

            ENSURE_SIZE();
            struct token_val tok_type = READ_TOK();
            if (tok_type.type != TOK_IDENTIFIER) {
                add_error(helper->errors, CREATE_ERROR("expected identifier", tok_type.line, tok_type.char_pos));
                return NULL;
            }

            ENSURE_SIZE();
            struct token_val iden_val = READ_TOK();
            if (iden_val.type != TOK_IDENTIFIER) {
                add_error(helper->errors, CREATE_ERROR("expected identifier", iden_val.line, iden_val.char_pos));
                return NULL;
            }

            struct function_arg arg = { tok_type, iden_val };
            arguments[(*argument_count)++] = arg;

        } while (READ_TOK().type == TOK_COMMA);

        PREVIOUS_TOK();
        CONSUME_TOK(TOK_RIGHT_PAREN, "expected )");
        return arguments;
    }
    READ_TOK();
    return NULL;
}

static struct function *parse_function(struct parser_helper *helper) {

    ENSURE_SIZE();
    struct token_val type_tok = READ_TOK();

    if (type_tok.type != TOK_IDENTIFIER) {
        add_error(helper->errors, CREATE_ERROR("expected type", type_tok.line, type_tok.char_pos));
        return NULL;
    }

    ENSURE_SIZE();
    struct token_val name = READ_TOK();

    if (name.type != TOK_IDENTIFIER) {
        add_error(helper->errors, CREATE_ERROR("expected identifier", name.line, name.char_pos));
        return NULL;
    }

    CONSUME_TOK(TOK_LEFT_PAREN, "expected (");

    int argument_count = 0;
    struct function_arg *arguments = parse_fn_arguments(helper, &argument_count);

    struct block *body = parse_block(helper);
    
    return init_function(body, name, type_tok, arguments, argument_count);
}

// PARSE CLASS

static enum access_modifier get_access_modifier(enum token_type type) {
    if (type == TOK_PUBLIC) {
        return PUBLIC_MOD;
    } else if (type == TOK_PROTECTED) {
        return PROTECTED_MOD;
    } else if (type == TOK_PRIVATE) {
        return PRIVATE_MOD;
    }
}

static struct class_member *parse_class_member(struct parser_helper *helper) {
    ENSURE_SIZE();
    struct token_val tok_access = READ_TOK();

    if (tok_access.type != TOK_PRIVATE && tok_access.type != TOK_PROTECTED && tok_access.type != TOK_PUBLIC) {
        add_error(helper->errors, CREATE_ERROR("expected public, private or protected", tok_access.line, tok_access.char_pos));
        return NULL;
    }

    enum access_modifier mod = get_access_modifier(tok_access.type);
    struct token_val tok = ACTUAL_TOK();

    if (tok.type == TOK_VAR) {
        struct base_stmt *decl = parse_var(helper);
        if (decl == NULL) {
            return NULL;
        }
        return init_class_property(mod, PROPERTY_MEMBER, decl);
    } else if (tok.type == TOK_IDENTIFIER) {
        struct function *fn = parse_function(helper);
        if (fn == NULL) {
            return NULL;
        }
        return init_class_method(mod, FUNCTION_MEMBER, fn);
    } else {
        add_error(helper->errors, CREATE_ERROR("expected var function or constructor", tok.line, tok.char_pos));
        return NULL;
    }
}

static struct class_decl *parse_class(struct parser_helper *helper) {
    CONSUME_TOK(TOK_CLASS, "expected class");

    ENSURE_SIZE();
    struct token_val class_name = READ_TOK();

    if (class_name.type != TOK_IDENTIFIER) {
        add_error(helper->errors, CREATE_ERROR("expected type", class_name.line, class_name.char_pos));
        return NULL;
    }

    CONSUME_TOK(TOK_LEFT_BRACES, "expected {");

    struct token_val tok = ACTUAL_TOK();

    // methods array
    int members_cnt = 0;
    int members_arrsize = ARRAY_FACTOR;
    struct class_member **members = malloc(members_arrsize * sizeof(struct class_member*));

    while (tok.type == TOK_PRIVATE || tok.type == TOK_PROTECTED || tok.type == TOK_PUBLIC) {
        struct class_member *member = parse_class_member(helper);
        if (member == NULL) {
            return NULL;
        }
        add_toarray(&members, &members_arrsize, &members_cnt, member);
        tok = ACTUAL_TOK();
    }

    CONSUME_TOK(TOK_RIGHT_BRACES, "expected }");
    return init_class(class_name, members_cnt, members);
}

struct prlang_file *parse_prlang_file(struct parser_helper *helper) {
    struct token_val tok = ACTUAL_TOK();
    
    // functions array
    int function_cnt = 0;
    int function_arrsize = ARRAY_FACTOR;
    struct function **functions = malloc(function_arrsize * sizeof(struct function*));

    // globals array
    int global_cnt = 0;
    int global_arrsize = ARRAY_FACTOR;
    struct var_decl **globals = malloc(global_arrsize * sizeof(struct var_decl*));

    // classes array
    int class_cnt = 0;
    int class_arrsize = ARRAY_FACTOR;
    struct class_decl **classes = malloc(class_arrsize * sizeof(struct class_decl*));

    while (tok.type != TOK_EXIT) {
        switch (tok.type) {
            case TOK_IDENTIFIER:
            {
                struct function *fn = parse_function(helper);
                if (fn == NULL) {
                    return NULL;
                }
                add_toarray(&functions, &function_arrsize, &function_cnt, fn);
                break;
            }
            case TOK_VAR:
            {
                struct var_decl *decl = parse_var(helper);
                if (decl == NULL) {
                    return NULL;
                }
                add_toarray(&globals, &global_arrsize, &global_cnt, decl);
                break;
            }
            case TOK_CLASS:
            {
                struct class *klass = parse_class(helper);
                if (klass == NULL) {
                    return NULL;
                }
                add_toarray(&classes, &class_arrsize, &class_cnt, klass);
                break;
            }
            default:
                add_error(helper->errors, CREATE_ERROR("unexpected token", tok.line, tok.char_pos));
                return NULL;
        }
        tok = ACTUAL_TOK();
    }
    
    return init_prlang_file(globals, global_cnt, functions, function_cnt, classes, class_cnt);
}