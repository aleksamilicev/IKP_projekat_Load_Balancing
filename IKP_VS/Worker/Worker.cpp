#include "Worker.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void initialize_worker(Worker* worker, const char* id, const char* ip, int port, bool status) {
    strncpy_s(worker->ID, id, sizeof(worker->ID) - 1);
    strncpy_s(worker->IP, ip, sizeof(worker->IP) - 1);
    worker->Port = port;
    worker->Status = status;  // Worker status

    // Alociraj memoriju za niz pokazivaèa i inicijalizuj na nullptr
    worker->Data = (char**)malloc(MAX_MESSAGES * sizeof(char*));
    if (worker->Data == NULL) {
        printf("Failed to allocate memory for Data array.\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MAX_MESSAGES; ++i) {
        worker->Data[i] = nullptr;
    }

    worker->message_count = 0;  // Poèetni broj poruka
}

void display_worker(const Worker* worker) {
    printf("Worker ID: %s, IP: %s, Port: %d, Status: %s\n",
        worker->ID, worker->IP, worker->Port,
        worker->Status ? "Dostupan" : "Zauzet");

    // Ispisivanje poruka
    for (int i = 0; i < worker->message_count; ++i) {
        if (worker->Data[i] != nullptr) {
            printf("Poruka %d: %s\n", i + 1, worker->Data[i]);
        }
    }
}

void add_message(Worker* worker, const char* message) {
    if (worker->message_count < MAX_MESSAGES) {
        // Alociraj memoriju za novu poruku
        char* allocated_message = (char*)malloc(strlen(message) + 1);
        if (!allocated_message) {
            printf("Failed to allocate memory for the message.\n");
            return;
        }

        // Kopiraj poruku u alocirani prostor
        strcpy_s(allocated_message, strlen(message) + 1, message);

        // Dodaj poruku u Data
        worker->Data[worker->message_count] = allocated_message;
        worker->message_count++;  // Poveæaj broj poruka

        printf("Message successfully added to Worker %s.\n", worker->ID);
    }
    else {
        printf("Maximum message limit reached for Worker %s.\n", worker->ID);
    }
}

void free_worker_messages(Worker* worker) {
    for (int i = 0; i < worker->message_count; ++i) {
        if (worker->Data[i] != nullptr) {
            free(worker->Data[i]);  // Oslobodi memoriju za svaku poruku
            worker->Data[i] = nullptr;
        }
    }
    worker->message_count = 0;
}