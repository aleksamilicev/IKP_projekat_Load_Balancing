#ifndef CLIENT_H
#define CLIENT_H

#include <winsock2.h> // Za mrežnu komunikaciju u Windows okruženju
#include <ws2tcpip.h> // Za dodatne funkcije mreže
#include <stdio.h>    // Za standardni I/O
#include <stdlib.h>   // Za opšte funkcije
#include <string.h>   // Za rad sa stringovima

#pragma comment(lib, "Ws2_32.lib") // Linkovanje biblioteke za mrežu

// Definišemo konstantne vrednosti za IP adresu i port Load Balancera
#define LB_IP "127.0.0.1"
#define LB_PORT 5059

// Funkcije za mrežnu komunikaciju
void initialize_winsock();                   // Inicijalizacija Winsock-a
SOCKET create_client_socket(const char* ip, unsigned short port); // Kreiranje TCP socket-a
void cleanup_winsock();                      // Èišæenje Winsock-a

// Funkcija za slanje poruka Load Balancer-u
void send_messages(SOCKET client_socket, int num_messages);

#endif // CLIENT_H
