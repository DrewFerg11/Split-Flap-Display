#include "JsonSetting.h"

#include <cerrno>
#include <climits>
#include <cstdlib>

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

bool JsonSetting::validateIntVector(String str) {
    if (str.isEmpty()) return true;
    const char *cursor = str.c_str();
    while (*cursor) {
        if (*cursor != '-' && (*cursor < '0' || *cursor > '9')) break;
        char *end;
        errno = 0;
        const long value = strtol(cursor, &end, 10);
        if (end == cursor || errno == ERANGE || value < INT_MIN || value > INT_MAX || (*end != ',' && *end != '\0')) {
            lastValidationError = "Invalid integer list";
            return false;
        }
        if (*end == '\0') return true;
        cursor = end + 1;
        if (*cursor == '\0') break;
    }
    lastValidationError = "Invalid integer list";
    return false;
}
