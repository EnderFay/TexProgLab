#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#endif

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

#include "CLode_monet_new.h"
#include "network_protocol.h"
#include "utils.h"


// Server class for handling restaurant application network operations
class Server {
private:
#ifdef _WIN32
    WSADATA wsa_;    // Windows Sockets initialization data
#endif
    SOCKET listen_socket_;    // Main listening socket for incoming connections
    RestaurantApp app_;
    bool is_admin_;

    std::string receive(SOCKET client) {
        char buffer[4096] = { 0 };
        int bytes = recv(client, buffer, sizeof(buffer) - 1, 0);    // Receive data
        if (bytes <= 0) return "";
        buffer[bytes] = '\0';
        return std::string(buffer);
    }

    // Sends a string message to client with message terminator
    void send_str(SOCKET client, const std::string& msg) {
        std::string full = msg + END_MSG;
        send(client, full.c_str(), (int)full.size(), 0);
    }
    // Sends standard OK response to client
    void send_ok(SOCKET client) { send_str(client, RES_OK); }

     // Sends error response to client with optional error message
    void send_error(SOCKET client, const std::string& err = "") {
        send_str(client, RES_ERROR + (err.empty() ? "" : " " + err));
    }

    // Sends a list of items as multiple messages terminated with RES_END
    void send_list(SOCKET client, const std::vector<std::string>& items) {
        for (const auto& item : items) {
            send_str(client, item);
        }
        send_str(client, RES_END);
    }

public:
    // Constructor - initializes socket as invalid and admin flag as false
    Server() : listen_socket_(INVALID_SOCKET), is_admin_(false) {}

    // Initializes server socket and binds to specified IP and port
    bool init(const std::string& ip, int port) {
        #ifdef _WIN32
        // Initialize Winsock on Windows
        if (WSAStartup(MAKEWORD(2, 2), &wsa_) != 0) {
            std::cerr << "WSAStartup failed\n";
            return false;
        }
#endif
        // Create TCP socket for IPv4
        listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listen_socket_ == INVALID_SOCKET) {
            std::cerr << "Socket creation failed\n";
            #ifdef _WIN32
            WSACleanup();    // Cleanup Winsock on failure
            #endif
            return false;
        }

