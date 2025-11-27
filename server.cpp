#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>


#include <iostream>
#include <thread>
#include <sstream>
#include <algorithm>
#include <string>

#include "Clode_monet.h"
#include "network_protocol.h"

#pragma comment(lib, "ws2_32.lib")

class Server {
private:
    SOCKET listenSocket;
    RestaurantApp app;

    void handleClient(SOCKET clientSock) {
        char buffer[4096];
        while (true) {
            int bytes = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
            if (bytes <= 0) break;
            buffer[bytes] = '\0';
            std::string request = buffer;

            std::cout << "Запрос: " << request << std::endl;

            std::string response = processRequest(request);
            send(clientSock, response.c_str(), response.size(), 0);
            send(clientSock, "\n", 1, 0);
        }
        closesocket(clientSock);
    }

    std::string processRequest(const std::string& req) {
        auto parts = split(req, ' ');
        if (parts.empty()) return RES_ERROR + " Empty request";

        std::string cmd = parts[0];

        if (cmd == CMD_SHOW_MENU) {
            std::ostringstream oss;
            for (const auto& dish : app.getMenu()) {
                oss << dish.name << " - " << dish.price << " руб.\n";
            }
            return oss.str() + RES_END;
        }
        else if (cmd == CMD_CREATE_ORDER && parts.size() >= 3) {
            int userId = std::stoi(parts[1]);
            std::vector<std::string> dishes(parts.begin() + 2, parts.end());
            app.createOrderNetwork(userId, dishes);
            return RES_OK + " Order created";
        }
        else if (cmd == CMD_CHECK_STATUS && parts.size() == 2) {
            int userId = std::stoi(parts[1]);
            return app.getOrderStatusNetwork(userId);
        }
        else if (cmd == CMD_DEPOSIT && parts.size() == 3) {
            int userId = std::stoi(parts[1]);
            double amount = std::stod(parts[2]);
            app.depositAccount(userId, amount);
            return RES_OK + " Deposited";
        }
        else if (cmd == CMD_GET_BALANCE && parts.size() == 2) {
            int userId = std::stoi(parts[1]);
            return std::to_string(app.getAccountBalance(userId));
        }
        else if (cmd == CMD_ADD_DISH && parts.size() == 3) {
            app.addDishToMenuNetwork(parts[1], std::stod(parts[2]));
            return RES_OK;
        }
        else if (cmd == CMD_REMOVE_DISH && parts.size() == 2) {
            app.removeDishFromMenuNetwork(std::stoi(parts[1]));
            return RES_OK;
        }
        else if (cmd == CMD_UPDATE_STATUS && parts.size() == 3) {
            app.updateOrderStatusNetwork(std::stoi(parts[1]), parts[2]);
            return RES_OK;
        }
        else if (cmd == CMD_REMOVE_USER && parts.size() == 2) {
            app.removeUserNetwork(std::stoi(parts[1]));
            return RES_OK;
        }

        return RES_ERROR + " Unknown command";
    }

public:
    Server() {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2,2), &wsa);
        listenSocket = socket(AF_INET, SOCK_STREAM, 0);

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(PORT);
        addr.sin_addr.s_addr = INADDR_ANY;

        bind(listenSocket, (sockaddr*)&addr, sizeof(addr));
        listen(listenSocket, SOMAXCONN);
        std::cout << "Сервер запущен на порту " << PORT << std::endl;
    }

    void run() {
        while (true) {
            SOCKET client = accept(listenSocket, nullptr, nullptr);
            std::thread(&Server::handleClient, this, client).detach();
        }
    }

    ~Server() {
        closesocket(listenSocket);
        WSACleanup();
    }
};

int main() {
    Server server;
    server.run();
    return 0;
}