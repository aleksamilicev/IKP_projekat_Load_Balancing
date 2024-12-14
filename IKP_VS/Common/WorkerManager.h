#ifndef WORKER_MANAGER_H
#define WORKER_MANAGER_H

#pragma region Include
#include "../Common/Threadpool.cpp"
#include "../Common/CircularBuffer.cpp"
#include "../Worker/Worker.h"
#include "../Common/network.cpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <atomic>
#include <functional>
#include <sstream>
#pragma endregion

#pragma region Define
#define WR_PORT 5060
#define LB_PORT 5059
#define MAX_THREADS 4
#define MAX_QUEUE_SIZE 100
#pragma endregion

class WorkerManager {
private:
    std::mutex workers_mutex; // Stiti pristup vektoru workera
    std::mutex socket_mutex;  // Stiti pristup soketu
    std::condition_variable workers_condition; // Signalizacija izmedju niti

    ThreadPool threadPool; // Pool za obradu poruka

    std::vector<Worker> workers; // Lista svih workera
    std::atomic<int> worker_index; // Indeks za dodelu radnika (round-robin), jer LB ima neki neobican delay

    SOCKET wr_server_socket; // UDP soket za prijem podataka
    int num_workers; // Broj workera

    void safe_log(const char* message); // Log funkcija
    void notify_workers(const char* message, const char* sender_id); // Notifikacija radnicima
    void setup_socket(); // Postavlja UDP soket za komunikaciju
    void register_workers(); // Registruje workere kod Load Balancera
    void process_message(const char* buffer); // Obrada primljene poruke
    void send_response_to_lb(const struct sockaddr_in& lb_addr); // Odgovor Load Balanceru nakon obrade

public:
    WorkerManager(int num_workers_count = 3); // Konstruktor za inicijalizaciju workera
    ~WorkerManager(); // Destruktor za oslobaðanje resursa

    void start(); // Glavna funkcija za pokretanje workera i slusanje poruka
};

#endif // WORKER_MANAGER_H
