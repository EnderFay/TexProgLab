#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <thread>
#include <mutex>
#include "Clode_monet_new.h"
#include "network_protocol.h"
#include "utils.h"
#include "logger.h"

// GLOBAL OBJECT ANG MUTEX 
RestaurantApp* global_app = nullptr;
std::mutex app_mutex;

std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%H:%M:%S");
    return ss.str();
}

class Server {
private:
    WSADATA wsa_;
    SOCKET listen_socket_;
    bool is_admin_;
    Logger& logger_;

    std::string receive(SOCKET client) {
        char buffer[4096] = { 0 };
        int bytes = recv(client, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            logger_.network_error("Connection closed or error");
            return "";
        }
        buffer[bytes] = '\0';
        std::string msg = buffer;

        if (!msg.empty() && msg.back() == '\n') msg.pop_back();
        if (!msg.empty() && msg.back() == '\r') msg.pop_back();

        return msg;
    }

    void send_str(SOCKET client, const std::string& msg) {
        std::string full = msg + END_MSG;
        logger_.network_out(msg);
        send(client, full.c_str(), (int)full.size(), 0);
    }

    void send_ok(SOCKET client) {
        logger_.debug("Sending OK response");
        send_str(client, RES_OK);
    }

    void send_error(SOCKET client, const std::string& err = "") {
        std::string error_msg = RES_ERROR + (err.empty() ? "" : " " + err);
        logger_.warn("Sending ERROR: {}", error_msg);
        send_str(client, error_msg);
    }

    void send_list(SOCKET client, const std::vector<std::string>& items) {
        logger_.debug("Sending list with {} items", items.size());
        for (const auto& item : items) {
            send_str(client, item);
        }
        send_str(client, RES_END);
    }

public:
    Server() : listen_socket_(INVALID_SOCKET), is_admin_(false),
               logger_(get_server_logger()) {
        logger_.info("Server object created");
    }

    bool init(const std::string& ip, int port) {
        logger_.info("Initializing server on {}:{}", ip, port);

        if (WSAStartup(MAKEWORD(2, 2), &wsa_) != 0) {
            logger_.error("WSAStartup failed");
            return false;
        }

        listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listen_socket_ == INVALID_SOCKET) {
            logger_.error("Socket creation failed");
            WSACleanup();
            return false;
        }

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        if (bind(listen_socket_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            logger_.error("Bind failed on port {}", port);
            closesocket(listen_socket_);
            WSACleanup();
            return false;
        }

        if (listen(listen_socket_, SOMAXCONN) == SOCKET_ERROR) {
            logger_.error("Listen failed");
            closesocket(listen_socket_);
            WSACleanup();
            return false;
        }

        // creating a global RestaurantApp one once
        {
            std::lock_guard<std::mutex> lock(app_mutex);
            if (!global_app) {
                global_app = new RestaurantApp();
                logger_.info("Global RestaurantApp created");
            }
        }

        std::cout << "\n==========================================" << std::endl;
        std::cout << "[" << getCurrentTime() << "] ✅ SERVER STARTED SUCCESSFULLY" << std::endl;
        std::cout << "Address: " << ip << ":" << port << std::endl;
        std::cout << "Waiting for connections..." << std::endl;
        std::cout << "==========================================\n" << std::endl;

        logger_.info("Server successfully started on {}:{}", ip, port);
        return true;
    }

    void run() {
        logger_.info("Server entering main loop, waiting for connections");

        while (true) {
            SOCKET client = accept(listen_socket_, nullptr, nullptr);
            if (client == INVALID_SOCKET) {
                logger_.error("Accept failed");
                continue;
            }

            sockaddr_in client_addr;
            int addr_len = sizeof(client_addr);
            getpeername(client, (sockaddr*)&client_addr, &addr_len);
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

            logger_.info("Client connected (IP: {}, Socket: {})", client_ip, (int)client);
            
            std::cout << "\n==========================================" << std::endl;
            std::cout << "[" << getCurrentTime() << "] 🔗 NEW CLIENT CONNECTED" << std::endl;
            std::cout << "Client IP: " << client_ip << std::endl;
            std::cout << "==========================================\n" << std::endl;

            std::thread([this, client, client_ip]() {
                this->handle_client(client, client_ip);
            }).detach();
        }
    }

