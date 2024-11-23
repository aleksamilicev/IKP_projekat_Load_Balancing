#include "../common/network.h"

#define LB_IP "127.0.0.1"
#define LB_PORT 5059

int main() {
    initialize_winsock();

    SOCKET client_socket = create_client_socket(LB_IP, LB_PORT);
    printf("Connected to Load Balancer.\n");

    const char* message = "Hello from Client!";
    send(client_socket, message, strlen(message), 0);

    char buffer[1024] = { 0 };
    recv(client_socket, buffer, sizeof(buffer), 0);
    printf("Received from LB: %s\n", buffer);

    closesocket(client_socket);
    cleanup_winsock();
    return 0;
}
