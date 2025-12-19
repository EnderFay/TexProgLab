#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <limits>
#include <string>
#include "CLode_monet_new.h"
#include "utils.h"
#include "network_protocol.h"
#include <iomanip>
#include "logger.h"

// MenuDB implementations (Only for server!)
// 1.1 loading from menu.txt
bool menu_db::Load(std::vector<Dish>* menu, const std::string& filename) {
    Logger& logger = get_database_logger();
    logger.info("Loading menu from: {}", filename);

    std::ifstream file(filename);
    if (!file.is_open()) {
        logger.error("Cannot open menu file: {}", filename);
        return false;
    }

    menu->clear();
    std::string line;
    int loaded_count = 0;

    while (std::getline(file, line)) {
        size_t pos = line.find(';');
        if (pos != std::string::npos) {
            std::string name = line.substr(0, pos);
            double price = std::stod(line.substr(pos + 1));
            menu->push_back({ name, price });
            loaded_count++;
            logger.debug("Loaded dish: {} (price: {})", name, price);
        }
        else {
            logger.warn("Invalid format in menu file line: {}", line);
        }
    }
    file.close();

    logger.data_loaded("dishes", loaded_count);
    return true;
}

// 1.2 saving after changes
void menu_db::Save(const std::vector<Dish>& menu, const std::string& filename) {
    Logger& logger = get_database_logger();
    logger.info("Saving menu to: {}", filename);

    std::ofstream file(filename);
    if (!file.is_open()) {
        logger.error("Cannot write to menu file: {}", filename);
        return;
    }

    for (const auto& dish : menu) {
        file << dish.name << ";" << dish.price << "\n";
    }
    file.close();

    logger.data_saved("dishes", menu.size());
}

// 1.3 add new
void menu_db::AddDish(std::vector<Dish>* menu, const Dish& dish) {
    Logger& logger = get_database_logger();
    logger.info("Adding dish to menu: {} (price: {})", dish.name, dish.price);
    menu->push_back(dish);
    logger.debug("Menu now has {} dishes", menu->size());
}

// 1.4 remove
void menu_db::RemoveDish(std::vector<Dish>* menu, size_t index) {
    Logger& logger = get_database_logger();

    if (index < menu->size()) {
        std::string dish_name = (*menu)[index].name;
        double dish_price = (*menu)[index].price;

        logger.info("Removing dish from menu at index {}: {} (price: {})",
            index, dish_name, dish_price);

        menu->erase(menu->begin() + index);
        logger.debug("Menu now has {} dishes", menu->size());
    }
    else {
        logger.warn("Attempt to remove dish at invalid index: {} (menu size: {})",
            index, menu->size());
    }
}

// OrdersDB implementations (Only for server!)
// 2.1 loading from orders.txt
bool orders_db::Load(std::vector<Order>* orders, const std::string& filename) {
    Logger& logger = get_database_logger();
    logger.info("Loading orders from: {}", filename);

    std::ifstream file(filename);
    if (!file.is_open()) {
        logger.error("Cannot open orders file: {}", filename);
        return false;
    }

    orders->clear();
    std::string line;
    int loaded_count = 0;

    while (std::getline(file, line)) {
        std::vector<std::string> parts;
        std::string temp;

        for (auto it = line.begin(); it != line.end(); ++it) {
            char c = *it;
            if (c == ';') {
                parts.push_back(temp);
                temp.clear();
            }
            else {
                temp += c;
            }
        }
        parts.push_back(temp);

        if (parts.size() == 5) {
            int id = std::stoi(parts[0]);
            int user_id = std::stoi(parts[1]);

            std::vector<std::string> dishes;
            std::stringstream ss(parts[2]);
            std::string dish;

            while (std::getline(ss, dish, ',')) {
                dishes.push_back(dish);
            }

            std::string status = parts[3];
            double total = std::stod(parts[4]);

            Order order;
            order.id = id;
            order.user_id = user_id;
            order.dish_names = dishes;
            order.status = status;
            order.total = total;

            orders->push_back(order);
            loaded_count++;

            logger.debug("Loaded order: ID={}, user={}, status={}, total={}",
                id, user_id, status, total);
        }
        else {
            logger.warn("Invalid format in orders file line (expected 5 parts, got {}): {}",
                parts.size(), line);
        }
    }

    file.close();

    logger.data_loaded("orders", loaded_count);
    return true;
}

