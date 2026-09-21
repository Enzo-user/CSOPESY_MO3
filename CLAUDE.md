# CLAUDE.md — Marquee Project: Command Line Interface Exercise

## Goal

Implement **only** the requirements below. Add nothing that is not listed here: no extra commands, features, files, libraries, or refactors.

## Requirements

Develop a C++ program that acts as the main menu console, using standard C++ output streams (`std::cout`) for display.

1. Display a welcome header that includes dummy text for **"Group developer:"** and **"Version date:"**.
2. Continuously display the prompt `Command>` and wait for user input.
3. Implement a command interpreter that accepts and safely processes the following inputs using `std::string` comparisons:
   - `help` — displays the commands and their descriptions.
   - `set_text <your_string>` — parses the text provided immediately after the command on the same line, saves it in memory, and accepts it as the marquee text.
   - `exit` — terminates the console.
4. If an unrecognized command is entered, print an error message and return the user to the `Command>` prompt.
5. Marquee console (MO3 specification): `start_marquee` starts the marquee "animation", `stop_marquee` stops it, and `set_speed <milliseconds>` sets the animation refresh interval. The marquee scrolls the saved text across the bottom eight rows of the console window while the transcript above keeps scrolling like a normal console.

## Reference output

Match this sample for the header, `help`, `set_text`, and `exit`:

```
Welcome to CSOPESY!

Group developer:
De La Cruz, Juan
Santos, Alex

Version date: 2026-09-18

Command> help
help - displays the commands and its description
start_marquee - starts the marquee "animation"
stop_marquee - stops the marquee "animation"
set_text - accepts a text input and displays it as a marquee
set_speed - sets the marquee animation refresh in milliseconds
exit - terminates the console

Command> set_text Operating Systems are fun!
Text saved for marquee: Operating Systems are fun!

Command> exit
Terminating console...
```

The header prints exactly the dummy names and date shown in the sample. The actual group members and the documentation's version date are listed in `README.md`, `README.txt`, and `CONTRIBUTIONS.md`; keep those three files in sync with each other.

## Scope rules

- All six commands listed by `help` are implemented: `help`, `set_text`, and `exit` as required by the exercise, plus `start_marquee`, `stop_marquee`, and `set_speed` from the marquee console specification. The marquee engine (ASCII font loader, animation thread, keyboard-polling input loop) was integrated from a group member's prototype, `marquee.cpp`.
- The `help` output reproduces the sample text above. `set_text` prints `Text saved for marquee: <text>`; `exit` prints `Terminating console...`.
- Any other input is an unrecognized command: print an error message and return to the `Command>` prompt.
- The program targets the Windows console: `<conio.h>` for non-blocking keyboard polling, `<windows.h>` to enable ANSI escape sequences, and the standard `<thread>`, `<atomic>`, `<mutex>` headers for the animation thread. Output goes through `std::cout`; command comparison uses `std::string`.
- The transcript (header, prompts, commands and their output) scrolls exactly like the sample inside an ANSI scrolling region; the marquee is drawn in the bottom eight rows of the window and the region is reset at exit.
- The marquee font is `ascii_big.txt` (8 rows per printable character, `!` to `~`), loaded at run time from the working directory or the executable's directory. If it is missing, `start_marquee` prints a warning and the text scrolls as a single plain row.
- Keep the program in a single source file, `main.cpp`, with a `main()` function. The only other runtime file is `ascii_big.txt`.
