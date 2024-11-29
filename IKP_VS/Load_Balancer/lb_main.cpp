#include "../common/network.cpp"
#include "../Common/DynamicArray.cpp" // Ukljuèujemo DynamicArray za rad sa nizovima
#include "../Worker/Worker.cpp" // Da bismo koristili Worker strukturu

#define LB_PORT 5059
#define WORKER_PORT 5060 // Port Worker-a
#define WORKER_IP "127.0.0.1" // IP adresa Worker-a (localhost)

DynamicArray workersArray; // Niz sa Worker-ima

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

    // Kreiranje UDP socket-a za komunikaciju sa Worker-ima
    SOCKET worker_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (worker_socket == INVALID_SOCKET) {
        printf("Failed to create UDP socket for Worker communication: %d\n", WSAGetLastError());
        cleanup_winsock();
        return 1;
    }

    // Kreiranje sockaddr_in strukture za Load Balancer
    struct sockaddr_in lb_addr = { 0 };
    lb_addr.sin_family = AF_INET;
    lb_addr.sin_port = htons(LB_PORT);  // Port na kojem Load Balancer sluša
    lb_addr.sin_addr.s_addr = INADDR_ANY;  // Sluša na svim interfejsima

    // Povezivanje socket-a na port LB-a
    if (bind(worker_socket, (struct sockaddr*)&lb_addr, sizeof(lb_addr)) == SOCKET_ERROR) {
        printf("Bind failed with error: %d\n", WSAGetLastError());
        closesocket(worker_socket);
        cleanup_winsock();
        return 1;
    }

    char buffer[1024] = { 0 };
    struct sockaddr_in from_addr;
    int from_addr_len = sizeof(from_addr);

    printf("Load Balancer listening for worker registrations on port %d...\n", LB_PORT);

    while (true) {
        // Prijem podataka od Workera (registracija)
        int bytes_received = recvfrom(worker_socket, buffer, sizeof(buffer) - 1, 0,
            (struct sockaddr*)&from_addr, &from_addr_len);
        if (bytes_received == SOCKET_ERROR) {
            printf("recvfrom failed with error: %d\n", WSAGetLastError());
            continue;
        }

        buffer[bytes_received] = '\0';  // Završni karakter
        printf("Received registration: %s\n", buffer);

        // Obrada registracije Workera
        register_worker(buffer);
    }

    closesocket(worker_socket);
    cleanup_winsock();
    return 0;
}
