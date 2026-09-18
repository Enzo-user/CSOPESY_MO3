# Workload Contribution

**Project:** CSOPESY — Marquee Project: Command Line Interface Exercise
**Version date:** 2026-09-18

The following is the group's agreed division of work for this exercise. The work
was split into four equal shares of 25%, one per member.

| Member | Focus area | Share |
|---|---|---|
| Eleydo, Renzel Vince | Console UI and program scaffolding | 25% |
| Martin, Sean | Command interpreter and input parsing | 25% |
| Obcena, Hans Gabriel | Required command implementations | 25% |
| Suerte, Lorenzo | Out-of-scope commands, error handling and edge cases | 25% |

## Eleydo, Renzel Vince — Console UI and program scaffolding

- Welcome header: the `Welcome to CSOPESY!` banner, the group developer list and
  the version date, matching the layout in the specification's sample output.
- Overall program structure of `main.cpp`: includes, helper function layout and
  the `main()` entry point.
- The console loop that continuously redisplays the `Command>` prompt and keeps
  the blank-line spacing between prompts consistent with the sample output.

## Martin, Sean — Command interpreter and input parsing

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
- `set_text` — saving the parsed text in memory and printing the
  `Text saved for marquee:` confirmation.
- `exit` — printing `Terminating console...` and terminating the console loop.

## Suerte, Lorenzo — Out-of-scope commands, error handling and edge cases

- `start_marquee`, `stop_marquee` and `set_speed`: listed by `help` because they
  appear in the sample output, but reported as not required for this exercise,
  whether or not text follows the command.
- Unrecognized command handling: printing an error message and returning the user
  to the `Command>` prompt.
- Edge cases: `set_text` with no text supplied, empty input lines, and clean
  termination when the input stream ends.

## Shared responsibilities

Held jointly by all four members:

- Reviewing the final source file together.
- Testing the console against the specification's reference output.
- Writing and maintaining the project documentation.
- Repository setup and version control.
