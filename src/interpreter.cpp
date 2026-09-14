// interpreter.cpp - command recognition (exact-match table) and execution.
#include "interpreter.hpp"
#include "console.hpp"
#include "platform.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>

namespace interpreter {
namespace {

struct CommandEntry {
    const char* name;
    CommandKind kind;
};

// The exact-match table. Names are compared after lower-casing the typed token.
constexpr CommandEntry kCommands[] = {
    {"help",          CommandKind::Help},
    {"start_marquee", CommandKind::StartMarquee},
    {"stop_marquee",  CommandKind::StopMarquee},
    {"set_text",      CommandKind::SetText},
    {"set_speed",     CommandKind::SetSpeed},
    {"exit",          CommandKind::Exit},
    {"stats",         CommandKind::Stats},
    {"set_poll",      CommandKind::SetPoll},
};

bool is_space(char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; }

std::string to_lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

// "set_text "hello world"" -> hello world (only when the quotes match).
std::string strip_matching_quotes(const std::string& s) {
    if (s.size() >= 2 && (s.front() == '"' || s.front() == '\'') && s.back() == s.front()) {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

std::string preview(const std::string& text, std::size_t max_len = 40) {
    if (text.size() <= max_len) return text;
    return text.substr(0, max_len - 3) + "...";
}

std::string fmt(double v) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.2f", v);
    return buf;
}

std::vector<std::string> note_extra_argument(const Command& cmd) {
    std::vector<std::string> out;
    if (!cmd.arg.empty()) out.push_back("Note: '" + cmd.name + "' takes no argument; ignoring '" + preview(cmd.arg) + "'.");
    return out;
}

// Shared by set_speed and set_poll: validates and reports in one place.
bool apply_milliseconds(const std::string& command, const std::string& what, const std::string& arg,
                        int min, int max, std::atomic<int>& target, std::vector<std::string>& out) {
    ParsedNumber n = parse_milliseconds(arg, min, max);
    const std::string range = "(" + std::to_string(min) + "-" + std::to_string(max) + ")";
    switch (n.status) {
    case NumberStatus::Empty:
        out.push_back(command + ": missing argument. Usage: " + command + " <milliseconds> " + range);
        return false;
    case NumberStatus::NotANumber:
        out.push_back(command + ": '" + preview(arg) + "' is not a whole number of milliseconds " + range + ".");
        return false;
    case NumberStatus::TooSmall:
        out.push_back(command + ": " + what + " must be at least " + std::to_string(min) + " ms (got " + preview(arg) + ").");
        return false;
    case NumberStatus::Clamped:
        out.push_back(command + ": " + preview(arg) + " is above the maximum; using " + std::to_string(max) + " ms.");
        break;
    case NumberStatus::Ok:
        break;
    }
    target.store(static_cast<int>(n.value));
    return true;
}

// Measurements describe one setting; start over when the setting changes so
// `stats` never averages a 1 ms run together with a 10000 ms run.
void reset_stats(MarqueeState& st) {
    std::lock_guard<std::mutex> lk(st.m);
    st.stats = RenderStats{};
}

} // namespace

std::string trim(const std::string& s) {
    std::size_t b = 0, e = s.size();
    while (b < e && is_space(s[b])) ++b;
    while (e > b && is_space(s[e - 1])) --e;
    return s.substr(b, e - b);
}

ParsedNumber parse_milliseconds(const std::string& raw, long long min, long long max) {
    ParsedNumber r;
    std::string s = trim(raw);
    if (s.empty()) { r.status = NumberStatus::Empty; return r; }

    // Accept an optional "ms" suffix ("100ms", "100 ms").
    std::string lower = to_lower(s);
    if (lower.size() > 2 && lower.compare(lower.size() - 2, 2, "ms") == 0) {
        s = trim(s.substr(0, s.size() - 2));
        if (s.empty()) { r.status = NumberStatus::NotANumber; return r; }
    }

    bool negative = false;
    std::size_t i = 0;
    if (s[i] == '+' || s[i] == '-') { negative = (s[i] == '-'); ++i; }
    if (i >= s.size()) { r.status = NumberStatus::NotANumber; return r; }
    for (std::size_t j = i; j < s.size(); ++j) {
        if (!std::isdigit(static_cast<unsigned char>(s[j]))) { r.status = NumberStatus::NotANumber; return r; }
    }
    if (negative) { r.status = NumberStatus::TooSmall; return r; }

    // Accumulate with an overflow guard so "99999999999999999999" is simply
    // treated as "too large" instead of wrapping around.
    long long value = 0;
    for (std::size_t j = i; j < s.size(); ++j) {
        value = value * 10 + (s[j] - '0');
        if (value > max) { value = max + 1; break; }
    }
    if (value < min) { r.status = NumberStatus::TooSmall; r.value = value; return r; }
    if (value > max) { r.status = NumberStatus::Clamped; r.value = max; return r; }
    r.status = NumberStatus::Ok;
    r.value = value;
    return r;
}

Command parse(const std::string& line) {
    Command cmd;
    std::string s = trim(line);
    if (s.empty()) return cmd;   // CommandKind::Empty

    std::size_t end = 0;
    while (end < s.size() && !is_space(s[end])) ++end;
    cmd.name = s.substr(0, end);
    cmd.arg = trim(s.substr(end));

    const std::string key = to_lower(cmd.name);
    cmd.kind = CommandKind::Unknown;
    for (const CommandEntry& e : kCommands) {
        if (key == e.name) { cmd.kind = e.kind; break; }
    }
    return cmd;
}

std::vector<std::string> help_lines() {
    return {
        "help                 Show this list of commands.",
        "start_marquee        Start (resume) the marquee animation.",
        "stop_marquee         Stop (pause) the marquee; the text stays frozen on screen.",
        "set_text <text>      Replace the marquee text (spaces allowed).",
        "set_speed <ms>       Set the marquee refresh interval in milliseconds ("
            + std::to_string(limits::kMinSpeedMs) + "-" + std::to_string(limits::kMaxSpeedMs) + ").",
        "exit                 Terminate the console.",
        "Diagnostics (not part of the spec):",
        "stats [reset]        Show measured refresh interval and key-to-screen latency.",
        "set_poll <ms>        Set the keyboard polling interval in milliseconds ("
            + std::to_string(limits::kMinPollMs) + "-" + std::to_string(limits::kMaxPollMs) + ").",
    };
}

std::vector<std::string> execute(const Command& cmd, MarqueeState& st) {
    std::vector<std::string> out;

    switch (cmd.kind) {
    case CommandKind::Empty:
        break;

    case CommandKind::Help:
        out = note_extra_argument(cmd);
        for (const std::string& l : help_lines()) out.push_back(l);
        break;

    case CommandKind::StartMarquee:
        out = note_extra_argument(cmd);
        if (st.running.exchange(true)) {
            out.push_back("Marquee is already running.");
        } else {
            out.push_back("Marquee started.");
        }
        break;

    case CommandKind::StopMarquee:
        out = note_extra_argument(cmd);
        if (!st.running.exchange(false)) {
            out.push_back("Marquee is already stopped.");
        } else {
            out.push_back("Marquee stopped. The text stays on screen; 'start_marquee' resumes it.");
        }
        break;

    case CommandKind::SetText: {
        std::string text = strip_matching_quotes(cmd.arg);
        if (text.empty()) {
            out.push_back("set_text: missing text. Usage: set_text <text>");
            break;
        }
        {
            std::lock_guard<std::mutex> lk(st.m);
            st.text = text;
            st.offset = 0;   // start the new text from the left edge
        }
        out.push_back("Marquee text set to \"" + preview(text, 50) + "\" (" + std::to_string(text.size()) + " characters).");
        break;
    }

    case CommandKind::SetSpeed:
        if (apply_milliseconds("set_speed", "speed", cmd.arg, limits::kMinSpeedMs, limits::kMaxSpeedMs, st.speed_ms, out)) {
            reset_stats(st);
            out.push_back("Marquee speed set to " + std::to_string(st.speed_ms.load()) + " ms per frame.");
        }
        break;

    case CommandKind::SetPoll:
        if (apply_milliseconds("set_poll", "polling interval", cmd.arg, limits::kMinPollMs, limits::kMaxPollMs, st.poll_ms, out)) {
            reset_stats(st);
            out.push_back("Keyboard polling interval set to " + std::to_string(st.poll_ms.load()) + " ms.");
        }
        break;

    case CommandKind::Stats: {
        const std::string sub = to_lower(cmd.arg);
        if (sub == "reset") {
            std::lock_guard<std::mutex> lk(st.m);
            st.stats = RenderStats{};
            out.push_back("Statistics reset.");
            break;
        }
        if (!sub.empty()) {
            out.push_back("stats: unknown option '" + preview(cmd.arg) + "'. Usage: stats [reset]");
            break;
        }
        RenderStats s;
        {
            std::lock_guard<std::mutex> lk(st.m);
            s = st.stats;
        }
        platform::ConsoleSize size = platform::console_size();
        console::Layout layout = console::compute_layout(size, st.width_override.load());
        const double interval_avg = s.tick_frames ? s.interval_sum_ms / static_cast<double>(s.tick_frames) : 0.0;
        const double draw_avg = s.frames ? s.draw_sum_ms / static_cast<double>(s.frames) : 0.0;
        const double latency_avg = s.latency_samples ? s.latency_sum_ms / static_cast<double>(s.latency_samples) : 0.0;
        const double poll_avg = s.poll_samples ? s.poll_sum_ms / static_cast<double>(s.poll_samples) : 0.0;
        // Every line is kept under 79 columns so nothing is cut on an 80-column terminal.
        out.push_back("Refresh : set " + std::to_string(st.speed_ms.load()) + " ms | measured avg " + fmt(interval_avg)
                      + ", max " + fmt(s.interval_max_ms) + ", last " + fmt(s.interval_last_ms) + " ms");
        out.push_back("Frames  : " + std::to_string(s.frames) + " drawn, " + std::to_string(s.tick_frames)
                      + " marquee ticks, " + std::to_string(s.late_frames) + " late (interval > 1.5x setting)");
        out.push_back("Draw    : avg " + fmt(draw_avg) + ", max " + fmt(s.draw_max_ms) + " ms per frame (compose + one write)");
        out.push_back("Polling : set " + std::to_string(st.poll_ms.load()) + " ms | measured sleep avg " + fmt(poll_avg)
                      + ", max " + fmt(s.poll_max_ms) + " ms (max key wait)");
        out.push_back("Typing  : seen-to-drawn avg " + fmt(latency_avg) + ", max " + fmt(s.latency_max_ms)
                      + " ms | worst ~" + fmt(s.poll_max_ms + s.latency_max_ms) + " ms | " + std::to_string(s.keys) + " keys");
        out.push_back("Terminal: " + std::to_string(size.cols) + "x" + std::to_string(size.rows) + " | marquee width "
                      + std::to_string(layout.width) + " | log rows " + std::to_string(layout.log_rows));
        break;
    }

    case CommandKind::Exit:
        out = note_extra_argument(cmd);
        out.push_back("Exiting...");
        st.quit.store(true);
        break;

    case CommandKind::Unknown:
        out.push_back("Unknown command: " + preview(cmd.name) + ". Type 'help' for a list of commands.");
        break;
    }
    return out;
}

} // namespace interpreter
