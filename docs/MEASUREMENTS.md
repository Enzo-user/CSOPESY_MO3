# Refresh rate vs. polling rate — measurement procedure

The technical report must give recommended values *for the group's hardware* and the
limits where screen tearing or typing delay appears. The `stats` command in the
console reports what was actually measured, so nothing has to be guessed. This page
says exactly what to type, what to watch, and where each number in the tables comes
from.

## The two rates in this program

| Term | Meaning | Where it is set |
|---|---|---|
| Refresh rate | How often the marquee thread redraws the eight marquee rows: one frame every `speed_ms`. | `set_speed <ms>` (default 100) |
| Polling rate | How often the console thread checks the keyboard with `_kbhit()`: once every `poll_ms`. | `set_poll <ms>` (default 10) |

Both are sleeps, so the operating system decides the real granularity. On Windows the
default timer resolution is about 15.6 ms: a sleep of 1 or 10 ms really lasts about
15.5 ms. The `stats` output shows this directly.

## Before you start (once)

1. Build in Release and run the console in the **same terminal you will use for the
   video** (the Visual Studio debug console is the classic Windows console host).
2. Use a window of at least 100 columns by 30 rows, so the transcript keeps the six
   `stats` lines visible above the marquee. Do not resize the window during a run.
3. Fill in the machine table. The report asks for values "for your current hardware".

| Field | Value |
|---|---|
| CPU | *(e.g. AMD Ryzen 5 5600)* |
| RAM | |
| OS | *(e.g. Windows 10 22H2 / Windows 11 24H2)* |
| Terminal | *(Windows console host / Windows Terminal 1.x)* |
| Build | *(Release, MSVC 19.x or MinGW-w64 g++ x.y)* |

## One run, step by step

Every row in the tables below is one run:

1. `start_marquee` once at the beginning (the refresh interval is only measured while
   the marquee is running).
2. **Set the value under test.** `set_speed <ms>` for Experiment A, `set_poll <ms>` for
   Experiment B. Both commands clear the measurements, so the run starts clean.
   (`stats reset` does the same; use it if you did anything else in between.)
3. **Wait about 10 seconds and type while you wait.** Type a sentence on the prompt
   line so the console thread has keystrokes to time. You do not have to press Enter:
   Backspace it away afterwards.
4. **Watch the screen** during those seconds. Two columns are yours to judge:
   - Experiment A: does the marquee tear, flicker or stutter?
   - Experiment B: do the typed characters appear late, or in bursts?
5. Type `stats` and copy the six lines (a screenshot per run is the safest record).
6. Fill in one table row, then move to the next value.

### Reading the `stats` output

A real print looks like this (numbers from the reference run below):

```
Refresh : set 100 ms | measured avg 99.97, max 111.03, last 109.34 ms
Frames  : 90 drawn, 0 late (interval > 1.5x setting)
Draw    : avg 0.40, max 0.64 ms per frame (8 rows, one write)
Polling : set 10 ms | measured sleep avg 15.49, max 16.38 ms (longest a key can wait to be noticed)
Typing  : key-to-screen avg 0.32, max 0.54 ms | worst ~16.91 ms | 91 keys
Terminal: 120x30 | transcript rows 1-21 | marquee rows 23-30
```

| Table column | `stats` line | Take this field |
|---|---|---|
| measured avg (ms) — Exp. A | `Refresh :` | `measured avg` |
| max (ms) — Exp. A | `Refresh :` | `max` |
| late / frames — Exp. A | `Frames  :` | `late` and `drawn`, written as `late / drawn` (e.g. `0 / 90`) |
| draw avg (ms) — Exp. A | `Draw    :` | `avg` |
| measured poll avg (ms) — Exp. B | `Polling :` | `measured sleep avg` |
| max (ms) — Exp. B | `Polling :` | `max` |
| key-to-screen max (ms) — Exp. B | `Typing  :` | `max` |
| worst-case delay (ms) — Exp. B | `Typing  :` | `worst ~` (max poll sleep + max key-to-screen) |
| tearing? / typing feels delayed? | *(your eyes)* | yes / no / slight, plus a note |

Definitions: *measured refresh* is the real time between two marquee frames; a frame
is *late* when that time exceeded 1.5 times the requested interval; *draw* is the time
to build and write one 8-row frame; *measured poll* is the real duration of the
polling sleep, which is the longest a key can wait before it is noticed;
*key-to-screen* is the time from reading a key to the prompt being redrawn.

## Experiment A — refresh rate (polling fixed at 10 ms)

Setup: `start_marquee`, then `set_poll 10` once. For each row: `set_speed <value>`,
wait about 10 s while typing, `stats`, fill in the row.

| `set_speed` | measured avg (ms) | max (ms) | late / frames | draw avg (ms) | visible tearing / flicker? | notes |
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
| 5000 | | | | | | |

Slow rows need more time: an interval needs two frames before it has a sample, so at
5000 ms wait at least 15 s. Note the frame count next to those numbers.

