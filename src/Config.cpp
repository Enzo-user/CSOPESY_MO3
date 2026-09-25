#include "Config.h"

#include <istream>
#include <string>

#include "Text.h"

namespace {

// Strips one pair of surrounding double quotes, so leading or trailing
// spaces can be kept: marquee-text "  spaced  " style values.
std::string unquote(const std::string &value) {
    if (value.size() >= 2 && value[0] == '"' && value[value.size() - 1] == '"') {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

} // namespace

std::vector<std::string> Config::read(std::istream &in) {
    std::vector<std::string> notes;
    std::string line;

    while (std::getline(in, line)) {
        const std::string::size_type comment = line.find('#');
        if (comment != std::string::npos) {
            line.erase(comment);
        }
        const std::string entry = text::trim(line);
        if (entry.empty()) {
            continue;
        }

        const std::string::size_type separator = entry.find_first_of(text::WHITESPACE);
        std::string key = (separator == std::string::npos) ? entry : entry.substr(0, separator);
        const std::string value =
            (separator == std::string::npos) ? "" : text::trim(entry.substr(separator + 1));
        for (std::string::size_type i = 0; i < key.size(); ++i) {
            if (key[i] == '_') {
                key[i] = '-'; // refresh_rate and refresh-rate both work
            }
        }

        if (key == "marquee-text") {
            if (value.empty()) {
                notes.push_back("Config: marquee-text has no text, keeping \"" + marquee_text_ + "\".");
            } else {
                marquee_text_ = unquote(value);
            }
        } else if (key == "refresh-rate" || key == "polling-rate") {
            const bool refresh = (key == "refresh-rate");
            int &setting = refresh ? refresh_ms_ : polling_ms_;
            const long long maximum = refresh ? MAX_REFRESH_MS : MAX_POLLING_MS;
            int milliseconds = 0;
            if (text::parse_milliseconds(value, maximum, milliseconds)) {
                setting = milliseconds;
            } else {
                notes.push_back("Config: " + key + " '" + value +
                                "' is not a whole number of milliseconds from 1 to " +
                                std::to_string(maximum) + ", keeping " +
                                std::to_string(setting) + ".");
            }
        } else {
            notes.push_back("Config: unknown setting '" + key + "' ignored.");
        }
    }

    notes.push_back("Config: text \"" + marquee_text_ + "\", refresh " + std::to_string(refresh_ms_) +
                    " ms, polling " + std::to_string(polling_ms_) + " ms");
    return notes;
}
