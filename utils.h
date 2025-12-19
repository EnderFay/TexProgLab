#pragma once
#include <string>
#include <vector>
#include <fstream>
#include "logger.h"

inline std::string Join(const std::vector<std::string>& vec, const std::string& separator) {
    if (vec.empty()) return "";
    std::string result;
    for (size_t i = 0; i < vec.size(); ++i) {
        result += vec[i];
        if (i < vec.size() - 1) result += separator;
    }
    return result;
}

// stoi
inline int safe_stoi(const std::string& str, int default_val = 0) {
    try {
        int result = std::stoi(str);
        get_app_logger().trace("safe_stoi: '{}' -> {}", str, result);
        return result;
    }
    catch (const std::invalid_argument& e) {
        get_app_logger().warn("safe_stoi: invalid argument '{}', using default {}", str, default_val);
        return default_val;
    }
    catch (const std::out_of_range& e) {
        get_app_logger().warn("safe_stoi: out of range '{}', using default {}", str, default_val);
        return default_val;
    }
}

// stod
inline double safe_stod(const std::string& str, double default_val = 0.0) {
    try {
        double result = std::stod(str);
        get_app_logger().trace("safe_stod: '{}' -> {}", str, result);
        return result;
    }
    catch (const std::invalid_argument& e) {
        get_app_logger().warn("safe_stod: invalid argument '{}', using default {}", str, default_val);
        return default_val;
    }
    catch (const std::out_of_range& e) {
        get_app_logger().warn("safe_stod: out of range '{}', using default {}", str, default_val);
        return default_val;
    }
}
