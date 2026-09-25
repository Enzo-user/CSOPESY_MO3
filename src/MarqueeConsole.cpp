#include "MarqueeConsole.h"

#include <chrono>
#include <fstream>
#include <thread>

#include <conio.h>

#include "DataFile.h"
#include "Text.h"

namespace {

const std::string PROMPT = "Command> ";

double ms_between(std::chrono::steady_clock::time_point from, std::chrono::steady_clock::time_point to) {
    return std::chrono::duration<double, std::milli>(to - from).count();
}

// The welcome header with the group developers and the version date.
std::string header_text() {
    return "Welcome to CSOPESY!\n"
           "\n"
           "Group developer:\n"
           "Obcena, Hans Gabriel\n"
           "Suerte, Lorenzo Enrique\n"
           "Cordero, Ramuel Sean\n"
           "Eleydo, Renzel Vince\n"
           "\n"
           "Version date: 2026-09-21\n";
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

} // namespace

MarqueeConsole::MarqueeConsole()
    : console_(AsciiFont::ROWS), marquee_(console_, font_, metrics_),
      polling_ms_(Config::DEFAULT_POLLING_MS) {
    std::ifstream font_file = open_data_file(AsciiFont::FILE_NAME);
    font_.read(font_file);

    std::ifstream config_file = open_data_file(Config::FILE_NAME);
    if (!config_file.is_open()) {
        config_notes_.push_back(std::string("Config: ") + Config::FILE_NAME +
                                " was not found, using the defaults.");
    }
    const std::vector<std::string> notes = config_.read(config_file);
    config_notes_.insert(config_notes_.end(), notes.begin(), notes.end());

    // The marquee thread is already running but idle, so the settings can be
    // handed over here rather than through its constructor.
    marquee_.set_text(config_.marquee_text());
    marquee_.set_refresh_ms(config_.refresh_ms());
    polling_ms_ = config_.polling_ms();
}

void MarqueeConsole::run() {
    console_.begin_transcript();

    std::string opening = header_text();
    for (std::size_t i = 0; i < config_notes_.size(); ++i) {
        opening += config_notes_[i] + "\n";
    }
    console_.print(opening);

    bool running = true;
    while (running) {
        console_.print("\n" + PROMPT);

        const std::string input = text::trim(read_command_line());
        if (input.empty()) {
            continue; // nothing typed, show the prompt again
        }

        std::string output;
        running = execute(input, output);
        console_.print(output);
    }
}

std::string MarqueeConsole::read_command_line() {
    std::string input;

    while (true) {
        if (!_kbhit()) {
            // The polling rate: a key pressed right after this check waits for
            // the whole sleep, so the sleep is timed for the "stats" command.
            const std::chrono::steady_clock::time_point before = std::chrono::steady_clock::now();
            std::this_thread::sleep_for(std::chrono::milliseconds(polling_ms_));
            metrics_.record_poll(ms_between(before, std::chrono::steady_clock::now()));
            continue;
        }

        const std::chrono::steady_clock::time_point seen = std::chrono::steady_clock::now();
        int key = _getch();
        if (key == '\r' || key == '\n') {
            console_.print("\n");
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
        console_.redraw_prompt(PROMPT, input);

        metrics_.record_key(ms_between(seen, std::chrono::steady_clock::now()));
    }
}

bool MarqueeConsole::execute(const std::string &input, std::string &output) {
    // Split the line into the command and the text that follows it.
    const std::string::size_type separator = input.find_first_of(text::WHITESPACE);
    const std::string command =
        (separator == std::string::npos) ? input : input.substr(0, separator);
    const std::string argument =
        (separator == std::string::npos) ? "" : text::trim(input.substr(separator + 1));

    if (command == "help") {
        output = help_text();
    } else if (command == "start_marquee") {
        marquee_.start();
        output = "Marquee animation started.\n";
        if (!font_.loaded()) {
            output += std::string("Warning: ") + AsciiFont::FILE_NAME +
                      " was not found, so the marquee shows plain text.\n";
        }
    } else if (command == "stop_marquee") {
        marquee_.stop();
        output = "Marquee animation stopped.\n";
    } else if (command == "set_text") {
        if (argument.empty()) {
            output = "Error: set_text requires text. Usage: set_text <your_string>\n";
        } else {
            marquee_.set_text(argument); // saved in memory for the marquee
            output = "Text saved for marquee: " + argument + "\n";
        }
    } else if (command == "set_speed") {
        int milliseconds = 0;
        if (text::parse_milliseconds(argument, Config::MAX_REFRESH_MS, milliseconds)) {
            marquee_.set_refresh_ms(milliseconds);
            metrics_.reset(); // measurements restart at the new setting
            output = "Marquee speed set to " + std::to_string(milliseconds) + " milliseconds.\n";
        } else {
            output = "Error: set_speed requires a whole number of milliseconds greater than zero. "
                     "Usage: set_speed <milliseconds>\n";
        }
    } else if (command == "set_poll") { // diagnostic: keyboard polling interval
        int milliseconds = 0;
        if (text::parse_milliseconds(argument, Config::MAX_POLLING_MS, milliseconds)) {
            polling_ms_ = milliseconds;
            metrics_.reset();
            output = "Keyboard polling interval set to " + std::to_string(milliseconds) + " milliseconds.\n";
        } else {
            output = "Error: set_poll requires a whole number of milliseconds from 1 to " +
                     std::to_string(Config::MAX_POLLING_MS) + ". Usage: set_poll <milliseconds>\n";
        }
    } else if (command == "stats") { // diagnostic: measured refresh and polling behaviour
        if (argument == "reset") {
            metrics_.reset();
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

std::string MarqueeConsole::stats_text() const {
    std::string out = metrics_.format(marquee_.refresh_ms(), polling_ms_);
    out += "Terminal: " + std::to_string(console_.width() + 1) + "x" + std::to_string(console_.rows()) +
           " | transcript rows 1-" + std::to_string(console_.transcript_bottom()) + " | marquee rows " +
           std::to_string(console_.marquee_top()) + "-" +
           std::to_string(console_.marquee_top() + AsciiFont::ROWS - 1) + "\n";
    return out;
}
