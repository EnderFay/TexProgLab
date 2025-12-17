#ifndef CLODE_MONET_NEW_H_
#define CLODE_MONET_NEW_H_

#include <string>
#include <vector>
#include <map>
#include "network_protocol.h"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <netdb.h>
#endif

#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_TYPE int
#define SOCKET_ERROR -1
#define WSACleanup()
#define INVALID_SOCKET_TYPE -1
#define closesocket close
#define SOCKET_ERROR_TYPE -1
#define CLOSE_SOCKET close

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

// number 1 with 4 functions(parsing in "menu.txt")
namespace menu_db {
    bool Load(std::vector<Dish>* menu, const std::string& filename); // 1.1
    void Save(const std::vector<Dish>& menu, const std::string& filename); // 1.2
    void AddDish(std::vector<Dish>* menu, const Dish& dish); // 1.3
    void RemoveDish(std::vector<Dish>* menu, size_t index); // 1.4
}

// number 2 with 5 functions(parsing in "orders.txt")
namespace orders_db {
    bool Load(std::vector<Order>* orders, const std::string& filename); // 2.1
    void Save(const std::vector<Order>& orders, const std::string& filename); // 2.2
    void CreateOrder(std::vector<Order>* orders, int user_id, const std::vector<std::string>& dish_names, double total); // 2.3
    void UpdateStatus(std::vector<Order>* orders, int order_id, const std::string& new_status); // 2.4
    void RemoveUserOrders(std::vector<Order>* orders, int user_id); // 2.5
}

// number 3 with 5 functions(parsing in "users.txt")
namespace users_db {
    bool Load(std::map<int, double>* users, const std::string& filename); // 3.1
    void Save(const std::map<int, double>& users, const std::string& filename); // 3.2
    bool UserExists(const std::map<int, double>& users, int user_id); // 3.3
    void AddUser(std::map<int, double>* users, int user_id, double balance = 0.0); // 3.4
    void Deposit(std::map<int, double>* users, int user_id, double amount); // 3.5
}

// restaurant application class.
class RestaurantApp {
public:
    RestaurantApp();

    // User methods.
    double GetAccountBalance(int user_id) const; // U.1
    bool UserExists(int user_id) const; // U.2

    // Admin methods.
    void AddUser(int user_id); // A.1
  
    void ReloadUsers(); // H.U.1

    // function of server cpp
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

// class to server's work
class ClodeMonetClient {
public:
    void run(); // Main interface

    // server local constants
    SOCKET sock = INVALID_SOCKET;
    int user_id = -1;
    bool is_admin = false;

    // server main functions
    bool connect_to_server(const std::string& ip = "127.0.0.1", int port = 8080); // S.1
    void send_command(const std::string& cmd); // S.2

    // receivers for server command
    std::string receive_line(); // R.1
    std::vector<std::string> receive_list(); // R.2

    // Clear bufer
    void clear_input();

    // for void run()
    void admin_menu(); // Run.1
    void user_menu(); // Run.2

    // Helper methods
    void showServerMenu(); // H.1
    bool createOrderOnServer(const std::vector<std::string>& dishes); // H.2
    bool depositToServer(double amount); // H.3
    double getBalanceFromServer(); // H.4
};


#endif

