// marquee.cpp - renderer thread: snapshot under lock, compose + write outside it.
#include "marquee.hpp"
#include "console.hpp"
#include "platform.hpp"

#include <algorithm>
#include <chrono>

namespace marquee {
namespace {

using std::chrono::milliseconds;
using Ms = std::chrono::duration<double, std::milli>;

double to_ms(Clock::duration d) { return std::chrono::duration_cast<Ms>(d).count(); }

} // namespace

void run(MarqueeState& st) {
    Clock::time_point last_tick = Clock::now();      // when the marquee last advanced
    Clock::time_point last_tick_frame{};             // start of the last tick-driven frame (for interval stats)
    bool have_last_tick_frame = false;
    platform::ConsoleSize last_size{0, 0};
    bool first_frame = true;

    for (;;) {
        // 1. Sleep until the next scroll tick, or until someone asks for a redraw.
        //    While stopped there is no tick: we only wake for keystrokes/commands.
        const int speed = std::max(st.speed_ms.load(), 1);
        const bool running = st.running.load();
        Clock::time_point deadline = running ? last_tick + milliseconds(speed)
                                             : Clock::now() + std::chrono::hours(1);
        {
            std::unique_lock<std::mutex> lk(st.wake_m);
            st.wake_cv.wait_until(lk, deadline, [&] { return st.redraw_requested || st.quit.load(); });
            st.redraw_requested = false;
        }
        // On quit we still draw one last frame so the feedback of the final
        // command ("Exiting...") is on screen before main prints the farewell.
        const bool final_frame = st.quit.load();

        const Clock::time_point frame_start = Clock::now();
        const bool tick = running && !final_frame && frame_start >= deadline;
        if (tick) {
            // If the terminal fell more than one period behind, resynchronise
            // instead of trying to catch up with a burst of frames.
            last_tick = (frame_start - deadline > milliseconds(speed)) ? frame_start : deadline;
        }

        // 2. Snapshot the shared data under the lock (cheap copies only).
        platform::ConsoleSize size = platform::console_size();
        console::Layout layout = console::compute_layout(size, st.width_override.load());
        console::Snapshot snap;
        bool had_dirty_input = false;
        Clock::time_point dirty_since{};
        {
            std::lock_guard<std::mutex> lk(st.m);
            if (tick) st.offset = (st.offset + 1) % console::marquee_period(st.text, layout.width);
            snap.text = st.text;
            snap.offset = st.offset % console::marquee_period(st.text, layout.width);
            snap.input = st.input_buffer;
            snap.running = st.running.load();
            snap.speed_ms = speed;
            snap.poll_ms = st.poll_ms.load();
            snap.measured_interval_ms = st.stats.interval_ema_ms;
            std::size_t n = std::min(st.log.size(), static_cast<std::size_t>(layout.log_rows));
            snap.log_tail.assign(st.log.end() - static_cast<std::ptrdiff_t>(n), st.log.end());
            had_dirty_input = st.input_dirty;
            dirty_since = st.input_dirty_since;
            st.input_dirty = false;
        }

        // 3. Compose and write the whole frame - outside the lock, one write call.
        const bool resized = first_frame || size.cols != last_size.cols || size.rows != last_size.rows;
        std::string frame = console::compose_frame(snap, layout, resized);
        platform::write_out(frame);
        const Clock::time_point frame_end = Clock::now();
        first_frame = false;
        last_size = size;

        // 4. Record measurements for the `stats` command.
        {
            std::lock_guard<std::mutex> lk(st.m);
            RenderStats& s = st.stats;
            s.frames++;
            double draw = to_ms(frame_end - frame_start);
            s.draw_sum_ms += draw;
            s.draw_max_ms = std::max(s.draw_max_ms, draw);
            if (tick) {
                if (have_last_tick_frame) {
                    double interval = to_ms(frame_start - last_tick_frame);
                    s.tick_frames++;
                    s.interval_sum_ms += interval;
                    s.interval_last_ms = interval;
                    s.interval_max_ms = std::max(s.interval_max_ms, interval);
                    s.interval_ema_ms = s.interval_ema_ms <= 0.0 ? interval : 0.9 * s.interval_ema_ms + 0.1 * interval;
                    if (interval > 1.5 * speed) s.late_frames++;
                }
                last_tick_frame = frame_start;
                have_last_tick_frame = true;
            }
            if (had_dirty_input) {
                double latency = to_ms(frame_end - dirty_since);
                s.latency_samples++;
                s.latency_sum_ms += latency;
                s.latency_last_ms = latency;
                s.latency_max_ms = std::max(s.latency_max_ms, latency);
            }
        }
        if (final_frame) break;
    }
}

} // namespace marquee
