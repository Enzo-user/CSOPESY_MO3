# MO3 Compliance Review — Marquee Console

**Reviewed:** commit `67284a7` on `main`, 2026-09-27
**Against:** *MO3 - Marquee_Operator* specification (updated Sept 4, 2026) and the
*Marquee Console Submission* quiz instructions

**Verdict: the program meets every functional requirement of the specification.**
No defects were found in the source. The documentation errors the review turned up
are corrected in the same change as this file (see the last section).

## How it was checked

- **Build.** Every file in `src/` compiled as C++11, C++14 and C++17 (Clang with the
  MinGW-w64 Windows target) without errors. `-Wall -Wextra` reports nothing in the
  project's own code.
- **Unit checks.** `tests/smoke.cpp`: 56 checks passed.
- **Running program.** `marquee.exe` was started in its own 120 × 30 console on
  Windows 11 and driven with injected keystrokes (`WriteConsoleInput`). The console's
  screen buffer was read back after every step. Every result below is what the
  program actually put on screen.

## Functional requirements (specification, page 2)

| Requirement | Implementation | Result |
|---|---|---|
| Main menu console: `Welcome to CSOPESY!`, `Group developer:`, `Version date:`, `Command>` prompt | `header_text()`, `MarqueeConsole::run()` | Pass |
| Text marquee or ASCII-graphics marquee | `AsciiFont` (`ascii_big.txt`, 8 rows) drawn by the `Marquee` thread in the bottom 8 rows | Pass |
| Accepts commands that change the marquee's behaviour | `set_text`, `set_speed`, `start_marquee`, `stop_marquee` | Pass |
| `help`: displays the commands and their descriptions | `help_text()`: the six commands, word for word | Pass |
| `start_marquee`: starts the animation | `Marquee::start()`: draws a frame at once | Pass |
| `stop_marquee`: stops the animation | `Marquee::stop()`: marquee rows cleared | Pass |
| `set_text`: accepts a text input and displays it as a marquee | Rest of the line kept (inner spaces preserved); shown on the next frame, including while running | Pass |
| `set_speed`: sets the refresh in milliseconds | `text::parse_milliseconds()` → `Marquee::set_refresh_ms()` | Pass |
| `exit`: terminates the console | Loop ends, marquee thread joined, window restored, exit code 0 | Pass |
| Unrecognized command: error, back to the prompt | `Unrecognized command: <word>. Type 'help' to see the available commands.` | Pass |

## Quiz test cases

### `set_text Hello world!`, then `start_marquee`

**Expected:** the marquee shows the ASCII graphics of "Hello world!" moving.

Steps: run the program, `set_text Hello world!`, `start_marquee`. Screen rows 12–30,
about 2 s after `start_marquee`:

