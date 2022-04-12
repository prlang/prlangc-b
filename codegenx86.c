#include "codegenx86.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct prlang_codegen init_codegen(char *dir, char *filename, struct semantic_context *ctx) {
    dir = strcat(dir, "/");
    FILE *file_ptr = fopen(strcat(dir, filename), "w");

    if (file_ptr == NULL) {
        printf("[ERROR] Can't open the specified file, error %s\n", strerror(errno));
        exit(1);
    }
    
    struct prlang_codegen codegen;
    codegen.import = false;
    codegen.ctx = ctx;
    codegen.file = file_ptr;
    codegen.depth = 0;
    codegen.act_label = 0;
    codegen.if_cond = false;
    codegen.loop_cond = false;
    return codegen;
}

void free_codegen(struct prlang_codegen gen) {
    fclose(gen.file);
}

const char *arg_reg_x64[] = {
    "rdi",
    "rsi",
    "rdx",
    "rcx",
    "r8",
    "r9",
    "rax"
};

const char *arg_reg_x32[] = {
    "edi",
    "esi",
    "edx",
    "ecx",
    "r8d",
    "r9d",
    "eax"
};

bool free_regs[] = {
    true,
    true,
    true,
    true,
    true,
    true,
    true
};

int actual_label = 0;

int get_free_reg() {
    for (int i = 0 ; i < 7 ; i++) {
        if (free_regs[i]) {
            free_regs[i] = false;
            return i;
        }
    }
    printf("FATAL ERROR\n");
}

int codegen_expr(struct base_expr *node, struct prlang_codegen *gen);

static char *get_reverse_jmp_ins(enum token_type type) {
    if (type == TOK_EQUAL) {
        return "jne";
    } else if (type == TOK_NOT_EQUAL) {
        return "je";
    } else if (type == TOK_LESS) {
        return "jge";
    } else if (type == TOK_GREAT) {
        return "jle";
    }
    return "";
}

static char *get_jmp_ins(enum token_type type) {
    if (type == TOK_EQUAL) {
        return "je";
    } else if (type == TOK_NOT_EQUAL) {
        return "jne";
    } else if (type == TOK_LESS) {
        return "jl";
    } else if (type == TOK_GREAT) {
        return "jg";
    }
    return "";
}

struct arr_gen_ret {
    int reg;
    int size;
};

static struct arr_gen_ret codegen_array_access(struct array_access *node, struct prlang_codegen *gen, bool assign) {
    int base = codegen_expr(node->name, gen);
    fprintf(gen->file,
        "   ; init array access\n"
    );
    for (size_t i = 0 ; i < node->arg_count ; i++) {
        if (i + 1 == node->arg_count) {
            int reg = codegen_expr(node->arguments[i], gen);
            int new_reg = get_free_reg();

            int size = 0;

            if (node->name->type == EXPR_LITERAL) {
                struct symbol_val *sym = get(&gen->ctx->actual_fun->symbols, node->name->as.val.val, gen->depth, 0);
                if (sym->array > node->arg_count) {
                    size = 8;
                } else {
                    size = get_offset(gen->ctx, sym->type);
                }
            }

            fprintf(gen->file,
                "   imul %s, %d\n",
                arg_reg_x32[reg], size 
            );

            fprintf(gen->file,
                "   add %s, 4\n",
                arg_reg_x32[reg] 
            );

            if (assign) {
                fprintf(gen->file,
                    "   lea %s, [%s + %s]\n",
                    arg_reg_x64[new_reg], arg_reg_x64[base], arg_reg_x64[reg]
                );
                fprintf(gen->file,
                    "   push %s\n",
                    arg_reg_x64[new_reg]
                );
            } else {
                fprintf(gen->file,
                    "   mov %s, [%s + %s]\n",
                    arg_reg_x32[new_reg], arg_reg_x64[base], arg_reg_x64[reg]
                );
            }

            free_regs[reg] = true;
            free_regs[base] = true;
            fprintf(gen->file,
                "   ; end array access\n"
            );

            struct arr_gen_ret ret = {new_reg, size};
            return ret;
        }

        int reg = codegen_expr(node->arguments[i], gen);
        int new_reg = get_free_reg();

        fprintf(gen->file,
            "   imul %s, 8\n",
            arg_reg_x32[reg] 
        );

        fprintf(gen->file,
            "   add %s, 4\n",
            arg_reg_x32[reg] 
        );

        fprintf(gen->file,
            "   mov %s, [%s + %s]\n",
            arg_reg_x64[new_reg], arg_reg_x64[base], arg_reg_x64[reg]
        );

        free_regs[reg] = true;

        if (i != 0) {
            free_regs[base] = true;
        }

        base = new_reg;
    }

