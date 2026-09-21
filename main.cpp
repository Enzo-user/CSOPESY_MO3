// CSOPESY - Marquee Project: Command Line Interface Exercise
// Main menu console with a command interpreter and a live text marquee.
//
// The console behaves like the reference sample: the welcome header is printed
// first, then every "Command>" prompt, the command typed and its output scroll
// upwards as a transcript. The marquee animation (contributed by a group member
// as the standalone prototype marquee.cpp and integrated here) is drawn in the
// bottom eight rows of the window. An ANSI scrolling region keeps the
// transcript above those rows, so the marquee keeps animating while commands
// are typed.
//
// Windows only: <conio.h> provides non-blocking keyboard polling and
// <windows.h> is used to enable ANSI escape sequences in the console.

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <conio.h>
#include <windows.h>

namespace {

// ---- Layout and timing --------------------------------------------------

const int MARQUEE_ROWS = 8;        // the font is 8 rows tall
const int INPUT_POLL_MS = 10;      // keyboard polling interval
const int DEFAULT_SPEED_MS = 100;  // marquee refresh interval
const long long MAX_SPEED_MS = 999999999; // largest value set_speed accepts
const int FALLBACK_WIDTH = 99;     // marquee width if the console size is unknown
const int FALLBACK_ROWS = 30;      // window height if the console size is unknown
const char FONT_FILE[] = "ascii_big.txt";
const std::string PROMPT = "Command> ";

// Characters treated as whitespace when trimming and splitting input.
// \r is included so input with Windows-style line endings is handled safely.
const std::string WHITESPACE = " \t\n\r\f\v";

// ---- State shared by the console thread and the marquee thread ----------

std::mutex console_mtx;                     // serialises every write to std::cout
std::mutex text_mtx;                        // guards marquee_text
std::string marquee_text = "Hello, World!"; // the text saved in memory by set_text
std::atomic<bool> animation_on(false);      // start_marquee / stop_marquee
std::atomic<bool> frame_requested(false);   // draw the next frame right away
std::atomic<int> speed_ms(DEFAULT_SPEED_MS);
std::atomic<bool> program_running(true);    // cleared by exit

std::map<char, std::vector<std::string>> font_map; // read-only once loaded
bool font_loaded = false;
int marquee_top = 0; // first row of the marquee area; set before the thread starts

// ---- Small helpers ------------------------------------------------------

// Removes leading and trailing whitespace from s.
std::string trim(const std::string &s) {
    const std::string::size_type first = s.find_first_not_of(WHITESPACE);
    if (first == std::string::npos) {
        return "";
    }
    const std::string::size_type last = s.find_last_not_of(WHITESPACE);
    return s.substr(first, last - first + 1);
}

// ANSI escape that moves the cursor to the given 1-based row and column.
std::string move_to(int row, int col = 1) {
    return "\033[" + std::to_string(row) + ";" + std::to_string(col) + "H";
}

// Lets the Windows console interpret ANSI escape sequences.
void enable_virtual_terminal() {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (out != INVALID_HANDLE_VALUE && GetConsoleMode(out, &mode)) {
        SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
}

// Number of columns the marquee may use: one less than the window width so
// the last column never wraps onto the next row.
int console_width() {
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

// Number of rows visible in the console window.
int console_rows() {
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

// If the program is interrupted (Ctrl+C, closing the window), give the
// console its normal full-screen scrolling back.
BOOL WINAPI on_console_ctrl(DWORD) {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written = 0;
    WriteConsoleA(out, "\033[s\033[r\033[u", 9, &written, NULL);
    return FALSE; // the default handler then terminates the program
}

// ---- ASCII font ---------------------------------------------------------

// Reads the font file: one 8-row block per printable character, starting at
// '!' (ASCII 33) and continuing in ASCII order up to '~' (ASCII 126). Each
// glyph is cropped to its real width plus one space of kerning.
bool load_font_file(const std::string &path) {
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        return false;
    }

    std::map<char, std::vector<std::string>> glyphs;
    std::string line;

    for (int code = 33; code <= 126; ++code) {
        std::vector<std::string> rows;
        for (int r = 0; r < MARQUEE_ROWS && std::getline(file, line); ++r) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back(); // hidden Windows carriage return
            }
            rows.push_back(line);
        }
        if (rows.size() != static_cast<std::size_t>(MARQUEE_ROWS)) {
            break; // the file ended early
        }

        std::string::size_type width = 0;
        for (std::size_t r = 0; r < rows.size(); ++r) {
            const std::string::size_type last = rows[r].find_last_not_of(' ');
            if (last != std::string::npos && last + 1 > width) {
                width = last + 1;
            }
        }
        for (std::size_t r = 0; r < rows.size(); ++r) {
            rows[r].resize(width, ' '); // crop (or pad) to the glyph width
            rows[r] += ' ';             // kerning
        }
        glyphs[static_cast<char>(code)] = rows;
    }

    if (glyphs.empty()) {
        return false;
    }
    glyphs[' '] = std::vector<std::string>(MARQUEE_ROWS, "     ");
    font_map = glyphs;
    return true;
}

// Looks for the font in the current working directory first, then next to
// the executable, so the program works whether it is started from an IDE or
// directly from its build folder.
bool load_font() {
    if (load_font_file(FONT_FILE)) {
        return true;
    }
    const std::string dir = exe_directory();
    return !dir.empty() && load_font_file(dir + FONT_FILE);
}

// ---- Marquee rendering --------------------------------------------------

// Stitches the glyphs of text side by side. Without a font the text is shown
// as a single plain row.
std::vector<std::string> build_marquee_rows(const std::string &text) {
    if (!font_loaded) {
        return std::vector<std::string>(1, text);
    }
    std::vector<std::string> rows(MARQUEE_ROWS);
    for (std::string::size_type i = 0; i < text.size(); ++i) {
        std::map<char, std::vector<std::string>>::const_iterator glyph = font_map.find(text[i]);
        if (glyph == font_map.end()) {
            glyph = font_map.find(' '); // unsupported character: draw a gap
        }
        for (int r = 0; r < MARQUEE_ROWS; ++r) {
            rows[r] += glyph->second[r];
        }
    }
    return rows;
}

// Draws one frame in the marquee area: every row is the text repeated end to
// end, shifted left by offset columns, cut to the console width. The cursor
// is saved and restored so the transcript is never disturbed.
void draw_marquee_frame(const std::vector<std::string> &rows, std::size_t offset) {
    const int width = console_width();

    std::string frame = "\033[s";
    for (std::size_t r = 0; r < rows.size(); ++r) {
        const std::string pattern = rows[r] + "   "; // gap between repeats
        std::string window;
        window.reserve(static_cast<std::size_t>(width));
        for (int i = 0; i < width; ++i) {
            window += pattern[(offset + static_cast<std::size_t>(i)) % pattern.size()];
        }
        frame += move_to(marquee_top + static_cast<int>(r)) + "\033[2K" + window;
    }
    frame += "\033[u";

    std::lock_guard<std::mutex> lock(console_mtx);
    if (!animation_on) {
        return; // stop_marquee ran while this frame was being built
    }
    std::cout << frame << std::flush;
}

// Blanks the marquee area (used by stop_marquee and at exit).
void clear_marquee_area() {
    std::string frame = "\033[s";
    for (int r = 0; r < MARQUEE_ROWS; ++r) {
        frame += move_to(marquee_top + r) + "\033[2K";
    }
    frame += "\033[u";

    std::lock_guard<std::mutex> lock(console_mtx);
    std::cout << frame << std::flush;
}

// Body of the animation thread: redraws the marquee every speed_ms
// milliseconds while start_marquee is in effect, until the program exits.
// The thread wakes every INPUT_POLL_MS to notice speed changes, start/stop
// requests and exit promptly; frames are scheduled from the previous frame's
// due time so the average interval matches the requested speed.
void marquee_thread_main() {
    std::size_t offset = 0;
    std::chrono::steady_clock::time_point next_frame = std::chrono::steady_clock::now();

    while (program_running) {
        const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        if (animation_on && (frame_requested || now >= next_frame)) {
            const bool forced = frame_requested.exchange(false);

            std::string text;
            {
                std::lock_guard<std::mutex> lock(text_mtx);
                text = marquee_text;
            }
            draw_marquee_frame(build_marquee_rows(text), offset);
            ++offset;

            const std::chrono::milliseconds interval(speed_ms.load());
            next_frame = forced ? now + interval : next_frame + interval;
            if (next_frame < now) {
                next_frame = now; // after a stall, resume instead of catching up
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(INPUT_POLL_MS));
    }
}

// ---- Console text -------------------------------------------------------

// The welcome header with the dummy group developer and version date text.
std::vector<std::string> header_lines() {
    std::vector<std::string> lines;
    lines.push_back("Welcome to CSOPESY!");
    lines.push_back("");
    lines.push_back("Group developer:");
    lines.push_back("De La Cruz, Juan");
    lines.push_back("Santos, Alex");
    lines.push_back("");
    lines.push_back("Version date: 2026-09-18");
    return lines;
}

// Every command and its description.
std::string help_text() {
    return "help - displays the commands and its description\n"
           "start_marquee - starts the marquee \"animation\"\n"
           "stop_marquee - stops the marquee \"animation\"\n"
           "set_text - accepts a text input and displays it as a marquee\n"
           "set_speed - sets the marquee animation refresh in milliseconds\n"
           "exit - terminates the console\n";
}

// Parses a whole number of milliseconds (digits only, from 1 to MAX_SPEED_MS).
bool parse_speed(const std::string &argument, int &milliseconds) {
    if (argument.empty()) {
        return false;
    }
    long long value = 0;
    for (std::string::size_type i = 0; i < argument.size(); ++i) {
        if (argument[i] < '0' || argument[i] > '9') {
            return false;
        }
        value = value * 10 + (argument[i] - '0');
        if (value > MAX_SPEED_MS) {
            return false;
        }
    }
    if (value < 1) {
        return false;
    }
    milliseconds = static_cast<int>(value);
    return true;
}

// ---- Command interpreter ------------------------------------------------

// Executes one trimmed, non-empty input line. The text to print goes in
// output; returns false when the console should terminate.
bool process_command(const std::string &input, std::string &output) {
    // Split the line into the command and the text that follows it.
    const std::string::size_type separator = input.find_first_of(WHITESPACE);
    const std::string command =
        (separator == std::string::npos) ? input : input.substr(0, separator);
    const std::string argument =
        (separator == std::string::npos) ? "" : trim(input.substr(separator + 1));

    if (command == "help") {
        output = help_text();
    } else if (command == "start_marquee") {
        animation_on = true;
        frame_requested = true;
        output = "Marquee animation started.\n";
        if (!font_loaded) {
            output += std::string("Warning: ") + FONT_FILE +
                      " was not found, so the marquee shows plain text.\n";
        }
    } else if (command == "stop_marquee") {
        animation_on = false;
        clear_marquee_area();
        output = "Marquee animation stopped.\n";
    } else if (command == "set_text") {
        if (argument.empty()) {
            output = "Error: set_text requires text. Usage: set_text <your_string>\n";
        } else {
            {
                std::lock_guard<std::mutex> lock(text_mtx);
                marquee_text = argument; // saved in memory for the marquee
            }
            output = "Text saved for marquee: " + argument + "\n";
        }
    } else if (command == "set_speed") {
        int milliseconds = 0;
        if (parse_speed(argument, milliseconds)) {
            speed_ms = milliseconds;
            frame_requested = true; // apply the new interval right away
            output = "Marquee speed set to " + std::to_string(milliseconds) + " milliseconds.\n";
        } else {
            output = "Error: set_speed requires a whole number of milliseconds greater than zero. "
                     "Usage: set_speed <milliseconds>\n";
        }
    } else if (command == "exit") {
        output = "Terminating console...\n";
        return false;
    } else {
        output = "Unrecognized command: " + command +
                 ". Type 'help' to see the available commands.\n";
    }
    return true;
}

// ---- Keyboard input -----------------------------------------------------

// Redraws the prompt line in place. Only the tail of a line longer than the
// console width is shown, so the prompt never wraps onto another row.
void redraw_prompt(const std::string &input) {
    int room = console_width() - static_cast<int>(PROMPT.size());
    if (room < 1) {
        room = 1;
    }
    const std::string::size_type visible = static_cast<std::string::size_type>(room);
    const std::string shown =
        (input.size() > visible) ? input.substr(input.size() - visible) : input;

    std::lock_guard<std::mutex> lock(console_mtx);
    std::cout << "\r\033[2K" << PROMPT << shown << std::flush;
}

// Polls the keyboard until Enter is pressed and returns the line typed. The
// prompt itself has already been printed. Polling (instead of std::getline)
// keeps the marquee animating while the user types.
std::string read_command_line() {
    std::string input;

    while (true) {
        if (!_kbhit()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(INPUT_POLL_MS));
            continue;
        }

        int key = _getch();
        if (key == '\r' || key == '\n') {
            std::lock_guard<std::mutex> lock(console_mtx);
            std::cout << "\n" << std::flush;
            return input;
        }
        if (key == 0 || key == 0xE0) {
            // Arrow, function and navigation keys arrive as two codes: discard
            // the second one too (it is already buffered, so this never blocks).
            if (_kbhit()) {
                _getch();
            }
            continue;
        }
        if (key == '\t') {
            key = ' '; // a tab separates words like a space
        }
        if (key == '\b') {
            if (input.empty()) {
                continue;
            }
            input.pop_back();
        } else if (key >= 32 && key <= 126) {
            input += static_cast<char>(key);
        } else {
            continue; // other control keys are ignored
        }
        redraw_prompt(input);
    }
}

} // namespace

