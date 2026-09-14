# CSOPESY_MO3 — Marquee Console

Semi-Major Output 1 for CSOPESY (DLSU, Term 1 AY 2026–2027): a console "OS emulator"
whose main menu animates a text marquee while a command interpreter keeps accepting
commands. C++17, standard library only, built with CMake.

`README.txt` is the submission README required by the spec (members, build/run
instructions, entry file). This file is the developer-facing overview.

## Build and run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

./build/csopesy_mo3                  # Linux / macOS
.\build\Release\csopesy_mo3.exe      # Windows (MSVC)

ctest --test-dir build --output-on-failure   # interpreter unit tests
```

Entry point: [`src/main.cpp`](src/main.cpp). Visual Studio 2022 opens the folder
directly (CMake integration); the debugger working directory is preset to the repo
root so `config.txt` is found.

## Commands

| Command | Behaviour |
|---|---|
| `help` | List the commands. |
| `start_marquee` | Start (resume) the animation. |
| `stop_marquee` | Stop (pause) the animation; the text stays frozen on screen. |
| `set_text <text>` | Replace the marquee text (everything after the command, spaces included). |
| `set_speed <ms>` | Refresh interval in ms, 1–10000. Rejects 0/negative/non-numeric; clamps above 10000. |
| `exit` | Terminate the console cleanly (also Ctrl+C). |
| `stats [reset]` | *Diagnostic.* Measured refresh interval, draw time, polling interval, typing delay. |
| `set_poll <ms>` | *Diagnostic.* Keyboard polling interval, 1–1000 ms. |

Startup settings (`text`, `speed_ms`, `poll_ms`, `width`) can be placed in
[`config.txt`](config.txt); nothing needs a rebuild.

## Architecture

```
src/
├── main.cpp            wires the threads, runs the interpreter loop, owns shutdown
├── platform.hpp/.cpp   raw mode, non-blocking keys, console size, single-write output (Win32 / POSIX)
├── marquee_state.hpp   the one shared object: text/offset/input/log under a mutex, tunables as atomics
├── marquee.hpp/.cpp    renderer thread: one frame every speed_ms, plus event-driven redraws
├── input.hpp/.cpp      poller thread: drains the keyboard every poll_ms, queues finished lines
├── interpreter.hpp/.cpp parse(line) -> Command; execute(Command, state) -> feedback lines
├── console.hpp/.cpp    layout + frame composition (ANSI cursor-home, per-line clears)
└── banner.hpp          ASCII art, group members, version date
```

| Thread | Rate | Job |
|---|---|---|
| Renderer | every `speed_ms` (refresh rate) | Copy state under the lock, compose the whole screen into one string, write it with one call. Also redraws immediately when a key or command changes something. |
| Input poller | every `poll_ms` (polling rate) | Drain pending keystrokes into the line buffer; push the line to a queue on Enter. Never writes to the terminal. |
| Main | on demand | Pop lines, parse, execute, append feedback to the log region; set `quit` on `exit`, join the others, restore the terminal. |

Design rules: one writer to stdout, one write per frame, no busy-waiting, no
`system("cls")`, no third-party libraries. See [`CLAUDE.md`](CLAUDE.md) for the full
brief and [`docs/`](docs/) for the manual test script and the measurement procedure
used in the technical report.
