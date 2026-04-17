#pragma once
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#endif

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    bool connectToServer(const std::string& ip, int port);
    void sendAngles(float theta1, float theta2);
    void closeConnection();

private:
#ifdef _WIN32
    SOCKET socketHandle;
#endif
};