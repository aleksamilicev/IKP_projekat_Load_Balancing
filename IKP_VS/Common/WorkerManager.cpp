#pragma region Include
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
#pragma endregion

#pragma region Define
#define WR_PORT 5060
#define LB_PORT 5059
#define MAX_THREADS 4
#define MAX_QUEUE_SIZE 100
#pragma endregion


#pragma region Pomocna_funkcija
std::string int_to_string(int value) {  // Pomocna funkcija za konvertovanje int u string
    std::stringstream ss;
    ss << value;
    return ss.str();
}
#pragma endregion

#pragma region WorkerManager
class WorkerManager {
#pragma region Private
private:
#pragma region Attributes
    // Sinhronizacija
    std::mutex workers_mutex;
    std::mutex socket_mutex;

    std::condition_variable workers_condition;

    ThreadPool threadPool;

    std::vector<Worker> workers;
    std::atomic<int> worker_index;

    SOCKET wr_server_socket;
    int num_workers;    // Broj WR u sistemu
#pragma endregion

    void safe_log(const char* message) {    // U sustini isto kao i printf, zeleo sam da prosirim ovu f-ju ali sam na kraju ostavio ovako
        printf("%s\n", message);
    }

    void notify_workers(const char* message, const char* sender_id) {
        for (auto& worker : workers) {  // Sluzi za obavestenje svih WR-a i azuriranje poruka
            if (strcmp(worker.ID, sender_id) != 0) {
                if (!cb_push(&worker.messageBuffer, _strdup(message))) {
                    safe_log(("Buffer full for Worker " + std::string(worker.ID)).c_str());
                }
            }
        }
    }

    void setup_socket() {   // Bindujemo UDP socket na WR_PORT
        wr_server_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (wr_server_socket == INVALID_SOCKET) {
            throw std::runtime_error("Failed to create UDP socket");
        }

        struct sockaddr_in wr_addr = { 0 };
        wr_addr.sin_family = AF_INET;
        wr_addr.sin_port = htons(WR_PORT);
        wr_addr.sin_addr.s_addr = INADDR_ANY;

        // Prima poruke od LB-a
        if (bind(wr_server_socket, (struct sockaddr*)&wr_addr, sizeof(wr_addr)) == SOCKET_ERROR) {
            closesocket(wr_server_socket);
            throw std::runtime_error("Bind failed");
        }

        std::string log_message = "Worker is listening on port " + int_to_string(WR_PORT) + "...";
        safe_log(log_message.c_str());
    }

    void register_workers() {   // Inicijalizacija num_workers WR-a i slanje registracije LB-u
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

            // Inicijalizacija kruznog buffer-a
            cb_initialize(&worker.messageBuffer, 200);  // Buffer sa kapacitetom od 200 poruka

            workers.push_back(worker);

            display_worker(&worker);

            char registration_message[1024];
            snprintf(registration_message, sizeof(registration_message), "%s;%s;%d;%d",
                worker.ID, worker.IP, worker.Port, worker.Status);

            // Slanje registracije LB-u
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

    void process_message(const char* buffer) {  // Prihvatanje poruke od LB-a i obrada
        std::lock_guard<std::mutex> lock(workers_mutex);

        Worker* target_worker = &workers[worker_index];
        worker_index = (worker_index + 1) % num_workers;    // U LB-u ima neki cudan delay kad koristim RR, pa RR radim ovde

        // Dodajemo poruku u kruzni buffer
        if (!cb_push(&target_worker->messageBuffer, _strdup(buffer))) {
            safe_log(("Buffer full for Worker " + std::string(target_worker->ID)).c_str());
            return;
        }

        safe_log(("Message added to Worker " + std::string(target_worker->ID)).c_str());

        // Ispis stanja WR-a pre notifikacije i sinhronizacije poruka
        safe_log("Workers' state before notification:");
        for (const auto& worker : workers) {
            std::string state = worker.ID + std::string(": [");
            size_t current = worker.messageBuffer.tail;
            for (size_t i = 0; i < worker.messageBuffer.size; ++i) {
                char* message = static_cast<char*>(worker.messageBuffer.buffer[current]);
                if (message) {
                    state += message;
                    if (i < worker.messageBuffer.size - 1) {
                        state += ", ";
                    }
                }
                current = (current + 1) % worker.messageBuffer.capacity;
            }
            state += "]";
            safe_log(state.c_str());
        }

        // Saljemo obavestenje drugim WR-ima
        notify_workers(buffer, target_worker->ID);
        safe_log(("- " + std::string(target_worker->ID) + " notifies other workers").c_str());

        // Ispis stanja WR-a nakon notifikacije i sinhronizacije poruka
        safe_log("Workers' state after notification:");
        for (const auto& worker : workers) {
            std::string state = worker.ID + std::string(": [");
            size_t current = worker.messageBuffer.tail;
            for (size_t i = 0; i < worker.messageBuffer.size; ++i) {
                char* message = static_cast<char*>(worker.messageBuffer.buffer[current]);
                if (message) {
                    state += message;
                    if (i < worker.messageBuffer.size - 1) {
                        state += ", ";
                    }
                }
                current = (current + 1) % worker.messageBuffer.capacity;
            }
            state += "]";
            safe_log(state.c_str());
        }
    }

    void send_response_to_lb(const struct sockaddr_in& lb_addr) {   // Slanje poruke LB-u o uspesnosti skladistenja poruke
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
#pragma endregion

#pragma region Public
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
            // Primamo poruke od LB-a
            int bytes_received = recvfrom(wr_server_socket, buffer, sizeof(buffer) - 1, 0,
                (struct sockaddr*)&lb_addr, &from_addr_len);

            if (bytes_received == SOCKET_ERROR) {
                safe_log("recvfrom failed");
                continue;
            }

            buffer[bytes_received] = '\0';
            safe_log(("Received from LB: " + std::string(buffer)).c_str());

            // Primamo poruku i uz ThreadPool paralelno obradjujemo zahtev
            threadPool.enqueue([this, buffer_copy = std::string(buffer), lb_addr]() {
                process_message(buffer_copy.c_str());
                send_response_to_lb(lb_addr);
            });// Lambda f-ja kreira kopiju poruke da bi se izbegle greske usled prepisivanja buffer-a
               // Na ovaj nacin, svaka nit radi sa svojim privatnim podacima i ovo eliminise rizik od race condition-a

        }
    }

    ~WorkerManager() {  // Oslobadjamo resurse
        closesocket(wr_server_socket);
        cleanup_winsock();
    }
#pragma endregion
};
#pragma endregion

