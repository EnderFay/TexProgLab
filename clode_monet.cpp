#include <iostream>
#include "clode_monet.h"

int main() {
    RestaurantApp app;
    const std::string adminPassword = "1234";

    while (true) {
        std::cout << "\tRestaurant Clode Monet welcomes you!\n"
            << "\nChoose the type of authorization\n"
            << "1. Administrator\n"
            << "2. User\n"
            << "0. Exit\n"
            << "Your choice: ";

        int choice_auth;
        std::cin >> choice_auth;

        if (choice_auth == 0) {
            break;
        }

        if (choice_auth == 1) {
            std::cout << "\nEnter the password: ";
            std::string password;
            std::cin >> password;

            std::cin.clear();

            if (password != adminPassword) {
                std::cout << "Wrong password!\n\n";
                continue;
            }
            while (true) {
                std::cout << "\n\tAdministrator mode\n"
                    << "1. Update order status\n"
                    << "2. Add new dish\n"
                    << "3. Remove the dish\n"
                    << "4. Remove the user\n"
                    << "5. Show menu\n"
                    << "6. Show list of Orders\n"
                    << "7. Show list of users\n"
                    << "0. Exit\n"
                    << "Your choice: ";

                int choice_admin;
                std::cin >> choice_admin;

                if (choice_admin == 0) {
                    break;
                }

                switch (choice_admin) {
                case 1:
                    app.UpdateOrderStatus();
                    break;
                case 2:
                    app.AddDishToMenu();
                    break;
                case 3:
                    app.RemoveDishFromMenu();
                    break;
                case 4:
                    app.RemoveUser();
                    break;
                case 5:
                    app.ShowMenu1();
                    break;
                case 6:
                    app.ShowOrdersList();
                    break;
                case 7:
                    app.ShowUserList();
                    break;
                default:
                    std::cout << "Wrong input!\n";
                    break;
                }
            }
        }
        else if (choice_auth == 2) {
            int user_id;
            std::cout << "\nWrite user ID: ";
            std::cin >> user_id;

            if (!app.UserExists(user_id)) {
                std::cout << "\nThere is no existing user. We are creating new one\n";
                app.AddUser(user_id);
            }

            while (true) {
                std::cout << "\n\tUser (ID " << user_id << ")\n"
                    << "1. Show menu\n"
                    << "2. Make an order\n"
                    << "3. Check the order status\n"
                    << "4. Make a deposit\n"
                    << "5. Check balance\n"
                    << "0. Exit\n"
                    << "Your choice: ";

                int choice_user;
                std::cin >> choice_user;

                if (choice_user == 0) {
                    break;
                }

                switch (choice_user) {
                case 1:
                    app.ShowMenu();
                    break;
                case 2:
                    app.CreateOrder(user_id);
                    break;
                case 3:
                    app.CheckOrderStatus(user_id);
                    break;
                case 4:
                    app.DepositAccount(user_id);
                    break;
                case 5:
                    std::cout << "\nBalance: " << app.GetAccountBalance(user_id)
                        << " rub.\n";
                    break;
                default:
                    std::cout << "Wrong input!\n";
                    break;
                }
            }
        }
        else {
            std::cout << "Invalid action!\n";
            std::cin.clear();
        }
    }

    std::cout << "\nGood bye! See you later!\n";
    return 0;
}