// 2.2 saving after changes
void orders_db::Save(const std::vector<Order>& orders, const std::string& filename) {
    Logger& logger = get_database_logger();
    logger.info("Saving orders to: {}", filename);

    std::ofstream file(filename);
    if (!file.is_open()) {
        logger.error("Cannot write to orders file: {}", filename);
        return;
    }

    for (const auto& order : orders) {
        file << order.id << ";"
            << order.user_id << ";"
            << Join(order.dish_names, ",") << ";"
            << order.status << ";"
            << order.total << "\n";
    }
    file.close();

    logger.data_saved("orders", orders.size());
}

// 2.3 adding new order
void orders_db::CreateOrder(std::vector<Order>* orders,
    int user_id,
    const std::vector<std::string>& dish_names,
    double total) {

    Logger& logger = get_database_logger();

    int new_id = orders->empty() ? 1 : orders->back().id + 1;

    logger.info("Creating new order: ID={}, user={}, dishes={}, total={}",
        new_id, user_id, dish_names.size(), total);

    orders->push_back({ new_id, user_id, dish_names, "is waiting", total });

    logger.debug("Order created successfully, orders count: {}", orders->size());
}

// 2.4 updating status
void orders_db::UpdateStatus(std::vector<Order>* orders,
    int order_id,
    const std::string& new_status) {

    Logger& logger = get_database_logger();

    logger.info("Updating order status: ID={}, new_status={}", order_id, new_status);

    for (auto& order : *orders) {
        if (order.id == order_id) {
            std::string old_status = order.status;
            order.status = new_status;

            logger.info("[ORDER] Status updated: ID={}, {} -> {}", order_id, old_status, new_status);
            break;
        }
    }
}

// 2.5 removing order
void orders_db::RemoveUserOrders(std::vector<Order>* orders, int user_id) {
    Logger& logger = get_database_logger();

    logger.info("Removing all orders for user: {}", user_id);

    int initial_count = orders->size();
    orders->erase(
        std::remove_if(orders->begin(), orders->end(),
            [user_id](const Order& o) { return o.user_id == user_id; }),
        orders->end());

    int removed_count = initial_count - orders->size();
    logger.info("[ORDER] Removed {} orders for user {}, remaining: {}",
        removed_count, user_id, orders->size());
}

// UsersDB implementations (ONLY for server!)
// 3.1 loading from users.txt
bool users_db::Load(std::map<int, double>* users, const std::string& filename) {
    Logger& logger = get_database_logger();
    logger.info("Loading users from: {}", filename);

    std::ifstream file(filename);
    if (!file.is_open()) {
        logger.error("Cannot open users file: {}", filename);
        return false;
    }

    users->clear();
    std::string line;
    int loaded_count = 0;

    while (std::getline(file, line)) {
        size_t pos = line.find(';');
        if (pos != std::string::npos) {
            int id = std::stoi(line.substr(0, pos));
            double balance = std::stod(line.substr(pos + 1));
            (*users)[id] = balance;
            loaded_count++;

            logger.debug("Loaded user: ID={}, balance={}", id, balance);
        }
        else {
            logger.warn("Invalid format in users file line: {}", line);
        }
    }
    file.close();

    logger.data_loaded("users", loaded_count);
    return true;
}