    free_regs[base] = true;
}

static int codegen_binary_expr(struct binary_expr *node, struct prlang_codegen *gen) {
    if (node->op.type == TOK_ASSIGN) {
        int reg = 0;
        if (node->left->type == EXPR_LITERAL) {
            struct token_val tmp_lit = node->left->as.val;
            struct symbol_val *sym = get(&gen->ctx->actual_fun->symbols, tmp_lit.val, gen->depth, 0);
            reg = codegen_expr(node->right, gen);
            fprintf(gen->file,
                "   mov DWORD [rbp - %d], %s\n",
                sym->offset, arg_reg_x32[reg]
            );
        } else if (node->left->type == EXPR_ARRAY) {
            struct array_access *tmp_arr = node->left->as.arr;
            struct arr_gen_ret arr_gen = codegen_array_access(tmp_arr, gen, true);
            reg = codegen_expr(node->right, gen);
            fprintf(gen->file,
                "   pop %s\n",
                arg_reg_x64[arr_gen.reg]
            );
            fprintf(gen->file,
                "   mov %s [%s], %s\n",
                arr_gen.size == 8 ? "QWORD" : "DWORD" , arg_reg_x64[arr_gen.reg], arr_gen.size == 8 ? arg_reg_x64[reg] : arg_reg_x32[reg]
            );
            free_regs[arr_gen.reg] = true;
        }
        return reg;
    } else if (node->op.type == TOK_DOT) {
        int reg = 0;
        // special cases for array length property access
        if (node->left->type == EXPR_ARRAY) {
            struct array_access *tmp_arr = node->left->as.arr;
            struct arr_gen_ret arr_gen = codegen_array_access(tmp_arr, gen, false);
            reg = get_free_reg();
            fprintf(gen->file,
                "   mov %s, DWORD [%s]\n",
                arg_reg_x32[reg], arg_reg_x64[arr_gen.reg]
            );

            free_regs[arr_gen.reg] = true;

        } else if (node->left->type == EXPR_LITERAL) {
            struct token_val tmp_lit = node->left->as.val;
            struct symbol_val *sym = get(&gen->ctx->actual_fun->symbols, tmp_lit.val, gen->depth, 0);
            if (sym->array > 0) {
                reg = get_free_reg();
                fprintf(gen->file,
                    "   mov %s, QWORD [rbp - %d]\n",
                    arg_reg_x64[reg], sym->offset
                );

                fprintf(gen->file,
                    "   mov %s, DWORD [%s]\n",
                    arg_reg_x32[reg], arg_reg_x64[reg]
                );
            }
        }
        return reg;
    }
    int left = codegen_expr(node->left, gen);
    int right = codegen_expr(node->right, gen);
    if (node->op.type == TOK_ADD) {
        fprintf(gen->file,
            "   add %s, %s\n",
            arg_reg_x32[left], arg_reg_x32[right]
        );
    } else if (node->op.type == TOK_MULT) {
        fprintf(gen->file,
            "   imul %s, %s\n",
            arg_reg_x32[left], arg_reg_x32[right]
        );
    } else if (node->op.type == TOK_SUBSTRACT) {
        fprintf(gen->file,
            "   sub %s, %s\n",
            arg_reg_x32[left], arg_reg_x32[right]
        );
    } else if (node->op.type == TOK_EQUAL 
                || node->op.type == TOK_NOT_EQUAL 
                || node->op.type == TOK_LESS 
                || node->op.type == TOK_GREAT) {

        fprintf(gen->file,
            "   cmp %s, %s\n",
            arg_reg_x32[left], arg_reg_x32[right]
        );
        if (gen->if_cond) {
            fprintf(gen->file,
                "   %s .L%d\n",
                get_reverse_jmp_ins(node->op.type), gen->act_label
            );
            free_regs[left] = true;
        } else if (gen->loop_cond) {
            fprintf(gen->file,
                "   %s .L%d\n",
                get_jmp_ins(node->op.type), gen->act_label
            );
            free_regs[left] = true;
        }
    }
    free_regs[right] = true;
    return left;
}

int codegen_array_cons(struct array_access *node, struct prlang_codegen *gen) {
    // TODO: this is temporal
    fprintf(gen->file,
        "   ; init array constructor\n"
    );
    int size = node->arg_count > 1 ? 8 : 4;
    int reg = codegen_expr(node->arguments[0], gen);
    fprintf(gen->file,
        "   push %s\n"
        "   mov edi, %s\n"
        "   imul edi, %d\n"
        "   add edi, 4\n"
        "   call alloc_lang\n"
        "   pop rsi\n"
        "   mov [rax], esi\n",
        arg_reg_x64[reg], arg_reg_x32[reg], size
    );

    fprintf(gen->file,
        "   ; end array constructor\n"
    );

    free_regs[reg] = true;
    return 6;
}

