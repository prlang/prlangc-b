#include <string.h>

#include "token.h"

struct token_val get_tok_keyword(char *val, int line, int char_pos) {
    for (int i = 0 ; i < TOK_EXIT ; i++) {
        if (strcmp(val, tok_to_str(i)) == 0) {
            return CREATE_TOK(i, "", line, char_pos);
        }
    }
    return CREATE_TOK(TOK_IDENTIFIER, val, line, char_pos);
}

char* tok_to_str(enum token_type val) {
    switch (val) {
        case TOK_LEFT_BRACES: return "{";
        case TOK_RIGHT_BRACES: return "}";
        case TOK_LEFT_BRACKETS: return "[";
        case TOK_RIGHT_BRACKETS: return "]";
        case TOK_LEFT_PAREN: return  "(";
        case TOK_RIGHT_PAREN: return ")";
        case TOK_NEGATION: return "!";
        case TOK_ADD: return "+";
        case TOK_ADD_ASSIGN: return "+=";
        case TOK_SUBSTRACT: return "-";
        case TOK_SUBSTRACT_ASSIGN: return "-=";
        case TOK_INCREMENT: return "++";
        case TOK_DECREMENT: return "--";
        case TOK_MULT: return "*";
        case TOK_MULT_ASSIGN: return "*=";
        case TOK_DIV: return "/";
        case TOK_DIV_ASSIGN: return "/=";
        case TOK_MOD: return "%";
        case TOK_MOD_ASSIGN: return "%=";
        case TOK_GREAT: return ">";
        case TOK_GREAT_EQUAL: return ">=";
        case TOK_LESS: return "<";
        case TOK_LESS_EQUAL: return "<=";
        case TOK_AND: return "&&";
        case TOK_OR: return "||";
        case TOK_DOT: return ".";
        case TOK_COMMA: return ",";
        case TOK_SEMICOLON: return ";";
        case TOK_COLON: return ":";
        case TOK_QUESTION: return "?";
        case TOK_NOT_EQUAL: return "!=";
        case TOK_EQUAL: return "==";
        case TOK_ASSIGN: return "=";
        case TOK_FOR: return "for";
        case TOK_FALSE: return "false";
        case TOK_TRUE: return "true";
        case TOK_NULLV: return "null";
        case TOK_FUN: return "fun";
        case TOK_CLASS: return "class";
        case TOK_IF: return "if";
        case TOK_ELSE: return "else";
        case TOK_WHILE: return "while";
        case TOK_PRIVATE: return "private";
        case TOK_PUBLIC: return "public";
        case TOK_PROTECTED: return "protected";
        case TOK_DO: return "do";
        case TOK_VAR: return "var";
        case TOK_RETURN: return "return";
        case TOK_CONSTANT: return "const";
        case TOK_CONTINUE: return "continue";
        case TOK_IMPORT: return "import";
        case TOK_MODULE: return "module";
        case TOK_BREAK: return "break";
        case TOK_NUMBER: return "&number";
        case TOK_FLOAT: return "&float";
        case TOK_IDENTIFIER: return "&identifier";
        case TOK_STRING: return "&string";
        case TOK_ERROR: return "&error";
        case TOK_EXIT: return "&exit";
        default: return "";
    }
}