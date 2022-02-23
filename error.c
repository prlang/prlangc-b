#include <stdlib.h>
#include <stdio.h>

#include "error.h"

struct errors init_errors(int max_capacity) {
    struct errors errors;
    errors.count = 0;
    errors.max_capacity = max_capacity;
    errors.values = malloc(sizeof(struct error) * max_capacity);
    return errors;
}

void free_errors(struct errors *errors) {
    free(errors->values);
    errors->count = 0;
    errors->max_capacity = 0;
}

void add_error(struct errors *errors, struct error err) {
    if (errors->count >= errors->max_capacity) {
        errors->count++;
        return;
    }
    errors->values[errors->count++] = err;
}

void print_errors(struct errors *errors) {
    int size = errors->count > errors->max_capacity ? errors->max_capacity : errors->count;
    for (int i = 0 ; i < size; i++) {
        struct error val = errors->values[i];
        printf("[ERROR] At line %d, %d > %s\n", val.line, val.char_pos + 1, val.message);
    }
}