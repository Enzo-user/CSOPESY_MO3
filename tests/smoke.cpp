// Smoke test for the parts of the console that do not touch the Windows API:
// the config.txt reader, the ASCII font, the string helpers and the
// measurements. Plain asserts, no framework.
//
//   g++ -std=c++17 -o smoke tests/smoke.cpp src/Text.cpp src/Config.cpp src/AsciiFont.cpp src/Metrics.cpp
//   ./smoke
//
// The Windows console, the marquee thread and the keyboard loop are not
// covered here; docs/TESTING.md is the manual script for those.

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../src/AsciiFont.h"
#include "../src/Config.h"
#include "../src/Metrics.h"
#include "../src/Text.h"

namespace {

int checks = 0;

void check(bool ok, const std::string &what) {
    ++checks;
    if (!ok) {
        std::cout << "FAILED: " << what << "\n";
    }
    assert(ok && "see the line above");
}

bool has_note(const std::vector<std::string> &notes, const std::string &fragment) {
    for (std::size_t i = 0; i < notes.size(); ++i) {
        if (notes[i].find(fragment) != std::string::npos) {
            return true;
        }
    }
    return false;
}

void text_helpers() {
    check(text::trim("  help  ") == "help", "trim strips both ends");
    check(text::trim("set_text a  b") == "set_text a  b", "trim keeps inner spacing");
    check(text::trim("\t\r\n ") == "", "all-whitespace trims to empty");

    int ms = 0;
    check(text::parse_milliseconds("50", 1000, ms) && ms == 50, "plain value");
    check(text::parse_milliseconds("007", 1000, ms) && ms == 7, "leading zeros");
    check(!text::parse_milliseconds("abc", 1000, ms), "letters rejected");
    check(!text::parse_milliseconds("10x", 1000, ms), "trailing letter rejected");
    check(!text::parse_milliseconds("0", 1000, ms), "zero rejected");
    check(!text::parse_milliseconds("-5", 1000, ms), "negative rejected");
    check(!text::parse_milliseconds("", 1000, ms), "empty rejected");
    check(!text::parse_milliseconds("2000", 1000, ms), "above the maximum rejected");
    check(!text::parse_milliseconds("99999999999999999999", 1000, ms), "no overflow on a huge value");
}

void config_defaults() {
    Config config;
    std::istringstream empty("");
    const std::vector<std::string> notes = config.read(empty);

    check(config.marquee_text() == "Hello, World!", "default text");
    check(config.refresh_ms() == 100, "default refresh");
    check(config.polling_ms() == 10, "default polling");
    check(notes.size() == 1, "an empty file only reports the settings in force");
    check(notes.back() == "Config: text \"Hello, World!\", refresh 100 ms, polling 10 ms", "summary line");
}

void config_reads_the_shipped_file() {
    Config config;
    std::ifstream shipped("config.txt");
    check(shipped.is_open(), "config.txt is in the repository");
    const std::vector<std::string> notes = config.read(shipped);

    check(notes.size() == 1, "the shipped config.txt parses without a complaint");
    check(config.marquee_text() == "Hello, World!", "shipped text (quotes stripped)");
    check(config.refresh_ms() == 100, "shipped refresh, inline comment ignored");
    check(config.polling_ms() == 10, "shipped polling, inline comment ignored");
}

void config_accepts_edits() {
    Config config;
    std::istringstream in(
        "# a comment line\n"
        "\n"
        "marquee_text   CSOPESY MO3 is running  \n"
        "refresh-rate 500   # inline comment\n"
        "polling_rate 50\n");
    const std::vector<std::string> notes = config.read(in);

    check(config.marquee_text() == "CSOPESY MO3 is running", "unquoted text, ends trimmed");
    check(config.refresh_ms() == 500, "refresh from the file");
    check(config.polling_ms() == 50, "polling from an underscore key");
    check(notes.size() == 1, "a clean file reports no problems");
}

void config_keeps_defaults_on_bad_input() {
    Config config;
    std::istringstream in(
        "refresh-rate abc\n"
        "polling-rate 0\n"
        "polling-rate 2000\n"
        "bogus-key 5\n"
        "marquee-text\n");
    const std::vector<std::string> notes = config.read(in);

    check(config.refresh_ms() == 100, "an unusable refresh keeps the default");
    check(config.polling_ms() == 10, "an out-of-range polling keeps the default");
    check(has_note(notes, "refresh-rate 'abc' is not a whole number"), "refresh is reported");
    check(has_note(notes, "polling-rate '0' is not a whole number"), "zero polling is reported");
    check(has_note(notes, "polling-rate '2000' is not a whole number"), "2000 ms polling is reported");
    check(has_note(notes, "unknown setting 'bogus-key' ignored"), "unknown key is reported");
    check(has_note(notes, "marquee-text has no text"), "empty text is reported");
}

void config_reads_windows_line_endings() {
    Config config;
    std::istringstream in("marquee-text \"CSOPESY MO3\"\r\nrefresh-rate 500\r\npolling-rate 50\r\n");
    const std::vector<std::string> notes = config.read(in);

    check(config.marquee_text() == "CSOPESY MO3", "quotes stripped, no stray carriage return");
    check(config.refresh_ms() == 500 && config.polling_ms() == 50, "CRLF values parse");
    check(notes.size() == 1, "a CRLF file parses without a complaint");
}

void config_keeps_padding_inside_quotes() {
    Config config;
    std::istringstream in("marquee-text \"  padded  \"\n");
    config.read(in);
    check(config.marquee_text() == "  padded  ", "quotes keep leading and trailing spaces");
}

void font_without_a_file() {
    AsciiFont font;
    std::istringstream nothing("");
    check(!font.read(nothing), "an empty font file does not load");
    check(!font.loaded(), "the font reports itself as missing");

    const std::vector<std::string> rows = font.render("Hi");
    check(rows.size() == 1 && rows[0] == "Hi", "without a font the text is one plain row");
}

void font_from_the_shipped_file() {
    AsciiFont font;
    std::ifstream shipped("ascii_big.txt");
    check(shipped.is_open(), "ascii_big.txt is in the repository");
    check(font.read(shipped), "the shipped font loads");
    check(font.loaded(), "the font reports itself as loaded");

    const std::vector<std::string> rows = font.render("A");
    check(rows.size() == static_cast<std::size_t>(AsciiFont::ROWS), "a glyph is 8 rows tall");
    check(!rows[0].empty(), "the glyph has width");

    const std::vector<std::string> word = font.render("AB");
    check(word.size() == static_cast<std::size_t>(AsciiFont::ROWS), "two glyphs are still 8 rows");
    check(word[0].size() > rows[0].size(), "glyphs are stitched side by side");

    const std::vector<std::string> spaced = font.render(" ");
    check(spaced.size() == static_cast<std::size_t>(AsciiFont::ROWS), "space is a glyph too");

    // A character the font has no block for is drawn as a gap, not a crash.
    const std::vector<std::string> unknown = font.render(std::string(1, '\x01'));
    check(unknown.size() == static_cast<std::size_t>(AsciiFont::ROWS), "an unsupported character renders");
}

void metrics_report() {
    Metrics metrics;
    const std::string idle = metrics.format(100, 10);
    check(idle.find("Refresh : set 100 ms | measured n/a") == 0, "no interval measured yet");
    check(idle.find("Frames  : 0 drawn, 0 late") != std::string::npos, "no frames yet");

    metrics.record_frame(1.5);
    metrics.record_interval(99.0, 100);
    metrics.record_interval(200.0, 100); // late: more than 1.5x the setting
    metrics.record_poll(15.6);
    metrics.record_key(0.4);

    const std::string busy = metrics.format(100, 10);
    check(busy.find("measured avg 149.50, max 200.00, last 200.00 ms") != std::string::npos, "interval maths");
    check(busy.find("Frames  : 1 drawn, 1 late") != std::string::npos, "the late frame is counted");
    check(busy.find("Polling : set 10 ms | measured sleep avg 15.60") != std::string::npos, "polling sleep");
    check(busy.find("Typing  : key-to-screen avg 0.40") != std::string::npos, "key latency");

    metrics.reset();
    check(metrics.format(100, 10) == idle, "reset returns every counter to zero");
}

} // namespace

int main() {
    text_helpers();
    config_defaults();
    config_reads_the_shipped_file();
    config_accepts_edits();
    config_keeps_defaults_on_bad_input();
    config_reads_windows_line_endings();
    config_keeps_padding_inside_quotes();
    font_without_a_file();
    font_from_the_shipped_file();
    metrics_report();

    std::cout << checks << " checks passed\n";
    return 0;
}