int codegen_cons(struct base_expr *node, struct prlang_codegen *gen) {
    if (node->type == EXPR_ARRAY) {
        return codegen_array_cons(node->as.arr, gen);
    }
}

int codegen_literal(struct token_val node, struct prlang_codegen *gen) {
    int free_reg = get_free_reg();
    if (node.type == TOK_IDENTIFIER) {
        struct symbol_val *sym = get(&gen->ctx->actual_fun->symbols, node.val, gen->depth, 0);
        if (sym->arg_pos < 6) {
            fprintf(gen->file,
                "   mov %s, DWORD [rbp - %d]\n",
                arg_reg_x32[free_reg], sym->offset
            );
        } else {
            fprintf(gen->file,
                "   mov %s, DWORD [rbp + %d]\n",
                arg_reg_x32[free_reg], sym->positive_offset
            );
        }
    } else if (node.type == TOK_NUMBER) {
        fprintf(gen->file,
            "   mov %s, %s\n",
            arg_reg_x32[free_reg], node.val
        );
    }
    return free_reg;
}

int codegen_call(struct call_expr *node, struct prlang_codegen *gen) {
    if (node->arg_count != 0) {
        for (size_t i = node->arg_count - 1; i >= 0 ; i--) {
            int reg = codegen_expr(node->arguments[i], gen);
        
            fprintf(gen->file,
                "   push %s\n",
                arg_reg_x64[reg]
            );

            free_regs[reg] = true;

            if (i == 0) break;
            // fprintf(gen->file,
            //    "   mov %s, %s\n",
            //    arg_reg_x32[i], arg_reg_x32[reg]
            // );
        }
        for (size_t i = 0 ; i < node->arg_count ; i++) {
            if (i < 6) {
                fprintf(gen->file,
                    "   pop %s\n",
                    arg_reg_x64[i]
                );
                free_regs[i] = true;
            }
        }
    }
    if (node->val->type == EXPR_LITERAL) {
        struct fun_hash *hash = get(&gen->ctx->table->funcs, node->val->as.val.val, 0, node->arg_count);
        struct fun_table *table = hash->table;
        if (table->total_pos_offset != 0) {
            fprintf(gen->file,
                "   call %s\n"
                "   add rsp, %d\n", 
                node->val->as.val.val, table->total_pos_offset
            );
        } else {
            fprintf(gen->file,
                "   call %s\n",
                node->val->as.val.val
            );
        }
    }
    // call return eax register
    return 6;
}

int codegen_unary(struct unary_expr *node, struct prlang_codegen *gen) {
    switch (node->op.type) {
        case TOK_DECREMENT:
        case TOK_INCREMENT:
        {
            if (node->val->type == EXPR_LITERAL) {
                int free_reg = get_free_reg();
                struct token_val tmp_lit = node->val->as.val;
                struct symbol_val *sym = get(&gen->ctx->actual_fun->symbols, tmp_lit.val, gen->depth, 0);
                // example : ++variable | --variable;
                if (node->ut == UT_PREFIX) {
                    fprintf(gen->file,
                        "   %s DWORD [rbp - %d], 1\n",
                        node->op.type == TOK_DECREMENT ? "sub" : "add" ,sym->offset
                    );
                    fprintf(gen->file,
                        "   mov %s, DWORD [rbp - %d]\n",
                        arg_reg_x32[free_reg], sym->offset
                    );
                // example : variable++;
                } else {
                    fprintf(gen->file,
                        "   mov %s, DWORD [rbp - %d]\n",
                        arg_reg_x32[free_reg], sym->offset
                    );
                    fprintf(gen->file,
                        "   %s DWORD [rbp - %d], 1\n",
                        node->op.type == TOK_DECREMENT ? "sub" : "add", sym->offset
                    );
                }
                return free_reg;
            }
            break;
        }
        case TOK_NEGATION:
            break;
        case TOK_SUBSTRACT:
            break;
        default:
            break;
    }
    return 0;
}