// 3.2 saving after changes
void users_db::Save(const std::map<int, double>& users,
    const std::string& filename) {

    Logger& logger = get_database_logger();
    logger.info("Saving users to: {}", filename);

    std::ofstream file(filename);
    if (!file.is_open()) {
        logger.error("Cannot write to users file: {}", filename);
        return;
    }

    for (const auto& pair : users) {
        file << pair.first << ";" << pair.second << "\n";
    }
    file.close();

    logger.data_saved("users", users.size());
}

// 3.3 user existing
bool users_db::UserExists(const std::map<int, double>& users, int user_id) {
    Logger& logger = get_database_logger();

    bool exists = users.find(user_id) != users.end();

    if (exists) {
        logger.debug("User exists check: ID={} -> TRUE", user_id);
    }
    else {
        logger.debug("User exists check: ID={} -> FALSE", user_id);
    }

    return exists;
}

// 3.4 add new user
void users_db::AddUser(std::map<int, double>* users, int user_id, double balance) {
    Logger& logger = get_database_logger();

    logger.info("Adding new user: ID={}, initial_balance={}", user_id, balance);

    (*users)[user_id] = balance;

    logger.debug("User added successfully, total users: {}", users->size());
}

// 3.5 make a deposite
void users_db::Deposit(std::map<int, double>* users, int user_id, double amount) {
    Logger& logger = get_database_logger();

    if (users_db::UserExists(*users, user_id)) {
        double old_balance = (*users)[user_id];
        (*users)[user_id] += amount;
        double new_balance = (*users)[user_id];

        logger.info("[USER] Deposit: ID={}, amount={}, {} -> {}",
            user_id, amount, old_balance, new_balance);
    }
    else {
        logger.warn("Deposit failed - user not found: {}", user_id);
    }
}

// RestaurantApp implementations
RestaurantApp::RestaurantApp() {
    Logger& logger = get_app_logger();
    logger.info("RestaurantApp constructor called");

    menu_db::Load(&menu_, menu_file_);
    users_db::Load(&user_accounts_, users_file_);
    orders_db::Load(&orders_, orders_file_);

    logger.debug("RestaurantApp initialized: {} dishes, {} users, {} orders",
        menu_.size(), user_accounts_.size(), orders_.size());
}

void RestaurantApp::ReloadUsers() {
    Logger& logger = get_app_logger();
    logger.info("Reloading users from file");

    int old_count = user_accounts_.size();
    users_db::Load(&user_accounts_, users_file_);

    logger.debug("Users reloaded: old_count={}, new_count={}",
        old_count, user_accounts_.size());
}

// U. 1
bool RestaurantApp::UserExists(int user_id) const {
    Logger& logger = get_app_logger();

    bool exists = user_accounts_.find(user_id) != user_accounts_.end();

    if (exists) {
        logger.debug("User exists: ID={} -> TRUE", user_id);
    }
    else {
        logger.debug("User exists: ID={} -> FALSE", user_id);
    }

    return exists;
}

// A. 1
void RestaurantApp::AddUser(int user_id) {
    Logger& logger = get_app_logger();
    logger.info("Adding user: ID={}", user_id);

    if (!UserExists(user_id)) {
        users_db::AddUser(&user_accounts_, user_id, 0.0);
        users_db::Save(user_accounts_, users_file_);
        logger.info("User added successfully: ID={}", user_id);
    }
    else {
        logger.warn("User already exists: {}", user_id);
    }
}

// H.4
double RestaurantApp::GetAccountBalance(int user_id) const {
    Logger& logger = get_app_logger();

    auto it = user_accounts_.find(user_id);
    if (it != user_accounts_.end()) {
        logger.debug("GetAccountBalance: user={}, balance={}", user_id, it->second);
        return it->second;
    }

    logger.debug("GetAccountBalance: user={} not found, returning 0.0", user_id);
    return 0.0;
}

