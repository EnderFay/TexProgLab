// clode_monet.cpp — СЕТЕВОЙ КЛИЕНТ
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <limits>
#include "network_protocol.h"

#pragma comment(lib, "Ws2_32.lib")

class ClodeMonetClient {
private:
    SOCKET sock = INVALID_SOCKET;
    int user_id = -1;
    bool is_admin = false;

    bool connect_to_server(const std::string& ip = "127.0.0.1", int port = 8080) {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
            std::cerr << "WSAStartup failed\n";
            return false;
        }

        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) {
            std::cerr << "Socket creation failed\n";
            WSACleanup();
            return false;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
            std::cerr << "Invalid IP address\n";
            closesocket(sock);
            WSACleanup();
            return false;
        }

        if (::connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            std::cerr << "Connection failed. Is server running?\n";
            closesocket(sock);
            WSACleanup();
            return false;
        }

        std::cout << "\n\tRestaurant Clode Monet - connected to server!\n\n";
        return true;
    }

    void send_command(const std::string& cmd) {
        std::string msg = cmd + "\n";
        send(sock, msg.c_str(), (int)msg.size(), 0);
    }

    std::string receive_line() {
        char buffer[4096] = {0};
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) return "";
        buffer[bytes] = '\0';
        std::string line = buffer;
        if (!line.empty() && line.back() == '\n') line.pop_back();
        if (!line.empty() && line.back() == '\r') line.pop_back();
        return line;
    }

    std::vector<std::string> receive_list() {
        std::vector<std::string> lines;
        while (true) {
            std::string line = receive_line();
            if (line.empty() || line == RES_END) break;
            if (line.rfind(RES_ERROR, 0) == 0) {
                std::cout << "Server error: " << line.substr(6) << "\n";
                break;
            }
            lines.push_back(line);
        }
        return lines;
    }

    void clear_input() {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

public:
    void run() {
        if (!connect_to_server()) {
            std::cout << "Failed to connect. Check if the server is running.\n";
            return;
        }

        while (true) {
            std::cout << "\tRestaurant Clode Monet welcomes you!\n"
                      << "\nChoose the type of authorization\n"
                      << "1. Administrator\n"
                      << "2. User\n"
                      << "0. Exit\n"
                      << "Your choice: ";

            int choice_auth;
            std::cin >> choice_auth;
            clear_input();

            if (choice_auth == 0) break;

            if (choice_auth == 1) {
                std::cout << "\nEnter the password: ";
                std::string password;
                std::cin >> password;
                clear_input();

                send_command(CMD_LOGIN_ADMIN " " + password);
                std::string response = receive_line();

                if (response == RES_OK) {
                    is_admin = true;
                    user_id = -1;
                    std::cout << "Administrator mode activated.\n";
                    admin_menu();
                } else {
                    std::cout << "Wrong password!\n";
                }
            }
            else if (choice_auth == 2) {
                std::cout << "\nWrite user ID: ";
                std::cin >> user_id;
                clear_input();

                send_command(CMD_LOGIN_USER " " + std::to_string(user_id));
                if (receive_line() == RES_OK) {
                    is_admin = false;
                    std::cout << "User (ID " << user_id << ") logged in.\n";
                    user_menu();
                } else {
                    std::cout << "Login failed.\n";
                }
            }
            else {
                std::cout << "Invalid action!\n";
            }
        }

        if (sock != INVALID_SOCKET) {
            closesocket(sock);
            WSACleanup();
        }
        std::cout << "\nGood bye! See you later!\n";
    }

private:
    void user_menu() {
        while (true) {
            std::cout << "\n\tUser (ID " << user_id << ")\n"
                      << "1. Show menu\n"
                      << "2. Make an order\n"
                      << "3. Check the order status\n"
                      << "4. Make a deposit\n"
                      << "5. Check balance\n"
                      << "0. Exit\n"
                      << "Your choice: ";

            int choice;
            std::cin >> choice;
            clear_input();

            if (choice == 0) break;

            switch (choice) {
            case 1:
                send_command(CMD_SHOW_MENU);
                for (const auto& line : receive_list()) {
                    std::cout << line << "\n";
                }
                break;

            case 2: {
                send_command(CMD_SHOW_MENU);
                auto menu_lines = receive_list();
                for (const auto& line : menu_lines) {
                    std::cout << line << "\n";
                }
                if (menu_lines.empty() || menu_lines[0].find("empty") != std::string::npos) break;

                std::cout << "\nEnter dish names (space-separated, 0 to finish): ";
                std::string input;
                std::getline(std::cin, input);
                if (input == "0") break;

                std::stringstream ss(input);
                std::string dish, cmd = CMD_CREATE_ORDER;
                while (ss >> dish) {
                    cmd += " " + dish;
                }
                send_command(cmd);
                std::string res = receive_line();
                if (res.rfind(RES_OK, 0) == 0) {
                    std::cout << "Order created! Total: " << res.substr(3) << " rub.\n";
                } else {
                    std::cout << "Failed to create order.\n";
                }
                break;
            }

            case 3:
                send_command(CMD_CHECK_STATUS);
                for (const auto& line : receive_list()) {
                    std::cout << line << "\n";
                }
                break;

            case 4: {
                double amount;
                std::cout << "Enter deposit amount: ";
                std::cin >> amount;
                clear_input();
                if (amount <= 0) {
                    std::cout << "Amount must be positive.\n";
                    break;
                }
                send_command(CMD_DEPOSIT " " + std::to_string(amount));
                std::string res = receive_line();
                if (res.rfind(RES_OK, 0) == 0) {
                    std::cout << "Deposited! New balance: " << res.substr(3) << " rub.\n";
                }
                break;
            }

            case 5:
                send_command(CMD_GET_BALANCE);
                std::cout << "Balance: " << receive_line() << " rub.\n";
                break;

            default:
                std::cout << "Wrong input!\n";
            }
        }
    }

    void admin_menu() {
        while (true) {
            std::cout << "\n\tAdministrator mode\n"
                      << "1. Update order status\n"
                      << "2. Add new dish\n"
                      << "3. Remove the dish\n"
                      << "4. Remove the user\n"
                      << "5. Show menu\n"
                      << "6. Show list of Orders\n"
                      << "7. Show list of users\n"
                      << "0. Exit\n"
                      << "Your choice: ";

            int choice;
            std::cin >> choice;
            clear_input();

            if (choice == 0) break;

            switch (choice) {
            case 1: {
                int order_id;
                std::string status;
                std::cout << "Enter order ID: ";
                std::cin >> order_id;
                std::cout << "Enter new status (is_waiting/in_progress/completed): ";
                std::cin >> status;
                clear_input();
                send_command(CMD_ADMIN_UPDATE_STATUS " " + std::to_string(order_id) + " " + status);
                if (receive_line() == RES_OK) {
                    std::cout << "Status updated.\n";
                }
                break;
            }
            case 2: {
                std::string name;
                double price;
                std::cout << "Enter dish name: ";
                std::getline(std::cin, name);
                std::cout << "Enter price: ";
                std::cin >> price;
                clear_input();
                if (price <= 0) {
                    std::cout << "Price must be positive.\n";
                    break;
                }
                send_command(CMD_ADMIN_ADD_DISH " " + name + ";" + std::to_string(price));
                if (receive_line() == RES_OK) {
                    std::cout << "Dish added.\n";
                }
                break;
            }
            case 3: {
                send_command(CMD_ADMIN_SHOW_MENU);
                auto menu = receive_list();
                for (size_t i = 0; i < menu.size(); ++i) {
                    std::cout << (i + 1) << ". " << menu[i].substr(menu[i].find(". ") + 2) << "\n";
                }
                std::cout << "Enter dish number to remove: ";
                int idx;
                std::cin >> idx;
                clear_input();
                if (idx > 0 && idx <= (int)menu.size()) {
                    send_command(CMD_ADMIN_REMOVE_DISH " " + std::to_string(idx));
                    if (receive_line() == RES_OK) {
                        std::cout << "Dish removed.\n";
                    }
                } else {
                    std::cout << "Invalid number.\n";
                }
                break;
            }
            case 4: {
                int uid;
                std::cout << "Enter user ID to remove: ";
                std::cin >> uid;
                clear_input();
                send_command(CMD_ADMIN_REMOVE_USER " " + std::to_string(uid));
                if (receive_line() == RES_OK) {
                    std::cout << "User removed.\n";
                }
                break;
            }
            case 5:
                send_command(CMD_ADMIN_SHOW_MENU);
                for (const auto& line : receive_list()) {
                    std::cout << line << "\n";
                }
                break;
            case 6:
                send_command(CMD_ADMIN_SHOW_ORDERS);
                for (const auto& line : receive_list()) {
                    std::cout << line << "\n";
                }
                break;
            case 7:
                send_command(CMD_ADMIN_SHOW_USERS);
                for (const auto& line : receive_list()) {
                    std::cout << line << "\n";
                }
                break;
            default:
                std::cout << "Wrong input!\n";
            }
        }
    }
};

int main() {
    ClodeMonetClient client;
    client.run();
    return 0;
}