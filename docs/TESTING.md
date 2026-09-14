# Manual test script (demo video and black-box checks)

Run from the IDE (the video must show Run/Debug being pressed). Terminal at least
80×24; 100×30 or larger shows the full banner. Type each line and check the result.
The same sequence is automated for POSIX in the developers' pty harness; the
interpreter rules are unit-tested by `ctest`.

| # | Type | Expect |
|---|---|---|
| 1 | *(nothing)* | Banner, marquee scrolling left, log says which `config.txt` was loaded, `Command> _` prompt. |
| 2 | `hel` (no Enter) | Characters appear on the prompt line while the marquee keeps moving. Backspace removes one; arrow keys do nothing. |
| 3 | `help` | Six spec commands plus the two diagnostics, one line each. Marquee still moving. |
| 4 | `bogus` | `Unknown command: bogus. Type 'help' for a list of commands.` |
| 5 | `HELP` | Same as `help` (names are case-insensitive). |
| 6 | `set_text` | `set_text: missing text. Usage: set_text <text>` |
| 7 | `set_text hello world there` | Log confirms 17 characters; marquee now shows `hello world there`. |
| 8 | `set_text "quoted text"` | Text is `quoted text` (quotes stripped). |
| 9 | `set_text` + a 150+ character string | Scrolls inside one row; never wraps or garbles. |
| 10 | `set_speed` | `set_speed: missing argument. Usage: set_speed <milliseconds> (1-10000)` |
| 11 | `set_speed abc`, `set_speed 12.5` | `... is not a whole number of milliseconds (1-10000).` |
| 12 | `set_speed 0`, `set_speed -50` | `set_speed: speed must be at least 1 ms ...` |
| 13 | `set_speed 99999999999999999999` | Clamped: `... above the maximum; using 10000 ms.` then `Marquee speed set to 10000 ms per frame.` |
| 14 | `set_speed 10000` then type `abc` | Marquee moves one column every 10 s; typed characters still appear instantly. |
| 15 | `set_speed 1` then type quickly | Marquee blurs; every typed character still arrives (check with a `set_text` of a long word, then read the marquee). |
| 16 | `set_speed 100` | Back to normal. |
| 17 | `stop_marquee` | `Marquee stopped...`; row frozen; typing still echoes. |
| 18 | `stop_marquee` | `Marquee is already stopped.` |
| 19 | `start_marquee` | `Marquee started.`; row moves again. |
| 20 | `start_marquee` | `Marquee is already running.` |
| 21 | `start_marquee extra` | Runs, plus `Note: 'start_marquee' takes no argument; ignoring 'extra'.` |
| 22 | Resize the window | Frame re-fits on the next redraw; short windows switch to a one-line header. No crash. |
| 23 | `stats` | Measured refresh interval, draw time, polling interval, typing delay. |
| 24 | `exit` | `Exiting...`, then `Console terminated. Goodbye!`; shell prompt and cursor are back to normal. |
| 25 | Run again, press Ctrl+C | Same clean shutdown. |

Run the sequence on Windows Terminal **and** on the legacy conhost (`cmd.exe` /
Windows PowerShell 5 window): they process escape sequences differently.
