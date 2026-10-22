#include "JsonSetting.h"

#include <sstream>
#include <stdexcept>
#include <string>

String JsonSetting::intVectorToString(const std::vector<int> &vec) {
    String result;
    for (size_t i = 0; i < vec.size(); ++i) {
        result += String(vec[i]);
        if (i < vec.size() - 1) {
            result += ",";
        }
    }
    return result;
}

bool JsonSetting::validate(String str) {
    switch (type) {
        case JsonSettingType::JST_INT_VECTOR: return validateIntVector(str);
        default: return true;
    }
}

// Accept exactly what getIntVector() can parse back, so a saved value can never make
// std::stoi throw on the next boot: each comma-separated token is an optional '-' then
// digits, and fits in an int.
bool JsonSetting::validateIntVector(String str) {
    std::istringstream stream(str.c_str());
    std::string token;
    while (std::getline(stream, token, ',')) {
        size_t digits = token.size() > 0 && token[0] == '-' ? 1 : 0;
        if (digits == token.size() || token.find_first_not_of("0123456789", digits) != std::string::npos) {
            lastValidationError = "Non-integer value found";
            return false;
        }
        try {
            std::stoi(token);
        } catch (const std::out_of_range &) {
            lastValidationError = "Integer value out of range";
            return false;
        }
    }
    return true;
}
