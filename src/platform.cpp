// platform.cpp - console I/O for Windows (conio + Win32) and POSIX (termios).
#include "platform.hpp"

#include <cstdio>

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  include <conio.h>
#  include <io.h>
#else
#  include <csignal>
#  include <cerrno>
#  include <poll.h>
#  include <sys/ioctl.h>
#  include <termios.h>
#  include <unistd.h>
#endif

namespace platform {
namespace {

std::atomic<bool>* g_quit_flag = nullptr;

bool is_printable_ascii(int c) { return c >= 0x20 && c <= 0x7E; }

#if defined(_WIN32)

HANDLE g_out = INVALID_HANDLE_VALUE;
DWORD g_saved_out_mode = 0;
bool g_out_mode_saved = false;
bool g_out_is_console = false;

// Runs on a separate thread created by the OS. We only touch an atomic so
// there is nothing here that could race with the render/input threads.
BOOL WINAPI ctrl_handler(DWORD type) {
    switch (type) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
        if (g_quit_flag) g_quit_flag->store(true);
        return TRUE;
    default:
        return FALSE;
    }
}

#else

termios g_saved_termios{};
bool g_termios_saved = false;

// Signal handlers may only do async-signal-safe work: storing an atomic is.
void signal_handler(int) {
    if (g_quit_flag) g_quit_flag->store(true);
}

#endif

} // namespace

// ---------------------------------------------------------------------------
// init / shutdown
// ---------------------------------------------------------------------------
bool init() {
#if defined(_WIN32)
    g_out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (g_out == INVALID_HANDLE_VALUE) return false;

    DWORD mode = 0;
    if (!GetConsoleMode(g_out, &mode)) {
        // stdout is redirected (not a console): nothing to configure.
        g_out_is_console = false;
        return true;
    }
    g_out_is_console = true;
    g_saved_out_mode = mode;
    g_out_mode_saved = true;

    // Without this flag conhost/PowerShell print ANSI escapes literally.
    // ENABLE_PROCESSED_OUTPUT is required alongside it.
    mode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    return SetConsoleMode(g_out, mode) != 0;
#else
    if (!isatty(STDIN_FILENO)) {
        // Input is a pipe (e.g. automated test): bytes are read as they come.
        return true;
    }
    if (tcgetattr(STDIN_FILENO, &g_saved_termios) != 0) return false;
    g_termios_saved = true;

    termios raw = g_saved_termios;
    // Raw-ish mode: no line buffering (ICANON), no echo (we draw the input
    // buffer ourselves), no Ctrl+C signal (ISIG - we decode byte 3 instead),
    // no Ctrl+V literal-next (IEXTEN). Keep OPOST so "\n" still works.
    raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO | ISIG | IEXTEN));
    // No CR->NL translation (Enter arrives as '\r'), no Ctrl+S/Ctrl+Q flow control.
    raw.c_iflag &= static_cast<tcflag_t>(~(ICRNL | IXON | BRKINT | INPCK | ISTRIP));
    raw.c_cflag |= CS8;
    // read() returns immediately with whatever is available (we poll() first anyway).
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    return tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0;
#endif
}

void shutdown() {
#if defined(_WIN32)
    if (g_out_mode_saved) {
        SetConsoleMode(g_out, g_saved_out_mode);
        g_out_mode_saved = false;
    }
#else
    if (g_termios_saved) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_saved_termios);
        g_termios_saved = false;
    }
#endif
}

