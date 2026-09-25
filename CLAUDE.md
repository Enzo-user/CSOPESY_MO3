# CLAUDE.md — CSOPESY MO3: Marquee Console

## Goal

Implement **only** the requirements below. Add nothing that is not listed here: no extra commands, features, files, libraries, or refactors. This repository is the MO3 (Semi-Major Output 1, Marquee Console) submission, due 2026-09-23.

## Requirements

Develop a C++ program that acts as the main menu console, using standard C++ output streams (`std::cout`) for display.

1. Display a welcome header with **"Group developer:"** (the four group members) and **"Version date:"**.
2. Continuously display the prompt `Command>` and wait for user input.
3. Implement a command interpreter that accepts and safely processes the following inputs using `std::string` comparisons:
   - `help` — displays the commands and their descriptions.
   - `set_text <your_string>` — parses the text provided immediately after the command on the same line, saves it in memory, and accepts it as the marquee text.
   - `exit` — terminates the console.
4. If an unrecognized command is entered, print an error message and return the user to the `Command>` prompt.
5. Marquee console (MO3 specification): `start_marquee` starts the marquee "animation", `stop_marquee` stops it, and `set_speed <milliseconds>` sets the animation refresh interval. The marquee scrolls the saved text across the bottom eight rows of the console window while the transcript above keeps scrolling like a normal console.

## Reference output

Match this sample's layout for the header and its text exactly for `help`, `set_text`, and `exit` (the sample's names and date are placeholders; the program prints the real ones):

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

The header lists the four group members in the sample's `Last name, First name` format (Obcena, Hans Gabriel; Suerte, Lorenzo Enrique; Cordero, Ramuel Sean; Eleydo, Renzel Vince) and the version date. Keep the names and the date in sync across `main.cpp`, `README.md`, `README.txt`, and `CONTRIBUTIONS.md`.

## Scope rules

- All six commands listed by `help` are implemented: `help`, `set_text`, and `exit` as required by the exercise, plus `start_marquee`, `stop_marquee`, and `set_speed` from the marquee console specification. The marquee engine (ASCII font loader, animation thread, keyboard-polling input loop) was integrated from a group member's prototype, `marquee.cpp`.
- The `help` output reproduces the sample text above. `set_text` prints `Text saved for marquee: <text>`; `exit` prints `Terminating console...`.
- Any other input is an unrecognized command: print an error message and return to the `Command>` prompt.
- The program targets the Windows console: `<conio.h>` for non-blocking keyboard polling, `<windows.h>` to enable ANSI escape sequences, and the standard `<thread>`, `<atomic>`, `<mutex>` headers for the animation thread. Output goes through `std::cout`; command comparison uses `std::string`.
- The transcript (header, prompts, commands and their output) scrolls exactly like the sample inside an ANSI scrolling region; the marquee is drawn in the bottom eight rows of the window and the region is reset at exit.
- The marquee font is `ascii_big.txt` (8 rows per printable character, `!` to `~`), loaded at run time from the working directory or the executable's directory. If it is missing, `start_marquee` prints a warning and the text scrolls as a single plain row.
- Two diagnostic commands, `stats` (with `stats reset`) and `set_poll <milliseconds>`, expose the measured refresh interval, polling sleep and key-to-screen delay for the technical report. They are not listed by `help`; `docs/MEASUREMENTS.md` and `docs/TESTING.md` describe how they are used.
- `config.txt` is read once at startup for `marquee-text`, `refresh-rate` and `polling-rate` (the same three values as `set_text`, `set_speed` and `set_poll`). It is the only file the black-box quiz allows to be modified, so the program must never need a rebuild to change them: `key value` per line, `#` comments, unknown keys and unusable values keep the defaults, and the values in force are printed under the welcome header.
- Keep the program in a single source file, `main.cpp`, with a `main()` function. The only other runtime files are `ascii_big.txt` and `config.txt`.
