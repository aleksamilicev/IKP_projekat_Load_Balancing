#pragma once
#ifndef WORKER_H
#define WORKER_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char ID[10];
    char IP[16];
    int Port;
    bool Status; // true = dostupan, false = zauzet
    void* Data;  // Pointer na strukturu podataka (možemo kasnije proširiti ako bude potrebno)
} Worker;

void initialize_worker(Worker* worker, const char* id, const char* ip, int port, bool status);
void display_worker(const Worker* worker);

#endif // WORKER_H
