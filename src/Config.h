#pragma once

#include <iosfwd>
#include <string>
#include <vector>

// The settings read from config.txt: the marquee text, the animation refresh
// interval and the keyboard polling interval.
//
// config.txt is the only file the black-box quiz allows to be modified, so the
// program never has to be rebuilt to change one of these; each also has a
// run-time equivalent (set_text, set_speed and set_poll).
class Config {
public:
    static constexpr const char *FILE_NAME = "config.txt";
    static constexpr const char *DEFAULT_TEXT = "Hello, World!";
    static constexpr int DEFAULT_REFRESH_MS = 100;
    static constexpr int DEFAULT_POLLING_MS = 10;
    static constexpr long long MAX_REFRESH_MS = 999999999; // largest value set_speed accepts
    static constexpr long long MAX_POLLING_MS = 1000;      // largest value set_poll accepts

    const std::string &marquee_text() const { return marquee_text_; }
    int refresh_ms() const { return refresh_ms_; }
    int polling_ms() const { return polling_ms_; }

    // Reads an open config.txt: one "key value" pair per line, "#" starts a
    // comment, blank lines are ignored and "-" and "_" in a key are
    // interchangeable. An unknown key or an unusable value keeps the built-in
    // default. Returns the notes to print under the welcome header, ending with
    // the settings actually in force, so they are visible on screen and on the
    // video. A stream that could not be opened simply reads no lines.
    std::vector<std::string> read(std::istream &in);

private:
    std::string marquee_text_ = DEFAULT_TEXT;
    int refresh_ms_ = DEFAULT_REFRESH_MS;
    int polling_ms_ = DEFAULT_POLLING_MS;
};
