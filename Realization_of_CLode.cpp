#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define NOMINMAX
#include <winsock2.h> //
#include <ws2tcpip.h> //
#include <iostream> //
#include <fstream> 
#include <sstream> //
#include <algorithm>
#include <limits> //
#include <string> //
#include "CLode_monet_new.h" //
#include "utils.h"
#include "network_protocol.h"
#include <iomanip>//

// MenuDB implementations (ONLY for server!)
// 1.1 loading from menu.txt
bool menu_db::Load(std::vector<Dish>* menu, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    menu->clear();
    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find(';');
        if (pos != std::string::npos) {
            std::string name = line.substr(0, pos);
            double price = std::stod(line.substr(pos + 1));
            menu->push_back({ name, price });
        }
    }
    file.close();
    return true;
}

// 1.2 saving after changes
void menu_db::Save(const std::vector<Dish>& menu, const std::string& filename) {
    std::ofstream file(filename);
    for (const auto& dish : menu) {
        file << dish.name << ";" << dish.price << "\n";
    }
    file.close();
}

// 1.3 add new
void menu_db::AddDish(std::vector<Dish>* menu, const Dish& dish) {
    menu->push_back(dish);
}

// 1.4 remove
void menu_db::RemoveDish(std::vector<Dish>* menu, size_t index) {
    if (index < menu->size()) {
        menu->erase(menu->begin() + index);
    }
}

// OrdersDB implementations (ONLY for server!)
// 2.1 loading from orders.txt
bool orders_db::Load(std::vector<Order>* orders, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    orders->clear();
    std::string line;

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
        }
    }

    file.close();
    return true;
}

// 2.2 saving after changes
void orders_db::Save(const std::vector<Order>& orders, const std::string& filename) {
    std::ofstream file(filename);
    for (const auto& order : orders) {
        file << order.id << ";"
            << order.user_id << ";"
            << Join(order.dish_names, ",") << ";"
            << order.status << ";"
            << order.total << "\n";
    }
    file.close();
}

// 2.3 adding new order
void orders_db::CreateOrder(std::vector<Order>* orders,
    int user_id,
    const std::vector<std::string>& dish_names,
    double total) {
    int new_id = orders->empty() ? 1 : orders->back().id + 1;
    orders->push_back({ new_id, user_id, dish_names, "is waiting", total });
}

// 2.4 updating status
void orders_db::UpdateStatus(std::vector<Order>* orders,
    int order_id,
    const std::string& new_status) {
    for (auto& order : *orders) {
        if (order.id == order_id) {
            order.status = new_status;
            break;
        }
    }
}

// 2.5 removing order
void orders_db::RemoveUserOrders(std::vector<Order>* orders, int user_id) {
    orders->erase(
        std::remove_if(orders->begin(), orders->end(),
            [user_id](const Order& o) { return o.user_id == user_id; }),
        orders->end());
}

// UsersDB implementations (ONLY for server!)
// 3.1 loading from users.txt
bool users_db::Load(std::map<int, double>* users, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    users->clear();
    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find(';');
        if (pos != std::string::npos) {
            int id = std::stoi(line.substr(0, pos));
            double balance = std::stod(line.substr(pos + 1));
            (*users)[id] = balance;
        }
    }
    file.close();
    return true;
}

// 3.2 saving after changes
void users_db::Save(const std::map<int, double>& users,
    const std::string& filename) {
    std::ofstream file(filename);
    for (const auto& pair : users) {
        file << pair.first << ";" << pair.second << "\n";
    }
    file.close();
}

// 3.3 user existing
bool users_db::UserExists(const std::map<int, double>& users, int user_id) {
    return users.find(user_id) != users.end();
}

// 3.4 add new user
void users_db::AddUser(std::map<int, double>* users, int user_id, double balance) {
    (*users)[user_id] = balance;
}

// 3.5 make a deposite
void users_db::Deposit(std::map<int, double>* users, int user_id, double amount) {
    if (users_db::UserExists(*users, user_id)) {
        (*users)[user_id] += amount;
    }
}

