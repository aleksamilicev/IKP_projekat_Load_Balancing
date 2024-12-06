#include "DynamicArray.h"
#include "../Worker/Worker.cpp"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void initialize_dynamic_array(DynamicArray* da, size_t initial_capacity) {
    da->array = (Worker*)malloc(initial_capacity * sizeof(Worker));
    if (da->array == NULL) {
        printf("Failed to allocate memory for DynamicArray.\n");
        exit(EXIT_FAILURE); // Ili neki drugi naèin da signalizirate grešku
    }
    da->size = 0;
    da->capacity = initial_capacity;
}


void copy_worker(Worker* dest, const Worker* src) {
    initialize_worker(dest, src->ID, src->IP, src->Port, src->Status);
    for (int i = 0; i < src->message_count; ++i) {
        add_message(dest, src->Data[i]); // Kopiraj svaku poruku
    }
}

void add_worker(DynamicArray* da, const Worker* worker) {
    if (da->size >= da->capacity) {
        Worker* temp = (Worker*)realloc(da->array, da->capacity * 2 * sizeof(Worker));
        if (temp == NULL) {
            printf("Memory reallocation failed.\n");
            return;
        }
        da->array = temp;
        da->capacity *= 2;
    }
    copy_worker(&da->array[da->size], worker); // Duboka kopija Workera
    da->size++;
}



Worker* get_worker(const DynamicArray* da, size_t index) {
    if (index < da->size) {
        return &da->array[index];
    }
    printf("Invalid worker index: %zu\n", index);
    return NULL;
}


void remove_worker(DynamicArray* da, size_t index) {
    if (index < da->size) {
        free_worker_messages(&da->array[index]); // Oslobaðanje resursa
        for (size_t i = index; i < da->size - 1; i++) {
            da->array[i] = da->array[i + 1];
        }
        da->size--;
    }
}


void clear_dynamic_array(DynamicArray* da) {
    if (da->array != NULL) {
        free(da->array);
        da->array = NULL;
    }
    da->size = 0;
    da->capacity = 0;
}

void display_dynamic_array(const DynamicArray* da) {
    printf("Registered Workers (%zu):\n", da->size);
    for (size_t i = 0; i < da->size; i++) {
        display_worker(&da->array[i]);
    }
}
