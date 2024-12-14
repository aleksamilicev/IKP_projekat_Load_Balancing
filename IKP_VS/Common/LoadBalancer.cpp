#pragma region Include
#include "../common/network.cpp"
#include "../Common/DynamicArray.cpp"
#include "../Common/ThreadPool.cpp"
#include "../Worker/Worker.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <atomic>
#include <functional>
#pragma endregion

#pragma region Define
#define LB_PORT 5059            // port na kojem LB slusa zahteve klijenata i registraciju WR-a
#define WORKER_PORT 5060
#define WORKER_IP "127.0.0.1"
#define MAX_THREADS 10          // max broj niti u Thread Pool-u
#define MAX_QUEUE_SIZE 100      // velicina reda za obradu zahteva
#pragma endregion

#pragma region LoadBalancer
class LoadBalancer {
#pragma region Private
private:
    // Sinhronizacija
    std::mutex workers_mutex;
    std::mutex client_mutex;    // Zakljucavanja za sinhronizaciju pristupa WR-ima i klijentima

    std::condition_variable worker_condition;   // Blokira nit dok WR-i ne dubu dostupni

    // Strukture podataka
    DynamicArray workersArray;  // Skladistimo podatke o registrovanim WR-ima
    ThreadPool threadPool;

    std::atomic<int> worker_index;  // Index za RR distribuciju klijentskih zahteva

    // Mrezni soketi
    SOCKET send_socket;         // Za slanje poruka WR-ima preko UDP-a
    SOCKET udp_worker_socket;   // Koristi se za registraciju novih WR-a
    SOCKET tcp_client_socket;   // Slusa dolazne konekcije sa klijentima
#pragma endregion

#pragma region Public
public:
    LoadBalancer() :
        threadPool(MAX_THREADS),    // inicijalizacija ThreadPool-a
        worker_index(0)
    {
        initialize_winsock();
        initialize_dynamic_array(&workersArray, 10);
        setup_sockets();    // za kreiranje i konfiguraciju mreznih soketa
    }

    void setup_sockets() {  // kreairamo UDP socket za WR i TCP socket za klijenta
        send_socket = socket(AF_INET, SOCK_DGRAM, 0);
        udp_worker_socket = socket(AF_INET, SOCK_DGRAM, 0);
        tcp_client_socket = socket(AF_INET, SOCK_STREAM, 0);

        if (send_socket == INVALID_SOCKET || udp_worker_socket == INVALID_SOCKET || tcp_client_socket == INVALID_SOCKET) {
            throw std::runtime_error("Socket creation failed");
        }

        setup_udp_socket();
        setup_tcp_socket();
    }

    void setup_udp_socket() {   // Bindujemo UDP socket na LB_PORT
        struct sockaddr_in lb_udp_addr = { 0 };
        lb_udp_addr.sin_family = AF_INET;
        lb_udp_addr.sin_port = htons(LB_PORT);
        lb_udp_addr.sin_addr.s_addr = INADDR_ANY;

        // LB prima registracije od WR-a
        if (bind(udp_worker_socket, (struct sockaddr*)&lb_udp_addr, sizeof(lb_udp_addr)) == SOCKET_ERROR) {
            throw std::runtime_error("UDP Bind failed");
        }
    }

    void setup_tcp_socket() {   // Binduje TCP socket na LB_PORT i postavljamo ga u listen mode
        struct sockaddr_in lb_tcp_addr = { 0 };
        lb_tcp_addr.sin_family = AF_INET;
        lb_tcp_addr.sin_port = htons(LB_PORT);
        lb_tcp_addr.sin_addr.s_addr = INADDR_ANY;

        // Omogucava LB da prihvata klijentske konekcije
        if (bind(tcp_client_socket, (struct sockaddr*)&lb_tcp_addr, sizeof(lb_tcp_addr)) == SOCKET_ERROR) {
            throw std::runtime_error("TCP Bind failed");
        }

        if (listen(tcp_client_socket, SOMAXCONN) == SOCKET_ERROR) {
            throw std::runtime_error("Listen failed");
        }
    }

