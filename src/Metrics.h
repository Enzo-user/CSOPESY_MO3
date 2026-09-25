#pragma once

#include <mutex>
#include <string>

// What was actually measured while the console ran: the real interval between
// marquee frames, the real duration of the keyboard polling sleep and the delay
// from a keystroke being read to the prompt being redrawn.
//
// Behind the "stats" diagnostic, so the refresh-rate versus polling-rate
// trade-off in the technical report can be argued with numbers instead of
// guesses (see docs/MEASUREMENTS.md). Written by the marquee thread and the
// console thread, so every counter is guarded by the one mutex here.
class Metrics {
public:
    void record_frame(double draw_ms);
    void record_interval(double interval_ms, int refresh_ms);
    void record_poll(double slept_ms);
    void record_key(double latency_ms);
    void reset();

    // The measurement lines of the "stats" output, without the layout line the
    // console adds from its own geometry.
    std::string format(int refresh_ms, int polling_ms) const;

private:
    mutable std::mutex mutex_;
    unsigned long long frames_ = 0;       // marquee frames drawn
    unsigned long long late_frames_ = 0;  // frames whose interval exceeded 1.5x the setting
    unsigned long long intervals_ = 0;    // frame-to-frame intervals measured
    double interval_sum_ms_ = 0.0;
    double interval_max_ms_ = 0.0;
    double interval_last_ms_ = 0.0;
    double draw_sum_ms_ = 0.0;            // time to build and write one frame
    double draw_max_ms_ = 0.0;
    unsigned long long polls_ = 0;        // keyboard polling sleeps measured
    double poll_sum_ms_ = 0.0;
    double poll_max_ms_ = 0.0;
    unsigned long long keys_ = 0;         // keystrokes echoed
    double key_sum_ms_ = 0.0;             // from reading the key to the prompt being redrawn
    double key_max_ms_ = 0.0;
};