// RestaurantApp implementations
RestaurantApp::RestaurantApp() {
    menu_db::Load(&menu_, menu_file_);
    users_db::Load(&user_accounts_, users_file_);
    orders_db::Load(&orders_, orders_file_);
}

void RestaurantApp::ReloadUsers() {
    users_db::Load(&user_accounts_, users_file_);
}

// U.1
bool RestaurantApp::UserExists(int user_id) const {
    return user_accounts_.find(user_id) != user_accounts_.end();
}

// A.1
void RestaurantApp::AddUser(int user_id) {
    if (!UserExists(user_id)) {
        users_db::AddUser(&user_accounts_, user_id, 0.0);
        users_db::Save(user_accounts_, users_file_);
    }
}

// H.4
double RestaurantApp::GetAccountBalance(int user_id) const {
    auto it = user_accounts_.find(user_id);
    if (it != user_accounts_.end()) {
        return it->second;
    }
    return 0.0;
}

// Client interface
void ClodeMonetClient::run() {
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
            // admin path
            if (!connect_to_server()) {
                std::cout << "Failed to connect. Check if the server is running.\n";
                continue;
            }

            std::cout << "\nEnter the password: ";
            std::string password;
            std::cin >> password;
            clear_input();

            send_command(CMD_LOGIN_ADMIN " " + password);
            std::string response = receive_line();

            if (response == RES_OK) {
                is_admin = true;
                user_id = -1;
                std::cout << "Admin logged in.\n";
                admin_menu();
            }
            else {
                std::cout << "Wrong password!\n";
            }

            if (sock != INVALID_SOCKET) {
                closesocket(sock);
                WSACleanup();
                sock = INVALID_SOCKET;
            }
        }
        else if (choice_auth == 2) {
            if (!connect_to_server()) {
                std::cout << "Failed to connect. Check if the server is running.\n";
                continue;
            }

            std::cout << "\nWrite user ID: ";
            std::cin >> user_id;
            clear_input();

            send_command(CMD_LOGIN_USER " " + std::to_string(user_id));
            std::string response = receive_line();

            if (response == RES_OK) {
                is_admin = false;
                std::cout << "User (ID " << user_id << ") logged in.\n";
                user_menu();
            }
            else {
                std::cout << "Login failed: " << response << "\n";
            }

            if (sock != INVALID_SOCKET) {
                closesocket(sock);
                WSACleanup();
                sock = INVALID_SOCKET;
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

// Helper methods for client
// H.1
void ClodeMonetClient::showServerMenu() {
    send_command(CMD_SHOW_MENU);
    auto lines = receive_list();
    if (lines.empty()) {
        std::cout << "Menu is empty.\n";
    }
    else {
        for (const auto& line : lines) {
            std::cout << line << "\n";
        }
    }
}

// H.2
bool ClodeMonetClient::createOrderOnServer(const std::vector<std::string>& dishes) {
    if (dishes.empty()) return false;

    std::string cmd = CMD_CREATE_ORDER;
    for (const auto& dish : dishes) {
        cmd += " " + dish;
    }

    send_command(cmd);
    std::string response = receive_line();
    return response.find(RES_OK) == 0;
}

// H.3
bool ClodeMonetClient::depositToServer(double amount) {
    if (amount <= 0) return false;

    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << amount;
    send_command(std::string(CMD_DEPOSIT) + " " + ss.str());

    std::string response = receive_line();
    std::cout << "DEBUG: Raw server response = '" << response << "'" << std::endl;
    std::cout << "DEBUG: RES_OK = '" << RES_OK << "'" << std::endl;

    return response.find(RES_OK) == 0;

}

// H.4
double ClodeMonetClient::getBalanceFromServer() {
    send_command(CMD_GET_BALANCE);
    std::string response = receive_line();
    try {
        return std::stod(response);
    }
    catch (...) {
        return 0.0;
    }
}

// S.1
bool ClodeMonetClient::connect_to_server(const std::string& ip, int port) {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        WSACleanup();
        sock = INVALID_SOCKET;
    }

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
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

    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        std::cerr << "Invalid IP address\n";
        closesocket(sock);
        WSACleanup();
        return false;
    }

    if (::connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed. Is server running?\n";
        closesocket(sock);
        WSACleanup();
        sock = INVALID_SOCKET;
        return false;
    }

    std::cout << "\n\tRestaurant Clode Monet - connected to server!\n\n";
    return true;
}

