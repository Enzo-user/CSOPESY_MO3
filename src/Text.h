#pragma once

#include <string>

// String helpers shared by the configuration reader and the command interpreter.
namespace text {

// Characters treated as whitespace when trimming and splitting input.
// \r is included so input with Windows-style line endings is handled safely.
extern const std::string WHITESPACE;

// Removes leading and trailing whitespace from s.
std::string trim(const std::string &s);

// Parses a whole number of milliseconds (digits only, from 1 to maximum).
bool parse_milliseconds(const std::string &argument, long long maximum, int &milliseconds);

} // namespace text
