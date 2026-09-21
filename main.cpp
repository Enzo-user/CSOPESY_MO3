// CSOPESY MO3 - Marquee Console
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
// Two diagnostic commands, "stats" and "set_poll", expose the measured
// refresh interval and keyboard polling behaviour for the technical report;
// they are not part of the specification and are not listed by "help".
//
// Windows only: <conio.h> provides non-blocking keyboard polling and
// <windows.h> is used to enable ANSI escape sequences in the console.

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <atomic>
#include <chrono>
#include <cstdio>
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
const int INPUT_POLL_MS = 10;      // default keyboard polling interval (set_poll changes it)
const int MARQUEE_TICK_MS = 10;    // how often the marquee thread checks its timers
const int DEFAULT_SPEED_MS = 100;  // marquee refresh interval
const long long MAX_SPEED_MS = 999999999; // largest value set_speed accepts
const long long MAX_POLL_MS = 1000;       // largest value set_poll accepts
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
std::atomic<int> poll_ms(INPUT_POLL_MS);
std::atomic<bool> program_running(true);    // cleared by exit

std::map<char, std::vector<std::string>> font_map; // read-only once loaded
bool font_loaded = false;
int marquee_top = 0;    // first row of the marquee area; set before the thread starts
int region_bottom = 0;  // last row of the transcript; set before the thread starts

// Measurements behind the "stats" command, so the refresh-rate versus
// polling-rate trade-off can be argued with numbers (see docs/MEASUREMENTS.md).
struct Stats {
    unsigned long long frames = 0;        // marquee frames drawn
    unsigned long long late_frames = 0;   // frames whose interval exceeded 1.5x the setting
    unsigned long long intervals = 0;     // frame-to-frame intervals measured
    double interval_sum_ms = 0.0;
    double interval_max_ms = 0.0;
    double interval_last_ms = 0.0;
    double draw_sum_ms = 0.0;             // time to build and write one frame
    double draw_max_ms = 0.0;
    unsigned long long polls = 0;         // keyboard polling sleeps measured
    double poll_sum_ms = 0.0;
    double poll_max_ms = 0.0;
    unsigned long long keys = 0;          // keystrokes echoed
    double key_sum_ms = 0.0;              // from reading the key to the prompt being redrawn
    double key_max_ms = 0.0;
};
std::mutex stats_mtx;
Stats stats;

double ms_between(std::chrono::steady_clock::time_point from, std::chrono::steady_clock::time_point to) {
    return std::chrono::duration<double, std::milli>(to - from).count();
}

std::string fmt_ms(double ms) {
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%.2f", ms);
    return buffer;
}

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
    std::chrono::steady_clock::time_point last_frame;
    bool have_last_frame = false;

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
            const std::chrono::steady_clock::time_point drawn = std::chrono::steady_clock::now();

            const int speed = speed_ms.load();
            {
                std::lock_guard<std::mutex> lock(stats_mtx);
                ++stats.frames;
                const double draw = ms_between(now, drawn);
                stats.draw_sum_ms += draw;
                if (draw > stats.draw_max_ms) stats.draw_max_ms = draw;
                if (have_last_frame && !forced) {
                    const double interval = ms_between(last_frame, now);
                    ++stats.intervals;
                    stats.interval_sum_ms += interval;
                    stats.interval_last_ms = interval;
                    if (interval > stats.interval_max_ms) stats.interval_max_ms = interval;
                    if (interval > 1.5 * speed) ++stats.late_frames;
                }
            }
            last_frame = now;
            have_last_frame = true;

            const std::chrono::milliseconds interval(speed);
            next_frame = forced ? now + interval : next_frame + interval;
            if (next_frame < now) {
                next_frame = now; // after a stall, resume instead of catching up
            }
        } else if (!animation_on) {
            have_last_frame = false; // the next start begins a fresh measurement
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(MARQUEE_TICK_MS));
    }
}

// ---- Console text -------------------------------------------------------

