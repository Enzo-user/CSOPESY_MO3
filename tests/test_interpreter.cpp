// test_interpreter.cpp - plain-assert unit tests for parsing and execution.
// Build + run:  cmake --build build && ctest --test-dir build --output-on-failure
#include "interpreter.hpp"
#include "marquee_state.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

#define CHECK(cond)                                                                   \
    do {                                                                              \
        if (!(cond)) {                                                                \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);               \
            ++g_failures;                                                             \
        }                                                                             \
    } while (0)

bool contains(const std::vector<std::string>& lines, const std::string& needle) {
    for (const auto& l : lines) if (l.find(needle) != std::string::npos) return true;
    return false;
}

using interpreter::CommandKind;
using interpreter::NumberStatus;

void test_parse() {
    CHECK(interpreter::parse("").kind == CommandKind::Empty);
    CHECK(interpreter::parse("   \t ").kind == CommandKind::Empty);
    CHECK(interpreter::parse("help").kind == CommandKind::Help);
    CHECK(interpreter::parse("  HELP  ").kind == CommandKind::Help);
    CHECK(interpreter::parse("start_marquee").kind == CommandKind::StartMarquee);
    CHECK(interpreter::parse("stop_marquee").kind == CommandKind::StopMarquee);
    CHECK(interpreter::parse("exit").kind == CommandKind::Exit);
    CHECK(interpreter::parse("stats").kind == CommandKind::Stats);
    CHECK(interpreter::parse("set_poll 20").kind == CommandKind::SetPoll);

    auto c = interpreter::parse("set_text hello world there");
    CHECK(c.kind == CommandKind::SetText);
    CHECK(c.arg == "hello world there");

    c = interpreter::parse("   set_speed    250   ");
    CHECK(c.kind == CommandKind::SetSpeed);
    CHECK(c.arg == "250");

    c = interpreter::parse("frobnicate 1 2 3");
    CHECK(c.kind == CommandKind::Unknown);
    CHECK(c.name == "frobnicate");

    c = interpreter::parse("set-text abc");   // hyphen is not the spec spelling
    CHECK(c.kind == CommandKind::Unknown);
}

void test_parse_milliseconds() {
    auto p = interpreter::parse_milliseconds("100", 1, 10000);
    CHECK(p.status == NumberStatus::Ok && p.value == 100);
    p = interpreter::parse_milliseconds(" 1 ", 1, 10000);
    CHECK(p.status == NumberStatus::Ok && p.value == 1);
    p = interpreter::parse_milliseconds("10000", 1, 10000);
    CHECK(p.status == NumberStatus::Ok && p.value == 10000);
    p = interpreter::parse_milliseconds("+42", 1, 10000);
    CHECK(p.status == NumberStatus::Ok && p.value == 42);
    p = interpreter::parse_milliseconds("250ms", 1, 10000);
    CHECK(p.status == NumberStatus::Ok && p.value == 250);
    p = interpreter::parse_milliseconds("250 MS", 1, 10000);
    CHECK(p.status == NumberStatus::Ok && p.value == 250);

    CHECK(interpreter::parse_milliseconds("", 1, 10000).status == NumberStatus::Empty);
    CHECK(interpreter::parse_milliseconds("abc", 1, 10000).status == NumberStatus::NotANumber);
    CHECK(interpreter::parse_milliseconds("12.5", 1, 10000).status == NumberStatus::NotANumber);
    CHECK(interpreter::parse_milliseconds("1e3", 1, 10000).status == NumberStatus::NotANumber);
    CHECK(interpreter::parse_milliseconds("ms", 1, 10000).status == NumberStatus::NotANumber);
    CHECK(interpreter::parse_milliseconds("-", 1, 10000).status == NumberStatus::NotANumber);
    CHECK(interpreter::parse_milliseconds("0", 1, 10000).status == NumberStatus::TooSmall);
    CHECK(interpreter::parse_milliseconds("-5", 1, 10000).status == NumberStatus::TooSmall);

    p = interpreter::parse_milliseconds("10001", 1, 10000);
    CHECK(p.status == NumberStatus::Clamped && p.value == 10000);
    p = interpreter::parse_milliseconds("99999999999999999999999999", 1, 10000);   // would overflow long long
    CHECK(p.status == NumberStatus::Clamped && p.value == 10000);
}

