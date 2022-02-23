#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>

#define CREATE_ERROR(message, line, char) ((struct error){message, line, char})

struct error {
    char* message;
    int line;
    int char_pos;
};

struct errors {
    // the max errors that can contains
    int max_capacity;

    int count;
    struct error *values;
};

struct errors init_errors(int max_capacity);
void free_errors(struct errors *errors);

void add_error(struct errors *errors, struct error err);
void print_errors(struct errors *errors);

#endif