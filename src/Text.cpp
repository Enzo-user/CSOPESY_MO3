#include "Text.h"

namespace text {

const std::string WHITESPACE = " \t\n\r\f\v";

std::string trim(const std::string &s) {
    const std::string::size_type first = s.find_first_not_of(WHITESPACE);
    if (first == std::string::npos) {
        return "";
    }
    const std::string::size_type last = s.find_last_not_of(WHITESPACE);
    return s.substr(first, last - first + 1);
}

bool parse_milliseconds(const std::string &argument, long long maximum, int &milliseconds) {
    if (argument.empty()) {
        return false;
    }
    long long value = 0;
    for (std::string::size_type i = 0; i < argument.size(); ++i) {
        if (argument[i] < '0' || argument[i] > '9') {
            return false;
        }
        value = value * 10 + (argument[i] - '0');
        if (value > maximum) {
            return false;
        }
    }
    if (value < 1) {
        return false;
    }
    milliseconds = static_cast<int>(value);
    return true;
}

} // namespace text
