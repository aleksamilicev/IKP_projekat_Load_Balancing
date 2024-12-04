#include "../common/network.cpp"
#include "../Common/DynamicArray.cpp" // Ukljuèujemo DynamicArray za rad sa nizovima
#include "../Worker/Worker.h" // Da bismo koristili Worker strukturu
#include <thread>
// LB - Round Robin ovde sluzi da zahtev klijenta rasporedimo na odgovarajuci WR
// WR - Round Robin ovde sluzi za biranje radnika u kojem ce podatak biti smesten
#define LB_PORT 5059
#define WORKER_PORT 5060 // Port Worker-a
#define WORKER_IP "127.0.0.1" // IP adresa Worker-a (localhost)


int worker_index = 0; // Indeks za odabir Workera, RR
int worker_num = 0;

DynamicArray workersArray; // Niz sa Worker-ima

// Funkcija za slanje poruke od Clienta ka Worker komponenti (koristi jedan socket)
void forward_to_worker(const char* message, Worker* worker, SOCKET send_socket) {
    struct sockaddr_in worker_addr = { 0 };
    worker_addr.sin_family = AF_INET;
    worker_addr.sin_port = htons(worker->Port);
    inet_pton(AF_INET, worker->IP, &worker_addr.sin_addr);

    int result = sendto(send_socket, message, strlen(message), 0,
        (struct sockaddr*)&worker_addr, sizeof(worker_addr));
    if (result == SOCKET_ERROR) {
        printf("Failed to send message to Worker %s: %d\n", worker->ID, WSAGetLastError());
    }
    else {
        printf("Message forwarded to Worker.\n");

        //printf("Message successfully sent to Worker.\n");
        //printf("Current worker_index after send: %d\n", worker_index);
    }
}

void register_worker(char* buffer) {
    char* token = NULL;
    char id[10], ip[16];
    int port{};
    bool status{};

    token = strtok_s(buffer, ";", &buffer);
    if (token) strcpy_s(id, sizeof(id), token);

    token = strtok_s(NULL, ";", &buffer);
    if (token) strcpy_s(ip, sizeof(ip), token);

    token = strtok_s(NULL, ";", &buffer);
    if (token) port = atoi(token);

    token = strtok_s(NULL, ";", &buffer);
    if (token) status = atoi(token);

    Worker worker;
    initialize_worker(&worker, id, ip, port, status);
    add_worker(&workersArray, &worker);

    printf("Worker %s registered to LB.\n", worker.ID);
    display_dynamic_array(&workersArray);
}

// Funkcija za obradu klijenta u zasebnom threadu
void handle_client(SOCKET client_socket, SOCKET send_socket) {
    char buffer[1024] = { 0 };

    while (true) {
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received == SOCKET_ERROR) {
            printf("recv() failed with error: %d\n", WSAGetLastError());
            break;
        }
        if (bytes_received == 0) {
            printf("Client disconnected.\n");
            break;
        }

        buffer[bytes_received] = '\0';
        printf("Received from Client: %s\n", buffer);

        if (workersArray.size > 0) {
            Worker* selected_worker = &workersArray.array[worker_index];
            //printf("Selected Worker: %s (Index: %d) for message: %s\n", selected_worker->ID, worker_index, buffer);
            forward_to_worker(buffer, selected_worker, send_socket);

            // Ciklièno prilagodimo indeks Workera
            worker_num = (worker_index + 1) % workersArray.size;    //Buni se kad uradim ovako, pa sam odlucio da stavim na 0
            worker_index = 0;

            const char* response = "Message successfully stored.";
            send(client_socket, response, strlen(response), 0);
        }
        else {
            const char* response = "No available Workers.";
            send(client_socket, response, strlen(response), 0);
        }
    }

    closesocket(client_socket);
}

int main() {
    initialize_winsock();
    initialize_dynamic_array(&workersArray, 10);

    SOCKET send_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (send_socket == INVALID_SOCKET) {
        printf("Failed to create socket for forwarding: %d\n", WSAGetLastError());
        return 1;
    }

    SOCKET udp_worker_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_worker_socket == INVALID_SOCKET) {
        printf("Failed to create UDP socket for Worker communication: %d\n", WSAGetLastError());
        cleanup_winsock();
        return 1;
    }

    struct sockaddr_in lb_udp_addr = { 0 };
    lb_udp_addr.sin_family = AF_INET;
    lb_udp_addr.sin_port = htons(LB_PORT);
    lb_udp_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udp_worker_socket, (struct sockaddr*)&lb_udp_addr, sizeof(lb_udp_addr)) == SOCKET_ERROR) {
        printf("UDP Bind failed with error: %d\n", WSAGetLastError());
        closesocket(udp_worker_socket);
        cleanup_winsock();
        return 1;
    }

    SOCKET tcp_client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_client_socket == INVALID_SOCKET) {
        printf("Failed to create TCP socket for Client communication: %d\n", WSAGetLastError());
        closesocket(udp_worker_socket);
        cleanup_winsock();
        return 1;
    }

    struct sockaddr_in lb_tcp_addr = { 0 };
    lb_tcp_addr.sin_family = AF_INET;
    lb_tcp_addr.sin_port = htons(LB_PORT);
    lb_tcp_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(tcp_client_socket, (struct sockaddr*)&lb_tcp_addr, sizeof(lb_tcp_addr)) == SOCKET_ERROR) {
        printf("TCP Bind failed with error: %d\n", WSAGetLastError());
        closesocket(udp_worker_socket);
        closesocket(tcp_client_socket);
        cleanup_winsock();
        return 1;
    }

    if (listen(tcp_client_socket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listen failed with error: %d\n", WSAGetLastError());
        closesocket(udp_worker_socket);
        closesocket(tcp_client_socket);
        cleanup_winsock();
        return 1;
    }

    printf("Load Balancer is listening for workers (UDP) and clients (TCP) on port %d...\n", LB_PORT);

    std::thread udp_thread([&]() {
        while (true) {
            struct sockaddr_in from_addr;
            int from_addr_len = sizeof(from_addr);
            char buffer[1024] = { 0 };

            int bytes_received = recvfrom(udp_worker_socket, buffer, sizeof(buffer) - 1, 0,
                (struct sockaddr*)&from_addr, &from_addr_len);
            if (bytes_received == SOCKET_ERROR) {
                printf("recvfrom failed with error: %d\n", WSAGetLastError());
                continue;
            }

            buffer[bytes_received] = '\0';
            printf("Received registration: %s\n", buffer);

            register_worker(buffer);
        }
        });

    while (true) {
        struct sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);

        SOCKET client_socket = accept(tcp_client_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket == INVALID_SOCKET) {
            printf("accept() failed with error: %d\n", WSAGetLastError());
            continue;
        }

        printf("Client connected.\n");

        // Kreiraj novi thread za svakog klijenta
        std::thread client_thread(handle_client, client_socket, send_socket);
        client_thread.detach();
    }

    udp_thread.join();
    closesocket(udp_worker_socket);
    closesocket(tcp_client_socket);
    cleanup_winsock();
    return 0;
}