int main() {
    enable_virtual_terminal();
    SetConsoleCtrlHandler(on_console_ctrl, TRUE);
    font_loaded = load_font();

    // The transcript scrolls inside rows 1..region_bottom; the marquee owns
    // the bottom MARQUEE_ROWS rows, with one blank row between them.
    int region_bottom = console_rows() - MARQUEE_ROWS - 1;
    if (region_bottom < 1) {
        region_bottom = 1;
    }
    marquee_top = region_bottom + 2;

    std::thread animation(marquee_thread_main);

    {
        std::lock_guard<std::mutex> lock(console_mtx);
        std::cout << "\033[2J" << "\033[1;" << region_bottom << "r" << move_to(1);
        const std::vector<std::string> header = header_lines();
        for (std::size_t i = 0; i < header.size(); ++i) {
            std::cout << header[i] << "\n";
        }
        std::cout.flush();
    }

    bool running = true;
    while (running) {
        {
            std::lock_guard<std::mutex> lock(console_mtx);
            std::cout << "\n" << PROMPT << std::flush;
        }

        const std::string input = trim(read_command_line());
        if (input.empty()) {
            continue; // nothing typed, show the prompt again
        }

        std::string output;
        running = process_command(input, output);

        std::lock_guard<std::mutex> lock(console_mtx);
        std::cout << output << std::flush;
    }

    program_running = false;
    animation.join();

    // Leave the console as a normal scrolling window again.
    clear_marquee_area();
    std::lock_guard<std::mutex> lock(console_mtx);
    std::cout << "\033[s\033[r\033[u" << std::flush;
    return 0;
}