    void handle_client(SOCKET client, const std::string& client_ip) {
        logger_.info("Starting new client session for {}", client_ip);

        int current_user_id = -1;
        bool is_admin_local = false;

        while (true) {
            std::string msg = receive(client);
            if (msg.empty()) {
                logger_.info("Client {} disconnected or connection lost", client_ip);
                break;
            }

            logger_.network_in(msg);
            
            std::cout << "\n[" << getCurrentTime() << "] 📨 RECEIVED FROM CLIENT: \"" << msg << "\"" << std::endl;

            std::istringstream iss(msg);
            std::string command;
            iss >> command;

            if (command == CMD_LOGIN_ADMIN) {
                std::string password;
                iss >> password;

                logger_.command_received("LOGIN_ADMIN");

                if (password == "1234") {
                    is_admin_local = true;
                    current_user_id = -1;
                    send_ok(client);
                    logger_.auth_success(-1, true);
                } else {
                    send_error(client, "Wrong password");
                    logger_.auth_failed(-1, true, "Wrong password");
                }
                continue;
            }
            else if (command == CMD_LOGIN_USER) {
                int user_id;
                iss >> user_id;

                logger_.command_received("LOGIN_USER", "user_id=" + std::to_string(user_id));

                std::lock_guard<std::mutex> lock(app_mutex);
                
                if (!global_app->UserExists(user_id)) {
                    global_app->AddUser(user_id);
                    logger_.user_created(user_id, 0.0);
                }

                current_user_id = user_id;
                is_admin_local = false;
                send_ok(client);
                logger_.auth_success(user_id, false);
                continue;
            }
            else if (command == CMD_LOGOUT) {
                logger_.info("Logout requested by {}",
                           is_admin_local ? "ADMIN" : "USER_" + std::to_string(current_user_id));
                is_admin_local = false;
                current_user_id = -1;
                send_ok(client);
                continue;
            }

            if (!is_admin_local && current_user_id == -1) {
                logger_.warn("Unauthorized access attempt: {}", command);
                send_error(client, "Not authorized");
                continue;
            }

            // processing user commands
            std::lock_guard<std::mutex> lock(app_mutex);
            
            if (command == CMD_SHOW_MENU) {
                logger_.command_received("SHOW_MENU", "user_id=" + std::to_string(current_user_id));
                
                std::vector<std::string> lines;
                if (global_app->GetMenu().empty()) {
                    lines.push_back("Menu is empty.");
                    logger_.debug("Menu is empty");
                } else {
                    for (size_t i = 0; i < global_app->GetMenu().size(); ++i) {
                        std::ostringstream oss;
                        oss << (i + 1) << ". " << global_app->GetMenu()[i].name
                            << " - " << global_app->GetMenu()[i].price << " rub.";
                        lines.push_back(oss.str());
                    }
                    logger_.debug("Sending menu with {} items", lines.size());
                }
                send_list(client, lines);
            }
            else if (command == CMD_CREATE_ORDER) {
                logger_.command_received("CREATE_ORDER", "user_id=" + std::to_string(current_user_id));
                
                std::vector<std::string> dishes;
                double total = 0.0;
                int token;
                std::stringstream dish_details;
                std::vector<int> dish_numbers;

                std::string remaining_input;
                std::getline(iss, remaining_input);

                std::istringstream token_stream(remaining_input);
                while (token_stream >> token) {
                    dish_numbers.push_back(token);
                    if (token > 0 && token <= (int)global_app->GetMenu().size()) {
                        const Dish& dish = global_app->GetMenu()[token - 1];
                        dishes.push_back(dish.name);
                        total += dish.price;
                        dish_details << dish.name << " (" << dish.price << " rub.), ";
                        logger_.debug("User selected dish: {} - {} rub.", dish.name, dish.price);
                    } else {
                        logger_.error("Invalid dish number: {}", token);
                        send_error(client, "Invalid dish number: " + std::to_string(token));
                        goto next_command;
                    }
                }

                if (dishes.empty()) {
                    logger_.warn("Empty order attempt by user {}", current_user_id);
                    send_error(client, "No valid dishes");
                    goto next_command;
                }

                double balance = global_app->GetAccountBalance(current_user_id);
                logger_.debug("Balance check: user={}, balance={}, required={}",
                            current_user_id, balance, total);

                if (balance < total) {
                    logger_.warn("Insufficient funds for user {} (has: {}, needs: {})",
                               current_user_id, balance, total);
                    send_error(client, "Insufficient funds. Required: " + std::to_string(total) +
                              " rub., Available: " + std::to_string(balance) + " rub.");
                    goto next_command;
                }

                int new_order_id = global_app->GetOrders().empty() ? 1 : global_app->GetOrders().back().id + 1;
                orders_db::CreateOrder(&global_app->GetOrdersMutable(), current_user_id, dishes, total);
                logger_.order_created(new_order_id, current_user_id, total);

                double old_balance = global_app->GetAccountBalance(current_user_id);
                users_db::Deposit(&global_app->GetUserAccountsMutable(), current_user_id, -total);
                users_db::Save(global_app->GetUserAccounts(), global_app->GetUsersFile());
                orders_db::Save(global_app->GetOrders(), global_app->GetOrdersFile());

                double new_balance = global_app->GetAccountBalance(current_user_id);
                logger_.user_balance_changed(current_user_id, old_balance, new_balance);

                send_str(client, std::string(RES_OK) + " " + std::to_string(total) +
                         " (Balance after order: " + std::to_string(new_balance) + ")");
                logger_.command_success("CREATE_ORDER", "user=" + std::to_string(current_user_id) +
                                        ", total=" + std::to_string(total));
            }
            else if (command == CMD_CHECK_STATUS) {
                logger_.command_received("CHECK_STATUS", "user_id=" + std::to_string(current_user_id));
                
                std::vector<std::string> lines;
                bool found = false;
                for (const auto& o : global_app->GetOrders()) {
                    if (o.user_id == current_user_id) {
                        std::ostringstream oss;
                        oss << "ID " << o.id << " | Status: " << o.status
                            << " | Total: " << o.total;
                        lines.push_back(oss.str());
                        found = true;
                    }
                }

                if (!found) {
                    lines.push_back("No orders.");
                    logger_.debug("No orders found for user {}", current_user_id);
                } else {
                    logger_.debug("Found {} orders for user {}", lines.size(), current_user_id);
                }
                send_list(client, lines);
            }
            else if (command == CMD_DEPOSIT) {
                if (current_user_id == -1) {
                    logger_.warn("Deposit attempt without authorization");
                    send_error(client, "Not authorized");
                    goto next_command;
                }

                double amount;
                if (!(iss >> amount)) {
                    logger_.error("Failed to parse deposit amount");
                    send_error(client, "Invalid amount format");
                    goto next_command;
                }

                logger_.command_received("DEPOSIT", "user=" + std::to_string(current_user_id) +
                                         ", amount=" + std::to_string(amount));

                if (amount <= 0) {
                    logger_.warn("Invalid deposit amount: {}", amount);
                    send_error(client, "Invalid amount");
                    goto next_command;
                }

                try {
                    double old_balance = global_app->GetAccountBalance(current_user_id);
                    users_db::Deposit(&global_app->GetUserAccountsMutable(), current_user_id, amount);
                    users_db::Save(global_app->GetUserAccounts(), global_app->GetUsersFile());

                    double new_balance = global_app->GetAccountBalance(current_user_id);
                    logger_.user_balance_changed(current_user_id, old_balance, new_balance);

                    send_str(client, std::string(RES_OK) + " " + std::to_string(new_balance));
                    logger_.command_success("DEPOSIT", "user=" + std::to_string(current_user_id) +
                                            ", +" + std::to_string(amount) + " rub.");
                } catch (const std::exception& e) {
                    logger_.error("Deposit failed - {}", e.what());
                    send_error(client, "Deposit operation failed");
                }
            }
            else if (command == CMD_GET_BALANCE) {
                logger_.command_received("GET_BALANCE", "user_id=" + std::to_string(current_user_id));
                
                double balance = global_app->GetAccountBalance(current_user_id);
                logger_.debug("Balance for user {}: {}", current_user_id, balance);
                send_str(client, std::to_string(balance));
            }
            else if (is_admin_local) {
                // admin commands
                if (command == CMD_ADMIN_SHOW_MENU) {
                    logger_.command_received("ADMIN_SHOW_MENU");
                    
                    std::vector<std::string> lines;
                    for (size_t i = 0; i < global_app->GetMenu().size(); ++i) {
                        std::ostringstream oss;
                        oss << (i + 1) << ". " << global_app->GetMenu()[i].name
                            << " - " << global_app->GetMenu()[i].price;
                        lines.push_back(oss.str());
                    }
                    logger_.debug("Sending admin menu with {} items", lines.size());
                    send_list(client, lines);
                }
                else if (command == CMD_ADMIN_ADD_DISH) {
                    std::string name;
                    double price;
                    std::getline(iss >> std::ws, name, ';');
                    iss >> price;

                    logger_.command_received("ADMIN_ADD_DISH", "name=" + name + ", price=" + std::to_string(price));

                    if (price > 0 && !name.empty()) {
                        Dish d = {name, price};
                        menu_db::AddDish(&global_app->GetMenuMutable(), d);
                        menu_db::Save(global_app->GetMenu(), global_app->GetMenuFile());
                        logger_.dish_added(name, price);
                        send_ok(client);
                    } else {
                        logger_.warn("Invalid dish data: name='{}', price={}", name, price);
                        send_error(client, "Invalid data");
                    }
                }
                else if (command == CMD_ADMIN_REMOVE_DISH) {
                    size_t idx;
                    iss >> idx;

                    logger_.command_received("ADMIN_REMOVE_DISH", "index=" + std::to_string(idx));

                    if (idx > 0 && idx <= global_app->GetMenu().size()) {
                        std::string dish_name = global_app->GetMenu()[idx - 1].name;
                        menu_db::RemoveDish(&global_app->GetMenuMutable(), idx - 1);
                        menu_db::Save(global_app->GetMenu(), global_app->GetMenuFile());
                        logger_.dish_removed(dish_name);
                        send_ok(client);
                    } else {
                        logger_.warn("Invalid dish index: {} (menu size: {})", idx, global_app->GetMenu().size());
                        send_error(client, "Invalid index");
                    }
                }
                else if (command == CMD_ADMIN_UPDATE_STATUS) {
                    int order_id;
                    std::string status;
                    iss >> order_id;
                    std::getline(iss >> std::ws, status);

                    status.erase(0, status.find_first_not_of(" \t"));
                    status.erase(status.find_last_not_of(" \t") + 1);

                    logger_.command_received("ADMIN_UPDATE_STATUS",
                                           "order_id=" + std::to_string(order_id) +
                                           ", status=" + status);

                    bool order_found = false;
                    for (const auto& order : global_app->GetOrders()) {
                        if (order.id == order_id) {
                            order_found = true;
                            std::string old_status = order.status;
                            orders_db::UpdateStatus(&global_app->GetOrdersMutable(), order_id, status);
                            orders_db::Save(global_app->GetOrders(), global_app->GetOrdersFile());
                            logger_.info("[ORDER] Status updated: ID={}, {} -> {}",
                                       order_id, old_status, status);
                            break;
                        }
                    }

                    if (order_found) {
                        send_ok(client);
                    } else {
                        logger_.warn("Order not found: {}", order_id);
                        send_error(client, "Order not found");
                    }
                }
                else if (command == CMD_ADMIN_REMOVE_USER) {
                    int user_id;
                    iss >> user_id;

                    logger_.command_received("ADMIN_REMOVE_USER", "user_id=" + std::to_string(user_id));

                    if (global_app->UserExists(user_id)) {
                        int orders_count = 0;
                        for (const auto& order : global_app->GetOrders()) {
                            if (order.user_id == user_id) orders_count++;
                        }

                        global_app->GetUserAccountsMutable().erase(user_id);
                        orders_db::RemoveUserOrders(&global_app->GetOrdersMutable(), user_id);
                        users_db::Save(global_app->GetUserAccounts(), global_app->GetUsersFile());
                        orders_db::Save(global_app->GetOrders(), global_app->GetOrdersFile());

                        logger_.info("[USER] Removed: ID={}, orders_removed={}", user_id, orders_count);
                        send_ok(client);
                    } else {
                        logger_.warn("User not found: {}", user_id);
                        send_error(client, "User not found");
                    }
                }
                else if (command == CMD_ADMIN_SHOW_ORDERS) {
                    logger_.command_received("ADMIN_SHOW_ORDERS");
                    
                    std::vector<std::string> lines;
                    for (const auto& o : global_app->GetOrders()) {
                        std::ostringstream oss;
                        oss << "ID:" << o.id << " User:" << o.user_id
                            << " Status:" << o.status << " Total:" << o.total
                            << " Dishes:" << Join(o.dish_names, ", ");
                        lines.push_back(oss.str());
                    }

                    if (lines.empty()) {
                        lines.push_back("No orders.");
                        logger_.debug("No orders in system");
                    } else {
                        logger_.debug("Sending {} orders to admin", lines.size());
                    }
                    send_list(client, lines);
                }
                else if (command == CMD_ADMIN_SHOW_USERS) {
                    logger_.command_received("ADMIN_SHOW_USERS");
                    
                    std::vector<std::string> lines;
                    for (const auto& p : global_app->GetUserAccounts()) {
                        std::ostringstream oss;
                        oss << "ID:" << p.first << " Balance:" << p.second;
                        lines.push_back(oss.str());
                    }

                    if (lines.empty()) {
                        lines.push_back("No users.");
                        logger_.debug("No users in system");
                    } else {
                        logger_.debug("Sending {} users to admin", lines.size());
                    }
                    send_list(client, lines);
                }
                else {
                    logger_.warn("Unknown admin command: {}", command);
                    send_error(client, "Unknown admin command");
                }
            }
            else {
                logger_.warn("Access denied for command: {} (user: {})", command, current_user_id);
                send_error(client, "Access denied");
            }

        next_command:
            continue;
        }

        closesocket(client);
        logger_.info("Client session ended for {}", client_ip);
        
        std::cout << "\n[" << getCurrentTime() << "] 🔌 CLIENT DISCONNECTED" << std::endl;
        std::cout << "IP: " << client_ip << std::endl;
        std::cout << "==========================================\n" << std::endl;
    }

    ~Server() {
        logger_.info("Server shutting down");
        if (listen_socket_ != INVALID_SOCKET) {
            closesocket(listen_socket_);
            logger_.debug("Listen socket closed");
        }
        
        // clean global object
        {
            std::lock_guard<std::mutex> lock(app_mutex);
            if (global_app) {
                delete global_app;
                global_app = nullptr;
                logger_.debug("Global RestaurantApp deleted");
            }
        }
        
        WSACleanup();
        logger_.info("Server cleanup complete");
    }
};

int main() {
    Server server;
    if (!server.init("0.0.0.0", 8080)) {
        return 1;
    }
    server.run();
    return 0;
}