    void register_worker(char* buffer) {    // Obradjuje registracionu poruku od WR-a(format: ID;IP;PORT;STATUS).
        std::lock_guard<std::mutex> lock(workers_mutex);

        char* token = NULL;
        char id[10], ip[16];
        int port{};
        bool status{};

        if (std::count(buffer, buffer + strlen(buffer), ';') != 3) {
            printf("Invalid registration format: %s\n", buffer);
            return;
        }

        // strtok_s, razbijamo string na vise manjih tokena
        token = strtok_s(buffer, ";", &buffer);
        if (token) strcpy_s(id, sizeof(id), token);

        token = strtok_s(NULL, ";", &buffer);
        if (token) strcpy_s(ip, sizeof(ip), token);

        token = strtok_s(NULL, ";", &buffer);
        if (token) port = atoi(token);

        token = strtok_s(NULL, ";", &buffer);
        if (token) status = atoi(token);

        // inicijalizujemo WR-a i dodajemo ga u workersArray 
        Worker worker;
        initialize_worker(&worker, id, ip, port, status);
        add_worker(&workersArray, &worker);

        printf("Worker %s registered to LB.\n", worker.ID);
        display_dynamic_array(&workersArray);

        // Obavestavamo cekajuce niti u vezi novog WR-a
        worker_condition.notify_all();
    }

    void forward_to_worker(const char* message, Worker* worker) {
        std::lock_guard<std::mutex> lock(client_mutex);

        struct sockaddr_in worker_addr = { 0 };
        worker_addr.sin_family = AF_INET;
        // Koristi adresu i port Workera iz workersArray-a
        worker_addr.sin_port = htons(worker->Port);
        inet_pton(AF_INET, worker->IP, &worker_addr.sin_addr);

        // Prosledjuje poruku od klijenta izabranom WR-u koristeci UDP
        int result = sendto(send_socket, message, strlen(message), 0,
            (struct sockaddr*)&worker_addr, sizeof(worker_addr));

        // Provera uspesnosti slanja poruke
        if (result == SOCKET_ERROR) {
            printf("Failed to send message to Worker %s: %d\n", worker->ID, WSAGetLastError());
        }
        else {
            printf("Message forwarded to Worker.\n");
        }
    }

    void handle_client_connection(SOCKET client_socket) {   // Prima i obradjuje zahtev od klijenta preko TCP-a
        char buffer[1024] = { 0 };

        while (true) {
            int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received <= 0) {
                printf("Client disconnected or recv failed.\n");
                break;
            }

            buffer[bytes_received] = '\0';
            printf("Received from Client: %s\n", buffer);

            std::unique_lock<std::mutex> lock(workers_mutex);
            worker_condition.wait(lock, [this] {    // Cekamo na dostupnog WR-a
                return workersArray.size > 0;
                });

            if (workersArray.size > 0) {
                Worker* selected_worker = &workersArray.array[worker_index];
                forward_to_worker(buffer, selected_worker); // Prosledjujemo poruku izabranom WR-u

                //worker_index = (worker_index + 1) % workersArray.size;
                worker_index = 0;

                // Javljamo klijentu da li je poruka uspesno skladistena
                const char* response = "Message successfully stored.";
                send(client_socket, response, strlen(response), 0);
            }
            else {
                const char* response = "No available Workers.";
                send(client_socket, response, strlen(response), 0);
            }
        }
        // Nakon zavrsetka zatvaramo socket
        closesocket(client_socket);
    }

    void worker_registration_thread() { // Slusa registracione poruke preko UDP-a
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

            // Registracija se obavlja pomocu niti iz ThreadPool-a pozivajuci register_worker()
            threadPool.enqueue([this, buffer_copy = std::string(buffer)]{
                register_worker(const_cast<char*>(buffer_copy.c_str()));
                });

        }
    }

    void start() {
        // Pokrecemo worker_registration_thread za obradu registracija
        std::thread udp_thread(&LoadBalancer::worker_registration_thread, this);
        udp_thread.detach();

        // Prihvatamo klijentske konekcije
        while (true) {
            struct sockaddr_in client_addr;
            int client_addr_len = sizeof(client_addr);

            SOCKET client_socket = accept(tcp_client_socket, (struct sockaddr*)&client_addr, &client_addr_len);
            if (client_socket == INVALID_SOCKET) {
                printf("accept() failed with error: %d\n", WSAGetLastError());
                continue;
            }

            printf("Client connected.\n");

            // Prosledjujemo klijentske konekcije ThreadPool-u za paralelnu obradu
            threadPool.enqueue([this, client_socket] {
                handle_client_connection(client_socket);
                });
        }
    }

    ~LoadBalancer() {   // Zatvaramo sve socket-e i oslobadjamo resurse vezane sa Winsock-om
        closesocket(udp_worker_socket);
        closesocket(tcp_client_socket);
        closesocket(send_socket);
        cleanup_winsock();
    }
#pragma endregion

};
#pragma endregion
