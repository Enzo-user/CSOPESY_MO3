# CSOPESY MO3 — Marquee Console

Semi-Major Output 1 for CSOPESY (DLSU, Term 1 AY 2026–2027): a single-file C++
main menu console with a command interpreter and a live ASCII-art text marquee,
built for the Windows console.

**[README.txt](README.txt) is the full submission document** — this page is the
short version. Workload contribution is in [CONTRIBUTIONS.md](CONTRIBUTIONS.md).

## Group Developer

- Obcena, Hans Gabriel
- Suerte, Lorenzo Enrique
- Cordero, Ramuel Sean
- Eleydo, Renzel Vince

**Version date:** 2026-09-21

## Entry Point

| | |
|---|---|
| **File** | [`main.cpp`](main.cpp) |
| **Function** | `int main()` (the last function in the file) |

The whole program is that one source file. Its two runtime files are
`ascii_big.txt`, the 8-row ASCII font the marquee is drawn with, and
`config.txt`, the startup settings; both are read at startup from the working
directory or from the folder holding the executable.

## How to Run

Needs a C++11-or-newer Windows compiler (MSVC or MinGW-w64 g++) and Windows 10
or later. No libraries, no build system.

```bat
cl /EHsc /std:c++17 main.cpp        :: MSVC
g++ -std=c++17 -o marquee main.cpp  :: MinGW-w64
```

In Visual Studio: empty **C++ Console App**, add `main.cpp`, copy
`ascii_big.txt` and `config.txt` next to the `.vcxproj`, press Run/Debug. Use a
window of at least 100 × 28 (the default 120 × 30 is ideal): the marquee owns
the bottom eight rows and the transcript scrolls above them.

## Commands

| Command | Description |
|---|---|
| `help` | displays the commands and its description |
| `start_marquee` | starts the marquee "animation" |
| `stop_marquee` | stops the marquee "animation" |
| `set_text <your_string>` | accepts a text input and displays it as a marquee |
| `set_speed <milliseconds>` | sets the marquee animation refresh in milliseconds |
| `exit` | terminates the console |

Anything else prints `Unrecognized command: <word>.` and returns to the prompt.
Two diagnostics for the technical report, `stats` and `set_poll`, are not listed
by `help`; see [docs/MEASUREMENTS.md](docs/MEASUREMENTS.md).

## Configuration (`config.txt`)

Read once at startup, so no rebuild is needed to change a setting — it is the
only file meant to be edited between black-box test cases.

| Setting | Meaning | Default | Run-time equivalent |
|---|---|---|---|
| `marquee-text` | Text the marquee scrolls. Quotes optional, stripped. | `Hello, World!` | `set_text` |
| `refresh-rate` | Marquee refresh in ms (1 or more). | `100` | `set_speed` |
| `polling-rate` | Keyboard polling in ms (1 to 1000). | `10` | `set_poll` |

One `key value` per line, `#` comments, `-` and `_` interchangeable in a key. A
missing file, unknown key or unusable value keeps the default and prints a note.
The settings in force appear under the welcome header:

```
Version date: 2026-09-21
Config: text "Hello, World!", refresh 100 ms, polling 10 ms
```

## Documentation

- [README.txt](README.txt) — full document: screen layout, every command, input handling
- [docs/TESTING.md](docs/TESTING.md) — manual test script for the demo video
- [docs/MEASUREMENTS.md](docs/MEASUREMENTS.md) — refresh vs. polling rate procedure
