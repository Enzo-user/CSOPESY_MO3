#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "DataFile.h"

#include <windows.h>

namespace {

// Directory containing the running executable, with a trailing separator.
std::string exe_directory() {
    char buffer[MAX_PATH];
    const DWORD length = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return "";
    }
    const std::string path(buffer, length);
    const std::string::size_type slash = path.find_last_of("\\/");
    return (slash == std::string::npos) ? "" : path.substr(0, slash + 1);
}

} // namespace

std::ifstream open_data_file(const std::string &name) {
    std::ifstream file(name.c_str());
    if (file.is_open()) {
        return file;
    }
    file.clear();
    file.open((exe_directory() + name).c_str());
    return file;
}
