#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class AsciiFont;
class Console;
class Metrics;

// The marquee animation: the text being scrolled, the refresh interval, and the
// thread that redraws the bottom rows of the window.
//
// The thread starts with the object and is joined when it is destroyed, so the
// console it draws on has to outlive it.
class Marquee {
public:
    Marquee(Console &console, const AsciiFont &font, Metrics &metrics);
    ~Marquee();
    Marquee(const Marquee &) = delete;
    Marquee &operator=(const Marquee &) = delete;

    void start();
    void stop();
    bool running() const { return running_; }

    void set_text(const std::string &text);
    std::string text() const;

    void set_refresh_ms(int milliseconds);
    int refresh_ms() const { return refresh_ms_; }

private:
    void run();                                                      // the thread body
    void draw_frame(const std::vector<std::string> &rows, std::size_t offset);

    Console &console_;
    const AsciiFont &font_;
    Metrics &metrics_;

    mutable std::mutex text_mutex_;           // guards text_
    std::string text_;
    std::atomic<bool> running_;               // start_marquee / stop_marquee
    std::atomic<bool> frame_requested_;       // draw the next frame right away
    std::atomic<int> refresh_ms_;
    std::atomic<bool> alive_;                 // cleared by the destructor
    std::thread thread_;
};
