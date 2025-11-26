#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include "network_protocol.h"

#pragma comment(lib, "ws2_32.lib")

SOCKET connectToServer() {
    WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    connect(sock, (sockaddr*)&addr, sizeof(addr));
    return sock;
}

std::string sendCommand(const std::string& cmd) {
    SOCKET sock = connectToServer();
    send(sock, cmd.c_str(), cmd.size(), 0);

    std::string response;
    char buffer[4096];
    while (true) {
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) break;
        buffer[bytes] = '\0';
        response += buffer;
        if (response.find(RES_END) != std::string::npos) break;
    }
    closesocket(sock);
    size_t pos = response.find(RES_END);
    if (pos != std::string::npos) response = response.substr(0, pos);
    return response;
}

int main() {
    while (true) {
        std::cout << "\tRestaurant Clode Monet\n";
        std::cout << "1. Пользователь\n2. Админ\n0. Выход\n> ";
        int choice; std::cin >> choice;
        if (choice == 0) break;

        if (choice == 1) {
            int userId; std::cout << "ID: "; std::cin >> userId;
            while (true) {
                std::cout << "1. Меню 2. Заказ 3. Статус 4. Пополнить 5. Баланс 0. Назад\n> ";
                int c; std::cin >> c;
                if (c == 0) break;
                std::string cmd;
                switch (c) {
                    case 1: std::cout << sendCommand(CMD_SHOW_MENU) << std::endl; break;
                    case 2: {
                        std::cout << "Блюда (через пробел): ";
                        std::cin.ignore();
                        std::string line; std::getline(std::cin, line);
                        cmd = CMD_CREATE_ORDER + " " + std::to_string(userId) + " " + line;
                        std::cout << sendCommand(cmd) << std::endl;
                        break;
                    }
                    case 3: std::cout << sendCommand(CMD_CHECK_STATUS + " " + std::to_string(userId)) << std::endl; break;
                    case 4: double a; std::cout << "Сумма: "; std::cin >> a;
                            std::cout << sendCommand(CMD_DEPOSIT + " " + std::to_string(userId) + " " + std::to_string(a)) << std::endl; break;
                    case 5: std::cout << "Баланс: " << sendCommand(CMD_GET_BALANCE + " " + std::to_string(userId)) << " руб.\n"; break;
                }
            }
        }
        else if (choice == 2) {
            std::string pass; std::cout << "Пароль: "; std::cin >> pass;
            if (pass != "1234") { std::cout << "Неверно!\n"; continue; }
        }
    }
    return 0;
}