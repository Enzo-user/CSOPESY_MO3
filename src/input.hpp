// input.hpp - the keyboard polling thread ("polling rate") and the queue of
// completed command lines it hands to the main thread.
#pragma once

#include "marquee_state.hpp"

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>

namespace input {

// Thread-safe FIFO of finished lines (Enter was pressed).
class LineQueue {
public:
    void push(std::string line);
    // Waits up to `timeout` for a line. Returns false on timeout.
    bool pop(std::string& out, std::chrono::milliseconds timeout);

private:
    std::mutex m_;
    std::condition_variable cv_;
    std::deque<std::string> lines_;
};

// Body of the input thread. Every `poll_ms` it drains all pending keystrokes
// into the shared line buffer; on Enter the line goes to `queue`.
void run(MarqueeState& state, LineQueue& queue);

} // namespace input
