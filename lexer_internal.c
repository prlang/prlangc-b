#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "lexer.h"

#define STRING_FACTOR 2

const char operators[] = {
    '{', '}', '\n', ' ', '\0', '(', ')', '.', ';', '+', '-', '/', '*', '!', '=', '%', '?', ':', ',', '[', ']', '|', '&', '<', '>', '\'', '"'
};

static bool chk_at(const char *str, const char *compare, int *actual) {
    int compare_size = strlen(compare);
    for (int i = 0 ; i < compare_size ; i++) {
        if (str[*actual + i] == compare[i]) {
            continue;
        }
        return false;
    }
    *actual += compare_size;
    return true;
}

static bool is_operator(char c) {
    int sz = sizeof(operators) / sizeof(operators[0]);
    for (int i = 0 ; i < sz ; i++) {
        if (c == operators[i]) {
            return true;
        }
    }
    return false;
}

static struct token_val get_operator(const char *str, int line, int *actual) {
    // TODO: Improve this !!!
    switch (str[*actual]) {
        case '+':
            if (chk_at(str, "++", actual)) {
                return CREATE_TOK(TOK_INCREMENT, "", line, *actual - 2);
            }
            if (chk_at(str, "+=", actual)) {
                return CREATE_TOK(TOK_ADD_ASSIGN, "", line, *actual - 2);
            }
            return CREATE_TOK(TOK_ADD, "", line, (*actual)++);
        case '-':
            if (chk_at(str, "--", actual)) {
                return CREATE_TOK(TOK_DECREMENT, "", line, *actual - 2);
            }
            if (chk_at(str, "-=", actual)) {
                return CREATE_TOK(TOK_SUBSTRACT_ASSIGN, "", line, *actual - 2);
            }
            return CREATE_TOK(TOK_SUBSTRACT, "", line, (*actual)++);
        case '*':
            if (chk_at(str, "*=", actual)) {
                return CREATE_TOK(TOK_MULT_ASSIGN, "", line, *actual - 2);
            }
            return CREATE_TOK(TOK_MULT, "", line, (*actual)++);
        case '/':
            if (chk_at(str, "/=", actual)) {
                return CREATE_TOK(TOK_DIV_ASSIGN, "", line, *actual - 2);
            }
            return CREATE_TOK(TOK_DIV, "", line, (*actual)++);
        case '%':
            if (chk_at(str, "%=", actual)) {
                return CREATE_TOK(TOK_MOD_ASSIGN, "", line, *actual - 2);
            }
            return CREATE_TOK(TOK_MOD, "", line, (*actual)++);
        case '=':
            if (chk_at(str, "==", actual)) {
                return CREATE_TOK(TOK_EQUAL, "", line, *actual - 2);
            }
            return CREATE_TOK(TOK_ASSIGN, "", line, (*actual)++);
        case '!':
            if (chk_at(str, "!=", actual)) {
                return CREATE_TOK(TOK_NOT_EQUAL, "", line, *actual - 2);
            }
            return CREATE_TOK(TOK_NEGATION, "", line, (*actual)++);
        case '<':
            if (chk_at(str, "<=", actual)) {
                return CREATE_TOK(TOK_LESS_EQUAL, "", line, *actual - 2);
            }
            return CREATE_TOK(TOK_LESS, "", line, (*actual)++);
        case '>':
            if (chk_at(str, ">=", actual)) {
                return CREATE_TOK(TOK_GREAT_EQUAL, "", line, *actual - 2);
            }
            return CREATE_TOK(TOK_GREAT, "", line, (*actual)++);
        case '&':
            if (chk_at(str, "&&", actual)) {
                return CREATE_TOK(TOK_AND, "", line, *actual - 2);
            }
            // temporal
            return CREATE_TOK(TOK_AND, "", line, (*actual)++);
        case '|':
            if (chk_at(str, "||", actual)) {
                return CREATE_TOK(TOK_OR, "", line, *actual - 2);
            }
            // temporal
            return CREATE_TOK(TOK_OR, "", line, (*actual)++);
        case '?':
            return CREATE_TOK(TOK_QUESTION, "", line, (*actual)++);
        case ';':
            return CREATE_TOK(TOK_SEMICOLON, "", line, (*actual)++);
        case ':':
            return CREATE_TOK(TOK_COLON, "", line, (*actual)++);
        case ',':
            return CREATE_TOK(TOK_COMMA, "", line, (*actual)++);
        case '.':
            return CREATE_TOK(TOK_DOT, "", line, (*actual)++);
        case '{':
            return CREATE_TOK(TOK_LEFT_BRACES, "", line, (*actual)++);
        case '}':
            return CREATE_TOK(TOK_RIGHT_BRACES, "", line, (*actual)++);
        case '(':
            return CREATE_TOK(TOK_LEFT_PAREN, "", line, (*actual)++);
        case ')':
            return CREATE_TOK(TOK_RIGHT_PAREN, "", line, (*actual)++);
        case '[':
            return CREATE_TOK(TOK_LEFT_BRACKETS, "", line, (*actual)++);
        case ']':
            return CREATE_TOK(TOK_RIGHT_BRACKETS, "", line, (*actual)++);
        default:
            break;
    }
}

static void add_tostr(char **val, int *count, int *arr_size, char c) {
    if (*count >= *arr_size  - 1) {
        *arr_size *= STRING_FACTOR;
        char* tmp = realloc(*val, sizeof(char) * *arr_size);
        if (tmp == NULL) {
            printf("[ERROR] Not enough memory\n");
            exit(1);
        }
        *val = tmp;
    }
    (*val)[(*count)++] = c;
}

