#include "NetworkClient.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

NetworkClient::NetworkClient() : socketHandle(INVALID_SOCKET) {}

NetworkClient::~NetworkClient() {
    closeConnection();
}

bool NetworkClient::connectToServer(const std::string& ip, int port) {

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        std::cout << "WSAStartup failed\n";
        return false;
    }

    socketHandle = socket(AF_INET, SOCK_STREAM, 0);
    if (socketHandle == INVALID_SOCKET) {
        std::cout << "Socket creation failed\n";
        return false;
    }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &server.sin_addr) <= 0) {
        std::cout << "Invalid address\n";
        return false;
    }

    if (connect(socketHandle, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        std::cout << "Connection failed\n";
        closesocket(socketHandle);
        socketHandle = INVALID_SOCKET;
        return false;
    }

    std::cout << "Connected to ESP32\n";
    return true;
}

void NetworkClient::sendAngles(float theta1, float theta2) {
    if (socketHandle == INVALID_SOCKET) return;

    char buffer[64];
    sprintf_s(buffer, "%0.2f %0.2f\n", theta1, theta2);

    send(socketHandle, buffer, (int)strlen(buffer), 0);
}

void NetworkClient::closeConnection() {
    if (socketHandle != INVALID_SOCKET) {
        closesocket(socketHandle);
        WSACleanup();
        socketHandle = INVALID_SOCKET;
    }
}