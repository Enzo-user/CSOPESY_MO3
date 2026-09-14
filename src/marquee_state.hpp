// marquee_state.hpp - the single object shared by the three threads.
//
//   renderer thread : reads text/offset/input/log, advances offset, writes stats
//   input thread    : appends to input_buffer, marks input dirty
//   main thread     : runs commands, mutates text/speed/running/quit, appends log
//
// Locking rules (CLAUDE.md section 4): text, offset, input_buffer, log and
// the stats are guarded by `m`; the tunables and flags are atomics so the
// hot loops can read them without taking the lock.
#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace limits {
constexpr int kMinSpeedMs = 1;       // set_speed lower bound (rejected below this)
constexpr int kMaxSpeedMs = 10000;   // set_speed upper bound (clamped above this)
constexpr int kMinPollMs = 1;        // set_poll / config poll_ms lower bound
constexpr int kMaxPollMs = 1000;     // set_poll / config poll_ms upper bound
constexpr std::size_t kMaxInputChars = 1024;  // typed line buffer cap
constexpr std::size_t kMaxLogLines = 200;     // command feedback history
} // namespace limits

using Clock = std::chrono::steady_clock;

// Measurements the renderer collects so the group can argue the refresh-rate
// vs. polling-rate trade-off with numbers (`stats` command, CLAUDE.md section 5).
struct RenderStats {
    // Tick-driven frames (the marquee "refresh rate").
    std::uint64_t frames = 0;         // every frame drawn, tick- or event-driven
    std::uint64_t tick_frames = 0;    // frames that advanced the marquee
    std::uint64_t late_frames = 0;    // tick frames whose interval > 1.5x the requested speed
    double interval_sum_ms = 0.0;     // sum over tick frames, for the average
    double interval_max_ms = 0.0;
    double interval_last_ms = 0.0;
    double interval_ema_ms = 0.0;     // smoothed value shown on the status line
    // Time to compose + write one frame.
    double draw_sum_ms = 0.0;
    double draw_max_ms = 0.0;
    // Actual sleep of the input thread between polls (the OS decides the real
    // granularity: Windows often rounds a 1 ms sleep up to ~15 ms).
    std::uint64_t poll_samples = 0;
    double poll_sum_ms = 0.0;
    double poll_max_ms = 0.0;
    // Keystroke noticed by the poller -> first frame that shows it.
    std::uint64_t keys = 0;
    std::uint64_t latency_samples = 0;
    double latency_sum_ms = 0.0;
    double latency_max_ms = 0.0;
    double latency_last_ms = 0.0;
};

struct MarqueeState {
    // ----- guarded by `m` -----
    std::mutex m;
    std::string text;
    std::size_t offset = 0;
    std::string input_buffer;
    std::deque<std::string> log;
    bool input_dirty = false;                // a keystroke is waiting to be drawn
    Clock::time_point input_dirty_since{};   // when the oldest undrawn keystroke arrived
    RenderStats stats;

    // ----- atomics: read every loop iteration without locking -----
    std::atomic<int> speed_ms{100};
    std::atomic<int> poll_ms{15};
    std::atomic<int> width_override{0};      // 0 = follow the terminal width
    std::atomic<bool> running{true};
    std::atomic<bool> quit{false};

    // ----- renderer wake-up -----
    // The renderer sleeps until the next scroll tick *or* until another thread
    // calls request_redraw() (a keystroke, a command result). That is what keeps
    // typing responsive even when the marquee refreshes only every 10 seconds.
    std::mutex wake_m;
    std::condition_variable wake_cv;
    bool redraw_requested = false;

    void request_redraw() {
        {
            std::lock_guard<std::mutex> lk(wake_m);
            redraw_requested = true;
        }
        wake_cv.notify_one();
    }

    // Append command feedback; oldest lines fall off the history.
    void push_log(const std::string& line) {
        std::lock_guard<std::mutex> lk(m);
        push_log_locked(line);
    }
    void push_log(const std::vector<std::string>& lines) {
        std::lock_guard<std::mutex> lk(m);
        for (const auto& l : lines) push_log_locked(l);
    }

private:
    void push_log_locked(const std::string& line) {
        log.push_back(line);
        while (log.size() > limits::kMaxLogLines) log.pop_front();
    }
};
