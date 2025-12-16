#ifndef NETWORK_PROTOCOL_H
#define NETWORK_PROTOCOL_H

#include <string>
#include <vector>

// clients command
#define CMD_LOGIN_ADMIN     "LOGIN_ADMIN"
#define CMD_LOGIN_USER       "LOGIN_USER"
#define CMD_LOGOUT           "LOGOUT"
#define CMD_SHOW_MENU        "SHOW_MENU"
#define CMD_CREATE_ORDER     "CREATE_ORDER"
#define CMD_CHECK_STATUS     "CHECK_STATUS"
#define CMD_DEPOSIT          "DEPOSIT"
#define CMD_GET_BALANCE      "GET_BALANCE"

#define CMD_ADMIN_UPDATE_STATUS "ADMIN_UPDATE_STATUS"
#define CMD_ADMIN_ADD_DISH      "ADMIN_ADD_DISH"
#define CMD_ADMIN_REMOVE_DISH   "ADMIN_REMOVE_DISH"
#define CMD_ADMIN_REMOVE_USER   "ADMIN_REMOVE_USER"
#define CMD_ADMIN_SHOW_ORDERS   "ADMIN_SHOW_ORDERS"
#define CMD_ADMIN_SHOW_USERS    "ADMIN_SHOW_USERS"
#define CMD_ADMIN_SHOW_MENU     "ADMIN_SHOW_MENU"

// server's answers
#define RES_OK               "OK"
#define RES_ERROR            "ERROR"
#define RES_DENIED           "DENIED"
#define RES_NOT_FOUND        "NOT_FOUND"
#define RES_END              "END"  // end of list
#define RES_SEPARATOR        "|"

// special
#define SEP                  ";"  // parser
#define END_MSG              "\n"

#endif