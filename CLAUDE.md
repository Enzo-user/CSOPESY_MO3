# CLAUDE.md — Marquee Project: Command Line Interface Exercise

## Goal

Implement **only** the requirements below. Add nothing that is not listed here: no extra commands, features, files, libraries, or refactors.

## Requirements

Develop a C++ program that acts as the main menu console, using standard C++ input/output streams (`std::cin`, `std::cout`).

1. Display a welcome header that includes dummy text for **"Group developer:"** and **"Version date:"**.
2. Continuously display the prompt `Command>` and wait for user input.
3. Implement a command interpreter that accepts and safely processes the following inputs using `std::string` comparisons:
   - `help` — displays the commands and their descriptions.
   - `set_text <your_string>` — parses the text provided immediately after the command on the same line, saves it in memory, and simulates accepting it for a marquee.
   - `exit` — terminates the console.
4. If an unrecognized command is entered, print an error message and return the user to the `Command>` prompt.

## Reference output

Match this sample:

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

## Scope rules

- Only `help`, `set_text`, and `exit` are implemented, as required by the exercise.
- `start_marquee`, `stop_marquee`, and `set_speed` appear only in the sample output, not in the exercise requirements. Listing them in `help` follows the sample; not implementing them follows the requirements. When the user enters any of these three commands (with or without text after it), print `Not implemented yet (not required for this exercise).` and return to the `Command>` prompt.
- Any other input is an unrecognized command: print an error message and return to the `Command>` prompt.
- The `help` output reproduces the sample text above.
- Use only `std::cin`, `std::cout`, and `std::string` from the standard library for input, output, and command comparison.
- Keep the program in a single source file with a `main()` function.
