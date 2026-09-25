#pragma once

#include <string>
#include <vector>

#include "AsciiFont.h"
#include "Config.h"
#include "Console.h"
#include "Marquee.h"
#include "Metrics.h"

// The application: the welcome header, the "Command>" loop, the command
// interpreter, and the keyboard polling that keeps the marquee animating while
// the user types.
//
// It owns the parts in the order they have to be destroyed: the marquee thread
// is joined before the console gives the window its scrolling back.
class MarqueeConsole {
public:
    MarqueeConsole();

    // Prints the header and runs the command loop until "exit".
    void run();

private:
    // Polls the keyboard until Enter is pressed and returns the line typed.
    // Polling (instead of std::getline) keeps the marquee animating while the
    // user types.
    std::string read_command_line();

    // Executes one trimmed, non-empty input line. The text to print goes in
    // output; returns false when the console should terminate.
    bool execute(const std::string &input, std::string &output);

    std::string stats_text() const;

    Config config_;
    std::vector<std::string> config_notes_;
    AsciiFont font_;
    Metrics metrics_;
    Console console_;
    Marquee marquee_;
    int polling_ms_; // only the console thread reads or writes this
};