void test_execute() {
    MarqueeState st;
    st.text = "initial";

    auto out = interpreter::execute(interpreter::parse("help"), st);
    CHECK(contains(out, "start_marquee"));
    CHECK(contains(out, "set_speed <ms>"));
    CHECK(out.size() == interpreter::help_lines().size());

    out = interpreter::execute(interpreter::parse("set_text hello world there"), st);
    CHECK(st.text == "hello world there");
    CHECK(contains(out, "hello world there"));

    out = interpreter::execute(interpreter::parse("set_text \"quoted text\""), st);
    CHECK(st.text == "quoted text");

    out = interpreter::execute(interpreter::parse("set_text"), st);
    CHECK(st.text == "quoted text");          // unchanged
    CHECK(contains(out, "missing text"));

    std::string long_text(500, 'x');
    out = interpreter::execute(interpreter::parse("set_text " + long_text), st);
    CHECK(st.text == long_text);

    out = interpreter::execute(interpreter::parse("set_speed 250"), st);
    CHECK(st.speed_ms.load() == 250);
    CHECK(contains(out, "250 ms"));

    out = interpreter::execute(interpreter::parse("set_speed"), st);
    CHECK(st.speed_ms.load() == 250);
    CHECK(contains(out, "missing argument"));

    out = interpreter::execute(interpreter::parse("set_speed fast"), st);
    CHECK(st.speed_ms.load() == 250);
    CHECK(contains(out, "not a whole number"));

    out = interpreter::execute(interpreter::parse("set_speed 0"), st);
    CHECK(st.speed_ms.load() == 250);
    CHECK(contains(out, "at least 1 ms"));

    out = interpreter::execute(interpreter::parse("set_speed -20"), st);
    CHECK(st.speed_ms.load() == 250);

    out = interpreter::execute(interpreter::parse("set_speed 1"), st);
    CHECK(st.speed_ms.load() == 1);
    out = interpreter::execute(interpreter::parse("set_speed 10000"), st);
    CHECK(st.speed_ms.load() == 10000);
    out = interpreter::execute(interpreter::parse("set_speed 123456789"), st);
    CHECK(st.speed_ms.load() == 10000);
    CHECK(contains(out, "above the maximum"));

    CHECK(st.running.load());
    out = interpreter::execute(interpreter::parse("start_marquee"), st);
    CHECK(st.running.load() && contains(out, "already running"));
    out = interpreter::execute(interpreter::parse("stop_marquee"), st);
    CHECK(!st.running.load() && contains(out, "stopped"));
    out = interpreter::execute(interpreter::parse("stop_marquee"), st);
    CHECK(!st.running.load() && contains(out, "already stopped"));
    out = interpreter::execute(interpreter::parse("start_marquee"), st);
    CHECK(st.running.load() && contains(out, "Marquee started"));

    out = interpreter::execute(interpreter::parse("set_poll 2000"), st);
    CHECK(st.poll_ms.load() == 1000);
    out = interpreter::execute(interpreter::parse("set_poll 20"), st);
    CHECK(st.poll_ms.load() == 20);

    out = interpreter::execute(interpreter::parse("stats"), st);
    CHECK(contains(out, "Refresh"));
    out = interpreter::execute(interpreter::parse("stats reset"), st);
    CHECK(contains(out, "reset"));

    out = interpreter::execute(interpreter::parse("bogus"), st);
    CHECK(out.size() == 1);
    CHECK(out[0] == "Unknown command: bogus. Type 'help' for a list of commands.");

    CHECK(!st.quit.load());
    out = interpreter::execute(interpreter::parse("exit"), st);
    CHECK(st.quit.load());
}

} // namespace

int main() {
    test_parse();
    test_parse_milliseconds();
    test_execute();
    if (g_failures) {
        std::printf("%d check(s) failed\n", g_failures);
        return EXIT_FAILURE;
    }
    std::printf("all interpreter checks passed\n");
    return EXIT_SUCCESS;
}