int codegen_expr(struct base_expr *node, struct prlang_codegen *gen) {
    switch (node->type) {
        case EXPR_BINARY:
            return codegen_binary_expr(node->as.binary, gen);
        case EXPR_LITERAL:
            return codegen_literal(node->as.val, gen);
        case EXPR_CALL:
            return codegen_call(node->as.call, gen);
        case EXPR_UNARY:
            return codegen_unary(node->as.unary, gen);
        case EXPR_CONS:
            return codegen_cons(node->as.cons, gen);
        case EXPR_ARRAY:
            return codegen_array_access(node->as.arr, gen, false).reg;
        default:
            break;
    }
}

void codegen_var(struct var_decl *node, struct prlang_codegen *gen) {
    if (node->val->type == EXPR_BINARY) {
        struct binary_expr *tmp = node->val->as.binary;
        char *name = tmp->left->as.val.val;
        struct symbol_val *sym = (struct symbol_val*) get(&gen->ctx->actual_fun->symbols, name, gen->depth, 0);
        if (!sym) {
            printf("unexpected error\n");
            exit(1);
        }
        if (tmp->right->type == EXPR_LITERAL) {
            // only int now XD
            struct token_val val = tmp->right->as.val;
            if (val.type == TOK_IDENTIFIER) {
                int reg = codegen_literal(val, gen);
                fprintf(gen->file, 
                    "   mov %s [rbp - %d], %s\n", 
                    get_offset(gen->ctx, sym->type) == 8 ? "QWORD" : "DWORD", sym->offset, arg_reg_x32[reg]
                );
            } else if (val.type == TOK_NUMBER) {
                fprintf(gen->file, 
                    "   mov DWORD [rbp - %d], %s\n", 
                    sym->offset, val.val
                );
            }
            return;
        }

        int reg = codegen_expr(tmp->right, gen);
        
        if (tmp->right->type == EXPR_CONS) {
            fprintf(gen->file, 
                "   mov QWORD [rbp - %d], %s\n", 
                sym->offset, arg_reg_x64[reg]
            );
        } else {
            fprintf(gen->file, 
                "   mov DWORD [rbp - %d], %s\n", 
                sym->offset, arg_reg_x32[reg]
            );
        }

        free_regs[reg] = true;
    }
}

void codegen_block(struct block *node, struct prlang_codegen *gen);

void codegen_if(struct if_stmt *node, struct prlang_codegen *gen) {
    int endif_label = actual_label++;
    int next_label = actual_label++;
    for (size_t i = 0 ; i < node->body_count ; i++) {
        if (i != 0) {
            fprintf(gen->file, 
                ".L%d:\n", 
                next_label
            );
            next_label = actual_label++;
        }
        if (i < node->cond_count) {
            gen->if_cond = true;
            if (i + 1 >= node->cond_count) {
                gen->act_label = endif_label;
                codegen_expr(node->cond[i], gen);
            } else {
                gen->act_label = next_label;
                codegen_expr(node->cond[i], gen);
            }
            gen->if_cond = false;
        }
        codegen_block(node->blocks[i], gen);
    }
    fprintf(gen->file, 
        ".L%d:\n", 
        endif_label
    );
}

void codegen_while(struct while_stmt *node, struct prlang_codegen *gen) {
    int cond_label = actual_label++;
    int body_label = actual_label++;
    int endwhile_label = actual_label++;
    // jump to while
    fprintf(gen->file, 
        "   jmp .L%d\n", 
        cond_label
    );

    fprintf(gen->file, 
        ".L%d:\n", 
        cond_label
    );

    gen->loop_cond = true;
    gen->act_label = body_label;
    codegen_expr(node->cond, gen);
    gen->loop_cond = false;
    fprintf(gen->file, 
        "   jmp .L%d\n", 
        endwhile_label
    );

    fprintf(gen->file, 
        ".L%d:\n", 
        body_label
    );

    codegen_block(node->body, gen);
    // jump to while cond
    fprintf(gen->file, 
        "   jmp .L%d\n", 
        cond_label
    );
    // end label
    fprintf(gen->file, 
        ".L%d:\n", 
        endwhile_label
    );
}

void codegen_return(struct return_stmt *node, struct prlang_codegen *gen) {
    if (node->val) {
        int reg = codegen_expr(node->val, gen);
        if (reg != 6) {
            fprintf(gen->file,
                "   mov eax, %s\n",
                arg_reg_x32[reg]
            );
        }
    }
    fprintf(gen->file,
        "   nop\n"
        "   leave\n"
        "   ret\n"
    );
}