static struct token_val get_number(struct lexer *lex, const char *str, int line, int *act) {
    // dynamic char array details
    int count = 0;
    int arr_size = STRING_FACTOR;
    char *val = malloc(sizeof(char) * arr_size);

    bool floating = false;

    if (val == NULL) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }

    while (true) {
        if (str[*act] >= '0' && str[*act] <= '9') {
            add_tostr(&val, &count, &arr_size, str[(*act)++]);
        } else if (str[*act] == '.') {
            if (floating) {
                add_error(lex->errors, CREATE_ERROR("multiple floating point", line, *act - count));
                return CREATE_TOK(TOK_ERROR, "error", line, *act - count);
            }
            add_tostr(&val, &count, &arr_size, str[(*act)++]);
            floating = true;
        } else {
            break;
        }
    }
    val[count] = '\0';
    if (floating) {
        return CREATE_TOK(TOK_FLOAT, val, line, *act - count);
    }
    return CREATE_TOK(TOK_NUMBER, val, line, *act - count);
}

static struct token_val get_identifier(const char *str, int line, int *act) {
    
    int count = 0;
    int arr_size = STRING_FACTOR;
    char *val = malloc(sizeof(char) * arr_size);

    if (val == NULL) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }

    while (!is_operator(str[*act])) {
        add_tostr(&val, &count, &arr_size, str[(*act)++]);
    }

    val[count] = '\0';

    // TODO: add regex

    struct token_val tok_val = get_tok_keyword(val, line, *act - count);

    if (tok_val.type != TOK_IDENTIFIER) {
        free(val);
    }

    return tok_val;
}

static struct token_val get_string(struct lexer* lex, const char *str, int line, int *act, int line_size) {
    // check if is "" or ''
    char type = str[(*act)++];
    int count = 0;
    int arr_size = STRING_FACTOR;
    char *val = malloc(sizeof(char) * arr_size);

    if (val == NULL) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }

    while (str[*act] != type) {
        if (*act >= line_size) {
            add_error(lex->errors, CREATE_ERROR("can't find closing ' or \"", line, *act));
            break;
        }

        if (count >= arr_size - 1) {
            arr_size *= STRING_FACTOR;
            char* tmp = realloc(val, sizeof(char) * arr_size);
            if (tmp == NULL) {
                printf("[ERROR] Not enough memory\n");
                exit(1);
            }
            val = tmp;
        }

        if (chk_at(str, "\\", act)) {
            if (chk_at(str, "\"", act)) {
                val[count++] = '\"';
                continue;  
            }
            else if (chk_at(str, "n",act)) {
                val[count++] = '\n';
                continue;
            }
            else if (chk_at(str, "\\",act)) {
                val[count++] = '\\';
                continue;
            }
        }
        val[count++] = str[(*act)++];
    }

    val[count] = '\0';
    struct token_val tok_val = CREATE_TOK(TOK_STRING, val, line, *act - count);

    (*act)++;
    return tok_val;
}

void tokenize_line(struct lexer *lex, const char *str, int line, bool comment) {
    int act = 0;
    int line_size = strlen(str);
    while (act < line_size) {
        // skip whitespace or new line
        if (str[act] == ' ' || str[act] == '\n') {
            act++;
            continue;
        }
        // skip comment
        else if (str[act] == '#' || chk_at(str, "//", &act)) {
            break;
        }
        // check keywords
        else if ( (str[act] >= 'a' && str[act] <= 'z') || (str[act] >= 'A' && str[act] <= 'Z') ) {
            add_token(lex, get_identifier(str, line, &act));
        } 
        // check numbers
        else if (str[act] >= '0' && str[act] <= '9') {
            add_token(lex, get_number(lex, str, line, &act));
        }
        else if (str[act] == '\"' || str[act] == '\'') {
            add_token(lex, get_string(lex, str, line, &act, line_size));
        }
        // check operator
        else if (is_operator(str[act])) {
            add_token(lex, get_operator(str, line, &act));
        }
        // undefined character encountered, add error
        else {
            add_error(lex->errors, CREATE_ERROR("an undefined character have been encountered", line, act));
            add_token(lex, CREATE_TOK(TOK_ERROR, "error", line, act));
            act++;
        }
    } 
}

void tokenize(struct lexer *lex, struct src_file *file) {

    char *dir = strcat(file->src_loc, "/");
    FILE *file_ptr = fopen(strcat(dir, file->src_name), "r");

    if (file_ptr == NULL) {
        printf("[ERROR] Can't open the specified file, error %s\n", strerror(errno));
        exit(1);
    }

    int count = 0;
    int arr_size = STRING_FACTOR;

    // dynamically grow array
    char *str = malloc(sizeof(char) * arr_size);

    int c;

    while (c = fgetc(file_ptr)) {
        if (feof(file_ptr)) {
            str[count] = '\0';
            // if last char is encountered tokenize last line and free line
            tokenize_line(lex, str, lex->line, false);
            free(str);
            break;
        }
        if (ferror(file_ptr)) {
            printf("[ERROR] An error occurred while reading line");
            exit(1);
        }
        if (count >= arr_size  - 1) {
            arr_size *= STRING_FACTOR;
            char* tmp = realloc(str, sizeof(char) * arr_size);
            if (tmp == NULL) {
                printf("[ERROR] Not enough memory\n");
                exit(1);
            }
            str = tmp;
        }
        if (c != '\n') {
            str[count++] = c;
            continue;
        }
        str[count] = '\0';
        tokenize_line(lex, str, lex->line, false);
        lex->line++;

        // free actual line and malloc the next one
        free(str);
        arr_size = STRING_FACTOR;
        count = 0;
        str = malloc(sizeof(char) * arr_size);
    }

    add_token(lex, CREATE_TOK(TOK_EXIT, "", -1, -1));
    fclose(file_ptr);
}