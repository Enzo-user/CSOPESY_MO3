// platform.hpp - the only file that talks to OS-specific console APIs.
//
// Everything above this layer uses plain ANSI escape sequences and the small
// set of functions declared here. Windows uses <conio.h>/<windows.h>; POSIX
// uses termios raw mode + poll() on stdin. See CLAUDE.md section 2.
#pragma once

#include <atomic>
#include <cstddef>
#include <string>

namespace platform {

struct ConsoleSize {
    int cols;
    int rows;
};

// A single decoded keystroke. The input thread only needs to know about
// printable characters, Enter, Backspace, and "the user asked to quit".
enum class KeyKind {
    Char,       // printable ASCII in `ch`
    Enter,
    Backspace,
    Interrupt,  // Ctrl+C typed while in raw mode (POSIX only; Windows uses the ctrl handler)
    EndOfInput, // stdin closed (piped input finished): run what was queued, then exit
    Ignore      // arrows, function keys, tabs, non-ASCII bytes...
};

struct Key {
    KeyKind kind;
    char ch;
};

// Put the console into "raw" mode: no line buffering, no echo, and ANSI escape
// processing enabled (Windows needs an explicit opt-in). Returns false if the
// console could not be configured; the program still runs, best effort.
bool init();

// Restore whatever init() changed. Safe to call more than once.
void shutdown();

// Non-blocking keyboard read. Returns false when no key is waiting.
bool poll_key(Key& out);

// Current terminal size; falls back to 80x24 when it cannot be determined.
ConsoleSize console_size();

// Write a fully composed frame to the terminal with a single OS write call.
// This is the *only* function that writes to stdout while the console runs.
void write_out(const std::string& data);

// Make Ctrl+C / SIGINT / SIGTERM set *flag instead of killing the process,
// so the main thread can join the workers and restore the terminal.
void install_quit_handler(std::atomic<bool>* flag);

} // namespace platform
