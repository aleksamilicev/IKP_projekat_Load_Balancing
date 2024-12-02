#include "CircularBuffer.h"
#include <stdlib.h>
#include <string.h>

// Inicijalizacija kružnog buffera
void cb_initialize(CircularBuffer* cb, size_t capacity) {
    cb->buffer = (void**)malloc(capacity * sizeof(void*));
    cb->capacity = capacity;
    cb->head = 0;
    cb->tail = 0;
    cb->size = 0;
}

// Oslobaðanje resursa buffera
void cb_destroy(CircularBuffer* cb) {
    free(cb->buffer);
    cb->buffer = NULL;
    cb->capacity = 0;
    cb->head = 0;
    cb->tail = 0;
    cb->size = 0;
}

// Provera da li je buffer pun
bool cb_is_full(const CircularBuffer* cb) {
    return cb->size == cb->capacity;
}

// Provera da li je buffer prazan
bool cb_is_empty(const CircularBuffer* cb) {
    return cb->size == 0;
}

// Dodavanje elementa u buffer
bool cb_push(CircularBuffer* cb, void* item) {
    if (cb_is_full(cb)) {
        return false;  // Buffer je pun
    }
    cb->buffer[cb->head] = item;
    cb->head = (cb->head + 1) % cb->capacity;
    cb->size++;
    return true;
}

// Uklanjanje elementa iz buffera
void* cb_pop(CircularBuffer* cb) {
    if (cb_is_empty(cb)) {
        return NULL;  // Buffer je prazan
    }
    void* item = cb->buffer[cb->tail];
    cb->tail = (cb->tail + 1) % cb->capacity;
    cb->size--;
    return item;
}
