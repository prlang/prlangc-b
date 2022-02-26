
#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

#define ACTUAL_TOK() (helper->lex->values[helper->index])
#define READ_TOK() (helper->lex->values[helper->index++])
#define PREVIOUS_TOK() (helper->lex->values[helper->index--])
#define READ_TOK_ERR() (helper->lex->values[helper->index - 1])

#define ENSURE_SIZE()                                                                                   \
    do {                                                                                                \
        if (helper->index + 1 >= helper->lex->count) {                                                  \
            if (helper->index == 0) {                                                                   \
                add_error(helper->errors, CREATE_ERROR("no more tokens to read", 0, 0));                \
                return NULL;                                                                            \
            }                                                                                           \
            struct token_val err = READ_TOK_ERR();                                                      \
            add_error(helper->errors, CREATE_ERROR("no more tokens to read", err.line, err.char_pos));  \
            return NULL;                                                                                \
        }                                                                                               \
    } while (0)                                                                                         \

#define CONSUME_TOK(typet, message)                                                                     \
    do {                                                                                                \
        ENSURE_SIZE();                                                                                  \
        if (READ_TOK().type != typet) {                                                                 \
            struct token_val err = READ_TOK_ERR();                                                      \
            add_error(helper->errors, CREATE_ERROR(message, err.line, err.char_pos));                   \
            return NULL;                                                                                \
        }                                                                                               \
    } while (0)                                                                                         \


struct parser_helper {
    int index;
    struct lexer *lex;
    struct errors *errors;
};

/*
 * Pratt Parser
 * based on: https://journal.stuffwithstuff.com/2011/03/19/pratt-parsers-expression-parsing-made-easy/
 */

// precedence from https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Operator_Precedence
enum precedence {
    PREC_INIT,
    PREC_ASSIGNMENT,
    PREC_TERNARY,
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_TERM,
    PREC_FACTOR,
    PREC_PREFIX,
    PREC_POSTFIX,
    PREC_CONSTRUCTOR = 11,
    PREC_CALL = 11,
    PREC_ACCESS = 11 
};

struct parse_infix {
    int precedence;
    struct base_expr *(*parse)(struct parse_infix *infix, struct parser_helper *helper, struct base_expr *node, struct token_val val);
};

struct binary_infix {
    struct parse_infix base;
    bool is_right;
};

struct parse_prefix {
    struct base_expr *(*parse)(struct parse_prefix *prefix, struct parser_helper *helper, struct token_val val);
};

struct parser_helper init_parser_helper(struct lexer *lex, struct errors *errors);

struct prlang_file *parse_prlang_file(struct parser_helper *helper);

#endif