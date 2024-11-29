#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include "../Worker/Worker.h"
#include <stddef.h>

typedef struct {
    Worker* array;
    size_t size;
    size_t capacity;
} DynamicArray;

void initialize_dynamic_array(DynamicArray* da, size_t initial_capacity);
void add_worker(DynamicArray* da, const Worker* worker);
Worker* get_worker(const DynamicArray* da, size_t index);
void remove_worker(DynamicArray* da, size_t index);
void clear_dynamic_array(DynamicArray* da);
void display_dynamic_array(const DynamicArray* da);

#endif // DYNAMIC_ARRAY_H
