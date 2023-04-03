#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdbool.h>

#include "ast.h"

#define CREATE_SYMBOL(type, depth) ((struct symbol_val) { type, depth })

// TODO: change this implementation to a "class"

struct symbol_val {
    char *type;
    int offset;
    int arg_pos;
    int positive_offset;
    // this is temporal
    // TODO: change to enum
    char modifier;
    int array;
};

struct bucket {
    bool used;
    int depth;
    int arg;
    char *key;
    void *value;
};

struct symbol_hash {
    int count;
    int capacity;
    struct bucket *values;
};

struct fun_table {
    struct symbol_hash symbols;
    int total_offset;
    int total_pos_offset;
};

struct fun_hash {
    struct fun_table *table;
    struct function *fun;
};

struct fun_member_det {
    struct function *fun;
    struct fun_table *table;
    int offset;
    char modifier;
};

struct class_table {
    struct symbol_hash methods;
    struct symbol_hash properties;
    int size;
    char *class_name;
};

struct symbol_table {
    // global variables name
    struct symbol_hash global;
    // functions
    struct symbol_hash funcs;
    // classes
    // map <char*, class_table*>
    struct symbol_hash classes;
};

struct symbol_table init_symbol_table();
void free_table(struct symbol_table *table);

struct symbol_hash init_symbol_hash();
struct symbol_hash init_symbol_hash_(int capacity, int count);
void free_hash(struct symbol_hash hash);

void *get(struct symbol_hash *hash, char *key, int depth, int arg);
void put(struct symbol_hash *hash, char *key, void *val, int depth, int arg);
bool contains(struct symbol_hash *hash, char *key, int depth, int arg);

#endif