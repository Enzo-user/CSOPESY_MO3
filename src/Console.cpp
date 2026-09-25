#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "Console.h"

#include <iostream>

#include <windows.h>

namespace {

const int FALLBACK_WIDTH = 99; // marquee width if the console size is unknown
const int FALLBACK_ROWS = 30;  // window height if the console size is unknown

// Lets the Windows console interpret ANSI escape sequences.
void enable_virtual_terminal() {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (out != INVALID_HANDLE_VALUE && GetConsoleMode(out, &mode)) {
        SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
}

// If the program is interrupted (Ctrl+C, closing the window), give the
// console its normal full-screen scrolling back.
BOOL WINAPI on_console_ctrl(DWORD) {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written = 0;
    WriteConsoleA(out, "\033[s\033[r\033[u", 9, &written, NULL);
    return FALSE; // the default handler then terminates the program
}

} // namespace

Console::Console(int marquee_rows)
    : marquee_rows_(marquee_rows), transcript_bottom_(1), marquee_top_(3) {
    enable_virtual_terminal();
    SetConsoleCtrlHandler(on_console_ctrl, TRUE);

    // The transcript scrolls inside rows 1..transcript_bottom_; the marquee
    // owns the bottom marquee_rows_ rows, with one blank row between them.
    transcript_bottom_ = rows() - marquee_rows_ - 1;
    if (transcript_bottom_ < 1) {
        transcript_bottom_ = 1;
    }
    marquee_top_ = transcript_bottom_ + 2;
}

Console::~Console() {
    clear_marquee_area();
    // Leave the console as a normal scrolling window again.
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << "\033[s\033[r\033[u" << std::flush;
}

std::string Console::move_to(int row) {
    return "\033[" + std::to_string(row) + ";1H";
}

int Console::width() const {
    CONSOLE_SCREEN_BUFFER_INFO info;
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(out, &info)) {
        const int width = info.srWindow.Right - info.srWindow.Left;
        if (width >= 1) {
            return width;
        }
    }
    return FALLBACK_WIDTH;
}

int Console::rows() const {
    CONSOLE_SCREEN_BUFFER_INFO info;
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(out, &info)) {
        const int rows = info.srWindow.Bottom - info.srWindow.Top + 1;
        if (rows >= 1) {
            return rows;
        }
    }
    return FALLBACK_ROWS;
}

void Console::begin_transcript() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << "\033[2J" << "\033[1;" << transcript_bottom_ << "r" << move_to(1) << std::flush;
}

void Console::print(const std::string &text) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << text << std::flush;
}

void Console::write_if(const std::string &text, const std::function<bool()> &still_wanted) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!still_wanted()) {
        return;
    }
    std::cout << text << std::flush;
}

void Console::redraw_prompt(const std::string &prompt, const std::string &input) {
    int room = width() - static_cast<int>(prompt.size());
    if (room < 1) {
        room = 1;
    }
    const std::string::size_type visible = static_cast<std::string::size_type>(room);
    const std::string shown =
        (input.size() > visible) ? input.substr(input.size() - visible) : input;

    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << "\r\033[2K" << prompt << shown << std::flush;
}

void Console::clear_marquee_area() {
    std::string frame = "\033[s";
    for (int r = 0; r < marquee_rows_; ++r) {
        frame += move_to(marquee_top_ + r) + "\033[2K";
    }
    frame += "\033[u";

    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << frame << std::flush;
}
