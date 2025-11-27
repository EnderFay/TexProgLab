#ifndef RESTAURANT_CLUDE_MONET_H_
#define RESTAURANT_CLUDE_MONET_H_

#include <string>
#include <vector>
#include <map>

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

// ¹1 with 4 functions(parsing in "menu.txt")
namespace menu_db {
    bool Load(std::vector<Dish>* menu, const std::string& filename); // 1.1
    void Save(const std::vector<Dish>& menu, const std::string& filename); // 1.2
    void AddDish(std::vector<Dish>* menu, const Dish& dish); // 1.3
    void RemoveDish(std::vector<Dish>* menu, size_t index); // 1.4
}

// ¹2 with 5 functions(parsing in "orders.txt")
namespace orders_db {
    bool Load(std::vector<Order>* orders, const std::string& filename); // 2.1
    void Save(const std::vector<Order>& orders, const std::string& filename); // 2.2
    void CreateOrder(std::vector<Order>* orders, int user_id, const std::vector<std::string>& dish_names, double total); // 2.3
    void UpdateStatus(std::vector<Order>* orders, int order_id, const std::string& new_status); // 2.4
    void RemoveUserOrders(std::vector<Order>* orders, int user_id); // 2.5
}

// ¹3 with 5 functions(parsing in "users.txt")
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
    void ShowMenu() const; // U.1
    void CreateOrder(int user_id); // U.2
    void CheckOrderStatus(int user_id) const; // U.3
    void DepositAccount(int user_id); // U.4
    double GetAccountBalance(int user_id) const; // U.5
    bool UserExists(int user_id) const; // U.6

    // Admin methods.
    void UpdateOrderStatus(); // A.1
    void AddDishToMenu(); // A.2
    void RemoveDishFromMenu(); // A.3
    void AddUser(int user_id); // A.4
    void RemoveUser(); // A.5
    void ShowOrdersList(); // A.6
    void ShowUserList(); // A.7
    void ShowMenu1(); // A.8

private:
    std::vector<Dish> menu_;
    std::map<int, double> user_accounts_;
    std::vector<Order> orders_;

    const std::string menu_file_ = "menu.txt";
    const std::string users_file_ = "users.txt";
    const std::string orders_file_ = "orders.txt";
};
#endif
