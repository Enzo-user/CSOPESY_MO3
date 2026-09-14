// console.hpp - screen layout and single-buffered frame composition.
//
// The renderer copies what it needs out of MarqueeState into a Snapshot
// (under the lock), then calls compose_frame() *outside* the lock to build the
// whole screen as one std::string that platform::write_out() emits in a single
// write. No other code produces terminal output while the console runs.
#pragma once

#include "platform.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace console {

// Spaces appended after the text so the wrap-around point is visible even
// when the text is shorter than the screen (at least this many, or a quarter
// of the marquee width, whichever is larger).
constexpr std::size_t kMinMarqueeGap = 8;
constexpr int kMinWidth = 20;
constexpr int kMaxLogRows = 16;
constexpr const char* kPrompt = "Command> ";

// Everything the frame needs, copied out of MarqueeState under its mutex.
struct Snapshot {
    std::string text;
    std::size_t offset = 0;
    std::string input;
    std::vector<std::string> log_tail;   // the last `Layout::log_rows` lines
    bool running = true;
    int speed_ms = 100;
    int poll_ms = 15;
    double measured_interval_ms = 0.0;   // smoothed actual refresh interval
};

// How the frame is laid out for a given terminal size.
struct Layout {
    int width = 80;        // usable columns (we never write into the last column)
    int rows = 24;
    bool full_banner = true;
    int log_rows = 8;
};

// Number of columns the marquee scrolls over: text length + gap.
std::size_t marquee_period(const std::string& text, int width);

// One row of the marquee: a `width`-wide window over the cycled text.
std::string marquee_row(const std::string& text, std::size_t offset, int width);

// Decide banner/log sizing from the terminal size (and an optional width override).
Layout compute_layout(platform::ConsoleSize size, int width_override);

// Build the complete frame (ANSI cursor-home + per-line clears). `full_clear`
// additionally wipes the screen, used for the first frame and after a resize.
std::string compose_frame(const Snapshot& s, const Layout& layout, bool full_clear);

// Text printed after the last frame when the console exits.
std::string goodbye_sequence();

} // namespace console
