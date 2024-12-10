#include "../common/network.cpp"
#include "../Worker/Worker.h"
#include "../Common/CircularBuffer.cpp"
#include "../Common/Threadpool.cpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <atomic>
#include <chrono>
#include <functional>
#include <sstream>

#define WR_PORT 5060
#define LB_PORT 5059
#define MAX_THREADS 4
#define MAX_QUEUE_SIZE 100


// Manual string conversion function
std::string int_to_string(int value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

class WorkerManager {
private:
    std::mutex workers_mutex;
    std::mutex socket_mutex;
    std::condition_variable workers_condition;

    ThreadPool threadPool;

    std::vector<Worker> workers;
    std::atomic<int> worker_index;

    SOCKET wr_server_socket;
    int num_workers;

    // Thread-safe logging method
    void safe_log(const char* message) {
        //std::lock_guard<std::mutex> lock(socket_mutex);
        printf("%s\n", message);
    }

    void notify_workers(const char* message, const char* sender_id) {
        //std::lock_guard<std::mutex> lock(workers_mutex); // kad zakemontarisem ovo onda se uradi sync

        for (auto& worker : workers) {
            if (strcmp(worker.ID, sender_id) != 0) {
                add_message(&worker, message);
            }
        }
    }

    void setup_socket() {
        wr_server_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (wr_server_socket == INVALID_SOCKET) {
            throw std::runtime_error("Failed to create UDP socket");
        }

        struct sockaddr_in wr_addr = { 0 };
        wr_addr.sin_family = AF_INET;
        wr_addr.sin_port = htons(WR_PORT);
        wr_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(wr_server_socket, (struct sockaddr*)&wr_addr, sizeof(wr_addr)) == SOCKET_ERROR) {
            closesocket(wr_server_socket);
            throw std::runtime_error("Bind failed");
        }

        std::string log_message = "Worker is listening on port " + int_to_string(WR_PORT) + "...";
        safe_log(log_message.c_str());
    }

    void register_workers() {
        struct sockaddr_in lb_addr = { 0 };
        lb_addr.sin_family = AF_INET;
        lb_addr.sin_port = htons(LB_PORT);
        inet_pton(AF_INET, "127.0.0.1", &lb_addr.sin_addr);

        for (int i = 0; i < num_workers; ++i) {
            char id[10];
            snprintf(id, sizeof(id), "WR%d", i + 1);

            char ip[16] = "127.0.0.1";
            int port = WR_PORT + i;

            Worker worker;
            initialize_worker(&worker, id, ip, port, true);
            workers.push_back(worker);

            display_worker(&worker);

            char registration_message[1024];
            snprintf(registration_message, sizeof(registration_message), "%s;%s;%d;%d",
                worker.ID, worker.IP, worker.Port, worker.Status);

            int result = sendto(wr_server_socket, registration_message, strlen(registration_message), 0,
                (struct sockaddr*)&lb_addr, sizeof(lb_addr));

            if (result == SOCKET_ERROR) {
                safe_log(("Failed to send registration to LB for Worker " + std::string(worker.ID)).c_str());
            }
            else {
                safe_log(("Registration sent for Worker " + std::string(worker.ID)).c_str());
            }
        }
    }

    void process_message(const char* buffer) {
        std::lock_guard<std::mutex> lock(workers_mutex);

        Worker* target_worker = &workers[worker_index];
        worker_index = (worker_index + 1) % num_workers;

        add_message(target_worker, buffer);
        safe_log(("- " + std::string(target_worker->ID) + " stores " + buffer).c_str());

        // Log workers' state before notification
        safe_log("Workers' state before notification:");
        for (const auto& worker : workers) {
            std::string state = worker.ID + std::string(": [");
            for (int j = 0; j < worker.message_count; ++j) {
                state += worker.Data[j];
                if (j < worker.message_count - 1) {
                    state += ", ";
                }
            }
            state += "]";
            safe_log(state.c_str());
        }

        notify_workers(buffer, target_worker->ID);
        safe_log(("- " + std::string(target_worker->ID) + " notifies other workers").c_str());

        // Log workers' state after notification
        safe_log("Workers' state after notification:");
        for (const auto& worker : workers) {
            std::string state = worker.ID + std::string(": [");
            for (int j = 0; j < worker.message_count; ++j) {
                state += worker.Data[j];
                if (j < worker.message_count - 1) {
                    state += ", ";
                }
            }
            state += "]";
            safe_log(state.c_str());
        }
    }

    void send_response_to_lb(const struct sockaddr_in& lb_addr) {
        std::lock_guard<std::mutex> lock(socket_mutex);

        const char* response = "Message successfully stored.";
        int result = sendto(wr_server_socket, response, strlen(response), 0,
            (struct sockaddr*)&lb_addr, sizeof(lb_addr));

        if (result == SOCKET_ERROR) {
            safe_log("Failed to send response to LB");
        }
        else {
            safe_log("Response sent to LB.\n");
        }
    }
public:
    WorkerManager(int num_workers_count = 3) :
        threadPool(MAX_THREADS),
        worker_index(0),
        num_workers(num_workers_count)
    {
        initialize_winsock();
        setup_socket();
        register_workers();
    }

    void start() {
        char buffer[1024] = { 0 };
        struct sockaddr_in from_addr, lb_addr;
        int from_addr_len = sizeof(from_addr);

        while (true) {
            int bytes_received = recvfrom(wr_server_socket, buffer, sizeof(buffer) - 1, 0,
                (struct sockaddr*)&lb_addr, &from_addr_len);

            if (bytes_received == SOCKET_ERROR) {
                safe_log("recvfrom failed");
                continue;
            }

            buffer[bytes_received] = '\0';
            safe_log(("Received from LB: " + std::string(buffer)).c_str());

            // Use thread pool to process message
            threadPool.enqueue([this, buffer_copy = std::string(buffer), lb_addr]() {
                process_message(buffer_copy.c_str());
                send_response_to_lb(lb_addr);
            });// pravimo kopiju bafera za svaki zadatak u thread pool-u

        }
    }

    ~WorkerManager() {
        closesocket(wr_server_socket);
        cleanup_winsock();
    }
};