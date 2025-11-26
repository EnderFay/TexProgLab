// network_protocol.h
#pragma once
#include <string>
#include <vector>
#include <sstream>

const int PORT = 8080;

// Команды
const std::string CMD_SHOW_MENU = "SHOW_MENU";
const std::string CMD_CREATE_ORDER = "CREATE_ORDER";
const std::string CMD_CHECK_STATUS = "CHECK_STATUS";
const std::string CMD_DEPOSIT = "DEPOSIT";
const std::string CMD_GET_BALANCE = "GET_BALANCE";
const std::string CMD_ADD_DISH = "ADD_DISH";
const std::string CMD_REMOVE_DISH = "REMOVE_DISH";
const std::string CMD_UPDATE_STATUS = "UPDATE_STATUS";
const std::string CMD_REMOVE_USER = "REMOVE_USER";

// Ответы
const std::string RES_OK = "OK";
const std::string RES_ERROR = "ERROR";
const std::string RES_END = "END_OF_RESPONSE";

// Inline split
inline std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}