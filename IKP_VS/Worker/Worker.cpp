#include "Worker.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void initialize_worker(Worker* worker, const char* id, const char* ip, int port, bool status) {
    strncpy_s(worker->ID, id, sizeof(worker->ID) - 1);
    strncpy_s(worker->IP, ip, sizeof(worker->IP) - 1);
    worker->Port = port;
    worker->Status = status;  // Worker status
    worker->Data = (char**)malloc(MAX_MESSAGES * sizeof(char*)); // Alociraj memoriju za niz pokazivaèa
    worker->message_count = 0;  // Poèetni broj poruka
}

void display_worker(const Worker* worker) {
    printf("Worker ID: %s, IP: %s, Port: %d, Status: %s\n",
        worker->ID, worker->IP, worker->Port,
        worker->Status ? "Dostupan" : "Zauzet");

    // Kastuj Data u char**
    char** messages = (char**)worker->Data;

    // Ispisivanje poruka
    for (int i = 0; i < worker->message_count; ++i) {
        printf("Poruka %d: %s\n", i + 1, messages[i]);
    }
}

void add_message(Worker* worker, const char* message) {
    if (worker->message_count < MAX_MESSAGES) {
        // Alociraj memoriju za novu poruku i dodaj u Data
        worker->Data[worker->message_count] = (char*)malloc(strlen(message) + 1);
        strcpy_s(worker->Data[worker->message_count], strlen(message) + 1, message);
        worker->message_count++;  // Poveæaj broj poruka
    }
    else {
        printf("Premašili ste maksimalni broj poruka za ovog Workera.\n");
    }
}


