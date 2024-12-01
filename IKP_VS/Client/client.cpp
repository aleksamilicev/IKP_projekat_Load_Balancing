#include "../common/network.h"

#define LB_IP "127.0.0.1"
#define LB_PORT 5059

int main() {
    initialize_winsock();

    SOCKET client_socket = create_client_socket(LB_IP, LB_PORT);
    printf("Connected to Load Balancer.\n");

    // Poruke koje klijent šalje
    const char* messages[] = { "Message1", "Message2", "Message3"};
    int num_messages = sizeof(messages) / sizeof(messages[0]);

    for (int i = 0; i < num_messages; ++i) {
        // Slanje poruke Load Balancer-u
        int bytes_sent = send(client_socket, messages[i], strlen(messages[i]), 0);
        if (bytes_sent == SOCKET_ERROR) {
            printf("Failed to send message %d: %d\n", i + 1, WSAGetLastError());
        } else {
            printf("Client sent: %s\n", messages[i]);
        }

        // Prijem odgovora od Load Balancer-a
        char buffer[1024] = { 0 };
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0'; // Završni karakter
            printf("Received from LB: %s\n", buffer);
        } else if (bytes_received == SOCKET_ERROR) {
            printf("Failed to receive response for message %d: %d\n", i + 1, WSAGetLastError());
        }
    }

    closesocket(client_socket);
    cleanup_winsock();
    return 0;
}
