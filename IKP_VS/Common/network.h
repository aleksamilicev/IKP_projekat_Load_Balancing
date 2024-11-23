#ifndef NETWORK_H
#define NETWORK_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

void initialize_winsock();
SOCKET create_server_socket(int port);
SOCKET accept_connection(SOCKET server_socket);
SOCKET create_client_socket(const char* ip, int port);
void cleanup_winsock();

#endif