void codegen_for(struct for_stmt *node, struct prlang_codegen *gen) {
    int cond_label = actual_label++;
    int body_label = actual_label++;
    int endfor_label = actual_label++;

    if (node->left) {
        if (node->left_stmt) {
            codegen_var( ((struct base_stmt*) node->left)->as.var, gen);
        } else {
            int reg = codegen_expr(node->left, gen);
            free_regs[reg] = true;
        }
    }

    fprintf(gen->file, 
        "   jmp .L%d\n", 
        cond_label
    );

    fprintf(gen->file, 
        ".L%d:\n", 
        cond_label
    );

    if (node->med) {
        gen->loop_cond = true;
        gen->act_label = body_label;
        int reg = codegen_expr(node->med, gen);
        free_regs[reg] = true;
        gen->loop_cond = false;
    }
    fprintf(gen->file, 
        "   jmp .L%d\n", 
        endfor_label
    );

    fprintf(gen->file, 
        ".L%d:\n", 
        body_label
    );

    codegen_block(node->body, gen);
    if (node->right) {
        int reg = codegen_expr(node->right, gen);
        free_regs[reg] = true;
    }
    // jump to for cond
    fprintf(gen->file, 
        "   jmp .L%d\n", 
        cond_label
    );
    // end label
    fprintf(gen->file, 
        ".L%d:\n", 
        endfor_label
    );
}

void codegen_asm(struct asm_stmt *node, struct prlang_codegen *gen) {
    for (size_t i = 0 ; i < node->count ; i++) {
        fprintf(gen->file, 
            "%s\n", 
            node->code[i]
        );
    }
}

void codegen_stmt(struct base_stmt *node, struct prlang_codegen *gen) {
    switch (node->type) {
        case STMT_VAR:
            codegen_var(node->as.var, gen);
            break;
        case STMT_EXPR:
        {
            int expr = codegen_expr(node->as.expr, gen);
            free_regs[expr] = true;
            break;
        }
        case STMT_ASM:
            codegen_asm(node->as.asms, gen);
            break;
        case STMT_RETURN:
            codegen_return(node->as.rs, gen);
            break;
        case STMT_IF:
            codegen_if(node->as.is, gen);
            break;
        case STMT_WHILE:
            codegen_while(node->as.ws, gen);
            break;
        case STMT_BLOCK:
            codegen_block(node->as.bl, gen);
            break;
        case STMT_FOR:
            codegen_for(node->as.fs, gen);
            break;
        default:
            break;
    }
}

void codegen_block(struct block *node, struct prlang_codegen *gen) {
    gen->depth++;
    for (size_t i = 0 ; i < node->stmts_count ; i++) {
        codegen_stmt(node->statements[i], gen);
    }
    gen->depth--;
}

void codegen_fun(struct function *node, struct prlang_codegen *gen) {
    fprintf(gen->file, 
        "%s%s\n",
        node->name.val, ":"
    );

    fprintf(gen->file,
        "   push rbp\n"
        "   mov rbp, rsp\n"
    );

    struct fun_hash *hash = get(&gen->ctx->table->funcs, node->name.val, 0, node->arg_count);
    gen->ctx->actual_fun = hash->table;

    fprintf(gen->file,
        "   sub rsp, %d\n",
        gen->ctx->actual_fun->total_offset
    );

    for (int i = 0 ; i < node->arg_count ; i++) {
        struct symbol_val *sym = get(&gen->ctx->actual_fun->symbols, node->arguments[i].argument.val, 0, 0);
        if (i < 6) {
            fprintf(gen->file,
                "   mov %s [rbp - %d], %s\n",
                sym->offset == 8 ? "QWORD" : "DWORD", sym->offset, sym->offset == 8 ? arg_reg_x64[i] : arg_reg_x32[i]
            );
        }
    }

    codegen_block(node->block, gen);
}

void prlang_codegen(struct prlang_file *node, struct prlang_codegen *gen) {
    if (!gen->import) {
        fprintf(gen->file,
            "section .text\n"
            "   global _start\n"
            "extern alloc_lang\n"
        );
        // TODO GENERATE EXTERNS

        fprintf(gen->file, 
            "; entry point\n"
            "_start:\n"
            "   push rbp\n"
            "   mov rbp, rsp\n"
            "   mov rdi, [rbp + 8]\n"
            "   mov rsi, [rbp + 12]\n"
        );

        if (contains(&gen->ctx->table->funcs, "main", 0, 2)) {
            fprintf(gen->file, 
                "   ; calls main function\n"
                "   call main\n"
            );
        }

        fprintf(gen->file, 
            "   ; exit\n"
            "   mov rdi, 0\n"
            "   mov rax, 60 ; syscall name\n"
            "   syscall\n"
        );
    }

    for (int i = 0 ; i < node->function_cnt ; i++) {
        codegen_fun(node->functions[i], gen);
    }
}