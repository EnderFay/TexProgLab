#pragma once
#include <string>
#include <vector>

inline std::string Join(const std::vector<std::string>& vec, const std::string& separator) {
    if (vec.empty()) return "";
    std::string result;
    for (size_t i = 0; i < vec.size(); ++i) {
        result += vec[i];
        if (i < vec.size() - 1) result += separator;
    }
    return result;
}