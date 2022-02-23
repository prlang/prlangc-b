#include <stdlib.h>
#include <stdio.h>

#include "lexer.h"

struct lexer init_lex(struct errors *errors) {
    struct lexer lex;
    lex.errors = errors;
    lex.capacity = 0;
    lex.count = 0;
    lex.line = 1;
    lex.values = NULL;
    return lex;
}

void free_lex(struct lexer *lex) {
    free(lex->values);
}

void add_token(struct lexer *lex, struct token_val tok) {
    if (lex->count >= lex->capacity) {
        lex->capacity = lex->capacity == 0 ? 8 : lex->capacity * 2;
        struct token_val *tmp = realloc(lex->values, lex->capacity * sizeof(struct token_val));
        if (tmp == NULL) {
            printf("[ERROR] Not enough memory\n");
            exit(1);
        }
        lex->values = tmp;
    }
    lex->values[lex->count++] = tok;
}

void debug_lexer(struct lexer *lex) {
    printf("Tokens: \n");
    for (int i = 0 ; i < lex->count ; i++) {
        printf("char pos: %d, line %d, val %s, type %s\n", lex->values[i].char_pos, lex->values[i].line, lex->values[i].val, tok_to_str(lex->values[i].type));
    }
}