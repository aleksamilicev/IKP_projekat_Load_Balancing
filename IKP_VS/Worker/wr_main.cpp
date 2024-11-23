#include "../common/network.h"

#define WR_PORT 5060

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
    wr_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(wr_server_socket, (struct sockaddr*)&wr_addr, sizeof(wr_addr)) == SOCKET_ERROR) {
        printf("Bind failed with error: %d\n", WSAGetLastError());
        closesocket(wr_server_socket);
        cleanup_winsock();
        return 1;
    }

    printf("Worker is listening on port %d...\n", WR_PORT);

    char buffer[1024] = { 0 };
    struct sockaddr_in lb_addr;
    int lb_addr_len = sizeof(lb_addr);

    while (true) {
        // Prijem poruke od Load Balancer-a
        int bytes_received = recvfrom(wr_server_socket, buffer, sizeof(buffer) - 1, 0,
            (struct sockaddr*)&lb_addr, &lb_addr_len);
        if (bytes_received == SOCKET_ERROR) {
            printf("recvfrom failed with error: %d\n", WSAGetLastError());
            continue;
        }

        buffer[bytes_received] = '\0'; // Završni karakter
        printf("Received from LB: %s\n", buffer);

        // Opcionalno: Slanje odgovora LB-u (ako je potrebno)
        const char* response = "Message processed by Worker.";
        int result = sendto(wr_server_socket, response, strlen(response), 0,
            (struct sockaddr*)&lb_addr, lb_addr_len);
        if (result == SOCKET_ERROR) {
            printf("Failed to send response to LB: %d\n", WSAGetLastError());
        }
        else {
            printf("Response sent to LB.\n");
        }
    }

    closesocket(wr_server_socket);
    cleanup_winsock();
    return 0;
}
