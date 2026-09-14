// banner.hpp - the one place the group edits for names, version, and the
// ASCII-art header shown at the top of the console.
#pragma once

#include <array>

namespace banner {

// Fill these in before recording the demo (also update README.txt).
constexpr std::array<const char*, 4> kDevelopers = {
    "<Group member 1>",
    "<Group member 2>",
    "<Group member 3>",
    "<Group member 4>",
};

constexpr const char* kVersionDate = "2026-09-15";
constexpr const char* kVersion = "1.0.0";

// Hand-drawn block letters spelling CSOPESY (49 columns, 5 rows).
// Raw string literal so the backslashes need no escaping.
constexpr std::array<const char*, 5> kAsciiArt = {
    R"(  ____  ____    ___   ____   _____  ____  __   __)",
    R"( / ___|/ ___|  / _ \ |  _ \ | ____|/ ___| \ \ / /)",
    R"(| |    \___ \ | | | || |_) ||  _|  \___ \  \ V / )",
    R"(| |___  ___) || |_| ||  __/ | |___  ___) |  | |  )",
    R"( \____||____/  \___/ |_|    |_____||____/   |_|  )",
};

} // namespace banner
