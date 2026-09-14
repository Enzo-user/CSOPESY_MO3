# CLAUDE.md — CSOPESY_MO3 (Marquee Console)

This file is the working brief for Claude (and any other agent or teammate) on this repository.
Read it fully before writing code.

---

## 1. Project identity

| Field | Value |
|---|---|
| Repo | `CSOPESY_MO3` |
| Course | CSOPESY — Introduction to Operating Systems, DLSU, Term 1 AY 2026–2027 |
| Deliverable | **Semi-Major Output 1 — Marquee Console** ("MO3 - Marquee_Operator.pdf", updated Sept 4, 2026) |
| Due | **Wed, Sept 23, 2026, 11:59 PM** (Week 3) via AnimoSpace |
| Instructor | Gregory Cu (spec created by Neil Patrick Del Gallego, PhD) |
| Group type | Group project — same grouping carries over to MO1/MO2 |

### Group members

| Name | ID Number | Section |
|---|---|---|
| ______________________ | ___________ | ______ |
| ______________________ | ___________ | ______ |
| ______________________ | ___________ | ______ |
| ______________________ | ___________ | ______ |

---

## 2. Language and toolchain (decided — do not relitigate)

**C++17, built with CMake ≥ 3.16. Standard library only for MO3.**

Why C++ and not Python/Java/C#:

- **MO4 (Desktop-Style OS, Week 5) hard-requires Dear ImGui + GLFW + OpenGL.** That stack is C++-native. Choosing anything else now means rewriting the codebase in two weeks.
- **MO1/MO2 (Process Scheduler, Multitasking OS + Memory Manager)** need real OS threads, a CPU-tick loop, `uint16_t` arithmetic with clamping, and emulated paging/backing store. C++ gives direct control over all of that with no runtime surprises.
- MO3 itself is a two-thread problem (renderer + input poller) whose core lesson is the *refresh-rate vs. polling-rate* trade-off. `std::thread`, `std::atomic`, `std::mutex`, and `std::chrono` express that directly and are easy to explain on video.

Rules that follow from this:

- **No third-party libraries in MO3.** Not ncurses, not PDCurses, not FTXUI. Use `std::` plus the thin OS layer below. Every line must be explainable by a group member during the defense (syllabus: *"You must be able to explain, defend, and manually debug any AI-generated contribution."*).
- **Cross-platform console I/O** is isolated in one file (`platform.hpp/.cpp`):
  - Windows: `<conio.h>` (`_kbhit`, `_getch`) and `<windows.h>` (enable `ENABLE_VIRTUAL_TERMINAL_PROCESSING` so ANSI escapes work in PowerShell/Windows Terminal).
  - POSIX: `<termios.h>` raw mode + `select()`/`poll()` on stdin for non-blocking key reads.
  - Everything above this layer uses ANSI escape sequences only (`\x1b[H`, `\x1b[2J`, `\x1b[?25l`/`h`, `\x1b[<row>;<col>H`).
- Primary dev/demo target is **Windows** (the spec screenshots are PowerShell; the video must show pressing Run/Debug in an IDE). Visual Studio 2022 and CLion/VS Code + CMake must both work out of the box.
- Warnings on (`-Wall -Wextra` / `/W4`). Treat warnings as bugs.

### Build and run

```bash
# Configure + build (Release is what gets demoed)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Run
./build/csopesy_mo3            # POSIX
.\build\Release\csopesy_mo3.exe   # Windows / MSVC
```

Entry point: `src/main.cpp` (this is the "entry class file" the README.txt must name).

---

## 3. What the spec actually requires (checklist — all mandatory)

> *"An OS emulator needs a command interpreter and a display output."*

**A. Main menu console**
1. A text marquee **or** ASCII-graphics marquee that animates on screen.
2. Accepts commands that change the marquee's behavior — **while the marquee keeps animating**. Typing must not freeze the animation and the animation must not eat keystrokes.

**B. Command interpreter** — exactly these commands from the main menu:

| Command | Behavior |
|---|---|
| `help` | Display all commands with a one-line description each. |
| `start_marquee` | Start the marquee animation. |
| `stop_marquee` | Stop (pause) the animation. Text stays on screen, frozen. |
| `set_text <text>` | Replace the marquee text. Takes effect immediately, even mid-animation. |
| `set_speed <ms>` | Set the animation refresh interval in **milliseconds**. Takes effect immediately. |
| `exit` | Terminate the console cleanly (join threads, restore terminal). |

Use the exact command strings above (underscores, lowercase). The graders black-box test against them.

**C. Robustness** (graded: *"passed the test case using varying inputs"*). Handle gracefully, with a clear message, and never crash:
- Unknown command → `Unknown command: <x>. Type 'help' for a list of commands.`
- `set_text` with no argument, or `set_speed` with no / non-numeric / zero / negative / absurdly large argument.
- `set_text` with a very long string (longer than the terminal width) — it must scroll, not wrap or garble.
- `set_text` with spaces inside (`set_text hello world there` → text is `hello world there`; everything after the first token is the payload).
- `start_marquee` when already running, `stop_marquee` when already stopped → informative message, no state corruption.
- `set_speed` of 1 ms and 10,000 ms both work.
- Rapid typing while the marquee runs at a fast speed — no dropped or duplicated characters.
- Terminal resized mid-run (best effort; must not crash).

---

## 4. Architecture (target design)

Keep it small. Three threads, one shared state object, one platform layer.

```
src/
├── main.cpp            # wires everything, owns thread lifetime
├── platform.hpp/.cpp   # raw-mode toggle, non-blocking key read, cursor/ANSI helpers, console size
├── marquee_state.hpp   # struct MarqueeState { std::string text; std::atomic<int> speed_ms; std::atomic<bool> running; int offset; std::mutex m; }
├── marquee.hpp/.cpp    # Marquee thread: renders one frame every speed_ms
├── input.hpp/.cpp      # Input poller thread: non-blocking key read, builds the line buffer, echoes it
├── interpreter.hpp/.cpp# parse(line) -> Command; execute(Command, MarqueeState&)
└── console.hpp/.cpp    # Screen layout + single-buffered frame composition
```

**Threads**

| Thread | Rate | Responsibility |
|---|---|---|
| Marquee (renderer) | every `speed_ms` (the *refresh rate*) | Composes the whole frame (marquee row + separator + prompt line + current input buffer) into one `std::string`, writes it with **one** `fwrite`/`WriteConsole` call, then sleeps. |
| Input (poller) | fixed short interval, e.g. 10–20 ms (the *polling rate*) | Checks `kbhit()`. On a key: append/backspace to the line buffer. On Enter: push the completed line to a thread-safe queue and clear the buffer. |
| Main | — | Pops lines from the queue, runs the interpreter, mutates `MarqueeState`, prints command feedback in a fixed "log" region. Sets `quit` on `exit` and joins the other two. |

**Non-negotiable design rules**

- **Only the renderer writes to stdout.** The input thread updates the buffer; the renderer draws it. Two writers = tearing and interleaved output.
- **Render the entire frame as one string, one write, no `endl`/flush per line.** Use cursor-home (`\x1b[H`) + line-clear (`\x1b[K`) per line instead of full clear (`\x1b[2J`) every frame — full clears flicker badly on Windows.
- Hide the cursor while animating; restore it and raw mode on exit (also on Ctrl+C — install a handler).
- `text`, `offset`, and the input buffer are guarded by one `std::mutex`; `speed_ms`, `running`, and `quit` are `std::atomic`. Copy shared data out under the lock, then render/format *outside* the lock.
- `set_speed` clamps to a sane range (suggested `[10, 10000]` ms; document the chosen bounds in `help`). Reject 0/negative.
- Marquee scroll: circular shift over `text + padding`; offset wraps with `%`. Handle `text` shorter than the terminal width by padding with spaces so the wrap-around point is visible.
- Behavior must be tunable **at runtime without recompiling** (the quiz forbids recompiling). Commands cover text and speed; also read an optional `config.txt` at startup (`text`, `speed_ms`, `poll_ms`, `width`) so poll rate can be changed for the tearing/latency experiments without a rebuild.