        // Configure server address structure
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);    // Convert port to network byte order
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);    // Convert IP string to binary

        // Bind socket to specified address and port
        if (bind(listen_socket_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed\n";
            closesocket(listen_socket_);    // Close socket on failure
            #ifdef _WIN32
            WSACleanup();
            #endif
            return false;
        }

        // Start listening for incoming connections
        if (listen(listen_socket_, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Listen failed\n";
            closesocket(listen_socket_);
            #ifdef _WIN32
            WSACleanup();
            #endif
            return false;
        }

        std::cout << "Server running on " << ip << ":" << port << "\n";
        return true;
    }

    // Main server loop - accepts and handles client connections
    void run() {
        while (true) {
            // Accept incoming client connection
            SOCKET client = accept(listen_socket_, nullptr, nullptr);
            if (client == INVALID_SOCKET) {
                std::cerr << "Accept failed\n";
                continue;
            }

            std::cout << "Client connected\n";
            handle_client(client);    // Process client requests
            closesocket(client);    // Close client connection
            std::cout << "Client disconnected\n";
        }
    }

    // Handles communication with a single client
    void handle_client(SOCKET client) {
        RestaurantApp app;
        int current_user_id = -1;
        is_admin_ = false;

        // Load data from files for this client session
        app.ReloadUsers();
        menu_db::Load(&app.GetMenuMutable(), app.GetMenuFile());
        users_db::Load(&app.GetUserAccountsMutable(), app.GetUsersFile());
        orders_db::Load(&app.GetOrdersMutable(), app.GetOrdersFile());

        std::cout << "Data loaded for new client.\n";

        // Main command processing loop
        while (true) {
             // Receive message from client
            std::string msg = receive(client);
            if (msg.empty()) break;

            // Remove trailing newline characters (CR/LF)
            if (!msg.empty() && msg.back() == '\n') msg.pop_back();
            if (!msg.empty() && msg.back() == '\r') msg.pop_back();

            // Parse command from received message
            std::istringstream iss(msg);
            std::string command;
            iss >> command;

            // Admin login command
            if (command == CMD_LOGIN_ADMIN) {
                std::string password;
                iss >> password;
                if (password == "1234") {
                    is_admin_ = true;
                    current_user_id = -1;
                    send_ok(client);
                    std::cout << "Admin logged in successfully.\n";
                    continue;
                }
                else {
                    send_error(client, "Wrong password");
                    std::cout << "Admin login failed: wrong password '" << password << "'\n";
                    continue;
                }
            }

            // User login command
            else if (command == CMD_LOGIN_USER) {
                int user_id;
                iss >> user_id;
                // Create new user if doesn't exist
                if (!app.UserExists(user_id)) {
                    app.AddUser(user_id);
                    std::cout << "Created new user with ID " << user_id << " and balance 0.0\n";
                    users_db::Save(app.GetUserAccounts(), app.GetUsersFile());
                }

                current_user_id = user_id;
                is_admin_ = false;
                send_ok(client);
                continue;
            }

            // Logout command
            else if (command == CMD_LOGOUT) {
                is_admin_ = false;
                current_user_id = -1;
                send_ok(client);
                continue;
            }
            // Authorization check
            else if (!is_admin_ && current_user_id == -1 && command != CMD_LOGIN_USER && command != CMD_LOGIN_ADMIN) {
                send_error(client, "Not authorized");
                continue;
            }

            // Display menu to user
            if (command == CMD_SHOW_MENU) {
                std::vector<std::string> lines;
                if (app.GetMenu().empty()) {
                    lines.push_back("Menu is empty.");
                }
                else {
                    for (size_t i = 0; i < app.GetMenu().size(); ++i) {
                        std::ostringstream oss;
                        oss << (i + 1) << ". " << app.GetMenu()[i].name << " - " << app.GetMenu()[i].price << " rub.";
                        lines.push_back(oss.str());
                    }
                }
                send_list(client, lines);
                continue;
            }
                
            // Create new order
            else if (command == CMD_CREATE_ORDER) {
                std::vector<std::string> dishes;
                double total = 0.0;
                int token;
                // Parse dish numbers from command
                while (iss >> token) {
                    if (token > 0 && token <= app.GetMenu().size()) {
                        const Dish& dish = app.GetMenu()[token - 1];
                        dishes.push_back(dish.name);
                        total += dish.price;
                    }
                    else {
                        send_error(client, "Invalid dish number: " + std::to_string(token));
                        return;
                    }
                }

                // Validate at least one dish was selected
                if (dishes.empty()) {
                    send_error(client, "No valid dishes");
                    return;
                }

                // Check user has sufficient balance
                double balance = app.GetAccountBalance(current_user_id);
                if (balance < total) {
                    send_error(client,
                        "Insufficient funds. Required: " +
                        std::to_string(total) +
                        " rub., Available: " +
                        std::to_string(balance) + " rub."
                    );
                    return;
                }

                // Create order in database
                orders_db::CreateOrder(&app.GetOrdersMutable(), current_user_id, dishes,total
                );

                // Deduct amount from user balance
                try {
                    users_db::Deposit(&app.GetUserAccountsMutable(), current_user_id, -total);
                    users_db::Save(app.GetUserAccounts(), app.GetUsersFile());
                }
                catch (const std::exception& e) {
                    send_error(client, "Failed to deduct funds: " + std::string(e.what()));
                    return;
                }

                // Save orders to file
                orders_db::Save(app.GetOrders(), app.GetOrdersFile());

                // Send success response with remaining balance
                send_str(client,
                    std::string(RES_OK) + " " +
                    std::to_string(total) + " " +
                    "(Balance after order: " +
                    std::to_string(app.GetAccountBalance(current_user_id)) + ")"
                );
                continue;
            }

            // Check order status for current user
            else if (command == CMD_CHECK_STATUS) {
                std::vector<std::string> lines;
                bool found = false;
                // Find all orders for current user
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
                continue;
            }

            // Deposit money to user account
            else if (command == CMD_DEPOSIT) {
                if (current_user_id == -1) {
                    send_error(client, "Not authorized");
                    std::cout << "DEBUG: Deposit rejected - user not authorized" << std::endl;
                    continue;
                }

                std::cout << "DEBUG: DEPOSIT command received. User ID: " << current_user_id << std::endl;

                if (!app.UserExists(current_user_id)) {
                    std::cout << "DEBUG: User not found!" << std::endl;
                    send_error(client, "User not found");
                    continue;
                }

                double amount;
                if (!(iss >> amount)) {
                    std::cout << "DEBUG: Failed to parse deposit amount" << std::endl;
                    send_error(client, "Invalid amount format");
                    continue;
                }

                std::cout << "DEBUG: Amount = " << amount << std::endl;

                if (amount <= 0) {
                    std::cout << "DEBUG: Invalid deposit amount: " << amount << std::endl;
                    send_error(client, "Invalid amount");
                    continue;
                }

                // Process deposit
                try {
                    users_db::Deposit(&app.GetUserAccountsMutable(), current_user_id, amount);
                    users_db::Save(app.GetUserAccounts(), app.GetUsersFile());

                    double new_balance = app.GetAccountBalance(current_user_id);
                    std::cout << "DEBUG: New balance = " << new_balance << std::endl;

                    send_str(client, std::string(RES_OK) + " " + std::to_string(new_balance));
                    std::cout << "DEBUG: Deposit successful. Sent response: OK " << new_balance << std::endl;
                }
                catch (const std::exception& e) {
                    std::cerr << "ERROR: Deposit failed - " << e.what() << std::endl;
                    send_error(client, "Deposit operation failed");
                }
            }

            // Get current user balance
            else if (command == CMD_GET_BALANCE) {
                send_str(client, std::to_string(app.GetAccountBalance(current_user_id)));
                continue;
            }

            else if (is_admin_) {
                // Admin view of menu
                if (command == CMD_ADMIN_SHOW_MENU) {
                    std::vector<std::string> lines;
                    for (size_t i = 0; i < app.GetMenu().size(); ++i) {
                        std::ostringstream oss;
                        oss << (i + 1) << ". " << app.GetMenu()[i].name << " - " << app.GetMenu()[i].price;
                        lines.push_back(oss.str());
                    }
                    send_list(client, lines);
                    continue;
                }
                    
                // Add new dish to menu
                else if (command == CMD_ADMIN_ADD_DISH) {
                    std::string name;
                    double price;
                    std::getline(iss >> std::ws, name, ';');
                    iss >> price;
                    if (price > 0 && !name.empty()) {
                        Dish d = { name, price };
                        menu_db::AddDish(&app.GetMenuMutable(), d);
                        menu_db::Save(app.GetMenu(), app.GetMenuFile());
                        send_ok(client);
                    }
                    else {
                        send_error(client, "Invalid data");
                    }
                    continue;
                }

                // Remove dish from menu by index
                else if (command == CMD_ADMIN_REMOVE_DISH) {
                    size_t idx;
                    iss >> idx;
                    if (idx > 0 && idx <= app.GetMenu().size()) {
                        menu_db::RemoveDish(&app.GetMenuMutable(), idx - 1);
                        menu_db::Save(app.GetMenu(), app.GetMenuFile());
                        send_ok(client);
                    }
                    else {
                        send_error(client, "Invalid index");
                    }
                    continue;
                }

                // Update order status
                else if (command == CMD_ADMIN_UPDATE_STATUS) {
                    int order_id;
                    std::string status;
                    iss >> order_id;
                    std::getline(iss >> std::ws, status);
                    orders_db::UpdateStatus(&app.GetOrdersMutable(), order_id, status);
                    orders_db::Save(app.GetOrders(), app.GetOrdersFile());
                    send_ok(client);
                    continue;
                }

                // Remove user and their orders
                else if (command == CMD_ADMIN_REMOVE_USER) {
                    int user_id;
                    iss >> user_id;
                    if (app.UserExists(user_id)) {
                        app.GetUserAccountsMutable().erase(user_id);
                        orders_db::RemoveUserOrders(&app.GetOrdersMutable(), user_id);
                        users_db::Save(app.GetUserAccounts(), app.GetUsersFile());
                        orders_db::Save(app.GetOrders(), app.GetOrdersFile());
                        send_ok(client);
                    }
                    else {
                        send_error(client, "User not found");
                    }
                    continue;
                }

                // Display all orders
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
                    continue;
                }

                // Display all users with balances
                else if (command == CMD_ADMIN_SHOW_USERS) {
                    std::vector<std::string> lines;
                    for (const auto& p : app.GetUserAccounts()) {
                        std::ostringstream oss;
                        oss << "ID:" << p.first << " Balance:" << p.second;
                        lines.push_back(oss.str());
                    }
                    if (lines.empty()) {
                        lines.push_back("No users found");
                    }
                    send_list(client, lines);
                }
                continue;
            }
            else {
                std::cout << "DEBUG: UNKNOWN COMMAND: '" << command << "'" << std::endl;
                send_error(client, "Unknown command or access denied");
            }
        }

        // Save all data to files when client disconnects
        users_db::Save(app.GetUserAccounts(), app.GetUsersFile());
        orders_db::Save(app.GetOrders(), app.GetOrdersFile());
        menu_db::Save(app.GetMenu(), app.GetMenuFile());
    }

    // Destructor - cleanup resources
    ~Server() {
        if (listen_socket_ != INVALID_SOCKET) closesocket(listen_socket_);
        #ifdef _WIN32
        WSACleanup();    // Cleanup Winsock on Windows
        #endif
    }
};

// Main function - server entry point
int main() {
    Server server;    // Create server instance
    // Initialize server on localhost port 8080
    if (!server.init("127.0.0.1", 8080)) {
        return 1;    // Exit with error code if initialization fails
    }
    server.run();    // Start server main loop
    return 0;

}


