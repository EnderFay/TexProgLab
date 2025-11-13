#ifndef CLODE_MONET_H
#define CLODE_MONET_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>


using namespace std;

struct Dish {
    string name;
    double price;
};

struct Order {
    int id;
    int userId;
    vector<string> dishNames;
    string status;
    double total;
};

namespace MenuDB {
    bool load(vector<Dish>& menu, const string& filename); //Parsing in "menu.txt"
    void save(const vector<Dish>& menu, const string& filename);
    void addDish(vector<Dish>& menu, const Dish& dish);
    void removeDish(vector<Dish>& menu, size_t index);
}

namespace UsersDB {
    bool load(map<int, double>& users, const string& filename); //Parsing in "guests.txt"
    void save(const map<int, double>& users, const string& filename);
    bool userExists(const map<int, double>& users, int userId);
    void addUser(map<int, double>& users, int userId, double balance = 0.0);
    void deposit(map<int, double>& users, int userId, double amount);
}

namespace OrdersDB {
    bool load(vector<Order>& orders, const string& filename); //Parsing in "orders.txt"
    void save(const vector<Order>& orders, const string& filename);
    void createOrder(vector<Order>& orders, int userId, const vector<string>& dishNames, double total);
    void updateStatus(vector<Order>& orders, int orderId, const string& newStatus);
    void removeUserOrders(vector<Order>& orders, int userId);
}

class RestaurantApp {
private:
    vector<Dish> menu;
    map<int, double> userAccounts;
    vector<Order> orders;


    const string menuFile = "menu.txt";
    const string usersFile = "users.txt";
    const string ordersFile = "orders.txt";

public:
    RestaurantApp();
    //UserM
    void showMenu() const;
    void createOrder(int userId);
    void checkOrderStatus(int userId) const;
    void depositAccount(int userId);
    double getAccountBalance(int userId) const;
    bool userExists(int userId) const;
    void addUser(int userId);
    //AdminM
    void updateOrderStatus();
    void addDishToMenu();
    void removeDishFromMenu();
    void removeUser();
};

#endif