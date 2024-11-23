#include "network.h"

// Inicijalizacija Winsock-a
void initialize_winsock() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "Failed to initialize Winsock. Error Code: %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
}

// Kreira TCP server socket
SOCKET create_server_socket(int port) {
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        fprintf(stderr, "Failed to create socket. Error Code: %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Bind failed. Error Code: %d\n", WSAGetLastError());
        closesocket(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) == SOCKET_ERROR) {
        fprintf(stderr, "Listen failed. Error Code: %d\n", WSAGetLastError());
        closesocket(server_socket);
        exit(EXIT_FAILURE);
    }

    return server_socket;
}

// Prihvata dolaznu konekciju
SOCKET accept_connection(SOCKET server_socket) {
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    SOCKET client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
    if (client_socket == INVALID_SOCKET) {
        fprintf(stderr, "Accept failed. Error Code: %d\n", WSAGetLastError());
    }
    return client_socket;
}

// Kreira TCP klijent socket
SOCKET create_client_socket(const char* ip, int port) {
    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        fprintf(stderr, "Failed to create socket. Error Code: %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid IP address.\n");
        closesocket(client_socket);
        exit(EXIT_FAILURE);
    }

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Connection failed. Error Code: %d\n", WSAGetLastError());
        closesocket(client_socket);
        exit(EXIT_FAILURE);
    }

    return client_socket;
}

// Èisti Winsock resurse
void cleanup_winsock() {
    WSACleanup();
}
