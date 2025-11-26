#ifndef CLODE_MONET_H
#define CLODE_MONET_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>




struct Dish {
    std::string name;
    double price;
};

struct Order {
    int id;
    int userId;
    std::vector<std::string> dishNames;
    std::string status;
    double total;
};

namespace MenuDB {
    bool load(std::vector<Dish>& menu, const std::string& filename);
    void save(const std::vector<Dish>& menu, const std::string& filename);
    void addDish(std::vector<Dish>& menu, const Dish& dish);
    void removeDish(std::vector<Dish>& menu, size_t index);
}

namespace UsersDB {
    bool load(std::map<int, double>& users, const std::string& filename);
    void save(const std::map<int, double>& users, const std::string& filename);
    bool userExists(const std::map<int, double>& users, int userId);
    void addUser(std::map<int, double>& users, int userId, double balance = 0.0);
    void deposit(std::map<int, double>& users, int userId, double amount);
}

namespace OrdersDB {
    bool load(std::vector<Order>& orders, const std::string& filename);
    void save(const std::vector<Order>& orders, const std::string& filename);
    void createOrder(std::vector<Order>& orders, int userId, const std::vector<std::string>& dishNames, double total);
    void updateStatus(std::vector<Order>& orders, int orderId, const std::string& newStatus);
    void removeUserOrders(std::vector<Order>& orders, int userId);
}

class RestaurantApp {
private:
    std::vector<Dish> menu;
    std::map<int, double> userAccounts;
    std::vector<Order> orders;


    const std::string menuFile = "menu.txt";
    const std::string usersFile = "users.txt";
    const std::string ordersFile = "orders.txt";

public:
    RestaurantApp();

    const std::vector<Dish>& getMenu() const { return menu; };
    void showMenu() const;
    void createOrder(int userId);
    void checkOrderStatus(int userId) const;
    void depositAccount(int userId);
    double getAccountBalance(int userId) const;
    bool userExists(int userId) const;
    void addUser(int userId);


    void updateOrderStatus();
    void addDishToMenu();
    void removeDishFromMenu();
    void removeUser();

    void createOrderNetwork(int userId, const std::vector<std::string>& dishNames);
    std::string getOrderStatusNetwork(int userId) const;
    void addDishToMenuNetwork(const std::string& name, double price);
    void removeDishFromMenuNetwork(size_t index);
    void updateOrderStatusNetwork(int orderId, const std::string& status);
    void removeUserNetwork(int userId);

    const std::string RES_END = "END_OF_RESPONSE";
    void depositAccount(int userId, double amount);
};

#endif