// S.2
void ClodeMonetClient::send_command(const std::string& cmd) {
    std::string msg = cmd + "\n";
    send(sock, msg.c_str(), (int)msg.size(), 0);
}

// R.1
std::string ClodeMonetClient::receive_line() {
    std::string result;
    char buffer;

    while (true) {
        int bytes = recv(sock, &buffer, 1, 0);
        if (bytes <= 0) return "";

        if (buffer == '\n') {
            if (!result.empty() && result.back() == '\r') {
                result.pop_back();
            }
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

        if (line == RES_END) break;

        if (line.rfind(RES_ERROR, 0) == 0) {
            std::cerr << "Server error: " << line.substr(6) << "\n";
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

        if (choice == 0) break;

        switch (choice) {
        case 1: {
            RestaurantApp app;
            int order_id;
            std::string new_status;

            std::cout << "Enter order ID to update: ";
            std::cin >> order_id;
            clear_input();

            bool order_found = false;
            for (const auto& order : app.orders_) {
                if (order.id == order_id) {
                    order_found = true;
                    break;
                }
            }

            if (!order_found) {
                std::cout << "Error: Order with ID " << order_id << " not found.\n";
                return;
            }

            std::cout << "Enter new status: ";
            std::getline(std::cin, new_status);

            send_command(CMD_ADMIN_UPDATE_STATUS " " + std::to_string(order_id) + " " + new_status);
            std::string response = receive_line();
            if (response == RES_OK) {
                std::cout << "Order status updated.\n";
            }
            else {
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

            if (price <= 0) {
                std::cout << "Price must be positive.\n";
                break;
            }

            send_command(CMD_ADMIN_ADD_DISH " " + name + ";" + std::to_string(price));
            std::string response = receive_line();
            if (response == RES_OK) {
                std::cout << "Dish added.\n";
            }
            else {
                std::cout << "Error: " << response << "\n";
            }
            break;
        }
        case 3: {
            showServerMenu();

            size_t index;
            std::cout << "Enter dish number to remove: ";
            std::cin >> index;
            clear_input();

            send_command(CMD_ADMIN_REMOVE_DISH " " + std::to_string(index));
            std::string response = receive_line();
            if (response == RES_OK) {
                std::cout << "Dish removed.\n";
            }
            else {
                std::cout << "Error: " << response << "\n";
            }
            break;
        }
        case 4: {
            int user_id;
            std::cout << "Enter user ID to remove: ";
            std::cin >> user_id;
            clear_input();

            send_command(CMD_ADMIN_REMOVE_USER " " + std::to_string(user_id));
            std::string response = receive_line();
            if (response == RES_OK) {
                std::cout << "User " << user_id << " removed.\n";
            }
            else {
                std::cout << "Error: " << response << "\n";
            }
            break;
        }
        case 5:
            showServerMenu();
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

// Run.2
void ClodeMonetClient::user_menu() {
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
            showServerMenu();
            break;

        case 2: {
            showServerMenu();

            std::cout << "\nEnter dish names (separated by space, 0 to cancel): ";
            std::string input;
            std::getline(std::cin, input);

            if (input == "0") break;

            std::stringstream ss(input);
            std::vector<std::string> dishes;
            std::string dish;
            while (ss >> dish) {
                dishes.push_back(dish);
            }

            if (createOrderOnServer(dishes)) {
                std::cout << "Order created successfully!\n";
            }
            else {
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

            if (depositToServer(amount)) {
                std::cout << "Deposit successful!\n";
                std::cout << "New balance: " << getBalanceFromServer() << " rub.\n";
            }
            else {
                std::cout << "Deposit failed.\n";
            }
            break;
        }

        case 5:
            std::cout << "Balance: " << getBalanceFromServer() << " rub.\n";
            break;

        default:
            std::cout << "Wrong input!\n";
        }
    }
}