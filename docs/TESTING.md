# Manual test script (demo video and black-box checks)

Run the program from the IDE (the video must show Run/Debug being pressed) in a
console window of at least 100 columns by 28 rows; the Windows default of
120 by 30 is ideal. `ascii_big.txt` and `config.txt` must be in the working
directory (the Visual Studio project folder) or next to the executable. Type each line and check the
result. The same sequence is what the automated console harness checks.

| # | Type | Expect |
|---|---|---|
| 1 | *(nothing)* | The welcome header in the specification's layout (`Welcome to CSOPESY!`, `Group developer:` with the four members, `Version date: 2026-09-21`), then one `Config: text "...", refresh ... ms, polling ... ms` line matching `config.txt`, a blank line, then `Command> `. Nothing else on screen. |
| 2 | `hel` (no Enter) | The characters appear after the prompt. Backspace removes one; arrow and function keys do nothing. |
| 3 | `p` + Enter | `Command> help` followed by the six help lines, word for word as in the sample, then a blank line and a new prompt. |
| 4 | `set_text Operating Systems are fun!` | `Text saved for marquee: Operating Systems are fun!` The screen now matches the sample transcript line for line, apart from the names and date in the header. |
| 5 | `start_marquee` | `Marquee animation started.` The text scrolls left in the big ASCII font in the bottom eight rows of the window while the transcript above stays put. |
| 6 | `hel` (no Enter), wait, `p` + Enter | Typing echoes normally while the marquee keeps moving; `help` output appears and the transcript scrolls inside its region, never over the marquee. |
| 7 | `set_speed 50` | `Marquee speed set to 50 milliseconds.` and the marquee visibly speeds up at once. |
| 8 | `set_speed abc`, `set_speed 0`, `set_speed -5`, `set_speed`, `set_speed 10x` | Each prints `Error: set_speed requires a whole number of milliseconds greater than zero. Usage: set_speed <milliseconds>` and the speed is unchanged. |
| 9 | `set_speed 007` | Accepted as 7 ms (`Marquee speed set to 7 milliseconds.`). |
| 10 | `set_speed 5000` | The marquee moves one column every 5 s; typing still echoes instantly. |
| 11 | `set_speed 100` | Back to normal. |
| 12 | `set_text Hello` | `Text saved for marquee: Hello`; the marquee switches to the new text on its next frame. |
| 13 | `set_text` and `set_text    ` | `Error: set_text requires text. Usage: set_text <your_string>` |
| 14 | `stop_marquee` | `Marquee animation stopped.` and the marquee rows are cleared. |
| 15 | `start_marquee` | `Marquee animation started.` and the animation resumes immediately. |
| 16 | `start_marquee now` | Same as `start_marquee` (text after the command is ignored). |
| 17 | `foo bar` | `Unrecognized command: foo. Type 'help' to see the available commands.` |
| 18 | `HELP` | Unrecognized (commands are case-sensitive, like the sample). |
| 19 | Enter on an empty line | Just a new prompt. |
| 20 | `   help   ` | Works; surrounding spaces are ignored. |
| 21 | A line of 150 characters | The prompt shows only the tail of the line; after Enter the whole line is processed (an unrecognized-command message quoting all of it). |
| 22 | `stats` | Six measurement lines (`Refresh`, `Frames`, `Draw`, `Polling`, `Typing`, `Terminal`). Not part of the spec; see MEASUREMENTS.md. |
| 23 | `exit` | `Terminating console...`; the marquee rows are cleared, the window scrolls normally again and the shell prompt returns. |
| 24 | Run again, press Ctrl+C | The program ends and the window scrolls normally again. |

Run the sequence in the console the video will use. The program has been
verified in the classic Windows console host (the window Visual Studio opens)
and in Windows Terminal.

## config.txt checks

`config.txt` is the only file the quiz allows to be edited, so each test case
starts here: edit it, save it, press Run/Debug again, and read the `Config:`
line under the header before typing anything.

| # | Edit `config.txt` to | Expect |
|---|---|---|
| 25 | `refresh-rate 500`, `polling-rate 50` | Header reports `refresh 500 ms, polling 50 ms`; after `start_marquee` the text moves one column every half second, and `stats` reports a measured interval near 500 ms. |
| 26 | `marquee-text CSOPESY MO3` | Header reports `text "CSOPESY MO3"`; `start_marquee` scrolls that text without any `set_text`. |
| 27 | `marquee-text "  Hello, World!  "` | The quotes are stripped and the spaces inside them are kept. |
| 28 | `refresh_rate 250` (underscore), a `# comment` line, a blank line | All accepted: underscores read as hyphens, comments and blank lines ignored. |
| 29 | `refresh-rate abc`, `polling-rate 0` | One note per bad value (`... is not a whole number of milliseconds ...`), and the header still reports the defaults `refresh 100 ms, polling 10 ms`. |
| 30 | `bogus-key 5` | `Config: unknown setting 'bogus-key' ignored.` and everything else is unaffected. |
| 31 | Rename `config.txt` away | `Config: config.txt was not found, using the defaults.` and the console runs normally at 100 ms / 10 ms. |
| 32 | Any values, then `set_speed 50` / `set_text Hi` at run time | The typed commands override the file for that run; the file is untouched and applies again on the next run. |

## Missing font check

Copy the executable alone into an empty folder and run it from there: the
header and prompt are unchanged, and `start_marquee` prints
`Warning: ascii_big.txt was not found, so the marquee shows plain text.` with
the text scrolling as a single plain row.
