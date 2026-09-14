# Refresh rate vs. polling rate — measurement procedure

The technical report must give recommended values *for the group's hardware* and the
limits where screen tearing or typing delay appears. The `stats` command reports what
was actually measured; nothing is guessed.

## Definitions used by the program

| Term | Meaning | Where |
|---|---|---|
| Refresh rate | How often the renderer redraws the marquee: one frame every `speed_ms`. | `set_speed`, `config.txt speed_ms` |
| Polling rate | How often the input thread checks the keyboard: once every `poll_ms`. | `set_poll`, `config.txt poll_ms` |
| Measured refresh | Real time between two marquee ticks (OS timer granularity and terminal speed add to the requested value). | `stats` → `Refresh` |
| Late frames | Ticks whose interval exceeded 1.5× the requested speed: the terminal (or the OS) could not keep up. | `stats` → `Frames` |
| Draw time | Compose + single write of one frame. | `stats` → `Draw` |
| Measured poll | Real duration of the poller's sleep. A key pressed just after a poll waits this long before it is noticed. | `stats` → `Polling` |
| Detect-to-draw | From the poller noticing a key to the frame that shows it. | `stats` → `Typing` |
| Worst-case typing delay | max measured poll + max detect-to-draw. | `stats` → `Typing` |

`set_speed` and `set_poll` reset the counters, so each row below is one clean run.
Type `stats reset`, wait ~10 s while typing a sentence, then `stats`.

## Machine under test

| Field | Value |
|---|---|
| CPU | *(e.g. Intel Core i5-12400)* |
| RAM | |
| OS | *(e.g. Windows 11 23H2)* |
| Terminal | *(Windows Terminal 1.x / conhost / PowerShell 7)* |
| Build | Release, MSVC 19.x |

## Experiment A — refresh rate (polling fixed at 15 ms)

| `set_speed` | measured avg (ms) | max (ms) | late frames / ticks | draw avg (ms) | visible tearing / flicker? | notes |
|---|---|---|---|---|---|---|
| 1 | | | | | | |
| 5 | | | | | | |
| 10 | | | | | | |
| 16 | | | | | | |
| 33 | | | | | | |
| 50 | | | | | | |
| 100 | | | | | | |
| 250 | | | | | | |
| 1000 | | | | | | |
| 10000 | | | | | | |

Limit = the smallest interval where `late frames` stays at 0 **and** no tearing is
seen. Below it the terminal drops behind (measured ≫ requested) and typing delay grows
because every redraw waits behind a slow write.

## Experiment B — polling rate (refresh fixed at the value chosen in A)

| `set_poll` | measured poll avg (ms) | max (ms) | detect-to-draw max (ms) | worst-case delay (ms) | typing feels delayed? |
|---|---|---|---|---|---|
| 1 | | | | | |
| 5 | | | | | |
| 10 | | | | | |
| 15 | | | | | |
| 20 | | | | | |
| 50 | | | | | |
| 100 | | | | | |
| 200 | | | | | |
| 500 | | | | | |
| 1000 | | | | | |

Limit = the largest interval before the delay becomes noticeable (people usually notice
around 100 ms). Also note where a smaller interval stops improving anything (Windows
often rounds a 1 ms sleep up to ~15 ms; the "measured poll" column shows it).

## Recommended values (fill in from A and B)

| Setting | Value | Reason |
|---|---|---|
| refresh (`speed_ms`) | | |
| polling (`poll_ms`) | | |

## Reference run (developer machine, not the group's demo hardware)

Apple M-series, macOS 15, pseudo-terminal 100x30, Release build with Apple Clang 17.
Given only to show the shape of the numbers; the report needs the group's Windows
machine.

| set_speed | set_poll | measured refresh avg / max (ms) | late / ticks | draw avg / max (ms) | measured poll avg / max (ms) | seen-to-drawn avg / max (ms) | worst-case typing delay (ms) |
|---|---|---|---|---|---|---|---|
| 100 | 15 | 100.13 / 104.92 | 0 / 38 | 0.09 / 0.16 | 17.99 / 18.82 | 0.14 / 0.36 | ~19.2 |
| 10 | 15 | 10.00 / 12.41 | 0 / 376 | 0.07 / 0.26 | 17.99 / 18.82 | 0.11 / 0.21 | ~19.0 |
| 1 | 15 | 1.00 / 1.63 | 1 / 3701 | 0.06 / 1.62 | 17.42 / 18.80 | 0.11 / 0.33 | ~19.1 |
| 100 | 1 | 100.01 / 104.86 | 0 / 37 | 0.09 / 0.14 | 1.26 / 1.38 | 0.13 / 0.35 | ~1.7 |

What the numbers say on that machine: the terminal keeps up even at a 1 ms refresh
(one late frame in 3701), so the refresh limit there is set by readability, not
tearing; the typing delay is dominated by the poll sleep, and the OS makes a 15 ms
sleep take ~18 ms. Expect different values on Windows, where a 1 ms sleep is often
rounded up to ~15.6 ms by the default timer resolution.

## Technical report outline (order required by the spec)

1. Command recognition — tokenizer (`interpreter::parse`), exact-match table `kCommands`.
2. Console UI implementation — `console::compose_frame`: ANSI cursor-home, per-line erase, one string, one write; layout adapts to the terminal size.
3. Command interpreter — parse → validate (`parse_milliseconds`) → execute; every error message.
4. Process representation — three threads over one `MarqueeState`: what is under the mutex, what is atomic, the redraw condition variable, the line queue.
5. Scheduler implementation — the program only chooses sleep intervals (`sleep_until` deadline, `sleep_for(poll_ms)`, `wait_until` on the condition variable); the OS scheduler decides when threads actually run (show measured vs requested).
6. Refresh vs. polling balance — Experiments A and B, recommended values, limits.
7. Embedded demo MP4.
