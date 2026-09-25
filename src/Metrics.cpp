#include "Metrics.h"

#include <cstdio>

namespace {

std::string fmt_ms(double ms) {
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%.2f", ms);
    return buffer;
}

} // namespace

void Metrics::record_frame(double draw_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    ++frames_;
    draw_sum_ms_ += draw_ms;
    if (draw_ms > draw_max_ms_) draw_max_ms_ = draw_ms;
}

void Metrics::record_interval(double interval_ms, int refresh_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    ++intervals_;
    interval_sum_ms_ += interval_ms;
    interval_last_ms_ = interval_ms;
    if (interval_ms > interval_max_ms_) interval_max_ms_ = interval_ms;
    if (interval_ms > 1.5 * refresh_ms) ++late_frames_;
}

void Metrics::record_poll(double slept_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    ++polls_;
    poll_sum_ms_ += slept_ms;
    if (slept_ms > poll_max_ms_) poll_max_ms_ = slept_ms;
}

void Metrics::record_key(double latency_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    ++keys_;
    key_sum_ms_ += latency_ms;
    if (latency_ms > key_max_ms_) key_max_ms_ = latency_ms;
}

void Metrics::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    frames_ = late_frames_ = intervals_ = polls_ = keys_ = 0;
    interval_sum_ms_ = interval_max_ms_ = interval_last_ms_ = 0.0;
    draw_sum_ms_ = draw_max_ms_ = 0.0;
    poll_sum_ms_ = poll_max_ms_ = key_sum_ms_ = key_max_ms_ = 0.0;
}

std::string Metrics::format(int refresh_ms, int polling_ms) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string out;
    out += "Refresh : set " + std::to_string(refresh_ms) + " ms | measured ";
    if (intervals_ == 0) {
        out += "n/a (run start_marquee and wait a moment)\n";
    } else {
        out += "avg " + fmt_ms(interval_sum_ms_ / static_cast<double>(intervals_)) + ", max " +
               fmt_ms(interval_max_ms_) + ", last " + fmt_ms(interval_last_ms_) + " ms\n";
    }
    out += "Frames  : " + std::to_string(frames_) + " drawn, " + std::to_string(late_frames_) +
           " late (interval > 1.5x setting)\n";
    out += "Draw    : avg " + fmt_ms(frames_ ? draw_sum_ms_ / static_cast<double>(frames_) : 0.0) +
           ", max " + fmt_ms(draw_max_ms_) + " ms per frame (8 rows, one write)\n";
    out += "Polling : set " + std::to_string(polling_ms) + " ms | measured sleep avg " +
           fmt_ms(polls_ ? poll_sum_ms_ / static_cast<double>(polls_) : 0.0) + ", max " +
           fmt_ms(poll_max_ms_) + " ms (longest a key can wait to be noticed)\n";
    out += "Typing  : key-to-screen avg " + fmt_ms(keys_ ? key_sum_ms_ / static_cast<double>(keys_) : 0.0) +
           ", max " + fmt_ms(key_max_ms_) + " ms | worst ~" + fmt_ms(poll_max_ms_ + key_max_ms_) +
           " ms | " + std::to_string(keys_) + " keys\n";
    return out;
}
