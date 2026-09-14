// console.cpp - frame composition. Pure string building, no I/O.
#include "console.hpp"
#include "banner.hpp"

#include <algorithm>
#include <cstdio>

namespace console {
namespace {

constexpr const char* kEsc = "\x1b[";

// Replace control characters so a stray tab/escape in the text cannot break
// the frame, and cut the line to `max_cols` visible characters.
std::string sanitize(std::string line, std::size_t max_cols) {
    for (char& c : line) {
        unsigned char u = static_cast<unsigned char>(c);
        if (u < 0x20 || u == 0x7F) c = ' ';
    }
    if (line.size() > max_cols) line.resize(max_cols);
    return line;
}

// Append one screen row: content, erase-to-end-of-line, newline.
// "\r\n" rather than "\n" so it is correct on Windows consoles and in POSIX
// raw mode alike.
void put_line(std::string& out, const std::string& line, int width) {
    out += sanitize(line, static_cast<std::size_t>(width));
    out += kEsc;
    out += "K\r\n";
}

std::string separator(int width) { return std::string(static_cast<std::size_t>(width), '-'); }

std::string join_developers() {
    std::string s;
    for (std::size_t i = 0; i < banner::kDevelopers.size(); ++i) {
        if (i) s += ", ";
        s += banner::kDevelopers[i];
    }
    return s;
}

std::string format_ms(double ms) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.1f", ms);
    return buf;
}

} // namespace

std::size_t marquee_period(const std::string& text, int width) {
    std::size_t gap = std::max(kMinMarqueeGap, static_cast<std::size_t>(std::max(width, 0)) / 4);
    return text.size() + gap;   // always >= kMinMarqueeGap, so never zero
}

std::string marquee_row(const std::string& text, std::size_t offset, int width) {
    if (width <= 0) return std::string();
    const std::size_t period = marquee_period(text, width);
    std::string row(static_cast<std::size_t>(width), ' ');
    // Circular window: column i shows source[(offset + i) mod period], where
    // the source is the text followed by the blank gap. Increasing `offset`
    // slides the text to the left.
    for (std::size_t i = 0; i < row.size(); ++i) {
        std::size_t src = (offset + i) % period;
        row[i] = src < text.size() ? text[src] : ' ';
    }
    return row;
}

Layout compute_layout(platform::ConsoleSize size, int width_override) {
    Layout l;
    int cols = std::max(size.cols, kMinWidth);
    if (width_override > 0) cols = std::min(cols, std::max(width_override, kMinWidth));
    l.width = cols - 1;   // leave the last column alone: writing it triggers auto-wrap on some terminals
    l.rows = std::max(size.rows, 1);

    // Rows used by everything except the log region.
    constexpr int kFixedFull = 15;    // 5 art + separator + 3 info + separator + marquee + separator + status + separator + prompt
    constexpr int kFixedCompact = 7;  // 1 header line instead of the 9 banner rows
    constexpr int kMinLogRowsForFull = 6;

    l.full_banner = (l.rows - kFixedFull) >= kMinLogRowsForFull;
    int fixed = l.full_banner ? kFixedFull : kFixedCompact;
    l.log_rows = std::clamp(l.rows - fixed, 3, kMaxLogRows);
    return l;
}

std::string compose_frame(const Snapshot& s, const Layout& L, bool full_clear) {
    std::string out;
    out.reserve(4096);

    out += kEsc; out += "?25l";          // hide the real cursor; we draw our own
    if (full_clear) { out += kEsc; out += "2J"; }
    out += kEsc; out += "H";             // cursor home; every line below overwrites in place

    const std::string sep = separator(L.width);

    if (L.full_banner) {
        for (const char* row : banner::kAsciiArt) put_line(out, row, L.width);
        put_line(out, sep, L.width);
        put_line(out, std::string("Welcome to CSOPESY!  Marquee Console v") + banner::kVersion, L.width);
        put_line(out, "Group developers: " + join_developers(), L.width);
        put_line(out, std::string("Version date: ") + banner::kVersionDate, L.width);
    } else {
        put_line(out, std::string("CSOPESY Marquee Console v") + banner::kVersion + " | " + join_developers(), L.width);
    }
    put_line(out, sep, L.width);

    put_line(out, marquee_row(s.text, s.offset, L.width), L.width);
    put_line(out, sep, L.width);

    std::string status = std::string("Marquee: ") + (s.running ? "RUNNING" : "STOPPED")
        + " | refresh " + std::to_string(s.speed_ms) + " ms";
    if (s.running && s.measured_interval_ms > 0.0) status += " (measured " + format_ms(s.measured_interval_ms) + ")";
    status += " | poll " + std::to_string(s.poll_ms) + " ms | " + std::to_string(L.width + 1) + "x" + std::to_string(L.rows);
    put_line(out, status, L.width);

    // Log region: fixed height, the newest lines at the bottom, blank rows above.
    int blank = L.log_rows - static_cast<int>(s.log_tail.size());
    for (int i = 0; i < blank; ++i) put_line(out, "", L.width);
    std::size_t start = s.log_tail.size() > static_cast<std::size_t>(L.log_rows)
        ? s.log_tail.size() - static_cast<std::size_t>(L.log_rows) : 0;
    for (std::size_t i = start; i < s.log_tail.size(); ++i) put_line(out, s.log_tail[i], L.width);
    put_line(out, sep, L.width);

    // Prompt line (last row, no trailing newline or the terminal would scroll).
    // If the typed line is wider than the screen, show its tail like a shell does.
    std::size_t room = static_cast<std::size_t>(std::max(L.width - static_cast<int>(std::char_traits<char>::length(kPrompt)) - 1, 0));
    std::string shown = s.input.size() > room ? s.input.substr(s.input.size() - room) : s.input;
    out += sanitize(std::string(kPrompt) + shown + "_", static_cast<std::size_t>(L.width));
    out += kEsc; out += "K";   // clear the rest of this line
    out += kEsc; out += "J";   // and anything below (leftovers from a taller frame)
    return out;
}

std::string goodbye_sequence() {
    std::string out;
    out += kEsc; out += "K\r\n";
    out += kEsc; out += "J";
    out += "Console terminated. Goodbye!\r\n";
    out += kEsc; out += "?25h";   // give the cursor back to the shell
    return out;
}

} // namespace console