// Client interface
void ClodeMonetClient::run() {
    Logger& client_logger = get_client_logger();

    client_logger.info("========================================================");
    client_logger.info("Restaurant Client Application Starting");
    client_logger.info("========================================================");

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

        client_logger.debug("Auth choice selected: {}", choice_auth);

        if (choice_auth == 0) {
            client_logger.info("Client application exiting");
            break;
        }

        if (choice_auth == 1) {
            client_logger.info("Admin authentication attempt");

            if (!connect_to_server()) {
                client_logger.error("Failed to connect to server");
                std::cout << "Failed to connect. Check if the server is running.\n";
                continue;
            }

            std::cout << "\nEnter the password: ";
            std::string password;
            std::cin >> password;
            clear_input();

            client_logger.debug("Admin password entered, length: {}", password.length());

            send_command(CMD_LOGIN_ADMIN " " + password);
            std::string response = receive_line();

            if (response == RES_OK) {
                is_admin = true;
                user_id = -1;
                client_logger.info("Admin logged in successfully");
                std::cout << "Admin logged in.\n";
                admin_menu();
            }
            else {
                client_logger.warn("Admin login failed - wrong password");
                std::cout << "Wrong password!\n";
            }

            if (sock != INVALID_SOCKET) {
                client_logger.debug("Closing admin socket");
                closesocket(sock);
#ifdef _WIN32
                WSACleanup();
#endif
                sock = INVALID_SOCKET;
            }
        }
        else if (choice_auth == 2) {
            client_logger.info("User authentication attempt");

            if (!connect_to_server()) {
                client_logger.error("Failed to connect to server");
                std::cout << "Failed to connect. Check if the server is running.\n";
                continue;
            }

            std::cout << "\nWrite user ID: ";
            std::cin >> user_id;
            clear_input();

            client_logger.debug("User ID entered: {}", user_id);

            send_command(CMD_LOGIN_USER " " + std::to_string(user_id));
            std::string response = receive_line();

            if (response == RES_OK) {
                is_admin = false;
                client_logger.info("User logged in successfully, ID: {}", user_id);
                std::cout << "User (ID " << user_id << ") logged in.\n";
                user_menu();
            }
            else {
                client_logger.error("User login failed: {}", response);
                std::cout << "Login failed: " << response << "\n";
            }

            if (sock != INVALID_SOCKET) {
                client_logger.debug("Closing user socket");
                closesocket(sock);
#ifdef _WIN32
                WSACleanup();
#endif
                sock = INVALID_SOCKET;
            }
        }
        else {
            client_logger.warn("Invalid authentication choice: {}", choice_auth);
            std::cout << "Invalid action!\n";
        }
    }

    if (sock != INVALID_SOCKET) {
        closesocket(sock);
#ifdef _WIN32
        WSACleanup();
#endif
    }

    client_logger.info("========================================================");
    client_logger.info("Restaurant Client Application Shutting Down");
    client_logger.info("========================================================");

    std::cout << "\nGood bye! See you later!\n";
}

// Helper methods for client
// H.1
void ClodeMonetClient::showServerMenu() {
    get_client_logger().debug("Requesting menu from server");

    send_command(CMD_SHOW_MENU);
    auto lines = receive_list();
    if (lines.empty()) {
        get_client_logger().debug("Menu is empty");
        std::cout << "Menu is empty.\n";
    }
    else {
        get_client_logger().debug("Received menu with {} items", lines.size());
        for (const auto& line : lines) {
            std::cout << line << "\n";
        }
    }
}

// H.2
bool ClodeMonetClient::createOrderOnServer(const std::vector<std::string>& dishes) {
    if (dishes.empty()) {
        get_client_logger().warn("createOrderOnServer: Empty dishes list");
        return false;
    }

    std::string cmd = CMD_CREATE_ORDER;
    for (const auto& dish : dishes) {
        cmd += " " + dish;
    }

    get_client_logger().debug("createOrderOnServer: Sending order for {} dishes", dishes.size());
    get_client_logger().debug("Dishes: {}", cmd.substr(std::string(CMD_CREATE_ORDER).length()));

    send_command(cmd);
    std::string response = receive_line();

    get_client_logger().debug("createOrderOnServer: Server response: {}", response);

    bool success = response.find(RES_OK) == 0;
    if (success) {
        get_client_logger().info("Order created successfully for user {}", user_id);
    }
    else {
        get_client_logger().warn("Order creation failed for user {}", user_id);
    }

    return success;
}

