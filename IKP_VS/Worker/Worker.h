#pragma once
#ifndef WORKER_H
#define WORKER_H

#define MAX_MESSAGES 100  // Definiši maksimalni broj poruka koji Worker može držati

#include <stdbool.h>
#include <stddef.h>
#include "../Common/CircularBuffer.h"

typedef struct {
    char ID[10];
    char IP[16];
    int Port;
    bool Status; // true = dostupan, false = zauzet
    char** Data;  // Pointer za niz poruka
    int message_count;  // Broj poruka koje Worker trenutno drži
    CircularBuffer messageBuffer; // Dodajemo kružni buffer
} Worker;

void initialize_worker(Worker* worker, const char* id, const char* ip, int port, bool status);
void display_worker(const Worker* worker);
void add_message(Worker* worker, const char* message);  // Funkcija za dodavanje poruka

#endif // WORKER_H
