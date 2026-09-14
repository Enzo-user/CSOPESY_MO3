// interpreter.hpp - parse a typed line into a Command, then execute it.
//
// Kept free of any terminal I/O so it can be unit-tested and lifted into the
// MO1 emulator later: execute() only mutates MarqueeState and returns the
// feedback lines the console should show.
#pragma once

#include "marquee_state.hpp"

#include <string>
#include <vector>

namespace interpreter {

enum class CommandKind {
    Empty,          // blank line: nothing to do
    Help,
    StartMarquee,
    StopMarquee,
    SetText,
    SetSpeed,
    Exit,
    Stats,          // diagnostics: measured refresh interval / typing latency
    SetPoll,        // diagnostics: change the keyboard polling interval
    Unknown
};

struct Command {
    CommandKind kind = CommandKind::Empty;
    std::string name;   // first token as typed (for error messages)
    std::string arg;    // everything after the first token, trimmed
};

// Split the line into the command name and its argument. Command names are
// matched case-insensitively (PowerShell style); the argument keeps its case.
Command parse(const std::string& line);

// Run the command against the shared state; returns the lines to display.
// `exit` sets state.quit; the caller decides what to do with it.
std::vector<std::string> execute(const Command& cmd, MarqueeState& state);

// The lines printed by `help`.
std::vector<std::string> help_lines();

// --- helpers exposed for the unit tests -------------------------------------

std::string trim(const std::string& s);

enum class NumberStatus { Ok, Empty, NotANumber, TooSmall, Clamped };

struct ParsedNumber {
    NumberStatus status = NumberStatus::Empty;
    long long value = 0;   // the value to use (already clamped when status == Clamped)
};

// Parse "<n>", "<n>ms" or "<n> ms" into an integer within [min, max].
// Anything above `max` is clamped; zero/negative and garbage are rejected.
ParsedNumber parse_milliseconds(const std::string& arg, long long min, long long max);

} // namespace interpreter
