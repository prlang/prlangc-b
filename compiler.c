#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "compiler.h"
#include "lexer.h"
#include "ast.h"
#include "parser.h"

void compile_dir_recursive(char *src_dir, char* src_name, struct compiler_opt options) {

}

void compile(char* src_dir, char* src_name, struct compiler_opt options) {

    printf("[INFO] Initiating compilation...\n");

    struct errors errors = init_errors(options.max_errors);
    struct src_file file = { src_dir, src_name };

    // tokenize input
    printf("[INFO] Tokenizing characters\n");

    struct lexer lex = init_lex(&errors);
    tokenize(&lex, &file);
    debug_lexer(&lex);

    printf("[INFO] Creating AST\n");

    struct parser_helper helper = init_parser_helper(&lex, &errors);
    struct prlang_file *root = parse_prlang_file(&helper);

    if (root == NULL || errors.count > 0) {
        print_errors(&errors);
        exit(1);
    }

    printf("[INFO] Finished checking\n");

    if (options.debug) {
        printf("[INFO] Debugging elements:\n");
        
        printf("\nLexer Debug:\n");
        debug_lexer(&lex);

        printf("\nAST Debug:\n");
        char prefix[DEBUG_LIMIT];
        prefix[0] = '\0';
        struct astnode_debug debug = init_debug(prefix);
        debug_prlang(root, &debug);
    }

    free_prlang(root);
    free_lex(&lex);
    free_errors(&errors);
}