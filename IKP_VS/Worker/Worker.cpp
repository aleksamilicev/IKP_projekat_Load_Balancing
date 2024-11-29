#include "Worker.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void initialize_worker(Worker* worker, const char* id, const char* ip, int port, bool status) {
    strncpy_s(worker->ID, id, sizeof(worker->ID) - 1);
    strncpy_s(worker->IP, ip, sizeof(worker->IP) - 1);
    worker->Port = port;
    worker->Status = status; // Ovo nam treba, jer WR1 skladisti podatke i on je zauzet pa sledecu poruku saljemo na WR2
                             // I to rasporedjivanje poruka radimo pomocu RR algoritma
    worker->Data = NULL; // Data je inicijalno NULL
}

void display_worker(const Worker* worker) {
    printf("Worker ID: %s, IP: %s, Port: %d, Status: %s\n",
        worker->ID, worker->IP, worker->Port,
        worker->Status ? "Dostupan" : "Zauzet");
}
