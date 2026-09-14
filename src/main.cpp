// main.cpp - entry point of the CSOPESY MO3 Marquee Console.
//
// Wires the three threads together:
//   renderer (marquee::run)  draws the screen every speed_ms      -> refresh rate
//   poller   (input::run)    reads the keyboard every poll_ms     -> polling rate
//   main     (this file)     runs commands taken from the poller's queue
// and owns their lifetime: `exit` (or Ctrl+C) sets state.quit, main joins both
// workers, then restores the terminal.
#include "console.hpp"
#include "input.hpp"
#include "interpreter.hpp"
#include "marquee.hpp"
#include "marquee_state.hpp"
#include "platform.hpp"

#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr const char* kDefaultText = "Welcome to CSOPESY! This is the marquee console.";
constexpr const char* kConfigName = "config.txt";

// Everything that can be tuned without recompiling (the quiz forbids rebuilds).
struct Config {
    std::string text = kDefaultText;
    int speed_ms = 100;
    int poll_ms = 15;
    int width = 0;                       // 0 = follow the terminal
    std::string source;                  // which file was read, for the startup log
    std::vector<std::string> warnings;   // problems found while reading it
};

// Look for config.txt in the working directory and up to three parents, so
// running from build/, build/Release/ or out/build/<preset>/ still finds the
// copy in the repository root.
std::string find_config(int argc, char** argv) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::strcmp(argv[i], "--config") == 0) return argv[i + 1];
    }
    const char* candidates[] = {"", "../", "../../", "../../../"};
    for (const char* prefix : candidates) {
        std::string path = std::string(prefix) + kConfigName;
        if (std::ifstream(path).good()) return path;
    }
    return std::string();
}

void read_int_setting(Config& cfg, const std::string& key, const std::string& value, int min, int max, int& target) {
    interpreter::ParsedNumber n = interpreter::parse_milliseconds(value, min, max);
    switch (n.status) {
    case interpreter::NumberStatus::Ok:
        target = static_cast<int>(n.value);
        break;
    case interpreter::NumberStatus::Clamped:
        target = static_cast<int>(n.value);
        cfg.warnings.push_back("config: " + key + " " + value + " is above the maximum; using " + std::to_string(max) + ".");
        break;
    default:
        cfg.warnings.push_back("config: ignoring invalid " + key + " '" + value + "' (expected " + std::to_string(min)
                               + "-" + std::to_string(max) + "); keeping " + std::to_string(target) + ".");
        break;
    }
}

Config load_config(const std::string& path) {
    Config cfg;
    if (path.empty()) return cfg;

    std::ifstream in(path);
    if (!in) return cfg;
    cfg.source = path;

    std::string line;
    int line_no = 0;
    while (std::getline(in, line)) {
        ++line_no;
        std::string s = interpreter::trim(line);
        if (s.empty() || s[0] == '#') continue;   // '#' only starts a comment at the beginning of a line

        std::size_t end = 0;
        while (end < s.size() && !std::isspace(static_cast<unsigned char>(s[end]))) ++end;
        std::string key = s.substr(0, end);
        std::string value = interpreter::trim(s.substr(end));

        if (key == "text") {
            if (value.empty()) cfg.warnings.push_back("config: 'text' on line " + std::to_string(line_no) + " is empty; keeping the default.");
            else cfg.text = value;
        } else if (key == "speed_ms") {
            read_int_setting(cfg, key, value, limits::kMinSpeedMs, limits::kMaxSpeedMs, cfg.speed_ms);
        } else if (key == "poll_ms") {
            read_int_setting(cfg, key, value, limits::kMinPollMs, limits::kMaxPollMs, cfg.poll_ms);
        } else if (key == "width") {
            if (value == "0") cfg.width = 0;
            else read_int_setting(cfg, key, value, console::kMinWidth, 1000, cfg.width);
        } else {
            cfg.warnings.push_back("config: unknown setting '" + key + "' on line " + std::to_string(line_no) + " ignored.");
        }
    }
    return cfg;
}

} // namespace

int main(int argc, char** argv) {
    Config cfg = load_config(find_config(argc, argv));

    MarqueeState state;
    state.text = cfg.text;
    state.speed_ms.store(cfg.speed_ms);
    state.poll_ms.store(cfg.poll_ms);
    state.width_override.store(cfg.width);

    // Ctrl+C must not kill us mid-frame with the terminal left in raw mode.
    platform::install_quit_handler(&state.quit);
    if (!platform::init()) {
        std::fprintf(stderr, "warning: could not configure the console; the display may be garbled.\n");
    }

    // Startup messages go through the same log region as command feedback.
    state.push_log(cfg.source.empty() ? std::string("No ") + kConfigName + " found; using built-in defaults."
                                      : "Loaded settings from " + cfg.source + ".");
    state.push_log(cfg.warnings);
    state.push_log("Type 'help' for a list of commands.");

    input::LineQueue queue;
    std::thread renderer(marquee::run, std::ref(state));
    std::thread poller(input::run, std::ref(state), std::ref(queue));

    // Main thread: the command interpreter. Wakes up for each finished line,
    // or every 50 ms to notice a quit request from Ctrl+C.
    while (!state.quit.load()) {
        std::string line;
        if (!queue.pop(line, std::chrono::milliseconds(50))) continue;

        interpreter::Command cmd = interpreter::parse(line);
        if (cmd.kind == interpreter::CommandKind::Empty) continue;

        std::vector<std::string> feedback = interpreter::execute(cmd, state);
        state.push_log(std::string(console::kPrompt) + interpreter::trim(line));   // echo, like a shell transcript
        state.push_log(feedback);
        state.request_redraw();
    }

    // Shutdown: wake the renderer so it notices `quit`, join both workers, then
    // main becomes the only writer and prints the farewell.
    state.request_redraw();
    renderer.join();
    poller.join();

    platform::write_out(console::goodbye_sequence());
    platform::shutdown();
    return 0;
}