// H.3
bool ClodeMonetClient::depositToServer(double amount) {
    if (amount <= 0) {
        get_client_logger().warn("depositToServer: Invalid amount {}", amount);
        return false;
    }

    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << amount;

    get_client_logger().debug("depositToServer: Sending deposit, amount={}, user={}",
        amount, user_id);

    send_command(std::string(CMD_DEPOSIT) + " " + ss.str());
    std::string response = receive_line();

    get_client_logger().debug("depositToServer: Raw server response = '{}'", response);
    get_client_logger().debug("depositToServer: RES_OK = '{}'", RES_OK);

    if (response.find(RES_OK) == 0) {
        get_client_logger().info("Deposit successful: user={}, amount={}", user_id, amount);
    }
    else {
        get_client_logger().warn("Deposit failed: user={}, amount={}, response={}",
            user_id, amount, response);
    }

    return response.find(RES_OK) == 0;
}

// H.4
double ClodeMonetClient::getBalanceFromServer() {
    get_client_logger().debug("getBalanceFromServer: Requesting balance for user {}", user_id);

    send_command(CMD_GET_BALANCE);
    std::string response = receive_line();

    try {
        double balance = std::stod(response);
        get_client_logger().debug("getBalanceFromServer: Balance for user {} = {}", user_id, balance);
        return balance;
    }
    catch (...) {
        get_client_logger().error("getBalanceFromServer: Failed to parse response: {}", response);
        return 0.0;
    }
}

