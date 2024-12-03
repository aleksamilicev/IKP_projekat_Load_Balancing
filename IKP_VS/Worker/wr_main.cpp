#include "../common/network.cpp"  // Umesto network.cpp
#include "Worker.h"
#include "../Common/CircularBuffer.cpp"  // Umesto CircularBuffer.cpp
#include <time.h>
#include <string.h>

#define WR_PORT 5060

int worker_index = 0; // Indeks koji ciklièno dodeljuje poruke Worker-ima

// Funkcija za slanje notifikacije svim ostalim Worker-ima
void notify_workers(Worker* workers, int num_workers, const char* message, const char* sender_id) {
    for (int i = 0; i < num_workers; ++i) {
        if (strcmp(workers[i].ID, sender_id) != 0) { // Preskoèi Workera koji je poslao poruku
            add_message(&workers[i], message); // Ažuriraj poruku kod ostalih Workera
        }
    }
}

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
    inet_pton(AF_INET, "127.0.0.1", &lb_addr.sin_addr); // IP adresa Load Balancera

    char buffer[1024] = { 0 };
    struct sockaddr_in from_addr;
    int from_addr_len = sizeof(from_addr);

    int num_workers = 4; // Generisanje 2 Workera za primer
    Worker* workers = new Worker[num_workers]; // Dinamièki niz Workera

    printf("Generating %d Workers...\n", num_workers);

    // Generisanje i inicijalizacija Workera
    for (int i = 0; i < num_workers; ++i) {
        char id[10];
        snprintf(id, sizeof(id), "WR%d", i + 1);

        char ip[16] = "127.0.0.1";
        int port = WR_PORT + i;

        initialize_worker(&workers[i], id, ip, port, true);
        display_worker(&workers[i]);

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

        // Pronaði dostupnog Workera (koristi ciklièno dodeljivanje)
        Worker* target_worker = &workers[worker_index];
        worker_index = (worker_index + 1) % num_workers;

        // Skladišti poruku kod izabranog Workera
        add_message(target_worker, buffer);
        printf("\n- %s stores %s\n", target_worker->ID, buffer);

        // Ispis stanja Workera pre notifikacije
        printf("Workers' state before notification:\n");
        for (int i = 0; i < num_workers; ++i) {
            printf("%s: [", workers[i].ID);
            for (int j = 0; j < workers[i].message_count; ++j) {
                printf("%s", workers[i].Data[j]);
                if (j < workers[i].message_count - 1) {
                    printf(", ");
                }
            }
            printf("]\n");
        }

        // Notifikacija svim ostalim Worker-ima
        notify_workers(workers, num_workers, buffer, target_worker->ID);
        printf("- %s notifies other workers\n", target_worker->ID);

        // Ispis stanja Workera posle notifikacije
        printf("Workers' state after notification:\n");
        for (int i = 0; i < num_workers; ++i) {
            printf("%s: [", workers[i].ID);
            for (int j = 0; j < workers[i].message_count; ++j) {
                printf("%s", workers[i].Data[j]);
                if (j < workers[i].message_count - 1) {
                    printf(", ");
                }
            }
            printf("]\n");
        }

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
    }

    // Oslobaðanje resursa
    delete[] workers;
    closesocket(wr_server_socket);
    cleanup_winsock();
    return 0;
}
