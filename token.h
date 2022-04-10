#ifndef TOKEN_H
#define TOKEN_H

#define CREATE_TOK(tok, src, line, char) ((struct token_val){tok, src, line, char})

enum token_type {
    // Operators

    TOK_ADD,
    TOK_ADD_ASSIGN,
    TOK_SUBSTRACT,
    TOK_SUBSTRACT_ASSIGN,
    TOK_NEGATION,
    TOK_MULT,
    TOK_MULT_ASSIGN,
    TOK_DIV,
    TOK_DIV_ASSIGN,
    TOK_MOD,
    TOK_MOD_ASSIGN,
    TOK_ASSIGN,
    TOK_EQUAL,
    TOK_NOT_EQUAL,
    TOK_AND,
    TOK_OR,
    TOK_LESS,
    TOK_LESS_EQUAL,
    TOK_GREAT,
    TOK_GREAT_EQUAL,
    TOK_DOT,
    TOK_COMMA,
    TOK_COLON,
    TOK_QUESTION,
    TOK_SEMICOLON,
    TOK_INCREMENT,
    TOK_DECREMENT,

    // Parenthesis, brackets, curly braces

    TOK_LEFT_PAREN,
    TOK_RIGHT_PAREN,
    TOK_LEFT_BRACKETS,
    TOK_RIGHT_BRACKETS,
    TOK_LEFT_BRACES,
    TOK_RIGHT_BRACES,

    // Constants

    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_FLOAT,
    TOK_STRING,
    TOK_NULLV,
    TOK_FALSE,
    TOK_TRUE,

    // Keywords

    TOK_FUN,
    TOK_CLASS,
    TOK_IF,
    TOK_ELSE,
    TOK_FOR,
    TOK_WHILE,
    TOK_DO,
    TOK_IMPORT,
    TOK_NEW,
    TOK_MODULE,
    TOK_CONSTANT,
    TOK_CONTINUE,
    TOK_PRIVATE,
    TOK_PUBLIC,
    TOK_ASM,
    TOK_PROTECTED,
    TOK_BREAK,
    TOK_VAR,
    TOK_RETURN,

    // Utils

    TOK_ERROR, // Token created for handle parsing, created when an invalid token exists. 
    TOK_EXIT // Token added to the final of token output

};

struct token_val {
    enum token_type type;
    char *val;
    int line;
    int char_pos;
};

struct token_val get_tok_keyword(char *val, int line, int char_pos);
char* tok_to_str(enum token_type val);

#endif