// ---------------------------------------------------------------------------
// keyboard
// ---------------------------------------------------------------------------
bool poll_key(Key& out) {
#if defined(_WIN32)
    if (!_kbhit()) return false;
    int c = _getch();
    if (c == 0 || c == 0xE0) {
        // Extended key (arrows, F-keys, Home...): a second byte carries the
        // scan code. Consume it so it is not mistaken for a printable char.
        if (_kbhit()) (void)_getch();
        out = {KeyKind::Ignore, 0};
        return true;
    }
    if (c == '\r' || c == '\n') { out = {KeyKind::Enter, 0}; return true; }
    if (c == 8 || c == 127)     { out = {KeyKind::Backspace, 0}; return true; }
    if (c == 3)                 { out = {KeyKind::Interrupt, 0}; return true; }
    if (is_printable_ascii(c))  { out = {KeyKind::Char, static_cast<char>(c)}; return true; }
    out = {KeyKind::Ignore, 0};
    return true;
#else
    pollfd pfd{};
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;
    int ready = ::poll(&pfd, 1, 0);       // timeout 0 => never blocks
    if (ready <= 0) return false;

    // Read even when POLLHUP is set: a closed pipe can still hold unread
    // bytes. read() returning 0 is the real end-of-input signal.
    unsigned char c = 0;
    ssize_t n = ::read(STDIN_FILENO, &c, 1);
    if (n < 0) {
        if (errno == EINTR || errno == EAGAIN) return false;
        out = {KeyKind::Interrupt, 0};   // dead fd: shut down instead of spinning
        return true;
    }
    if (n == 0) {
        // EOF (stdin closed): let the queued commands finish, then exit.
        out = {KeyKind::EndOfInput, 0};
        return true;
    }

    if (c == 27) {
        // Escape sequence (arrow keys etc.): the rest of the sequence is
        // already queued, so drain whatever is immediately available.
        for (;;) {
            pollfd p2{};
            p2.fd = STDIN_FILENO;
            p2.events = POLLIN;
            if (::poll(&p2, 1, 0) <= 0) break;
            unsigned char junk = 0;
            if (::read(STDIN_FILENO, &junk, 1) <= 0) break;
        }
        out = {KeyKind::Ignore, 0};
        return true;
    }
    if (c == '\r' || c == '\n') { out = {KeyKind::Enter, 0}; return true; }
    if (c == 8 || c == 127)     { out = {KeyKind::Backspace, 0}; return true; }
    if (c == 3 || c == 4)       { out = {KeyKind::Interrupt, 0}; return true; } // Ctrl+C / Ctrl+D
    if (is_printable_ascii(c))  { out = {KeyKind::Char, static_cast<char>(c)}; return true; }
    out = {KeyKind::Ignore, 0};
    return true;
#endif
}

// ---------------------------------------------------------------------------
// terminal size
// ---------------------------------------------------------------------------
ConsoleSize console_size() {
    ConsoleSize size{80, 24};
#if defined(_WIN32)
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (g_out != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(g_out, &info)) {
        int cols = info.srWindow.Right - info.srWindow.Left + 1;
        int rows = info.srWindow.Bottom - info.srWindow.Top + 1;
        if (cols > 0 && rows > 0) size = {cols, rows};
    }
#else
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
        size = {static_cast<int>(ws.ws_col), static_cast<int>(ws.ws_row)};
    }
#endif
    return size;
}

// ---------------------------------------------------------------------------
// output
// ---------------------------------------------------------------------------
void write_out(const std::string& data) {
    if (data.empty()) return;
#if defined(_WIN32)
    if (g_out == INVALID_HANDLE_VALUE) g_out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written = 0;
    if (g_out_is_console) {
        WriteConsoleA(g_out, data.data(), static_cast<DWORD>(data.size()), &written, nullptr);
    } else {
        WriteFile(g_out, data.data(), static_cast<DWORD>(data.size()), &written, nullptr);
    }
#else
    // A single write() normally takes the whole frame; loop only for the
    // rare partial write so the frame is never truncated.
    const char* p = data.data();
    std::size_t left = data.size();
    while (left > 0) {
        ssize_t n = ::write(STDOUT_FILENO, p, left);
        if (n < 0) {
            if (errno == EINTR) continue;
            return;
        }
        p += n;
        left -= static_cast<std::size_t>(n);
    }
#endif
}

// ---------------------------------------------------------------------------
// Ctrl+C
// ---------------------------------------------------------------------------
void install_quit_handler(std::atomic<bool>* flag) {
    g_quit_flag = flag;
#if defined(_WIN32)
    SetConsoleCtrlHandler(ctrl_handler, TRUE);
#else
    // Raw mode already turns Ctrl+C into a plain byte; these cover
    // `kill`, closing the terminal, etc.
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGHUP, signal_handler);
#endif
}

} // namespace platform
