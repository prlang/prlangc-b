#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdbool.h>

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

struct class_table {
    struct symbol_hash methods;
    struct symbol_hash methods_code;
    struct symbol_hash properties;
};

struct symbol_table {
    // global variables name
    struct symbol_hash global;
    // functions
    struct symbol_hash funcs;
    // classes
    // map <char*, class_table*>
    struct symbol_hash classes;
    // the actual code (TODO: change this to one hashtable)
    struct symbol_hash funcs_code;
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