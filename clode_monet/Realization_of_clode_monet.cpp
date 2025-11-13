#include "Clode_monet.h"

// MenuDB which you should pars in "menu.txt"
bool MenuDB::load(vector<Dish>& menu, const string& filename) {

}

void MenuDB::save(const vector<Dish>& menu, const string& filename) {

}

void MenuDB::addDish(vector<Dish>& menu, const Dish& dish) {
    menu.push_back(dish);
}

void MenuDB::removeDish(vector<Dish>& menu, size_t index) {

}

// OrdersDB which you should pars in "orders.txt"
bool OrdersDB::load(vector<Order>& orders, const string& filename) {

}

void OrdersDB::save(const vector<Order>& orders, const string& filename) {

}

void OrdersDB::createOrder(vector<Order>& orders, int userId, const vector<string>& dishNames, double total) {

}

void removeUserOrders(vector<Order>& orders, int userId) {

}

// UsersDB which you should pars in "users.txt"
bool UsersDB::load(map<int, double>& users, const string& filename) {

}

void UsersDB::save(const map<int, double>& users, const string& filename) {
    
}

bool UsersDB::userExists(const map<int, double>& users, int userId) {

}

void UsersDB::addUser(map<int, double>& users, int userId, double balance) {

}

void UsersDB::deposit(map<int, double>& users, int userId, double amount) {

}
