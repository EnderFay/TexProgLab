#define NOMINMAX
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <limits>
#include "clode_monet.h"
#include "utils.h"


// MenuDB implementations
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

// OrdersDB implementations
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

// 2.4 removing order
void orders_db::RemoveUserOrders(std::vector<Order>* orders, int user_id) {
    orders->erase(
        std::remove_if(orders->begin(), orders->end(),
            [user_id](const Order& o) { return o.user_id == user_id; }),
        orders->end());
}

// 2.5 updating status
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

// UsersDB implementations
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

// U.1 show menu
void RestaurantApp::ShowMenu() const {
    std::cout << "\nMenu\n";
    if (menu_.empty()) {
        std::cout << "Menu is empty.\n";
        return;
    }
    for (size_t i = 0; i < menu_.size(); ++i) {
        std::cout << i + 1 << ". " << menu_[i].name
            << " - " << menu_[i].price << " rub.\n";
    }
}

// U.2 make new order
void RestaurantApp::CreateOrder(int user_id) {
    std::vector<std::string> selected_dishes;
    double total = 0.0;

    std::cout << "\nCreating order for user ID " << user_id << ":\n";
    ShowMenu();

    int choice;
    do {
        std::cout << "\nEnter dish number (0 to finish): ";
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (choice > 0 && choice <= static_cast<int>(menu_.size())) {
            selected_dishes.push_back(menu_[choice - 1].name);
            total += menu_[choice - 1].price;
            std::cout << "Added: " << menu_[choice - 1].name << "\n";
        }
        else if (choice != 0) {
            std::cout << "Invalid dish number!\n";
        }
        else {
            std::cout << "Invalid dish number!\n";
        }
    } while (choice != 0);

    if (!selected_dishes.empty()) {
        orders_db::CreateOrder(&orders_, user_id, selected_dishes, total);
        orders_db::Save(orders_, orders_file_);
        std::cout << "Order created! Total: " << total << " rub.\n";
    }
    else {
        std::cout << "No dishes selected. Order cancelled.\n";
    }
}

// U.3 check order status
void RestaurantApp::CheckOrderStatus(int user_id) const {
    std::cout << "\nYour orders:\n";
    bool found = false;
    for (const auto& order : orders_) {
        if (order.user_id == user_id) {
            std::cout << "ID " << order.id
                << " | Status: " << order.status
                << " | Total: " << order.total << " ðóá.\n";
            found = true;
        }
    }
    if (!found) {
        std::cout << "No orders found.\n";
    }
}

// U.4 make a deposite
void RestaurantApp::DepositAccount(int user_id) {
    double amount;
    std::cout << "Enter deposit amount: ";
    std::cin >> amount;

    std::cin.clear();

    if (amount <= 0) {
        std::cout << "Amount must be positive.\n";
        return;
    }

    users_db::Deposit(&user_accounts_, user_id, amount);
    users_db::Save(user_accounts_, users_file_);
    std::cout << "Deposited " << amount << " rub. New balance: "
        << GetAccountBalance(user_id) << " rub.\n";
}

// U.5 get balance
double RestaurantApp::GetAccountBalance(int user_id) const {
    auto it = user_accounts_.find(user_id);
    if (it != user_accounts_.end()) {
        return it->second;
    }
    return 0.0;
}

// U.6 User exist
bool RestaurantApp::UserExists(int user_id) const {
    return user_accounts_.find(user_id) != user_accounts_.end();
}

void RestaurantApp::UpdateOrderStatus() {
    int order_id;
    std::string new_status;

    std::cout << "Enter order ID to update: ";
    std::cin >> order_id;

    std::cin.clear();

    std::cout << "Enter new status ('is_waiting', 'in_progress', 'completed'): ";
    std::cin >> new_status;

    orders_db::UpdateStatus(&orders_, order_id, new_status);
    orders_db::Save(orders_, orders_file_);
    std::cout << "Order status updated.\n";
}

// A.2 add dish
void RestaurantApp::AddDishToMenu() {
    std::string name;
    double price;
    std::cout << "Enter dish name: ";
    std::cin >> name;
    std::cout << "Enter price: ";
    std::cin >> price;
    std::cin.clear();
    if (price <= 0) {
        std::cout << "Price must be positive.\n";
        return;
    }

    Dish dish = { name, price };
    menu_db::AddDish(&menu_, dish);
    menu_db::Save(menu_, menu_file_);
    std::cout << "Dish added to menu.\n";
}

// A.3 remove dish
void RestaurantApp::RemoveDishFromMenu() {
    size_t index;
    RestaurantApp::ShowMenu();
    std::cout << "Enter dish number to remove (1-" << menu_.size() << "): ";
    std::cin >> index;

    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');


    if (index >= 1 && index <= menu_.size()) {
        menu_db::RemoveDish(&menu_, index - 1);
        menu_db::Save(menu_, menu_file_);
        std::cout << "Dish removed from menu.\n";
    }
    else {
        std::cout << "Invalid dish number.\n";
    }
}

// A.4 Add new user
void RestaurantApp::AddUser(int user_id) {
    if (!UserExists(user_id)) {
        users_db::AddUser(&user_accounts_, user_id);
        users_db::Save(user_accounts_, users_file_);
        std::cout << "User " << user_id << " created with 0.0 balance.\n";
    }
    else {
        std::cout << "User " << user_id << " already exists.\n";
    }
}

// A.5 Remove some user
void RestaurantApp::RemoveUser() {
    int user_id;
    std::cout << "Enter user ID to remove: ";
    std::cin >> user_id;

    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (UserExists(user_id)) {
        user_accounts_.erase(user_id);
        orders_db::RemoveUserOrders(&orders_, user_id);
        users_db::Save(user_accounts_, users_file_);
        orders_db::Save(orders_, orders_file_);
        std::cout << "User " << user_id << " and their orders removed.\n";
    }
    else {
        std::cout << "User not found.\n";
    }
}

void RestaurantApp::ShowMenu1() {
    std::cout << "\nMenu\n";
    if (menu_.empty()) {
        std::cout << "Menu is empty.\n";
        return;
    }
    for (size_t i = 0; i < menu_.size(); ++i) {
        std::cout << i + 1 << ". " << menu_[i].name
            << " - " << menu_[i].price << " rub.\n";
    }
}


void RestaurantApp::ShowOrdersList() {
    std::cout << "\nAll orders:\n";
    if (orders_.empty()) {
        std::cout << "No orders.\n";
        return;
    }
    for (const auto& order : orders_) {
        std::cout << "ID: " << order.id<< ", User: " << order.user_id<< ", Status: " << order.status<< ", Total: " << order.total<< ", Dishes: " << Join(order.dish_names, ", ")<< "\n";
    }
}

void RestaurantApp::ShowUserList() {
    std::cout << "\nAll users:\n";
    if (user_accounts_.empty()) {
        std::cout << "No users.\n";
        return;
    }

    for (const auto& pair : user_accounts_) {
        std::cout << "ID: " << pair.first
            << ", Balance: " << pair.second << " rub.\n";
    }
}