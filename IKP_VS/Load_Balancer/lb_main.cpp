#include "../common/network.cpp"
#include "../Common/DynamicArray.cpp" // Ukljuèujemo DynamicArray za rad sa nizovima
#include "../Worker/Worker.h" // Da bismo koristili Worker strukturu

#define LB_PORT 5059
#define WORKER_PORT 5060 // Port Worker-a
#define WORKER_IP "127.0.0.1" // IP adresa Worker-a (localhost)

int worker_index = 0; // Indeks za odabir Workera


DynamicArray workersArray; // Niz sa Worker-ima

// Funkcija za slanje poruke od Clienta ka Worker komponenti
void forward_to_worker(const char* message, Worker* worker) {
    // Kreiranje socket-a za slanje poruke Workeru
    SOCKET send_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (send_socket == INVALID_SOCKET) {
        printf("Failed to create socket for forwarding: %d\n", WSAGetLastError());
        return;
    }

    struct sockaddr_in worker_addr = { 0 };
    worker_addr.sin_family = AF_INET;
    worker_addr.sin_port = htons(worker->Port);
    inet_pton(AF_INET, worker->IP, &worker_addr.sin_addr);

    // Slanje poruke Workeru
    int result = sendto(send_socket, message, strlen(message), 0,
        (struct sockaddr*)&worker_addr, sizeof(worker_addr));
    if (result == SOCKET_ERROR) {
        printf("Failed to send message to Worker %s: %d\n", worker->ID, WSAGetLastError());
    }
    else {
        printf("Message forwarded to Worker %s.\n", worker->ID);
    }

    // Ne èekamo odgovor odmah, samo šaljemo poruku
    closesocket(send_socket);
}




void print_registered_workers() {
    printf("Currently registered workers:\n");
    for (int i = 0; i < workersArray.size; ++i) {
        display_worker(&workersArray.array[i]);
    }
}

void initialize_workers_array() {
    initialize_dynamic_array(&workersArray, 10);  // Kreiraj DynamicArray sa poèetnim kapacitetom 10
}


void register_worker(char* buffer) {
    // Buffer format: WR1;127.0.0.1;5060;1
    char* token = NULL;
    char id[10];
    char ip[16];
    int port{};
    bool status{};

    // Parsiranje podataka iz buffer-a
    token = strtok_s(buffer, ";", &buffer);  // Prvi token
    if (token != NULL) {
        strcpy_s(id, sizeof(id), token);
    }

    token = strtok_s(NULL, ";", &buffer);  // Drugi token
    if (token != NULL) {
        strcpy_s(ip, sizeof(ip), token);
    }

    token = strtok_s(NULL, ";", &buffer);  // Treæi token
    if (token != NULL) {
        port = atoi(token);
    }

    token = strtok_s(NULL, ";", &buffer);  // Èetvrti token
    if (token != NULL) {
        status = atoi(token);
    }

    // Kreiranje Workera
    Worker worker;
    initialize_worker(&worker, id, ip, port, status);

    // Dodavanje Workera u DynamicArray
    add_worker(&workersArray, &worker);

    printf("Worker %s registered to LB.\n", worker.ID);
    display_dynamic_array(&workersArray);  // Ispis svih registrovanih Workera
}





int main() {
    initialize_winsock();
    initialize_workers_array();

    // Kreiranje UDP socket-a za registraciju Workera
    SOCKET udp_worker_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_worker_socket == INVALID_SOCKET) {
        printf("Failed to create UDP socket for Worker communication: %d\n", WSAGetLastError());
        cleanup_winsock();
        return 1;
    }

    // Bindovanje UDP socket-a
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

    // Kreiranje TCP socket-a za komunikaciju sa klijentima
    SOCKET tcp_client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_client_socket == INVALID_SOCKET) {
        printf("Failed to create TCP socket for Client communication: %d\n", WSAGetLastError());
        closesocket(udp_worker_socket);
        cleanup_winsock();
        return 1;
    }

    // Bindovanje TCP socket-a
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

    // Postavi oba socket-a za selektivno slušanje
    fd_set read_fds;
    SOCKET max_fd = max(udp_worker_socket, tcp_client_socket);

    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(udp_worker_socket, &read_fds);
        FD_SET(tcp_client_socket, &read_fds);

        int activity = select((int)(max_fd + 1), &read_fds, NULL, NULL, NULL);
        if (activity == SOCKET_ERROR) {
            printf("select() failed with error: %d\n", WSAGetLastError());
            break;
        }

        // Obrada registracije Workera (UDP)
        if (FD_ISSET(udp_worker_socket, &read_fds)) {
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

        // Obrada zahteva klijenata (TCP)
        if (FD_ISSET(tcp_client_socket, &read_fds)) {
            struct sockaddr_in client_addr;
            int client_addr_len = sizeof(client_addr);

            SOCKET client_socket = accept(tcp_client_socket, (struct sockaddr*)&client_addr, &client_addr_len);
            if (client_socket == INVALID_SOCKET) {
                printf("accept() failed with error: %d\n", WSAGetLastError());
                continue;
            }

            char buffer[1024] = { 0 };

            while (true) { // Ovaj while omoguæava višekratnu komunikaciju
                int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
                if (bytes_received == SOCKET_ERROR) {
                    printf("recv() failed with error: %d\n", WSAGetLastError());
                    break; // Prekini petlju ako je došlo do greške
                }
                if (bytes_received == 0) {
                    break; // Klijent je zatvorio konekciju
                }

                buffer[bytes_received] = '\0';
                printf("\nReceived from Client: %s\n", buffer);

                // Prosledi poruku odgovarajuæem Worker-u
                if (workersArray.size > 0) {   
                    for (int i = 0; i < workersArray.size; ++i) {
                        Worker* selected_worker = &workersArray.array[(worker_index + i) % workersArray.size];
                        forward_to_worker(buffer, selected_worker);
                    }

                    // Prilagodimo indeks Workera za sledeæu poruku (ciklièno)
                    worker_index = (worker_index + 1) % workersArray.size;

                    // Odgovori klijentu
                    const char* response = "Message successfully stored.";
                    send(client_socket, response, strlen(response), 0);
                }
                else {
                    const char* response = "No available Workers.";
                    send(client_socket, response, strlen(response), 0);
                }
            }

            closesocket(client_socket); // Zatvori socket tek nakon što klijent završi sa slanjem poruka
        }

    }

    closesocket(udp_worker_socket);
    closesocket(tcp_client_socket);
    cleanup_winsock();
    return 0;
}