// The welcome header with the group developers and the version date.
std::vector<std::string> header_lines() {
    std::vector<std::string> lines;
    lines.push_back("Welcome to CSOPESY!");
    lines.push_back("");
    lines.push_back("Group developer:");
    lines.push_back("Obcena, Hans Gabriel");
    lines.push_back("Suerte, Lorenzo Enrique");
    lines.push_back("Cordero, Ramuel Sean");
    lines.push_back("Eleydo, Renzel Vince");
    lines.push_back("");
    lines.push_back("Version date: 2026-09-21");
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

// Parses a whole number of milliseconds (digits only, from 1 to maximum).
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

// The "stats" diagnostic: what was actually measured since the last reset.
std::string stats_text() {
    Stats s;
    {
        std::lock_guard<std::mutex> lock(stats_mtx);
        s = stats;
    }
    std::string out;
    out += "Refresh : set " + std::to_string(speed_ms.load()) + " ms | measured ";
    if (s.intervals == 0) {
        out += "n/a (run start_marquee and wait a moment)\n";
    } else {
        out += "avg " + fmt_ms(s.interval_sum_ms / static_cast<double>(s.intervals)) + ", max " +
               fmt_ms(s.interval_max_ms) + ", last " + fmt_ms(s.interval_last_ms) + " ms\n";
    }
    out += "Frames  : " + std::to_string(s.frames) + " drawn, " + std::to_string(s.late_frames) +
           " late (interval > 1.5x setting)\n";
    out += "Draw    : avg " + fmt_ms(s.frames ? s.draw_sum_ms / static_cast<double>(s.frames) : 0.0) +
           ", max " + fmt_ms(s.draw_max_ms) + " ms per frame (8 rows, one write)\n";
    out += "Polling : set " + std::to_string(poll_ms.load()) + " ms | measured sleep avg " +
           fmt_ms(s.polls ? s.poll_sum_ms / static_cast<double>(s.polls) : 0.0) + ", max " +
           fmt_ms(s.poll_max_ms) + " ms (longest a key can wait to be noticed)\n";
    out += "Typing  : key-to-screen avg " + fmt_ms(s.keys ? s.key_sum_ms / static_cast<double>(s.keys) : 0.0) +
           ", max " + fmt_ms(s.key_max_ms) + " ms | worst ~" + fmt_ms(s.poll_max_ms + s.key_max_ms) +
           " ms | " + std::to_string(s.keys) + " keys\n";
    out += "Terminal: " + std::to_string(console_width() + 1) + "x" + std::to_string(console_rows()) +
           " | transcript rows 1-" + std::to_string(region_bottom) + " | marquee rows " +
           std::to_string(marquee_top) + "-" + std::to_string(marquee_top + MARQUEE_ROWS - 1) + "\n";
    return out;
}

void reset_stats() {
    std::lock_guard<std::mutex> lock(stats_mtx);
    stats = Stats();
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
        if (parse_milliseconds(argument, MAX_SPEED_MS, milliseconds)) {
            speed_ms = milliseconds;
            frame_requested = true; // apply the new interval right away
            reset_stats();          // measurements restart at the new setting
            output = "Marquee speed set to " + std::to_string(milliseconds) + " milliseconds.\n";
        } else {
            output = "Error: set_speed requires a whole number of milliseconds greater than zero. "
                     "Usage: set_speed <milliseconds>\n";
        }
    } else if (command == "set_poll") { // diagnostic: keyboard polling interval
        int milliseconds = 0;
        if (parse_milliseconds(argument, MAX_POLL_MS, milliseconds)) {
            poll_ms = milliseconds;
            reset_stats();
            output = "Keyboard polling interval set to " + std::to_string(milliseconds) + " milliseconds.\n";
        } else {
            output = "Error: set_poll requires a whole number of milliseconds from 1 to " +
                     std::to_string(MAX_POLL_MS) + ". Usage: set_poll <milliseconds>\n";
        }
    } else if (command == "stats") { // diagnostic: measured refresh and polling behaviour
        if (argument == "reset") {
            reset_stats();
            output = "Statistics reset.\n";
        } else if (argument.empty()) {
            output = stats_text();
        } else {
            output = "Error: unknown option '" + argument + "'. Usage: stats [reset]\n";
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
            // The polling rate: a key pressed right after this check waits for
            // the whole sleep, so the sleep is timed for the "stats" command.
            const std::chrono::steady_clock::time_point before = std::chrono::steady_clock::now();
            std::this_thread::sleep_for(std::chrono::milliseconds(poll_ms.load()));
            const double slept = ms_between(before, std::chrono::steady_clock::now());
            std::lock_guard<std::mutex> lock(stats_mtx);
            ++stats.polls;
            stats.poll_sum_ms += slept;
            if (slept > stats.poll_max_ms) stats.poll_max_ms = slept;
            continue;
        }

        const std::chrono::steady_clock::time_point seen = std::chrono::steady_clock::now();
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

        const double latency = ms_between(seen, std::chrono::steady_clock::now());
        std::lock_guard<std::mutex> lock(stats_mtx);
        ++stats.keys;
        stats.key_sum_ms += latency;
        if (latency > stats.key_max_ms) stats.key_max_ms = latency;
    }
}

} // namespace

int main() {
    enable_virtual_terminal();
    SetConsoleCtrlHandler(on_console_ctrl, TRUE);
    font_loaded = load_font();

    // The transcript scrolls inside rows 1..region_bottom; the marquee owns
    // the bottom MARQUEE_ROWS rows, with one blank row between them.
    region_bottom = console_rows() - MARQUEE_ROWS - 1;
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