**Limit** = the smallest interval where `late` stays at 0 and no tearing is seen.
Below the Windows timer resolution (about 16 ms) every frame is late because the
program cannot sleep for less than one timer tick; the marquee simply runs at the
tick rate.

## Experiment B — polling rate (refresh fixed at the value chosen in A)

Setup: `set_speed <the value you picked in A>` once (100 is a sensible default). For
each row: `set_poll <value>`, wait about 10 s while typing, `stats`, fill in the row.

| `set_poll` | measured poll avg (ms) | max (ms) | key-to-screen max (ms) | worst-case delay (ms) | typing feels delayed? |
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
notice around 100 ms). Also note where a smaller interval stops improving anything:
on Windows `set_poll 1` and `set_poll 10` measure the same ~15.5 ms because of the
timer resolution. That is the OS scheduler at work, not a bug, and it is exactly the
kind of limit the report asks for.

## Things that spoil a run

- Resizing the window: the layout is fixed at start-up, so restart the program.
- Running `help` or other commands mid-run scrolls the transcript; keep the run to
  typing only, then `stats`.
- Measuring in a different terminal than the video (console host vs. Windows Terminal
  vs. an IDE pane). Use one and name it in the table.
- Changing the *other* knob between rows. Fix polling during A and refresh during B.

## Recommended values (fill in from A and B)

| Setting | Value | Reason |
|---|---|---|
| refresh (`set_speed`) | | |
| polling (`set_poll`) | | |

## Reference run (development machine)

AMD Ryzen 5 5600, 32 GB RAM, Windows 10 IoT Enterprise LTSC 21H2 (build 19044),
classic console host at 120x30, Release build with MSVC 14.51. Each row: about 7 s of
typing 91 keystrokes after the setting was applied. Given to show the shape of the
numbers; the report needs the group's own machine.

| set_speed | set_poll | measured refresh avg / max (ms) | late / frames | draw avg / max (ms) | measured poll avg / max (ms) | key-to-screen avg / max (ms) | worst-case typing delay (ms) |
|---|---|---|---|---|---|---|---|
| 100 | 10 | 99.97 / 111.03 | 0 / 90 | 0.40 / 0.64 | 15.49 / 16.38 | 0.32 / 0.54 | ~16.9 |
| 10 | 10 | 15.66 / 16.35 | 560 / 572 | 0.36 / 0.60 | 15.49 / 16.26 | 0.38 / 0.53 | ~16.8 |
| 1 | 10 | 15.67 / 16.38 | 571 / 571 | 0.37 / 0.59 | 15.49 / 16.28 | 0.37 / 0.52 | ~16.8 |
| 100 | 1 | 99.88 / 110.82 | 0 / 90 | 0.38 / 0.54 | 15.45 / 16.27 | 0.32 / 0.49 | ~16.8 |
| 100 | 50 | 99.92 / 110.51 | 0 / 90 | 0.36 / 0.51 | 62.13 / 63.97 | 0.30 / 0.48 | ~64.5 |
| 100 | 200 | 100.03 / 110.88 | 0 / 90 | 0.35 / 0.60 | 204.52 / 214.68 | 0.28 / 0.49 | ~215.2 |

What the numbers say on that machine:

- Drawing an 8-row frame costs well under 1 ms, so the terminal keeps up at any
  refresh interval; the practical floor is the 15.6 ms Windows timer tick. `set_speed`
  values below about 16 ms all run at roughly 64 frames per second and are reported as
  late, which is the refresh limit for this setup. At 100 ms the average interval is
  within 0.1 ms of the setting.
- The polling sleep is rounded up to the same timer tick: `set_poll 1` and
  `set_poll 10` both measure about 15.5 ms, so the smallest useful polling interval is
  the default 10 ms and going lower buys nothing. Typing delay grows one-to-one with
  the polling interval above that: about 64 ms at `set_poll 50` and about 215 ms at
  `set_poll 200`, which is where characters start to feel late.

## Technical report outline (order required by the specification)

1. Command recognition — `read_command_line` (keyboard polling, Backspace, ignored
   keys), `trim`, splitting into command and argument.
2. Console UI implementation — the transcript region (`ESC[1;Nr`), the marquee rows,
   `draw_marquee_frame` with cursor save/restore, the 8-row font loader.
3. Command interpreter implementation — `process_command`: the six commands, the
   validation in `parse_milliseconds`, every error message.
4. Process representation — two threads (console thread and marquee thread) sharing
   `marquee_text`, `animation_on`, `speed_ms`, `poll_ms` and `program_running`; what is
   under a mutex and what is atomic.
5. Scheduler implementation — the program only chooses sleep intervals
   (`INPUT_POLL_MS` / `set_poll`, `MARQUEE_TICK_MS`, the frame deadline); the OS
   scheduler decides when the threads actually run (show measured vs. requested).
6. Refresh vs. polling balance — Experiments A and B, recommended values, limits.
7. Embedded demo MP4.