---

## 5. The core lesson: refresh rate vs. polling rate

This is the part the PPT must argue with numbers, and the part graders probe. Build the instrumentation early.

- **Refresh rate** = how often the marquee thread redraws (`speed_ms`). Too fast → CPU burn, visible tearing/flicker on slow terminals, and it starves the input echo.
- **Polling rate** = how often the input thread checks the keyboard (`poll_ms`). Too slow → typed characters appear late or, with a busy renderer, seem dropped. Too fast → wasted CPU for no perceptible gain.
- Add a hidden/debug command or a `--stats` flag that reports actual measured frame time and input latency (timestamp on key detected vs. timestamp when it is first drawn). Use these to find:
  1. **Recommended values for the group's hardware** (e.g. refresh 50–100 ms, poll 10–20 ms — measure, don't guess).
  2. **The limits**: the refresh interval below which tearing/flicker appears, and the poll interval above which typing delay becomes noticeable (~100 ms is usually where humans notice).
- Record the machine specs (CPU, OS, terminal app) alongside the numbers — the PPT asks for values "for your current hardware."

---

## 6. Submission package (prepare in advance — not at the last minute)

1. **SOURCE** — this repo (a GitHub link is accepted). Must include `README.txt` with: member names, how to build/run, and the entry file (`src/main.cpp`). Keep `README.txt` (the spec says `.txt`) even if a `README.md` also exists.
2. **PPT** — technical report covering, in this order:
   - Command recognition (tokenizer, exact-match table)
   - Console UI implementation (frame composition, ANSI, single-write rendering)
   - Command interpreter implementation (parse → validate → execute, error messages)
   - Process representation (how the marquee and the input poller are represented as "processes"/threads sharing state)
   - Scheduler implementation (how the OS schedules the threads; what the program controls — sleep intervals — versus what it doesn't)
   - Refresh-rate vs. polling-rate balance: recommended values + measured limits (screen tearing, typing delay)
   - **The demo MP4 embedded directly in the PPTX** (file size must reflect it).
3. **Video** — one seamless, uncut take, 480p–720p, ≤ 1 GB. Must show pressing Run/Debug in the IDE, then the full test sequence. No touching code once running. IDE font large enough to read.

Late submissions are not accepted. No exceptions.

---

## 7. Conventions for contributors and for Claude

- Small, reviewable commits; one concern per commit. Conventional-ish messages: `feat: set_speed clamping`, `fix: restore terminal on Ctrl+C`.
- Every group member should be able to run the demo and explain any file. When adding a non-obvious technique (ANSI sequence, raw mode, atomics), leave a short comment saying *why*, not what.
- Do not paste code from other CSOPESY repos or tutorials — the academic-honesty policy applies even though LLM use is allowed for this output.
- Before claiming a feature is done, run the manual test script in §3C on Windows Terminal **and** legacy `cmd.exe`/PowerShell host; they render escape sequences differently.
- Keep MO3 self-contained, but structure `interpreter/` and `platform/` so they can be lifted into the MO1 emulator (that project adds `initialize`, `screen -s/-r/-ls`, `scheduler-start/stop`, `report-util`, and a `config.txt`). Don't pre-build MO1 features here.
- When in doubt about a behavior the spec leaves open, pick the option that mirrors a real Linux/PowerShell shell (the spec names those as the reference) and note the choice in `README.txt`.

## 8. Things Claude should NOT do in this repo

- Don't introduce a dependency (curses, FTXUI, Boost, fmt) — see §2.
- Don't change command names or add required arguments to them.
- Don't write to stdout from more than one thread.
- Don't use `system("cls")`/`system("clear")` — slow, flickers, and spawns a process every frame.
- Don't busy-wait; every loop sleeps on its interval.
- Don't silently swallow bad input — print a message and keep running.
- Don't generate the PPT or the video; those are the group's job. You may draft outlines and measurement tables for them.