```
Command> set_text Hello world!
Text saved for marquee: Hello world!

Command> start_marquee
Marquee animation started.

Command>




  _                                          _       _   _      _   _          _   _
 | |                                        | |     | | | |    | | | |        | | | |
 | |   ___        __      __   ___    _ __  | |   __| | | |    | |_| |   ___  | | | |   ___        __      __   ___
 | |  / _ \       \ \ /\ / /  / _ \  | '__| | |  / _` | | |    |  _  |  / _ \ | | | |  / _ \       \ \ /\ / /  / _ \  |
 | | | (_) |       \ V  V /  | (_) | | |    | | | (_| | |_|    | | | | |  __/ | | | | | (_) |       \ V  V /  | (_) | |
 |_|  \___/         \_/\_/    \___/  |_|    |_|  \__,_| (_)    \_| |_/  \___| |_| |_|  \___/         \_/\_/    \___/  |
```

A capture taken 0.35 s before this one shows the same art at a different offset, so
the text is moving. **Pass.**

The quiz asks to "show the animation configuration first" when using ASCII graphics.
Before any command is typed, the header's `Config:` line already prints the text,
refresh and polling values read from `config.txt`. Show `config.txt` in the IDE before
pressing Run/Debug.

### `help`

**Expected:** all the available commands and their descriptions.

```
Welcome to CSOPESY!

Group developer:
Obcena, Hans Gabriel
Suerte, Lorenzo Enrique
Cordero, Ramuel Sean
Eleydo, Renzel Vince

Version date: 2026-09-21
Config: text "Hello, World!", refresh 100 ms, polling 10 ms

Command> help
help - displays the commands and its description
start_marquee - starts the marquee "animation"
stop_marquee - stops the marquee "animation"
set_text - accepts a text input and displays it as a marquee
set_speed - sets the marquee animation refresh in milliseconds
exit - terminates the console

Command>
```

**Pass.** `help` prints exactly the six commands the specification defines. Two
diagnostic commands used for the technical report's measurements, `stats` and
`set_poll`, are deliberately left out of `help` (see `README.txt`, *Diagnostics*).

## Error paths checked

| Input | Result |
|---|---|
| `set_speed abc`, `set_speed 0`, `set_speed -5`, `set_speed`, `set_speed 10x` | Usage error each time; speed unchanged |
| `set_text` with no text | `Error: set_text requires text. Usage: set_text <your_string>` |
| `foo bar` | `Unrecognized command: foo. …` |
| `HELP` | Unrecognized (commands are case-sensitive, like the sample) |
| `   help   ` | Works; surrounding spaces ignored |
| `hel`, Backspace, `lp`, Enter | Runs `help` |
| A 150-character line | Prompt shows the tail; the whole line is processed |
| `set_text Hello` while the marquee runs | New text on the next frame |
| `start_marquee now` | Starts (trailing text ignored) |
| `exit` | `Terminating console...`, marquee rows cleared, exit code 0 |

## Submission requirements (specification, page 3, and quiz instructions)

| Requirement | Where | Result |
|---|---|---|
| `README.txt` with the group's names, how to run, and the entry point | `README.txt`: entry point `src/main.cpp`, `int main()`, class `MarqueeConsole` | Pass |
| Only `config.txt` is modified between test cases | `marquee-text`, `refresh-rate`, `polling-rate` read at startup; values echoed under the header; an unknown key or bad value prints a note and keeps the default | Pass |
| Technical report (PPT) | [`MO3_Marquee_Console_Technical_Report.pptx`](MO3_Marquee_Console_Technical_Report.pptx) | Provided |

The `config.txt` check used an edited file (`marquee_text "CSOPESY MO3"`,
`refresh-rate 250`, `polling-rate 20`, `bogus-key 5`). The header printed
`Config: unknown setting 'bogus-key' ignored.` and
`Config: text "CSOPESY MO3", refresh 250 ms, polling 20 ms`, and `stats` measured an
average frame interval of 249.97 ms.

## Behaviour to be aware of (not defects)

- **`set_speed` below about 16 ms runs at 15.6 ms.** The value is accepted, but
  Windows cannot sleep shorter than one timer tick. `README.txt` documents this, and
  the measurements below show it.
- **The marquee starts idle.** `start_marquee` begins the animation, as the
  specification's command list defines.
- **An extra `Config:` line** under the header, not in the specification's sample,
  shows the settings in force. It doubles as on-screen evidence in a recording.
- **The layout is fixed at startup.** Resizing the window mid-run needs a restart.

## Measured refresh and polling rates

AMD Ryzen 5 5600, 32 GB RAM, Windows 11 Pro (build 26200), 120 × 30 console,
`-O2` build. Each row: about 8 s of typing, then `stats`.

| `set_speed` (ms) | measured avg (ms) | max (ms) | late / drawn |
|---|---|---|---|
| 1 | 15.62 | 16.78 | 542 / 543 |
| 10 | 15.61 | 16.85 | 532 / 544 |
| 16 | 16.02 | 31.68 | 13 / 530 |
| 20 | 20.02 | 32.84 | 119 / 424 |
| 33 | 33.03 | 47.92 | 0 / 257 |
| 50 | 50.04 | 63.66 | 0 / 170 |
| 100 | 100.10 | 110.84 | 0 / 85 |
| 250 | 250.06 | 264.74 | 0 / 34 |
| 1000 | 1000.31 | 1012.05 | 0 / 13 |

| `set_poll` (ms) | measured sleep avg (ms) | worst typing delay (ms) |
|---|---|---|
| 1 | 15.50 | 17.0 |
| 10 | 15.47 | 17.0 |
| 15 | 16.74 | 31.4 |
| 20 | 30.99 | 33.3 |
| 50 | 62.06 | 64.0 |
| 100 | 108.83 | 111.9 |
| 200 | 202.89 | 215.2 |
| 500 | 511.34 | 515.2 |

**Recommended:** refresh 100 ms and polling 10 ms (the shipped defaults).

**Limits:**

- **Refresh:** below 16 ms there is no gain and almost every frame is late. At
  16–20 ms the intervals alternate between 15.6 and 31.2 ms, so the scroll stutters.
  From 33 ms up there are no late frames.
- **Polling:** 15 ms or less all measure the same 15.5 ms floor. Typing delay becomes
  noticeable at 100 ms (worst 112 ms) and bursty from 200 ms.

Drawing a frame takes 0.16–0.19 ms, so the two rates don't compete: typing delay stayed
at about 17 ms even at `set_speed 1`.

## Documentation fixes in this change

- **`README.txt`**
  - The MSVC build produced `marquee.exe` but the next line ran `main.exe`.
  - "8 .cpp files" corrected to 9 `.cpp` files and 8 headers.
  - The minimum window read "100 columns by about 20 rows"; it is now 100 × 28, matching
    `README.md` and `docs/TESTING.md`.
- **`docs/MEASUREMENTS.md`**
  - The rate table and the technical-report outline named identifiers that the class
    split removed (`speed_ms`, `poll_ms`, `process_command`, `draw_marquee_frame`,
    `animation_on`, `program_running`, `INPUT_POLL_MS`, `MARQUEE_TICK_MS`). They now
    name the current members.
