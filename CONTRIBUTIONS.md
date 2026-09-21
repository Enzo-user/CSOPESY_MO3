# Workload Contribution

**Project:** CSOPESY — Marquee Project: Command Line Interface Exercise
**Version date:** 2026-09-21

The following is the group's agreed division of work. The work was split into
four equal shares of 25%, one per member.

| Member | Focus area | Share |
|---|---|---|
| Eleydo, Renzel Vince | Console UI and program scaffolding | 25% |
| Cordero, Ramuel Sean | Command interpreter and input parsing | 25% |
| Obcena, Hans Gabriel | Required command implementations | 25% |
| Suerte, Lorenzo Enrique | Marquee commands, error handling and edge cases | 25% |

## Eleydo, Renzel Vince — Console UI and program scaffolding

- Welcome header: the `Welcome to CSOPESY!` banner and the dummy group developer
  list and version date, matching the specification's sample output.
- Overall program structure of `main.cpp`: includes, helper function layout and
  the `main()` entry point.
- The screen layout: the console transcript scrolls inside an ANSI scrolling
  region exactly like the sample output, while the marquee owns the bottom eight
  rows of the window.

## Cordero, Ramuel Sean — Command interpreter and input parsing

- Reading a full line of user input so text containing spaces survives intact.
- Trimming leading and trailing whitespace, including carriage returns, so input
  is handled safely regardless of line-ending style.
- Splitting each line into a command token and the argument text that follows it,
  which is what allows `set_text <your_string>` to read its text from the same
  line.
- Dispatching the parsed command through `std::string` comparisons.

## Obcena, Hans Gabriel — Required command implementations

- `help` — the full six-command listing and descriptions, reproduced to match the
  specification's sample output exactly.
- `set_text` — saving the parsed text in memory as the marquee text and printing
  the `Text saved for marquee:` confirmation.
- `exit` — printing `Terminating console...`, stopping the animation thread and
  terminating the console loop.

## Suerte, Lorenzo Enrique — Marquee commands, error handling and edge cases

- `start_marquee`, `stop_marquee` and `set_speed`: wiring the commands to the
  marquee (starting and stopping the animation, clearing the marquee rows,
  validating and applying the refresh interval) and their confirmation messages.
- Unrecognized command handling: printing an error message and returning the user
  to the `Command>` prompt.
- Edge cases: `set_text` with no text supplied, `set_speed` with a missing or
  invalid value, empty input lines, and text typed after commands that take no
  argument.

## Marquee animation prototype

The marquee engine that `main.cpp` is built around was contributed by a group
member as a standalone prototype, `marquee.cpp`, together with the `ascii_big.txt`
font. The prototype is preserved in the repository history. It provided:

- The 8-row ASCII font file and the loader that reads one glyph per printable
  character and crops each glyph to its real width.
- The animation thread that stitches the glyphs of the saved text and redraws
  the marquee rows of the console every refresh interval.
- The keyboard-polling input loop (`_kbhit` / `_getch`) that lets the user type
  commands while the marquee keeps animating.

Prototype contributor: _(to be filled in by the group)_

## Shared responsibilities

Held jointly by all four members:

- Reviewing the final source file together.
- Testing the console against the specification's reference output.
- Writing and maintaining the project documentation.
- Repository setup and version control.
