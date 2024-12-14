#ifndef LOAD_BALANCER_H
#define LOAD_BALANCER_H

#include "../Common/network.cpp"
#include "../Common/DynamicArray.h"
#include "../Common/ThreadPool.cpp"
#include "../Worker/Worker.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <atomic>
#include <stdexcept>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// Portovi i mrezni parametri
#define LB_PORT 5059
#define WORKER_PORT 5060
#define WORKER_IP "127.0.0.1"
#define MAX_THREADS 10

class LoadBalancer {
private:
    // Sinhronizacija
    std::mutex workers_mutex;             // Zakljucavanje za pristup Workerima
    std::mutex client_mutex;              // Zakljucavanje za rad sa klijentima
    std::condition_variable worker_condition;  // Obavestenje o dostupnosti Workera

    // Strukture podataka
    DynamicArray workersArray;            // Dinamicki niz za skladistenje Workera
    ThreadPool threadPool;                // Pool niti za obradu zahteva

    std::atomic<int> worker_index;        // Indeks za dodeljivanje Workera klijentima

    // Soketi
    SOCKET send_socket;                   // UDP soket za komunikaciju sa Workerima
    SOCKET udp_worker_socket;             // UDP soket za prijem registracija
    SOCKET tcp_client_socket;             // TCP soket za prihvatanje klijentskih konekcija

    // Privatne metode
    void setup_sockets();                 // Inicijalizacija svih soketa
    void setup_udp_socket();              // Konfiguracija UDP soketa za registraciju
    void setup_tcp_socket();              // Konfiguracija TCP soketa za klijente

    void register_worker(char* buffer);   // Registruje novog Workera
    void forward_to_worker(const char* message, Worker* worker); // Prosledjuje poruku Workeru
    void handle_client_connection(SOCKET client_socket);         // Obrada klijentske konekcije
    void worker_registration_thread();    // Obradjuje registracije Workera u petlji

public:
    // Konstruktor i destruktor
    LoadBalancer();                       // Inicijalizacija resursa
    ~LoadBalancer();                      // Oslobadja resurse

    void start();                         // Pokrece Load Balancer (radne niti i obrada klijenata)
};

#endif // LOAD_BALANCER_H
