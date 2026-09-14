CSOPESY - Semi-Major Output 1: Marquee Console (MO3)
=====================================================

Course     : CSOPESY - Introduction to Operating Systems, DLSU, Term 1 AY 2026-2027
Instructor : Gregory Cu
Repository : https://github.com/Enzo-user/CSOPESY_MO3

Group members
-------------
  <Group member 1>   <ID number>   <section>
  <Group member 2>   <ID number>   <section>
  <Group member 3>   <ID number>   <section>
  <Group member 4>   <ID number>   <section>
(Also update the names in src/banner.hpp - they are printed in the console header.)

Entry point
-----------
  src/main.cpp  - contains main(). It loads config.txt, starts the renderer and
                  input threads, and runs the command interpreter loop.

Requirements
------------
  * A C++17 compiler: Visual Studio 2022 (MSVC), GCC 9+, or Clang 10+
  * CMake 3.16 or newer
  * No third-party libraries (standard library + Win32/POSIX console APIs only)

Build and run - command line
----------------------------
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build --config Release

  Windows (MSVC):   .\build\Release\csopesy_mo3.exe
  Linux / macOS:    ./build/csopesy_mo3

  Run it from the repository root so config.txt is picked up (the program also
  searches up to three parent folders, so build\Release works as well).

Build and run - IDE
-------------------
  Visual Studio 2022 : File > Open > Folder..., pick the repository root. VS
                       detects CMakeLists.txt. Choose the "csopesy_mo3.exe"
                       target and press Run/Debug (F5). The working directory
                       is preset to the repository root.
  CLion              : Open the repository folder; select the csopesy_mo3
                       configuration and press Run.
  VS Code            : Install the "CMake Tools" extension, open the folder,
                       select a kit, then "CMake: Run Without Debugging".

  The console must run in a real terminal window (Windows Terminal, PowerShell,
  cmd.exe, or a Linux/macOS terminal). IDE "output" panes that are not
  terminals cannot render the animation.

Commands
--------
  help                 Show the list of commands.
  start_marquee        Start (resume) the marquee animation.
  stop_marquee         Stop (pause) the marquee; the text stays frozen on screen.
  set_text <text>      Replace the marquee text. Everything after the command,
                       spaces included, becomes the text (e.g.
                       set_text hello world there).
  set_speed <ms>       Set the marquee refresh interval in milliseconds.
                       Accepted range: 1-10000. 0, negatives and non-numbers
                       are rejected with a message; values above 10000 are
                       clamped to 10000.
  exit                 Terminate the console (also Ctrl+C).

  Diagnostics (extra, not part of the spec; used for the refresh/polling report):
  stats [reset]        Measured refresh interval, draw time, polling interval
                       and typing delay. Measurements restart whenever
                       set_speed or set_poll changes.
  set_poll <ms>        Set the keyboard polling interval (1-1000 ms).

config.txt (optional, read at startup)
--------------------------------------
  text      <initial marquee text>
  speed_ms  <refresh interval, 1-10000>
  poll_ms   <keyboard polling interval, 1-1000>
  width     <marquee width in columns, 0 = follow the terminal>
  Invalid lines are reported in the console log and the default is kept. You can
  also pass an explicit file:  csopesy_mo3 --config path\to\config.txt

Design notes / behaviours the spec leaves open
----------------------------------------------
  * Three threads share one state object: the renderer (draws one frame every
    speed_ms - the refresh rate), the input poller (checks the keyboard every
    poll_ms - the polling rate) and the main thread (command interpreter).
  * Only the renderer writes to the terminal, and it writes each frame as one
    string with a single write call (cursor-home + per-line clears, no full
    screen clear per frame), which is what keeps the animation tear-free.
  * A keystroke or a command result triggers an immediate redraw, so typing
    stays responsive even when the marquee refreshes only every 10 seconds.
  * Command names are matched case-insensitively (PowerShell style); the
    argument of set_text keeps its case. A set_text argument wrapped in
    matching quotes has the quotes removed.
  * Extra arguments to commands that take none are ignored with a note.
  * The text scrolls to the left inside one row; text longer than the terminal
    never wraps. A gap of spaces separates repetitions so the wrap-around is
    visible for short texts.
  * Only printable ASCII is accepted from the keyboard; arrow keys and other
    special keys are ignored. Backspace edits the line.
  * The terminal is restored (cooked mode, cursor visible) on exit, Ctrl+C and
    SIGTERM. Resizing the window is handled on the next frame.

Tests
-----
  ctest --test-dir build --output-on-failure     (interpreter unit tests)
  docs/TESTING.md lists the manual test script used for the demo video.
  docs/MEASUREMENTS.md describes the refresh-rate vs. polling-rate experiment.
