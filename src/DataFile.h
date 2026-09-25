#pragma once

#include <fstream>
#include <string>

// Opens a data file from the working directory, or from the executable's
// directory when it is not there, so the program works whether it is started
// from an IDE or directly from its build folder. The returned stream is closed
// when the file is in neither place; reading it then simply yields no lines.
std::ifstream open_data_file(const std::string &name);