// S.1
bool ClodeMonetClient::connect_to_server(const std::string& ip, int port) {
    get_client_logger().info("Attempting to connect to {}:{}", ip, port);

    if (sock != INVALID_SOCKET) {
        closesocket(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        sock = INVALID_SOCKET;
    }

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        get_client_logger().error("WSAStartup failed");
        return false;
    }
#endif

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        get_client_logger().error("Socket creation failed");
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (addr.sin_addr.s_addr == INADDR_NONE) {
        get_client_logger().error("Invalid IP address: {}", ip);
        closesocket(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    if (::connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        get_client_logger().error("Connection failed to {}:{}. Is server running?", ip, port);
        closesocket(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        sock = INVALID_SOCKET;
        return false;
    }

    get_client_logger().info("Successfully connected to server {}:{}", ip, port);

    std::cout << "\n\tRestaurant Clode Monet - connected to server!\n\n";
    return true;
}

// S.2
void ClodeMonetClient::send_command(const std::string& cmd) {
    std::string msg = cmd + "\n";
    get_client_logger().network_out(cmd);
    send(sock, msg.c_str(), (int)msg.size(), 0);
}

// R.1
std::string ClodeMonetClient::receive_line() {
    std::string result;
    char buffer;

    while (true) {
        int bytes = recv(sock, &buffer, 1, 0);
        if (bytes <= 0) {
            get_client_logger().network_error("Connection closed or error");
            return "";
        }

        if (buffer == '\n') {
            if (!result.empty() && result.back() == '\r') {
                result.pop_back();
            }
            get_client_logger().network_in(result);
            return result;
        }

        result += buffer;
    }
}

// R.2
std::vector<std::string> ClodeMonetClient::receive_list() {
    std::vector<std::string> lines;

    while (true) {
        std::string line = receive_line();
        if (line.empty()) {
            break;
        }

        if (line == RES_END) {
            break;
        }

        if (line.rfind(RES_ERROR, 0) == 0) {
            get_client_logger().error("Server error: {}", line.substr(6));
            break;
        }

        lines.push_back(line);
    }

    return lines;
}

// Clear bufer
void ClodeMonetClient::clear_input() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// Run.1
void ClodeMonetClient::admin_menu() {
    get_client_logger().info("Admin menu started");

    while (true) {
        std::cout << "\n\tAdministrator mode\n"
            << "1. Update order status\n"
            << "2. Add new dish\n"
            << "3. Remove the dish\n"
            << "4. Remove the user\n"
            << "5. Show menu\n"
            << "6. Show list of orders\n"
            << "7. Show list of users\n"
            << "0. Exit\n"
            << "Your choice: ";

        int choice;
        std::cin >> choice;
        clear_input();

        get_client_logger().debug("Admin menu choice: {}", choice);

        if (choice == 0) {
            get_client_logger().info("Admin menu ended");
            break;
        }

        switch (choice) {
        case 1: {
            RestaurantApp app;
            int order_id;
            std::string new_status;

            std::cout << "Enter order ID to update: ";
            std::cin >> order_id;
            clear_input();

            get_client_logger().debug("Admin updating order status, ID={}", order_id);

            bool order_found = false;
            for (const auto& order : app.orders_) {
                if (order.id == order_id) {
                    order_found = true;
                    break;
                }
            }

            if (!order_found) {
                get_client_logger().warn("Order not found, ID={}", order_id);
                std::cout << "Error: Order with ID " << order_id << " not found.\n";
                return;
            }

            std::cout << "Enter new status: ";
            std::getline(std::cin, new_status);

            send_command(CMD_ADMIN_UPDATE_STATUS " " + std::to_string(order_id) + " " + new_status);
            std::string response = receive_line();

            if (response == RES_OK) {
                get_client_logger().info("Admin updated order status, ID={}, status={}",
                    order_id, new_status);
                std::cout << "Order status updated.\n";
            }
            else {
                get_client_logger().warn("Failed to update order status, ID={}, error={}",
                    order_id, response);
                std::cout << "Error: " << response << "\n";
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

            get_client_logger().debug("Admin adding dish: name='{}', price={}", name, price);

            if (price <= 0) {
                get_client_logger().warn("Invalid price for dish '{}': {}", name, price);
                std::cout << "Price must be positive.\n";
                break;
            }

            send_command(CMD_ADMIN_ADD_DISH " " + name + ";" + std::to_string(price));
            std::string response = receive_line();

            if (response == RES_OK) {
                get_client_logger().info("Admin added dish: {} ({} rub.)", name, price);
                std::cout << "Dish added.\n";
            }
            else {
                get_client_logger().warn("Failed to add dish '{}', error={}", name, response);
                std::cout << "Error: " << response << "\n";
            }
            break;
        }
        case 3: {
            get_client_logger().debug("Admin removing dish");

            showServerMenu();

            size_t index;
            std::cout << "Enter dish number to remove: ";
            std::cin >> index;
            clear_input();

            get_client_logger().debug("Admin removing dish at index: {}", index);

            send_command(CMD_ADMIN_REMOVE_DISH " " + std::to_string(index));
            std::string response = receive_line();

            if (response == RES_OK) {
                get_client_logger().info("Admin removed dish at index: {}", index);
                std::cout << "Dish removed.\n";
            }
            else {
                get_client_logger().warn("Failed to remove dish at index {}, error={}",
                    index, response);
                std::cout << "Error: " << response << "\n";
            }
            break;
        }
        case 4: {
            int user_id;
            std::cout << "Enter user ID to remove: ";
            std::cin >> user_id;
            clear_input();

            get_client_logger().debug("Admin removing user: ID={}", user_id);

            send_command(CMD_ADMIN_REMOVE_USER " " + std::to_string(user_id));
            std::string response = receive_line();

            if (response == RES_OK) {
                get_client_logger().info("Admin removed user: ID={}", user_id);
                std::cout << "User " << user_id << " removed.\n";
            }
            else {
                get_client_logger().warn("Failed to remove user {}, error={}",
                    user_id, response);
                std::cout << "Error: " << response << "\n";
            }
            break;
        }
        case 5:
            get_client_logger().debug("Admin viewing menu");
            showServerMenu();
            break;
        case 6:
            get_client_logger().debug("Admin viewing all orders");
            send_command(CMD_ADMIN_SHOW_ORDERS);
            for (const auto& line : receive_list()) {
                std::cout << line << "\n";
            }
            break;
        case 7:
            get_client_logger().debug("Admin viewing all users");
            send_command(CMD_ADMIN_SHOW_USERS);
            for (const auto& line : receive_list()) {
                std::cout << line << "\n";
            }
            break;
        default:
            get_client_logger().warn("Wrong admin menu input: {}", choice);
            std::cout << "Wrong input!\n";
        }
    }
}

// Run.2
void ClodeMonetClient::user_menu() {
    get_client_logger().info("User menu started for user {}", user_id);

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

        get_client_logger().debug("User menu choice: {} for user {}", choice, user_id);

        if (choice == 0) {
            get_client_logger().info("User {} exiting menu", user_id);
            break;
        }

        switch (choice) {
        case 1:
            get_client_logger().debug("User {} requested menu", user_id);
            showServerMenu();
            break;

        case 2: {
            get_client_logger().info("User {} starting order process", user_id);

            showServerMenu();

            std::cout << "\nEnter dish numbers (separated by space, 0 to cancel): ";
            std::string input;
            std::getline(std::cin, input);

            if (input == "0") {
                get_client_logger().debug("User {} cancelled order", user_id);
                break;
            }

            std::stringstream ss(input);
            std::vector<std::string> dishes;
            std::string dish_num_str;
            std::vector<int> dish_numbers;

            while (ss >> dish_num_str) {
                try {
                    int dish_num = std::stoi(dish_num_str);
                    dish_numbers.push_back(dish_num);
                }
                catch (...) {
                    get_client_logger().warn("Invalid dish number entered: '{}'", dish_num_str);
                }
            }

            if (dish_numbers.empty()) {
                get_client_logger().warn("User {} attempted empty order", user_id);
                std::cout << "No dishes selected!\n";
                break;
            }

            get_client_logger().debug("User {} selected {} dishes", user_id, dish_numbers.size());

            std::vector<std::string> dish_names;
            for (int dish_num : dish_numbers) {
                dish_names.push_back(std::to_string(dish_num));
            }

            if (createOrderOnServer(dish_names)) {
                get_client_logger().info("Order created successfully for user {}", user_id);
                std::cout << "Order created successfully!\n";
            }
            else {
                get_client_logger().warn("Failed to create order for user {}", user_id);
                std::cout << "Failed to create order.\n";
            }
            break;
        }
        case 3:
            get_client_logger().debug("User {} checking order status", user_id);

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

            get_client_logger().debug("User {} attempting deposit of {} rub.", user_id, amount);

            if (amount <= 0) {
                get_client_logger().warn("Invalid deposit amount: {} rub.", amount);
                std::cout << "Amount must be positive.\n";
                break;
            }

            if (depositToServer(amount)) {
                double new_balance = getBalanceFromServer();
                get_client_logger().info("Deposit successful for user {}", user_id);
                get_client_logger().debug("New balance for user {}: {} rub.", user_id, new_balance);

                std::cout << "Deposit successful!\n";
                std::cout << "New balance: " << new_balance << " rub.\n";
            }
            else {
                get_client_logger().warn("Deposit failed for user {}", user_id);
                std::cout << "Deposit failed.\n";
            }
            break;
        }

        case 5: {
            double balance = getBalanceFromServer();
            get_client_logger().debug("User {} checked balance: {} rub.", user_id, balance);

            std::cout << "Balance: " << balance << " rub.\n";
            break;
        }

        default:
            get_client_logger().warn("Invalid menu choice: {} for user {}", choice, user_id);

            std::cout << "Wrong input!\n";
        }
    }

    get_client_logger().info("User menu ended for user {}", user_id);
}
