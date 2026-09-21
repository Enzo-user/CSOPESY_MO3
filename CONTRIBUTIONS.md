# Workload Contribution

**Project:** CSOPESY MO3 — Marquee Console
**Version date:** 2026-09-21

The following is the group's agreed division of work. The work was split into
four equal shares of 25%, one per member.

| Member | Focus area | Share |
|---|---|---|
| Cordero, Ramuel Sean | Marquee engine: ASCII font and animation prototype | 25% |
| Obcena, Hans Gabriel | Console UI and program scaffolding | 25% |
| Suerte, Lorenzo Enrique | Command interpreter and input parsing | 25% |
| Eleydo, Renzel Vince | Command implementations, error handling and edge cases | 25% |

## Cordero, Ramuel Sean — Marquee engine: ASCII font and animation prototype

The marquee engine that `main.cpp` is built around was contributed as the
standalone prototype `marquee.cpp` together with the `ascii_big.txt` font. The
prototype is preserved unchanged in the repository history. It provided:

- The 8-row ASCII font file (`ascii_big.txt`, one glyph per printable character
  from `!` to `~`) and the loader that reads each glyph and crops it to its real
  width plus one column of kerning.
- The animation thread that stitches the glyphs of the saved text side by side
  and redraws the marquee rows every refresh interval, scrolling the text one
  column to the left per frame.
- The keyboard-polling input loop (`_kbhit` / `_getch`) that lets the user keep
  typing commands while the marquee animates.

## Obcena, Hans Gabriel — Console UI and program scaffolding

- Welcome header: the `Welcome to CSOPESY!` banner, the group developer list and
  the version date, in the layout of the specification's sample output.
- Overall program structure of `main.cpp`: includes, helper function layout and
  the `main()` entry point.
- The screen layout: the console transcript scrolls inside an ANSI scrolling
  region exactly like the sample output, while the marquee owns the bottom eight
  rows of the window.

## Suerte, Lorenzo Enrique — Command interpreter and input parsing

- Reading a full line of user input so text containing spaces survives intact.
- Trimming leading and trailing whitespace, including carriage returns, so input
  is handled safely regardless of line-ending style.
- Splitting each line into a command token and the argument text that follows it,
  which is what allows `set_text <your_string>` to read its text from the same
  line.
- Dispatching the parsed command through `std::string` comparisons.

## Eleydo, Renzel Vince — Command implementations, error handling and edge cases

- `help` — the full six-command listing and descriptions, reproduced to match the
  specification's sample output exactly.
- `set_text` — saving the parsed text in memory as the marquee text and printing
  the `Text saved for marquee:` confirmation.
- `start_marquee`, `stop_marquee` and `set_speed`: wiring the commands to the
  marquee engine (starting and stopping the animation, clearing the marquee rows,
  validating and applying the refresh interval) and their confirmation messages.
- `exit` — printing `Terminating console...`, stopping the animation thread and
  terminating the console loop.
- Unrecognized command handling: printing an error message and returning the user
  to the `Command>` prompt.
- Edge cases: `set_text` with no text supplied, `set_speed` with a missing or
  invalid value, empty input lines, and text typed after commands that take no
  argument.

## Shared responsibilities

Held jointly by all four members:

- Reviewing the final source file together.
- Testing the console against the specification's reference output.
- Writing and maintaining the project documentation.
- Repository setup and version control.
