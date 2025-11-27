#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include "Clode_monet.h"
#include "network_protocol.h"
#include "utils.h"


class Server {
private:
    WSADATA wsa;
    SOCKET listen_socket;
    RestaurantApp app;
    bool is_admin;

    std::string receive(SOCKET client) {
        char buffer[4096] = {0};
        int bytes = recv(client, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) return "";
        buffer[bytes] = '\0';
        return std::string(buffer);
    }

    void send_str(SOCKET client, const std::string& msg) {
        std::string full = msg + END_MSG;
        send(client, full.c_str(), (int)full.size(), 0);
    }

    void send_ok(SOCKET client) { send_str(client, RES_OK); }
    void send_error(SOCKET client, const std::string& err = "") {
        send_str(client, RES_ERROR + (err.empty() ? "" : " " + err));
    }

    void send_list(SOCKET client, const std::vector<std::string>& items) {
        for (const auto& item : items) {
            send_str(client, item);
        }
        send_str(client, RES_END);
    }

public:
    Server() : listen_socket(INVALID_SOCKET), is_admin(false) {}

    bool init(const std::string& ip, int port) {
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            std::cerr << "WSAStartup failed\n";
            return false;
        }

        listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listen_socket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed\n";
            WSACleanup();
            return false;
        }

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        if (bind(listen_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed\n";
            closesocket(listen_socket);
            WSACleanup();
            return false;
        }

        if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Listen failed\n";
            closesocket(listen_socket);
            WSACleanup();
            return false;
        }

        std::cout << "Server running on " << ip << ":" << port << "\n";
        return true;
    }

    void run() {
        while (true) {
            SOCKET client = accept(listen_socket, nullptr, nullptr);
            if (client == INVALID_SOCKET) {
                std::cerr << "Accept failed\n";
                continue;
            }

            std::cout << "Client connected\n";
            handle_client(client);
            closesocket(client);
            std::cout << "Client disconnected\n";
        }
    }

    void handle_client(SOCKET client) {
        int current_user_id = -1;
        is_admin = false;

        while (true) {
            std::string msg = receive(client);
            if (msg.empty()) break;

            // Убираем \n
            if (!msg.empty() && msg.back() == '\n') msg.pop_back();
            if (!msg.empty() && msg.back() == '\r') msg.pop_back();

            std::istringstream iss(msg);
            std::string command;
            iss >> command;

            if (command == CMD_LOGIN_ADMIN) {
                std::string password;
                iss >> password;
                if (password == "1234") {
                    is_admin = true;
                    current_user_id = -1;
                    send_ok(client);
                } else {
                    send_error(client, "Wrong password");
                }
            }
            else if (command == CMD_LOGIN_USER) {
                int user_id;
                iss >> user_id;
                if (!app.UserExists(user_id)) {
                    app.AddUser(user_id);
                }
                current_user_id = user_id;
                is_admin = false;
                send_ok(client);
            }
            else if (command == CMD_LOGOUT) {
                is_admin = false;
                current_user_id = -1;
                send_ok(client);
            }
            else if (!is_admin && current_user_id == -1 && command != CMD_LOGIN_USER && command != CMD_LOGIN_ADMIN) {
                send_error(client, "Not authorized");
                continue;
            }

            // === ПОЛЬЗОВАТЕЛЬСКИЕ КОМАНДЫ ===
            if (command == CMD_SHOW_MENU) {
                std::vector<std::string> lines;
                if (app.GetMenu().empty()) {
                    lines.push_back("Menu is empty.");
                } else {
                    for (size_t i = 0; i < app.GetMenu().size(); ++i) {
                        std::ostringstream oss;
                        oss << (i + 1) << ". " << app.GetMenu()[i].name << " - " << app.GetMenu()[i].price << " rub.";
                        lines.push_back(oss.str());
                    }
                }
                send_list(client, lines);
            }
            else if (command == CMD_CREATE_ORDER) {
                std::vector<std::string> dishes;
                double total = 0.0;
                std::string token;
                while (iss >> token) {
                    // token — это имя блюда
                    auto it = std::find_if(app.GetMenu().begin(), app.GetMenu().end(),
                        [&token](const Dish& d) { return d.name == token; });
                    if (it != app.GetMenu().end()) {
                        dishes.push_back(it->name);
                        total += it->price;
                    }
                }
                if (!dishes.empty()) {
                    orders_db::CreateOrder(&app.GetOrdersMutable(), current_user_id, dishes, total);
                    orders_db::Save(app.GetOrders(), app.GetOrdersFile());
                    send_str(client, RES_OK + std::string(" ") + std::to_string(total));
                } else {
                    send_error(client, "No valid dishes");
                }
            }
            else if (command == CMD_CHECK_STATUS) {
                std::vector<std::string> lines;
                bool found = false;
                for (const auto& o : app.GetOrders()) {
                    if (o.user_id == current_user_id) {
                        std::ostringstream oss;
                        oss << "ID " << o.id << " | Status: " << o.status << " | Total: " << o.total;
                        lines.push_back(oss.str());
                        found = true;
                    }
                }
                if (!found) lines.push_back("No orders.");
                send_list(client, lines);
            }
            else if (command == CMD_DEPOSIT) {
                double amount;
                iss >> amount;
                if (amount > 0) {
                    users_db::Deposit(&app.GetUserAccountsMutable(), current_user_id, amount);
                    users_db::Save(app.GetUserAccounts(), app.GetUsersFile());
                    send_str(client, RES_OK + std::string(" ") + std::to_string(app.GetAccountBalance(current_user_id)));
                } else {
                    send_error(client, "Invalid amount");
                }
            }
            else if (command == CMD_GET_BALANCE) {
                send_str(client, std::to_string(app.GetAccountBalance(current_user_id)));
            }

            // === АДМИН КОМАНДЫ ===
            else if (is_admin) {
                if (command == CMD_ADMIN_SHOW_MENU) {
                    std::vector<std::string> lines;
                    for (size_t i = 0; i < app.GetMenu().size(); ++i) {
                        std::ostringstream oss;
                        oss << (i + 1) << ". " << app.GetMenu()[i].name << " - " << app.GetMenu()[i].price;
                        lines.push_back(oss.str());
                    }
                    send_list(client, lines);
                }
                else if (command == CMD_ADMIN_ADD_DISH) {
                    std::string name;
                    double price;
                    std::getline(iss >> std::ws, name, ';');
                    iss >> price;
                    if (price > 0 && !name.empty()) {
                        Dish d = {name, price};
                        menu_db::AddDish(&app.GetMenuMutable(), d);
                        menu_db::Save(app.GetMenu(), app.GetMenuFile());
                        send_ok(client);
                    } else {
                        send_error(client, "Invalid data");
                    }
                }
                else if (command == CMD_ADMIN_REMOVE_DISH) {
                    size_t idx;
                    iss >> idx;
                    if (idx > 0 && idx <= app.GetMenu().size()) {
                        menu_db::RemoveDish(&app.GetMenuMutable(), idx - 1);
                        menu_db::Save(app.GetMenu(), app.GetMenuFile());
                        send_ok(client);
                    } else {
                        send_error(client, "Invalid index");
                    }
                }
                else if (command == CMD_ADMIN_UPDATE_STATUS) {
                    int order_id;
                    std::string status;
                    iss >> order_id;
                    std::getline(iss >> std::ws, status);
                    orders_db::UpdateStatus(&app.GetOrdersMutable(), order_id, status);
                    orders_db::Save(app.GetOrders(), app.GetOrdersFile());
                    send_ok(client);
                }
                else if (command == CMD_ADMIN_REMOVE_USER) {
                    int user_id;
                    iss >> user_id;
                    if (app.UserExists(user_id)) {
                        app.GetUserAccountsMutable().erase(user_id);
                        orders_db::RemoveUserOrders(&app.GetOrdersMutable(), user_id);
                        users_db::Save(app.GetUserAccounts(), app.GetUsersFile());
                        orders_db::Save(app.GetOrders(), app.GetOrdersFile());
                        send_ok(client);
                    } else {
                        send_error(client, "User not found");
                    }
                }
                else if (command == CMD_ADMIN_SHOW_ORDERS) {
                    std::vector<std::string> lines;
                    for (const auto& o : app.GetOrders()) {
                        std::ostringstream oss;
                        oss << "ID:" << o.id << " User:" << o.user_id
                            << " Status:" << o.status << " Total:" << o.total
                            << " Dishes:" << Join(o.dish_names, ", ");
                        lines.push_back(oss.str());
                    }
                    if (lines.empty()) lines.push_back("No orders.");
                    send_list(client, lines);
                }
                else if (command == CMD_ADMIN_SHOW_USERS) {
                    std::vector<std::string> lines;
                    for (const auto& p : app.GetUserAccounts()) {
                        std::ostringstream oss;
                        oss << "ID:" << p.first << " Balance:" << p.second;
                        lines.push_back(oss.str());
                    }
                    if (lines.empty()) lines.push_back("No users.");
                    send_list(client, lines);
                }
            }
            else {
                send_error(client, "Unknown command or access denied");
            }
        }
    }

    ~Server() {
        if (listen_socket != INVALID_SOCKET) closesocket(listen_socket);
        WSACleanup();
    }
};

int main() {
    Server server;
    if (!server.init("127.0.0.1", 8080)) {
        return 1;
    }
    server.run();
    return 0;
}