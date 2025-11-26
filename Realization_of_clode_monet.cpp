#include "Clode_monet.h"
#include <sstream>
#include <algorithm>
#include <fstream>
#include <iostream>
#include "network_protocol.h"


bool MenuDB::load(std::vector<Dish>& menu, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    menu.clear();
    std::string line;
    while (getline(file, line)) {
        size_t pos = line.find(':');
        if (pos == std::string::npos) continue;
        std::string name = line.substr(0, pos);
        double price = stod(line.substr(pos + 1));
        menu.push_back({name, price});
    }
    return true;
}

void MenuDB::save(const std::vector<Dish>& menu, const std::string& filename) {
    std::ofstream file(filename);
    for (const auto& dish : menu) {
        file << dish.name << ":" << dish.price << "\n";
    }
}

void MenuDB::addDish(std::vector<Dish>& menu, const Dish& dish) {
    menu.push_back(dish);
}

void MenuDB::removeDish(std::vector<Dish>& menu, size_t index) {
    if (index < menu.size()) {
        menu.erase(menu.begin() + index);
    }
}


bool OrdersDB::load(std::vector<Order>& orders, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    orders.clear();
    std::string line;
    while (getline(file, line)) {
        size_t p1 = line.find(':');
        size_t p2 = line.find(':', p1 + 1);
        size_t p3 = line.find(':', p2 + 1);
        if (p1 == std::string::npos || p2 == std::string::npos || p3 == std::string::npos) continue;

        int id = stoi(line.substr(0, p1));
        int userId = stoi(line.substr(p1 + 1, p2 - p1 - 1));
        std::string status = line.substr(p2 + 1, p3 - p2 - 1);
        double total = stod(line.substr(p3 + 1, line.find(':', p3 + 1) - p3 - 1));
        std::string dishesStr = line.substr(line.find(':', p3 + 1) + 1);
        std::vector<std::string> dishNames = split(dishesStr, ',');

        orders.push_back({id, userId, dishNames, status, total});
    }
    return true;
}

void OrdersDB::save(const std::vector<Order>& orders, const std::string& filename) {
    std::ofstream file(filename);
    for (const auto& order : orders) {
        file << order.id << ":" << order.userId << ":" << order.status << ":" << order.total << ":";
        for (size_t i = 0; i < order.dishNames.size(); ++i) {
            file << order.dishNames[i];
            if (i < order.dishNames.size() - 1) file << ",";
        }
        file << "\n";
    }
}

void OrdersDB::createOrder(std::vector<Order>& orders, int userId, const std::vector<std::string>& dishNames, double total) {
    int id = orders.empty() ? 1 : orders.back().id + 1;
    orders.push_back({id, userId, dishNames, "Принят", total});
}

void OrdersDB::updateStatus(std::vector<Order>& orders, int orderId, const std::string& newStatus) {
    for (auto& order : orders) {
        if (order.id == orderId) {
            order.status = newStatus;
            break;
        }
    }
}

void OrdersDB::removeUserOrders(std::vector<Order>& orders, int userId) {
    orders.erase(
        remove_if(orders.begin(), orders.end(), [userId](const Order& o) { return o.userId == userId; }),
        orders.end()
    );
}


bool UsersDB::load(std::map<int, double>& users, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    users.clear();
    std::string line;
    while (getline(file, line)) {
        size_t pos = line.find(':');
        if (pos == std::string::npos) continue;
        int id = stoi(line.substr(0, pos));
        double balance = stod(line.substr(pos + 1));
        users[id] = balance;
    }
    return true;
}

void UsersDB::save(const std::map<int, double>& users, const std::string& filename) {
    std::ofstream file(filename);
    for (const auto& [id, balance] : users) {
        file << id << ":" << balance << "\n";
    }
}

bool UsersDB::userExists(const std::map<int, double>& users, int userId) {
    return users.find(userId) != users.end();
}

void UsersDB::addUser(std::map<int, double>& users, int userId, double balance) {
    users[userId] = balance;
}

void UsersDB::deposit(std::map<int, double>& users, int userId, double amount) {
    if (users.find(userId) != users.end()) {
        users[userId] += amount;
    }
}


void RestaurantApp::createOrderNetwork(int userId, const std::vector<std::string>& dishNames) {
    if (!userExists(userId)) addUser(userId);

    double total = 0.0;
    std::vector<std::string> validDishes;
    for (const auto& name : dishNames) {
        for (const auto& dish : menu) {
            if (dish.name == name) {
                total += dish.price;
                validDishes.push_back(name);
                break;
            }
        }
    }
    if (validDishes.empty()) return;

    if (userAccounts[userId] < total) return;

    userAccounts[userId] -= total;
    int orderId = orders.empty() ? 1 : orders.back().id + 1;
    Order order{orderId, userId, validDishes, "Принят", total};
    orders.push_back(order);

    OrdersDB::save(orders, ordersFile);
    UsersDB::save(userAccounts, usersFile);
}

std::string RestaurantApp::getOrderStatusNetwork(int userId) const {
    std::ostringstream oss;
    bool found = false;
    for (const auto& order : orders) {
        if (order.userId == userId) {
            oss << "Заказ #" << order.id
                << ": " << order.status
                << " (" << order.total << " руб.)\n";
            found = true;
        }
    }
    return found ? oss.str() + RES_END : "Нет заказов\n" + RES_END;
}

void RestaurantApp::addDishToMenuNetwork(const std::string& name, double price) {
    Dish d{name, price};
    MenuDB::addDish(menu, d);
    MenuDB::save(menu, menuFile);
}

void RestaurantApp::removeDishFromMenuNetwork(size_t index) {
    if (index < menu.size()) {
        MenuDB::removeDish(menu, index);
        MenuDB::save(menu, menuFile);
    }
}

void RestaurantApp::updateOrderStatusNetwork(int orderId, const std::string& status) {
    for (auto& order : orders) {
        if (order.id == orderId) {
            order.status = status;
            break;
        }
    }
    OrdersDB::save(orders, ordersFile);
}

void RestaurantApp::removeUserNetwork(int userId) {
    userAccounts.erase(userId);
    OrdersDB::removeUserOrders(orders, userId);
    UsersDB::save(userAccounts, usersFile);
    OrdersDB::save(orders, ordersFile);
}

void RestaurantApp::depositAccount(int userId, double amount) {
    if (userAccounts.find(userId) == userAccounts.end()) {
        addUser(userId);
    }
    userAccounts[userId] += amount;
    UsersDB::save(userAccounts, usersFile);
}

bool RestaurantApp::userExists(int userId) const {
    return userAccounts.find(userId) != userAccounts.end();
}

void RestaurantApp::addUser(int userId) {
    if (!userExists(userId)) {
        UsersDB::addUser(userAccounts, userId, 0.0);
        UsersDB::save(userAccounts, usersFile);
    }
}

double RestaurantApp::getAccountBalance(int userId) const {
    auto it = userAccounts.find(userId);
    return it != userAccounts.end() ? it->second : 0.0;
}

RestaurantApp::RestaurantApp() {
    MenuDB::load(menu, menuFile);
    UsersDB::load(userAccounts, usersFile);
    OrdersDB::load(orders, ordersFile);
}