#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "compiler.h"
#include "codegenx86.h"
#include "lexer.h"
#include "ast.h"
#include "parser.h"
#include "sema_analysis.h"

void compile_dir_recursive(char *src_dir, char* src_name, struct compiler_opt options) {

}

void compile(char* src_dir, char* src_name, struct compiler_opt options) {

    printf("[INFO] Initiating compilation...\n");

    struct errors errors = init_errors(options.max_errors);

    char cpy[strlen(src_dir)];
    cpy[strlen(src_dir) - 1] = '\0';
    strncpy(cpy, src_dir, strlen(src_dir));

    struct src_file file = { cpy, src_name };

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


    struct symbol_table table = init_symbol_table();
    struct semantic_context ctx = init_semantic(&table, &errors);

    printf("[INFO] Checking types\n");

    prlang_semapass(root, &ctx);

    if (errors.count > 0) {
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

    char output_name[strlen(src_name) + 2];
    output_name[strlen(src_name) + 1] = '\0';
    strncpy(output_name, src_name, strlen(src_name) - 2);
    output_name[strlen(src_name) - 2] = 'a';
    output_name[strlen(src_name) - 1] = 's';
    output_name[strlen(src_name)] = 'm';

    struct prlang_codegen gen = init_codegen(src_dir, output_name, &ctx); 
    prlang_codegen(root, &gen);

    printf("[INFO] Finished assembly generation\n");
    printf("%s\n", src_name);
    printf("%s\n", output_name);

    free_codegen(gen);
    free_prlang(root);
    free_lex(&lex);
    free_errors(&errors);
}