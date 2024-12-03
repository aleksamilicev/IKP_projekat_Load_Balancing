#include "../common/network.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LB_IP "127.0.0.1"
#define LB_PORT 5059

volatile sig_atomic_t stop = 0;

// Funkcija za obradu signala Ctrl+C
void handle_signal(int signal) {
    if (signal == SIGINT) {
        stop = 1;
    }
}

int main() {
    initialize_winsock();

    SOCKET client_socket = create_client_socket(LB_IP, LB_PORT);
    if (client_socket == INVALID_SOCKET) {
        printf("Failed to connect to Load Balancer.\n");
        cleanup_winsock();
        return 1;
    }
    printf("Connected to Load Balancer.\n");

    // Registracija funkcije za obradu signala Ctrl+C
    signal(SIGINT, handle_signal);

    /*
    // Dodavanje identifikatora klijenta
    printf("Enter client ID: ");
    char client_id[64];
    if (fgets(client_id, sizeof(client_id), stdin) == NULL) {
        printf("Error reading client ID. Exiting.\n");
        closesocket(client_socket);
        cleanup_winsock();
        return 1;
    }
    size_t id_len = strlen(client_id);
    if (id_len > 0 && client_id[id_len - 1] == '\n') {
        client_id[id_len - 1] = '\0';
    }

    int bytes_sent = send(client_socket, client_id, strlen(client_id), 0);
    if (bytes_sent == SOCKET_ERROR) {
        printf("Failed to send client ID: %d\n", WSAGetLastError());
        closesocket(client_socket);
        cleanup_winsock();
        return 1;
    }
    printf("Client ID sent: %s\n", client_id);
    */  // ne vodimo racuna o tome koji klijentt je poslao sta, nego vodimo racuna o primanju i rasporedu poruka

    char message[1024];
    while (!stop) {
        printf("\nUnesi poruku koju zelite da skladistite: ");
        if (fgets(message, sizeof(message), stdin) == NULL) {
            printf("\nError reading input. Try again.\n");
            continue;
        }

        // Uklanjanje newline karaktera na kraju unete poruke
        size_t len = strlen(message);
        if (len > 0 && message[len - 1] == '\n') {
            message[len - 1] = '\0';
        }

        // Slanje poruke Load Balanceru
        int bytes_sent = send(client_socket, message, strlen(message), 0);
        if (bytes_sent == SOCKET_ERROR) {
            printf("\nFailed to send message: %d\n", WSAGetLastError());
            break;
        }
        else {
            printf("Client sent: %s\n", message);
        }

        // Prijem odgovora od Load Balancera
        char buffer[1024] = { 0 };
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0'; // Završni karakter
            printf("Received from LB: %s\n", buffer);
        }
        else if (bytes_received == SOCKET_ERROR) {
            printf("Failed to receive response: %d\n", WSAGetLastError());
        }
    }

    printf("\nDisconnecting from Load Balancer...\n");
    closesocket(client_socket);
    cleanup_winsock();
    return 0;
}
