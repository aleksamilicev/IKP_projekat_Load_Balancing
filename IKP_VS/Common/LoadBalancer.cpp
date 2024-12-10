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

#define LB_PORT 5059
#define WORKER_PORT 5060
#define WORKER_IP "127.0.0.1"
#define MAX_THREADS 10
#define MAX_QUEUE_SIZE 100

class LoadBalancer {
private:
    std::mutex workers_mutex;
    std::mutex client_mutex;
    std::condition_variable worker_condition;

    DynamicArray workersArray;
    ThreadPool threadPool;

    std::atomic<int> worker_index;
    SOCKET send_socket;
    SOCKET udp_worker_socket;
    SOCKET tcp_client_socket;

public:
    LoadBalancer() :
        threadPool(MAX_THREADS),
        worker_index(0)
    {
        initialize_winsock();
        initialize_dynamic_array(&workersArray, 10);
        setup_sockets();
    }

    void setup_sockets() {
        send_socket = socket(AF_INET, SOCK_DGRAM, 0);
        udp_worker_socket = socket(AF_INET, SOCK_DGRAM, 0);
        tcp_client_socket = socket(AF_INET, SOCK_STREAM, 0);

        if (send_socket == INVALID_SOCKET || udp_worker_socket == INVALID_SOCKET || tcp_client_socket == INVALID_SOCKET) {
            throw std::runtime_error("Socket creation failed");
        }

        setup_udp_socket();
        setup_tcp_socket();
    }

    void setup_udp_socket() {
        struct sockaddr_in lb_udp_addr = { 0 };
        lb_udp_addr.sin_family = AF_INET;
        lb_udp_addr.sin_port = htons(LB_PORT);
        lb_udp_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(udp_worker_socket, (struct sockaddr*)&lb_udp_addr, sizeof(lb_udp_addr)) == SOCKET_ERROR) {
            throw std::runtime_error("UDP Bind failed");
        }
    }

    void setup_tcp_socket() {
        struct sockaddr_in lb_tcp_addr = { 0 };
        lb_tcp_addr.sin_family = AF_INET;
        lb_tcp_addr.sin_port = htons(LB_PORT);
        lb_tcp_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(tcp_client_socket, (struct sockaddr*)&lb_tcp_addr, sizeof(lb_tcp_addr)) == SOCKET_ERROR) {
            throw std::runtime_error("TCP Bind failed");
        }

        if (listen(tcp_client_socket, SOMAXCONN) == SOCKET_ERROR) {
            throw std::runtime_error("Listen failed");
        }
    }

    void register_worker(char* buffer) {
        std::lock_guard<std::mutex> lock(workers_mutex);

        char* token = NULL;
        char id[10], ip[16];
        int port{};
        bool status{};

        if (std::count(buffer, buffer + strlen(buffer), ';') != 3) {
            printf("Invalid registration format: %s\n", buffer);
            return;
        }


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

        // Notify waiting threads about new worker
        worker_condition.notify_all();
    }

    void forward_to_worker(const char* message, Worker* worker) {
        std::lock_guard<std::mutex> lock(client_mutex);

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
        }
    }

    void handle_client_connection(SOCKET client_socket) {
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
            worker_condition.wait(lock, [this] {
                return workersArray.size > 0;
                });

            if (workersArray.size > 0) {
                Worker* selected_worker = &workersArray.array[worker_index];
                forward_to_worker(buffer, selected_worker);

                //worker_index = (worker_index + 1) % workersArray.size;
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

    void worker_registration_thread() {
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

            threadPool.enqueue([this, buffer_copy = std::string(buffer)]{
                register_worker(const_cast<char*>(buffer_copy.c_str()));
                });

        }
    }

    void start() {
        // Start worker registration thread
        std::thread udp_thread(&LoadBalancer::worker_registration_thread, this);
        udp_thread.detach();

        // Accept client connections
        while (true) {
            struct sockaddr_in client_addr;
            int client_addr_len = sizeof(client_addr);

            SOCKET client_socket = accept(tcp_client_socket, (struct sockaddr*)&client_addr, &client_addr_len);
            if (client_socket == INVALID_SOCKET) {
                printf("accept() failed with error: %d\n", WSAGetLastError());
                continue;
            }

            printf("Client connected.\n");

            // Use thread pool to handle client connection
            threadPool.enqueue([this, client_socket] {
                handle_client_connection(client_socket);
                });
        }
    }

    ~LoadBalancer() {
        closesocket(udp_worker_socket);
        closesocket(tcp_client_socket);
        closesocket(send_socket);
        cleanup_winsock();
    }
};