# Refresh rate vs. polling rate — measurement procedure

The technical report must give recommended values *for the group's hardware* and the
limits where screen tearing or typing delay appears. The `stats` command reports what
was actually measured; nothing is guessed. This page tells you exactly what to type,
what to watch, and where each number in the tables comes from.

## Before you start (once)

1. Build in Release and run the console in the **same terminal app you will use for
   the video** (the tearing limit depends on the terminal, not only on the PC).
2. Use a window of at least 100×30 so all six `stats` lines stay visible in the log
   region. Do not resize the window during a run.
3. Fill in the machine table below. The report asks for values "for your current
   hardware", so record what the numbers were measured on.

| Field | Value |
|---|---|
| CPU | *(e.g. Intel Core i5-12400)* |
| RAM | |
| OS | *(e.g. Windows 11 23H2)* |
| Terminal | *(Windows Terminal 1.x / conhost / PowerShell 7)* |
| Build | Release, MSVC 19.x |

## One run, step by step

Every row in the tables below is one run. A run is:

1. **Set the value under test.** `set_speed <ms>` for Experiment A, `set_poll <ms>` for
   Experiment B. Both commands wipe the counters automatically, so the run starts
   clean. (`stats reset` does the same thing; use it only if you did something else in
   between, such as `help`, another `set_*`, or a window resize.)
2. **Wait about 10 seconds and type while you wait.** Type a sentence on the prompt
   line so the poller has keystrokes to time. You do not have to press Enter: you can
   Backspace it away afterwards, or type a `set_text ...` line and press Enter so the
   keystrokes also do something visible.
3. **Watch the screen** during those seconds. The two "subjective" columns are yours
   to judge; the program cannot measure them:
   - Experiment A: does the marquee tear, flicker, or stutter?
   - Experiment B: do the characters you type appear late, or in bursts?
4. **Type `stats`** and copy the six lines it prints. Copy them *immediately*: the log
   region only keeps the last few lines, and the next command scrolls them away. A
   screenshot per run is the safest record.
5. Fill in one table row from the numbers (mapping below), then go to the next value.

### Reading the `stats` output

A real `stats` print looks like this (numbers from the reference run at the bottom):

```
Refresh : set 100 ms | measured avg 100.13, max 104.92, last 99.98 ms
Frames  : 80 drawn, 38 marquee ticks, 0 late (interval > 1.5x setting)
Draw    : avg 0.09, max 0.16 ms per frame (compose + one write)
Polling : set 15 ms | measured sleep avg 17.99, max 18.82 ms (max key wait)
Typing  : seen-to-drawn avg 0.14, max 0.36 ms | worst ~19.18 ms | 92 keys
Terminal: 100x30 | marquee width 99 | log rows 15
```

| Table column | `stats` line | Take this field |
|---|---|---|
| measured avg (ms) — Exp. A | `Refresh :` | `measured avg` |
| max (ms) — Exp. A | `Refresh :` | `max` |
| late frames / ticks — Exp. A | `Frames  :` | `late` and `marquee ticks`, written as `late / ticks` (e.g. `0 / 38`) |
| draw avg (ms) — Exp. A | `Draw    :` | `avg` |
| measured poll avg (ms) — Exp. B | `Polling :` | `measured sleep avg` |
| max (ms) — Exp. B | `Polling :` | `max` |
| seen-to-drawn max (ms) — Exp. B | `Typing  :` | `seen-to-drawn ... max` |
| worst-case delay (ms) — Exp. B | `Typing  :` | `worst ~` |
| visible tearing? / typing feels delayed? | *(your eyes)* | yes / no / slight, plus a note |

## Definitions used by the program

| Term | Meaning | Where |
|---|---|---|
| Refresh rate | How often the renderer redraws the marquee: one frame every `speed_ms`. | `set_speed`, `config.txt speed_ms` |
| Polling rate | How often the input thread checks the keyboard: once every `poll_ms`. | `set_poll`, `config.txt poll_ms` |
| Measured refresh | Real time between two marquee ticks (OS timer granularity and terminal speed add to the requested value). | `stats` → `Refresh` |
| Late frames | Ticks whose interval exceeded 1.5× the requested speed: the terminal (or the OS) could not keep up. | `stats` → `Frames` |
| Draw time | Compose + single write of one frame. | `stats` → `Draw` |
| Measured poll | Real duration of the poller's sleep. A key pressed just after a poll waits this long before it is noticed. | `stats` → `Polling` |
| Seen-to-drawn | From the poller noticing a key to the end of the frame that shows it. | `stats` → `Typing` |
| Worst-case typing delay | max measured poll + max seen-to-drawn. What a key pressed at the worst moment would wait. | `stats` → `Typing` |

## Experiment A — refresh rate (polling fixed at 15 ms)

Setup: type `set_poll 15` once so the poller is the same for every row. Then, for each
row: `set_speed <value>` → wait ~10 s while typing → `stats` → fill the row.

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

Slow rows need more time: a refresh interval needs two ticks before it has a sample, so
at 10000 ms wait at least 20 s (30 s gives a second sample), and at 1000 ms a 10 s run
only gives ~9 ticks. Note the tick count next to those numbers.

**Limit** = the smallest interval where `late frames` stays at 0 **and** no tearing is
seen. Below it the terminal drops behind (measured ≫ requested) and typing delay grows
because every redraw waits behind a slow write.

## Experiment B — polling rate (refresh fixed at the value chosen in A)

Setup: type `set_speed <the value you picked in A>` once (100 is a sensible default).
Then, for each row: `set_poll <value>` → wait ~10 s while typing → `stats` → fill the row.

| `set_poll` | measured poll avg (ms) | max (ms) | seen-to-drawn max (ms) | worst-case delay (ms) | typing feels delayed? |
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

**Limit** = the largest interval before the delay becomes noticeable (people usually
notice around 100 ms). Also note where a smaller interval stops improving anything: on
Windows a 1 ms sleep is usually rounded up to ~15.6 ms by the default timer resolution,
and the "measured poll" column will show it. That is the OS scheduler at work, not a
bug, and it is exactly the kind of limit the report asks for.

## Things that spoil a run

- Resizing the window forces a full-screen redraw; it shows up as one slow frame.
- Running `help` or `stats` mid-run adds a burst of redraws. Keep the run to typing only.
- Measuring in a different terminal than the video (Windows Terminal vs. conhost vs.
  an IDE console) gives different tearing limits. Use one and name it in the table.
- Comparing rows measured at different settings of the *other* knob. Fix polling
  during A and refresh during B, as the setup lines say.

## Recommended values (fill in from A and B)

| Setting | Value | Reason |
|---|---|---|
| refresh (`speed_ms`) | | |
| polling (`poll_ms`) | | |

Put the chosen values in `config.txt` so the demo starts with them.

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
