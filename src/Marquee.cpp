#include "Marquee.h"

#include <chrono>

#include "AsciiFont.h"
#include "Config.h"
#include "Console.h"
#include "Metrics.h"

namespace {

const int TICK_MS = 10; // how often the thread checks its timers

double ms_between(std::chrono::steady_clock::time_point from, std::chrono::steady_clock::time_point to) {
    return std::chrono::duration<double, std::milli>(to - from).count();
}

} // namespace

Marquee::Marquee(Console &console, const AsciiFont &font, Metrics &metrics)
    : console_(console), font_(font), metrics_(metrics), text_(Config::DEFAULT_TEXT),
      running_(false), frame_requested_(false), refresh_ms_(Config::DEFAULT_REFRESH_MS), alive_(true) {
    thread_ = std::thread(&Marquee::run, this);
}

Marquee::~Marquee() {
    alive_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
}

void Marquee::start() {
    running_ = true;
    frame_requested_ = true;
}

void Marquee::stop() {
    running_ = false;
    console_.clear_marquee_area();
}

void Marquee::set_text(const std::string &text) {
    std::lock_guard<std::mutex> lock(text_mutex_);
    text_ = text;
}

std::string Marquee::text() const {
    std::lock_guard<std::mutex> lock(text_mutex_);
    return text_;
}

void Marquee::set_refresh_ms(int milliseconds) {
    refresh_ms_ = milliseconds;
    frame_requested_ = true; // apply the new interval right away
}

// Draws one frame in the marquee area: every row is the text repeated end to
// end, shifted left by offset columns, cut to the console width. The cursor is
// saved and restored so the transcript is never disturbed.
void Marquee::draw_frame(const std::vector<std::string> &rows, std::size_t offset) {
    const int width = console_.width();

    std::string frame = "\033[s";
    for (std::size_t r = 0; r < rows.size(); ++r) {
        const std::string pattern = rows[r] + "   "; // gap between repeats
        std::string window;
        window.reserve(static_cast<std::size_t>(width));
        for (int i = 0; i < width; ++i) {
            window += pattern[(offset + static_cast<std::size_t>(i)) % pattern.size()];
        }
        frame += Console::move_to(console_.marquee_top() + static_cast<int>(r)) + "\033[2K" + window;
    }
    frame += "\033[u";

    console_.write_if(frame, [this] { return running_.load(); });
}

// Redraws the marquee every refresh_ms_ milliseconds while it is running. The
// thread wakes every TICK_MS to notice speed changes, start/stop requests and
// the destructor promptly; frames are scheduled from the previous frame's due
// time so the average interval matches the requested speed.
void Marquee::run() {
    std::size_t offset = 0;
    std::chrono::steady_clock::time_point next_frame = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point last_frame;
    bool have_last_frame = false;

    while (alive_) {
        const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        if (running_ && (frame_requested_ || now >= next_frame)) {
            const bool forced = frame_requested_.exchange(false);

            draw_frame(font_.render(text()), offset);
            ++offset;
            const std::chrono::steady_clock::time_point drawn = std::chrono::steady_clock::now();

            const int refresh = refresh_ms_.load();
            metrics_.record_frame(ms_between(now, drawn));
            if (have_last_frame && !forced) {
                metrics_.record_interval(ms_between(last_frame, now), refresh);
            }
            last_frame = now;
            have_last_frame = true;

            const std::chrono::milliseconds interval(refresh);
            next_frame = forced ? now + interval : next_frame + interval;
            if (next_frame < now) {
                next_frame = now; // after a stall, resume instead of catching up
            }
        } else if (!running_) {
            have_last_frame = false; // the next start begins a fresh measurement
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TICK_MS));
    }
}
