#include "DynamicArray.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void initialize_dynamic_array(DynamicArray* da, size_t initial_capacity) {
    da->array = (Worker*)malloc(initial_capacity * sizeof(Worker));
    da->size = 0;
    da->capacity = initial_capacity;
}

void add_worker(DynamicArray* da, const Worker* worker) {
    if (da->size >= da->capacity) {
        da->capacity *= 2;
        da->array = (Worker*)realloc(da->array, da->capacity * sizeof(Worker));
    }
    da->array[da->size++] = *worker;
}

Worker* get_worker(const DynamicArray* da, size_t index) {
    if (index < da->size) {
        return &da->array[index];
    }
    return NULL;
}

void remove_worker(DynamicArray* da, size_t index) {
    if (index < da->size) {
        for (size_t i = index; i < da->size - 1; i++) {
            da->array[i] = da->array[i + 1];
        }
        da->size--;
    }
}

void clear_dynamic_array(DynamicArray* da) {
    free(da->array);
    da->array = NULL;
    da->size = 0;
    da->capacity = 0;
}

void display_dynamic_array(const DynamicArray* da) {
    for (size_t i = 0; i < da->size; i++) {
        display_worker(&da->array[i]);
    }
}
