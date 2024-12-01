#include "../common/network.cpp" // referenciras .cpp, a ne .h
#include "Worker.h"
#include <time.h>
#define WR_PORT 5060

int worker_index = 0; // Indeks koji ciklièno dodeljuje poruke Worker-ima

int main() {
    initialize_winsock();

    // Kreiranje UDP socket-a za Worker
    SOCKET wr_server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (wr_server_socket == INVALID_SOCKET) {
        printf("Failed to create UDP socket: %d\n", WSAGetLastError());
        cleanup_winsock();
        return 1;
    }

    struct sockaddr_in wr_addr = { 0 };
    wr_addr.sin_family = AF_INET;
    wr_addr.sin_port = htons(WR_PORT);
    wr_addr.sin_addr.s_addr = INADDR_ANY; // Slušanje na svim interfejsima

    if (bind(wr_server_socket, (struct sockaddr*)&wr_addr, sizeof(wr_addr)) == SOCKET_ERROR) {
        printf("Bind failed with error: %d\n", WSAGetLastError());
        closesocket(wr_server_socket);
        cleanup_winsock();
        return 1;
    }

    printf("Worker is listening on port %d...\n", WR_PORT);

    // Inicijalizacija adrese LB-a za slanje podataka
    struct sockaddr_in lb_addr = { 0 };
    lb_addr.sin_family = AF_INET;
    lb_addr.sin_port = htons(5059); // Port na kojem Load Balancer sluša
    inet_pton(AF_INET, "127.0.0.1", &lb_addr.sin_addr);  // IP adresa Load Balancera

    char buffer[1024] = { 0 };
    struct sockaddr_in from_addr;
    int from_addr_len = sizeof(from_addr);

    //srand((unsigned int)time(NULL)); // Seed za random generisanje
    //int num_workers = 1 + rand() % 5; // Generišemo 1-5 workera
    int num_workers = 3; // Generišemo 1-5 workera
    Worker* workers = new Worker[num_workers];  // Dinamièki niz workera

    printf("Generating %d Workers...\n", num_workers);

    // Generisanje i inicijalizacija workera
    for (int i = 0; i < num_workers; ++i) {
        char id[10];
        snprintf(id, sizeof(id), "WR%d", i + 1); // Generisanje ID-ja workera

        char ip[16] = "127.0.0.1"; // IP adresa
        int port = WR_PORT + i; // Dodeljivanje portova za workere

        // Inicijalizacija workera
        initialize_worker(&workers[i], id, ip, port, true);

        // Ispis podataka o workeru
        display_worker(&workers[i]);

        // Slanje registracije Load Balanceru
        char registration_message[1024];
        snprintf(registration_message, sizeof(registration_message), "%s;%s;%d;%d",
            workers[i].ID, workers[i].IP, workers[i].Port, workers[i].Status);
        int result = sendto(wr_server_socket, registration_message, strlen(registration_message), 0,
            (struct sockaddr*)&lb_addr, sizeof(lb_addr));
        if (result == SOCKET_ERROR) {
            printf("Failed to send registration to LB: %d\n", WSAGetLastError());
        }
        else {
            printf("Registration sent for Worker %s\n", workers[i].ID);
        }
    }

    // Oslobaðanje memorije nakon korišæenja
    //delete[] workers;

    while (true) {
        // Prijem poruke od Load Balancer-a
        int bytes_received = recvfrom(wr_server_socket, buffer, sizeof(buffer) - 1, 0,
            (struct sockaddr*)&lb_addr, &from_addr_len);
        if (bytes_received == SOCKET_ERROR) {
            printf("recvfrom failed with error: %d\n", WSAGetLastError());
            continue;
        }

        buffer[bytes_received] = '\0'; // Završni karakter
        printf("\nReceived from LB: %s\n", buffer);

        // Pronaði dostupnog workera (koristi ciklièno dodeljivanje)
        Worker* target_worker = NULL;
        for (int i = 0; i < num_workers; ++i) {
            target_worker = &workers[(worker_index + i) % num_workers];  // Ovdje koristi globalni worker_index
            if (target_worker->Status == true) {  // Ako je Worker slobodan
                break;
            }
        }

        if (target_worker == NULL) {
            printf("No available Workers. Skipping.\n");
            continue;
        }

        // Dodavanje nove poruke u Worker
        add_message(target_worker, buffer);  // Dodaj poruku u Worker

        // Ispis stanja svih workera
        printf("\nWorkers' current state:\n");
        for (int i = 0; i < num_workers; ++i) {
            printf("%s: [", workers[i].ID);
            // Ispisivanje svih poruka za trenutnog Workera
            for (int j = 0; j < workers[i].message_count; ++j) {
                printf("%s", workers[i].Data[j]);
                if (j < workers[i].message_count - 1) {
                    printf(", ");  // Dodaj zarez izmeðu poruka
                }
            }
            printf("]\n");
        }
        printf("\n");


        // Slanje potvrde LB-u
        const char* response = "Message successfully stored.";
        int result = sendto(wr_server_socket, response, strlen(response), 0,
            (struct sockaddr*)&lb_addr, sizeof(lb_addr));
        if (result == SOCKET_ERROR) {
            printf("Failed to send response to LB: %d\n", WSAGetLastError());
        }
        else {
            printf("Response sent to LB.\n");
        }

        // Ažuriraj worker_index za sledeæu poruku
        worker_index = (worker_index + 1) % num_workers;
    }


    closesocket(wr_server_socket);
    cleanup_winsock();
    return 0;
}
