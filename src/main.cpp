// CSOPESY MO3 - Marquee Console
//
// Entry point. The program is a main menu console with a command interpreter
// and a live ASCII-art text marquee; MarqueeConsole is the object that runs it,
// and the classes it owns are described in their own headers:
//
//   Config        the settings read from config.txt at startup
//   AsciiFont     the ascii_big.txt glyphs the marquee is drawn with
//   Console       the Windows console window: ANSI support, layout, locked writes
//   Marquee       the scrolling text and the thread that animates it
//   Metrics       what was measured, behind the "stats" diagnostic
//
// Windows only, but only in three places: Console and DataFile use
// <windows.h> for the console API and the executable's path, and
// MarqueeConsole uses <conio.h> for non-blocking keyboard polling. The rest is
// plain standard C++, which is what tests/smoke.cpp exercises.

#include "MarqueeConsole.h"

int main() {
    MarqueeConsole console;
    console.run();
    return 0;
}
