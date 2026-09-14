// input.cpp - keyboard poller. Never writes to the terminal; the renderer
// draws the buffer this thread edits.
#include "input.hpp"
#include "platform.hpp"

#include <algorithm>
#include <thread>

namespace input {

void LineQueue::push(std::string line) {
    {
        std::lock_guard<std::mutex> lk(m_);
        lines_.push_back(std::move(line));
    }
    cv_.notify_one();
}

bool LineQueue::pop(std::string& out, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lk(m_);
    if (!cv_.wait_for(lk, timeout, [&] { return !lines_.empty(); })) return false;
    out = std::move(lines_.front());
    lines_.pop_front();
    return true;
}

namespace {

// Upper bound on keys handled per poll so a flood (paste) cannot starve the
// sleep; whatever is left is picked up on the next poll.
constexpr int kMaxKeysPerPoll = 512;

// Remember when the oldest not-yet-drawn keystroke arrived (for latency stats).
void mark_input_dirty(MarqueeState& st) {
    st.stats.keys++;
    if (!st.input_dirty) {
        st.input_dirty = true;
        st.input_dirty_since = Clock::now();
    }
}

} // namespace

void run(MarqueeState& st, LineQueue& queue) {
    while (!st.quit.load()) {
        bool changed = false;
        platform::Key key{};
        // Drain everything that arrived since the last poll. The OS buffers
        // keystrokes between polls, so a slow poll rate delays characters but
        // never loses them.
        for (int n = 0; n < kMaxKeysPerPoll && platform::poll_key(key); ++n) {
            bool stop = false;
            switch (key.kind) {
            case platform::KeyKind::Char: {
                std::lock_guard<std::mutex> lk(st.m);
                if (st.input_buffer.size() < limits::kMaxInputChars) {
                    st.input_buffer.push_back(key.ch);
                    mark_input_dirty(st);
                    changed = true;
                }
                break;
            }
            case platform::KeyKind::Backspace: {
                std::lock_guard<std::mutex> lk(st.m);
                if (!st.input_buffer.empty()) {
                    st.input_buffer.pop_back();
                    mark_input_dirty(st);
                    changed = true;
                }
                break;
            }
            case platform::KeyKind::Enter: {
                std::string line;
                {
                    std::lock_guard<std::mutex> lk(st.m);
                    line.swap(st.input_buffer);
                    mark_input_dirty(st);
                }
                queue.push(std::move(line));
                changed = true;
                break;
            }
            case platform::KeyKind::Interrupt:
                st.quit.store(true);
                changed = true;
                stop = true;
                break;
            case platform::KeyKind::EndOfInput:
                // Nothing more will ever arrive: queue an `exit` behind the
                // lines already typed and let this thread finish.
                queue.push("exit");
                st.request_redraw();
                return;
            case platform::KeyKind::Ignore:
                break;
            }
            if (stop) break;
        }

        if (changed) st.request_redraw();
        if (st.quit.load()) break;

        // The polling rate. Everything else in this loop is nearly free, so
        // this sleep is what decides the typing delay the user perceives.
        // We time the sleep because the OS scheduler, not the program, decides
        // how long it really takes (a key can wait at most this long to be seen).
        const Clock::time_point before = Clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(st.poll_ms.load()));
        const double slept_ms = std::chrono::duration<double, std::milli>(Clock::now() - before).count();
        {
            std::lock_guard<std::mutex> lk(st.m);
            st.stats.poll_samples++;
            st.stats.poll_sum_ms += slept_ms;
            st.stats.poll_max_ms = std::max(st.stats.poll_max_ms, slept_ms);
        }
    }
}

} // namespace input
