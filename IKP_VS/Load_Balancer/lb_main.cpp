#include "../common/network.h"

#define LB_PORT 5059
#define WORKER_PORT 5060 // Port Worker-a
#define WORKER_IP "127.0.0.1" // IP adresa Worker-a (localhost)

int main() {
    initialize_winsock();

    // Kreiranje TCP socket-a za klijent komunikaciju
    SOCKET lb_server_socket = create_server_socket(LB_PORT);
    printf("Load Balancer is listening on port %d...\n", LB_PORT);

    // Kreiranje UDP socket-a za komunikaciju sa Worker-om
    SOCKET worker_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (worker_socket == INVALID_SOCKET) {
        printf("Failed to create UDP socket for Worker communication: %d\n", WSAGetLastError());
        cleanup_winsock();
        return 1;
    }

    struct sockaddr_in worker_addr = { 0 };
    worker_addr.sin_family = AF_INET;
    worker_addr.sin_port = htons(WORKER_PORT);
    inet_pton(AF_INET, WORKER_IP, &worker_addr.sin_addr);

    char buffer[1024] = { 0 };

    while (true) {
        // Prihvatanje klijenta
        SOCKET client_socket = accept_connection(lb_server_socket);
        if (client_socket == INVALID_SOCKET) continue;

        printf("Client connected.\n");
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0'; // Završni karakter
            printf("Received from client: %s\n", buffer);

            // Prosleðivanje poruke Worker-u
            int result = sendto(worker_socket, buffer, bytes_received, 0,
                (struct sockaddr*)&worker_addr, sizeof(worker_addr));
            if (result == SOCKET_ERROR) {
                printf("Failed to send message to Worker: %d\n", WSAGetLastError());
            }
            else {
                printf("Message forwarded to Worker.\n");
            }

            // Èekanje odgovora od Worker-a
            struct sockaddr_in from_addr;
            int from_addr_len = sizeof(from_addr);
            int worker_response = recvfrom(worker_socket, buffer, sizeof(buffer) - 1, 0,
                (struct sockaddr*)&from_addr, &from_addr_len);
            if (worker_response == SOCKET_ERROR) {
                printf("Failed to receive response from Worker: %d\n", WSAGetLastError());
            }
            else {
                buffer[worker_response] = '\0'; // Završni karakter
                printf("Received response from Worker: %s\n", buffer);

                // Odgovor klijentu ukljuèuje i status od Worker-a
                send(client_socket, buffer, strlen(buffer), 0);
            }
        }

        closesocket(client_socket);
    }

    closesocket(lb_server_socket);
    closesocket(worker_socket);
    cleanup_winsock();
    return 0;
}
