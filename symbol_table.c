#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "symbol_table.h"

struct symbol_table init_symbol_table() {
    struct symbol_table sym;
    sym.funcs_code = init_symbol_hash();
    sym.classes = init_symbol_hash();
    sym.global = init_symbol_hash();
    sym.funcs = init_symbol_hash();
    return sym;
}

static void free_class_hash(struct class_table *table) {
    for (int i = 0 ; i < table->properties.count ; i++) {
        if (table->properties.values[i].used) {
            free(table->properties.values[i].value);
        }
    }
    free_hash(table->properties);
    free_hash(table->methods);
    free(table);
}

void free_table(struct symbol_table *table) {
    for (int i = 0 ; i < table->classes.count ; i++) {
        if (table->classes.values[i].used) {
            free_class_hash(table->classes.values[i].value);
        }
    }
    for (int i = 0 ; i < table->global.count ; i++) {
        if (table->global.values[i].used) {
            free(table->global.values[i].value);
        }
    }

    free_hash(table->classes);
    free_hash(table->global);
    free_hash(table->funcs);
}

// Implementation from https://github.com/rexim/aids

#define RESIZE_FACTOR 2
#define DEFAULT_CAPACITY 16

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

static void resize_hash(struct symbol_hash *hash);
static struct bucket *init_buckets(int capacity);

// from https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
static unsigned long fnv1_hash(char *key) {
    unsigned long hash = FNV_OFFSET;
    for (const char* p = key; *p; p++) {
        hash ^= (unsigned long)(unsigned char)(*p);
        hash *= FNV_PRIME;
    }
    return hash;
}

struct bucket *init_buckets(int capacity) {
    struct bucket *values = calloc(capacity, sizeof(struct bucket));
    if (!values) {
        printf("[ERROR] Not enough memory\n");
        exit(1);
    }
    return values;
}

struct symbol_hash init_symbol_hash() {
    return init_symbol_hash_(DEFAULT_CAPACITY, 0);
}

struct symbol_hash init_symbol_hash_(int capacity, int count) {
    int cap = capacity == 0 ? DEFAULT_CAPACITY : capacity;
    struct symbol_hash hash;
    hash.capacity = cap;
    hash.count = count;
    hash.values = init_buckets(cap);
    return hash;
}

void free_hash(struct symbol_hash hash) {
    free(hash.values);
}

void *get(struct symbol_hash *hash, char *key, int depth, int arg) {
    if (!hash || !hash->values) {
        return NULL;
    }
    int index = fnv1_hash(key) & (hash->capacity - 1);
    for (int i = 0 ; 
        i < hash->capacity 
        && hash->values[index].used 
        && (strcmp(hash->values[index].key, key) != 0
        || hash->values[index].arg != arg
        || hash->values[index].depth > depth) ; 
        i++) {
            index = (index + 1) & (hash->capacity - 1);
    }
    if (hash->values[index].used 
        && strcmp(hash->values[index].key, key) == 0 
        && hash->values[index].arg == arg 
        && hash->values[index].depth <= depth) {
        return hash->values[index].value;
    }
    return NULL;
}

void put(struct symbol_hash *hash, char *key, void *val, int depth, int arg) {
    if (!hash || !hash->values) {
        return;
    }
    if (hash->count >= hash->capacity) {
        resize_hash(hash);
    }

    int index = fnv1_hash(key) & (hash->capacity - 1);
    while (hash->values[index].used && (
                strcmp(hash->values[index].key, key) != 0 
                || hash->values[index].arg != arg 
                || hash->values[index].depth != depth)) {

        index = (index + 1) & (hash->capacity - 1);
    }

    hash->values[index].key = key;
    hash->values[index].value = val;
    hash->values[index].used = true;
    hash->values[index].depth = depth;
    hash->values[index].arg = arg;
    hash->count++;
}

bool contains(struct symbol_hash *hash, char *key, int depth, int arg) {
    void *val = get(hash, key, depth, arg);
    return val != NULL;
}

static void resize_hash(struct symbol_hash *hash) {
    if (!hash->values) {
        hash->values = init_buckets(DEFAULT_CAPACITY);
        hash->capacity = DEFAULT_CAPACITY;
        hash->count = 0;
    } else {
        struct symbol_hash new_val = init_symbol_hash_(hash->capacity * RESIZE_FACTOR, 0);
        for (int i = 0 ; i < hash->capacity ; i++) {
            struct bucket val = hash->values[i];
            if (val.used) {
                put(&new_val, val.key, val.value, val.depth, val.arg);
            }
        }
        free_hash(*hash);
        hash->capacity = new_val.capacity;
        hash->count = new_val.count;
        hash->values = new_val.values;
    }
}
