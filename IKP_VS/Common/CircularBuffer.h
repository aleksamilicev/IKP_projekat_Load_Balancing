#pragma once
#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    void** buffer;       // Niz pokazivaèa za skladištenje podataka
    size_t capacity;     // Maksimalni kapacitet buffera
    size_t head;         // Indeks na koji se dodaje novi element
    size_t tail;         // Indeks odakle se uklanja element
    size_t size;         // Trenutni broj elemenata u bufferu
} CircularBuffer;

// Funkcije za rad sa kružnim bufferom
void cb_initialize(CircularBuffer* cb, size_t capacity);
void cb_destroy(CircularBuffer* cb);
bool cb_is_full(const CircularBuffer* cb);
bool cb_is_empty(const CircularBuffer* cb);
bool cb_push(CircularBuffer* cb, void* item);
void* cb_pop(CircularBuffer* cb);

#endif // CIRCULARBUFFER_H
