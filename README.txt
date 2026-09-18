================================================================================
CSOPESY - Marquee Project: Command Line Interface Exercise
================================================================================

GROUP DEVELOPER
---------------
Eleydo, Renzel Vince
Martin, Sean
Obcena, Hans Gabriel
Suerte, Lorenzo

Version date: 2026-09-18


ENTRY POINT
-----------
File:     main.cpp
Function: int main()

The entire program is contained in the single source file main.cpp, located
at the root of this repository. The main() function begins on line 40 of
that file and contains the console's command loop. The lines above it are
the small helper functions that trim input and print the header and the
help text.


HOW TO RUN THE PROGRAM
----------------------
The program is standard C++ and uses only <iostream> and <string>. It needs
no external libraries, build system, or configuration files.

  macOS / Linux (g++ or clang++)
  -----------------------------
    g++ -std=c++17 -o marquee main.cpp
    ./marquee

  Windows (Developer Command Prompt / MSVC)
  -----------------------------------------
    cl /EHsc /std:c++17 main.cpp
    main.exe

  Visual Studio / any IDE
  -----------------------
    Add main.cpp to an empty C++ console project, then press Run/Debug.
    No project settings need to be changed.

Once running, the console prints its welcome header and then repeatedly
displays the "Command>" prompt until the user types "exit".


COMMANDS
--------
The "help" command lists all six commands shown in the specification:

    help          - displays the commands and its description
    start_marquee - starts the marquee "animation"
    stop_marquee  - stops the marquee "animation"
    set_text      - accepts a text input and displays it as a marquee
    set_speed     - sets the marquee animation refresh in milliseconds
    exit          - terminates the console

Of these, this exercise requires only three to be implemented:

    help      Prints the list of commands and their descriptions.

    set_text  Reads the text typed immediately after the command on the
              same line, saves it in memory, and confirms that it was
              accepted for the marquee.

                  Command> set_text Operating Systems are fun!
                  Text saved for marquee: Operating Systems are fun!

    exit      Prints "Terminating console..." and terminates the console.

start_marquee, stop_marquee and set_speed are listed by "help" because they
appear in the specification's sample output, but they are outside the scope
of this exercise. Entering any of them reports:

    Not implemented yet (not required for this exercise).

Any other input is treated as an unrecognized command. The console prints an
error message and returns the user to the "Command>" prompt.


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
  - Pressing Enter on an empty line simply shows the prompt again.
  - Reaching the end of the input stream (Ctrl+D on macOS/Linux, Ctrl+Z on
    Windows) terminates the console cleanly.
