#include <iostream>﻿
#include "Clode_monet.h"

int main() {
	RestaurantApp app;
	string role;
	const string AP = "1234";
	while (true) {
		cout << "\t Restaurant Clode Monet welcomes you!\n";
		cout << "\nChoose the type of autorization\n";
		cout << "1. Administrator\n";
		cout << "2. User\n";
		cout << "0. Exit\n";
		cout << "Your choise: ";

		int choiceAu;
		cin >> choiceAu;

		if (choiceAu == 0) break;

		if (choiceAu == 1) {
			cout << "\nEnter the password: ";
			string password;
			cin >> password;
			getline(cin, password);

			if (password != AP) {
				cout << "Wrong password!\n";
				continue;
			}
			while (true) {
				cout << "\tAdministrator mode\n";
				cout << "1. Update order status\n";
				cout << "2. Add new dish\n";
				cout << "3. Remove the dish\n";
				cout << "4. Remove the user\n";
				cout << "0. Exit\n";
				cout << "Your choice: ";

				int choiceAd;
				cin >> choiceAd;

				if (choiceAd == 0) break;

				switch (choiceAd) {
				case 1:
					app.updateOrderStatus();
					break;
				case 2:
					app.addDishToMenu();
					break;
				case 3:
					app.removeDishFromMenu();
					break;
				case 4:
					app.removeUser();
					break;
				default:
					cout << "Wrong enter!\n";
				}
			}
		}
		else if (choiceAu == 2) {
			int userId;
			cout << "\nWrite user ID: ";
			cin >> userId;

			if (!app.userExists(userId)) {
				cout << "There is no existing user. We are creating new one/\n";
				app.addUser(userId);
			}

			while (true) {
				cout << "\tUser(ID " << userId << ")\n";
				cout << "1. Show menu\n";
				cout << "2. Make a order\n";
				cout << "3. Check the order status\n";
				cout << "4. Make a deposit\n";
				cout << "5. Check balance\n";
				cout << "0. Exit\n";
				cout << "Your choice: ";

				int choiceUs;
				cin >> choiceUs;

				if (choiceUs == 0) break;

				switch (choiceUs) {
				case 1:
					app.showMenu();
					break;
				case 2:
					app.createOrder(userId);
					break;
				case 3:
					app.checkOrderStatus(userId);
					break;
				case 4:
					app.depositAccount(userId);
					break;
				case 5:
					cout << "Balance: " << app.getAccountBalance(userId) << "rub.\n";
					break;
				default:
					cout << "Wrong enter!\n";
				}


			}
		}
		else {
			cout << "Неверное действие!\n";
			break;
		}
	}
	cout << "\nGood bye! See you later!\n";
	return 0;
}