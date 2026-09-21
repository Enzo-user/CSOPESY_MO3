================================================================================
CSOPESY - Marquee Project: Command Line Interface Exercise
================================================================================

A single-file C++ main menu console with a command interpreter and a live
ASCII-art text marquee, built for the Windows console.


GROUP DEVELOPER
---------------
Obcena, Hans Gabriel
Suerte, Lorenzo Enrique
Cordero, Ramuel Sean
Eleydo, Renzel Vince

Version date: 2026-09-21

Workload contribution is documented in CONTRIBUTIONS.md.


ENTRY POINT
-----------
File:     main.cpp
Function: int main()

The entire program is contained in the single source file main.cpp, located
at the root of this repository. main() is the last function in the file.
Everything above it is helper code, grouped by section: layout constants and
shared state, the ASCII font loader, marquee rendering and the animation
thread, the console text (welcome header and help), the command interpreter,
and the keyboard input loop.

ascii_big.txt is the 8-row ASCII font the marquee is drawn with. It is read
when the program starts, so it must sit either in the working directory or
next to the built executable.


HOW TO RUN THE PROGRAM
----------------------
The program targets the Windows console. It uses <conio.h> for non-blocking
keyboard polling and <windows.h> to switch on ANSI escape sequence support,
and std::thread for the animation, so it needs a C++11 (or newer) Windows
compiler such as MSVC or MinGW-w64 g++. No external libraries or build system
are needed.

  Visual Studio (used for the demo video)
  ---------------------------------------
    1. Create an empty C++ Console App project and add main.cpp to it.
    2. Copy ascii_big.txt into the project folder (the folder containing the
       .vcxproj file). That folder is the working directory Visual Studio
       uses when you press Run/Debug.
    3. Press Run/Debug.

  Developer Command Prompt (MSVC)
  -------------------------------
    cl /EHsc /std:c++17 main.cpp
    main.exe

  MinGW-w64 (g++)
  ---------------
    g++ -std=c++17 -o marquee main.cpp
    marquee.exe

Run the program from the folder that contains ascii_big.txt, or copy the font
next to the executable. If the font cannot be found, the console prints a
warning and the marquee falls back to scrolling the text as a single plain
row.

Use a console window of at least 100 columns by 28 rows (the Windows default
is 120 by 30): the marquee takes the top eight rows, the header and prompt sit
below it, and the "help" output reaches row 28. On a window shorter than 28
rows the blank spacer rows are dropped so everything still fits in 25 rows.
The program needs Windows 10 or later with the standard console, which
understands the ANSI escape sequences it uses.


SCREEN LAYOUT
-------------
The console is drawn with ANSI cursor positioning instead of scrolling, so the
marquee can keep animating while commands are typed:

    Rows 1-8      Marquee animation area (the font is 8 rows tall)
    Rows 10-18    Welcome header: "Welcome to CSOPESY!", "Group developer:"
                  with the four names, "Version date:"
    Row 20        Live "Command>" prompt, redrawn on every keystroke
    Row 22        Echo of the last command entered, e.g. "Command> help"
    Row 23+       Output of the last command

The output area is cleared before each command's output is printed, so the
screen always shows the most recent command and its result. On a window
shorter than 28 rows the blank spacer rows 9, 19 and 21 are dropped: the
header starts on row 9, the prompt is on row 18 and the output starts on
row 19.


COMMANDS
--------
The "help" command lists the six commands of the marquee console:

    help          - displays the commands and its description
    start_marquee - starts the marquee "animation"
    stop_marquee  - stops the marquee "animation"
    set_text      - accepts a text input and displays it as a marquee
    set_speed     - sets the marquee animation refresh in milliseconds
    exit          - terminates the console

    help           Prints the list of commands and their descriptions.

    start_marquee  Starts scrolling the saved text (initially "Hello, World!")
                   across the top eight rows and prints
                   "Marquee animation started."

    stop_marquee   Stops the animation, clears the marquee rows and prints
                   "Marquee animation stopped."

    set_text       Reads the text typed immediately after the command on the
                   same line, saves it in memory as the marquee text, and
                   confirms that it was accepted:

                       Command> set_text Operating Systems are fun!
                       Text saved for marquee: Operating Systems are fun!

                   If the marquee is already running, the new text appears
                   on the next frame.

    set_speed      Sets the marquee refresh interval. The value must be a
                   whole number of milliseconds greater than zero, for
                   example "set_speed 50". The console confirms with
                   "Marquee speed set to 50 milliseconds." or, for anything
                   else, prints a usage error.

    exit           Prints "Terminating console..." and terminates the
                   console.

Any other input is treated as an unrecognized command. The console prints an
error message ("Unrecognized command: <word>. Type 'help' to see the
available commands.") and returns the user to the "Command>" prompt.


REFRESH RATE AND POLLING RATE
-----------------------------
Two intervals drive the console, both defined as constants at the top of
main.cpp:

  - Marquee refresh (DEFAULT_SPEED_MS, 100 ms; changed at run time with
    set_speed). The animation thread redraws the eight marquee rows once per
    interval, shifting the text one column to the left each frame.
  - Keyboard polling (INPUT_POLL_MS, 10 ms). The console thread checks
    _kbhit() every 10 ms and redraws the prompt line whenever a key arrives,
    so typing stays responsive while the marquee animates.

All writes to the console go through a single mutex, so a marquee frame and a
prompt redraw never interleave. A new set_speed value, and start_marquee
after a stop, take effect on the next poll rather than after the previous
interval has run out. Both intervals are implemented as sleeps, so their real
granularity is the Windows timer resolution (about 15.6 ms by default): a
set_speed value below that simply gives the fastest refresh the console
allows, and frames are scheduled from the previous frame's due time so the
average interval matches the requested value. Lower refresh values repaint
the screen more often and can flicker or delay the echo of typed characters
on slower consoles; the values that work best on a given machine, and the
point where tearing or typing delay becomes noticeable, should be measured
with set_speed and recorded in the technical report.


NOTES ON INPUT HANDLING
-----------------------
  - Commands are compared using std::string comparisons and are
    case-sensitive, so "help" is recognized while "HELP" is not.
  - Leading and trailing whitespace around a command is ignored, so
    "  exit  " still terminates the console.
  - For set_text, spacing inside the saved text is preserved exactly;
    only whitespace at the two ends is trimmed.
  - Typing set_text with no text after it prints a short usage message
    instead of saving an empty string.
  - start_marquee, stop_marquee and exit ignore any text typed after them.
  - Pressing Enter on an empty line simply shows the prompt again.
  - Backspace edits the line being typed. Arrow, function and other
    navigation keys are ignored.
  - Input is read by polling the keyboard rather than with std::cin, which
    would block the console thread and cannot be combined with a live
    marquee.
