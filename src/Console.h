#pragma once

#include <functional>
#include <mutex>
#include <string>

// The Windows console window.
//
// Owns everything about the screen: ANSI escape sequence support, the size
// queries, the scrolling region that keeps the transcript above the marquee,
// and the one mutex every write to std::cout goes through, so a marquee frame
// and a prompt redraw can never interleave.
//
// Creating it prepares the window; destroying it clears the marquee rows and
// gives the window its normal full-screen scrolling back.
class Console {
public:
    explicit Console(int marquee_rows);
    ~Console();
    Console(const Console &) = delete;
    Console &operator=(const Console &) = delete;

    // ANSI escape that moves the cursor to column 1 of the given 1-based row.
    static std::string move_to(int row);

    // Columns the marquee may use: one less than the window width, so the last
    // column never wraps onto the next row.
    int width() const;
    int rows() const;
    int transcript_bottom() const { return transcript_bottom_; }
    int marquee_top() const { return marquee_top_; }

    // Clears the window and confines the transcript to the rows above the marquee.
    void begin_transcript();

    void print(const std::string &text);

    // Writes only if still_wanted() holds once the lock is taken, so a frame
    // built just before stop_marquee is dropped instead of drawn over the
    // rows it has already cleared.
    void write_if(const std::string &text, const std::function<bool()> &still_wanted);

    // Redraws the prompt line in place. Only the tail of a line longer than the
    // console width is shown, so the prompt never wraps onto another row.
    void redraw_prompt(const std::string &prompt, const std::string &input);

    void clear_marquee_area();

private:
    const int marquee_rows_;
    int transcript_bottom_; // last row of the transcript
    int marquee_top_;       // first row of the marquee area
    std::mutex mutex_;      // serialises every write to std::cout
};
