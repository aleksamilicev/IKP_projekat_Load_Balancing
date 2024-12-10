#include "../common/network.cpp"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define LB_IP "127.0.0.1"
#define LB_PORT 5059

volatile sig_atomic_t stop = 0;

// Funkcija za obradu signala Ctrl+C
void handle_signal(int signal) {
    if (signal == SIGINT) {
        stop = 1;
    }
}

void send_messages(SOCKET client_socket, int num_messages) {
    char message[1024];
    for (int i = 0; i < num_messages; ++i) {
        snprintf(message, sizeof(message), "Message %d", i + 1);

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

    while (!stop) {
        // Prikaz menija
        printf("\nChoose an option: \n");
        printf("    0) Enter a single custom message.\n");
        printf("    1) Send 10 messages.\n");
        printf("    2) Send 30 messages.\n");
        printf("    3) Send 100 messages.\n");
        printf("    4) Enter the number of messages.\n");
        printf("    x) Exit\n");
        printf("> ");

        char choice[10];
        if (fgets(choice, sizeof(choice), stdin) == NULL) {
            printf("\nError reading input. Try again.\n");
            continue;
        }

        // Uklanjanje newline karaktera na kraju unete poruke
        size_t len = strlen(choice);
        if (len > 0 && choice[len - 1] == '\n') {
            choice[len - 1] = '\0';
        }

        // Obrada opcije
        if (strcmp(choice, "x") == 0 || strcmp(choice, "X") == 0) {
            printf("Exiting...\n");
            break;
        }
        else if (strcmp(choice, "0") == 0) {
            char message[1024];
            printf("Enter a custom message: ");
            if (fgets(message, sizeof(message), stdin) == NULL) {
                printf("\nError reading input. Try again.\n");
                continue;
            }

            len = strlen(message);
            if (len > 0 && message[len - 1] == '\n') {
                message[len - 1] = '\0';
            }

            // Slanje pojedinaène poruke
            int bytes_sent = send(client_socket, message, strlen(message), 0);
            if (bytes_sent == SOCKET_ERROR) {
                printf("\nFailed to send message: %d\n", WSAGetLastError());
            }
            else {
                printf("Client sent: %s\n", message);
            }

            // Prijem odgovora
            char buffer[1024] = { 0 };
            int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                printf("Received from LB: %s\n", buffer);
            }
            else if (bytes_received == SOCKET_ERROR) {
                printf("Failed to receive response: %d\n", WSAGetLastError());
            }
        }
        else if (strcmp(choice, "1") == 0) {
            send_messages(client_socket, 10);
        }
        else if (strcmp(choice, "2") == 0) {
            send_messages(client_socket, 30);
        }
        else if (strcmp(choice, "3") == 0) {
            send_messages(client_socket, 100);
        }
        else if (strcmp(choice, "4") == 0) {
            int num_messages;
            printf("Enter the number of messages to send: ");
            if (scanf_s("%d", &num_messages) != 1) {
                printf("Invalid input. Please enter a number.\n");
                // Oèisti ulazni bafer
                while (getchar() != '\n');
                continue;
            }
            // Oèisti newline iz ulaznog bafera
            while (getchar() != '\n');
            send_messages(client_socket, num_messages);
        }
        else {
            printf("Invalid option. Please try again.\n");
        }
    }

    printf("\nDisconnecting from Load Balancer...\n");
    closesocket(client_socket);
    cleanup_winsock();
    return 0;
}
