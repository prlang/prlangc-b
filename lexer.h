#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include "error.h"
#include "src_file.h"

struct lexer {
    struct errors *errors;
    int line;

    // dynamic array details
    int count;
    int capacity;
    struct token_val *values;
};

struct lexer init_lex(struct errors *errors);
void free_lex(struct lexer *lex);
void add_token(struct lexer *lex, struct token_val tok);
void debug_lexer(struct lexer *lex);

// tokenize a file
void tokenize(struct lexer *lex, struct src_file *file);

// tokenize a line
void tokenize_line(struct lexer *lex, const char *str, int line, bool comment);

#endif