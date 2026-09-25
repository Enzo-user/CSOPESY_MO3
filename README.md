# CSOPESY MO3 — Marquee Console

Semi-Major Output 1 for CSOPESY (DLSU, Term 1 AY 2026–2027): a C++ main menu
console with a command interpreter and a live ASCII-art text marquee, built for
the Windows console.

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
| **File** | [`src/main.cpp`](src/main.cpp) |
| **Function** | `int main()` |
| **Class** | [`MarqueeConsole`](src/MarqueeConsole.h) |

`main()` creates one `MarqueeConsole` and calls `run()`. The two runtime files
are `ascii_big.txt`, the 8-row ASCII font the marquee is drawn with, and
`config.txt`, the startup settings; both are read at startup from the working
directory or from the folder holding the executable.

## Program Structure

One class per job, header and source per class:

| Unit | Responsibility |
|---|---|
| [`MarqueeConsole`](src/MarqueeConsole.h) | The application: welcome header, the `Command>` loop, the command interpreter, keyboard polling |
| [`Config`](src/Config.h) | The settings read from `config.txt` at startup |
| [`AsciiFont`](src/AsciiFont.h) | The `ascii_big.txt` glyphs; renders text into 8 rows |
| [`Console`](src/Console.h) | The Windows console: ANSI support, window size, the scrolling region, and the one mutex every write to `std::cout` goes through |
| [`Marquee`](src/Marquee.h) | The scrolling text, the refresh interval and the thread that animates it |
| [`Metrics`](src/Metrics.h) | What was measured, behind the `stats` diagnostic |
| [`DataFile`](src/DataFile.h) | Finds `ascii_big.txt` and `config.txt` |
| [`Text`](src/Text.h) | `trim()` and the millisecond parser |

`MarqueeConsole` owns the parts in the order they must be destroyed: the marquee
thread is joined before the console gives the window its scrolling back. Only
`Console`, `DataFile` and `MarqueeConsole` touch the Windows API, so the rest is
plain standard C++ and runs anywhere:

```sh
g++ -std=c++17 -o smoke tests/smoke.cpp src/Text.cpp src/Config.cpp src/AsciiFont.cpp src/Metrics.cpp
./smoke        # 56 checks passed
```

## How to Run

Needs a C++11-or-newer Windows compiler (MSVC or MinGW-w64 g++) and Windows 10
or later. No libraries, no build system.

```bat
cl /EHsc /std:c++17 src\*.cpp /Fe:marquee.exe  :: MSVC
g++ -std=c++17 -o marquee src/*.cpp            :: MinGW-w64
```

In Visual Studio: empty **C++ Console App**, add every file in `src/`, copy
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

- [README.txt](README.txt) — full document: program structure, screen layout, every command, input handling
- [docs/TESTING.md](docs/TESTING.md) — manual test script for the demo video
- [docs/MEASUREMENTS.md](docs/MEASUREMENTS.md) — refresh vs. polling rate procedure
