#ifndef CLODE_MONET_NEW_H_
#define CLODE_MONET_NEW_H_

#include <string>
#include <vector>
#include <map>
#include "network_protocol.h"
#include "logger.h"
#include <winsock2.h>
#include <ws2tcpip.h>

struct Dish {
    std::string name;
    double price;
};

struct Order {
    int id;
    int user_id;
    std::vector<std::string> dish_names;
    std::string status;
    double total;
};

namespace menu_db {
    bool Load(std::vector<Dish>* menu, const std::string& filename);
    void Save(const std::vector<Dish>& menu, const std::string& filename);
    void AddDish(std::vector<Dish>* menu, const Dish& dish);
    void RemoveDish(std::vector<Dish>* menu, size_t index);
}

namespace orders_db {
    bool Load(std::vector<Order>* orders, const std::string& filename);
    void Save(const std::vector<Order>& orders, const std::string& filename);
    void CreateOrder(std::vector<Order>* orders, int user_id, const std::vector<std::string>& dish_names, double total);
    void UpdateStatus(std::vector<Order>* orders, int order_id, const std::string& new_status);
    void RemoveUserOrders(std::vector<Order>* orders, int user_id);
}

namespace users_db {
    bool Load(std::map<int, double>* users, const std::string& filename);
    void Save(const std::map<int, double>& users, const std::string& filename);
    bool UserExists(const std::map<int, double>& users, int user_id);
    void AddUser(std::map<int, double>* users, int user_id, double balance = 0.0);
    void Deposit(std::map<int, double>* users, int user_id, double amount);
}

class RestaurantApp {
public:
    RestaurantApp();
    double GetAccountBalance(int user_id) const;
    bool UserExists(int user_id) const;
    void AddUser(int user_id);
    void ReloadUsers();

    const std::vector<Dish>& GetMenu() const { return menu_; }
    const std::vector<Order>& GetOrders() const { return orders_; }
    const std::map<int, double>& GetUserAccounts() const { return user_accounts_; }

    std::vector<Dish>& GetMenuMutable() { return menu_; }
    std::vector<Order>& GetOrdersMutable() { return orders_; }
    std::map<int, double>& GetUserAccountsMutable() { return user_accounts_; }

    const std::string& GetMenuFile() const { return menu_file_; }
    const std::string& GetUsersFile() const { return users_file_; }
    const std::string& GetOrdersFile() const { return orders_file_; }

    std::vector<Dish> menu_;
    std::map<int, double> user_accounts_;
    std::vector<Order> orders_;

    const std::string menu_file_ = "menu.txt";
    const std::string users_file_ = "users.txt";
    const std::string orders_file_ = "orders.txt";
};

class ClodeMonetClient {
public:
    void run();

    SOCKET sock = INVALID_SOCKET;
    int user_id = -1;
    bool is_admin = false;

    bool connect_to_server(const std::string& ip = "127.0.0.1", int port = 8080);
    void send_command(const std::string& cmd);
    std::string receive_line();
    std::vector<std::string> receive_list();
    void clear_input();
    void admin_menu();
    void user_menu();
    void showServerMenu();
    bool createOrderOnServer(const std::vector<std::string>& dishes);
    bool depositToServer(double amount);
    double getBalanceFromServer();
};

#endif
