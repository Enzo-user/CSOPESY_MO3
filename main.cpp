// CSOPESY - Marquee Project: Command Line Interface Exercise
// Main menu console with a simple command interpreter.

#include <iostream>
#include <string>

// Characters treated as whitespace when trimming and splitting input.
// \r is included so input with Windows-style line endings is handled safely.
static const std::string WHITESPACE = " \t\n\r\f\v";

// Removes leading and trailing whitespace from s.
static std::string trim(const std::string &s) {
    const std::string::size_type first = s.find_first_not_of(WHITESPACE);
    if (first == std::string::npos) {
        return "";
    }
    const std::string::size_type last = s.find_last_not_of(WHITESPACE);
    return s.substr(first, last - first + 1);
}

// Prints the welcome header with the group developers and version date.
static void printHeader() {
    std::cout << "Welcome to CSOPESY!\n\n";
    std::cout << "Group developer:\n";
    std::cout << "De La Cruz, Juan\n";
    std::cout << "Santos, Alex\n\n";
    std::cout << "Version date: 2026-09-18\n";
}

// Prints every command and its description.
static void printHelp() {
    std::cout << "help - displays the commands and its description\n";
    std::cout << "start_marquee - starts the marquee \"animation\"\n";
    std::cout << "stop_marquee - stops the marquee \"animation\"\n";
    std::cout << "set_text - accepts a text input and displays it as a marquee\n";
    std::cout << "set_speed - sets the marquee animation refresh in milliseconds\n";
    std::cout << "exit - terminates the console\n";
}

int main() {
    std::string marqueeText; // the text saved in memory by set_text
    std::string line;

    printHeader();

    while (true) {
        std::cout << "\nCommand> ";

        // Stop cleanly if the input stream ends (e.g. Ctrl+D / Ctrl+Z).
        if (!std::getline(std::cin, line)) {
            std::cout << "\nTerminating console...\n";
            break;
        }

        const std::string input = trim(line);
        if (input.empty()) {
            continue; // nothing typed, show the prompt again
        }

        // Split the line into the command and the text that follows it.
        const std::string::size_type separator = input.find_first_of(WHITESPACE);
        const std::string command =
            (separator == std::string::npos) ? input : input.substr(0, separator);
        const std::string argument =
            (separator == std::string::npos) ? "" : trim(input.substr(separator + 1));

        if (command == "help") {
            printHelp();
        } else if (command == "set_text") {
            if (argument.empty()) {
                std::cout << "Error: set_text requires text. Usage: set_text <your_string>\n";
            } else {
                marqueeText = argument; // saved in memory for the marquee
                std::cout << "Text saved for marquee: " << marqueeText << "\n";
            }
        } else if (command == "start_marquee" || command == "stop_marquee" ||
                   command == "set_speed") {
            std::cout << "Not implemented yet (not required for this exercise).\n";
        } else if (command == "exit") {
            std::cout << "Terminating console...\n";
            break;
        } else {
            std::cout << "Unrecognized command: " << command
                      << ". Type 'help' to see the available commands.\n";
        }
    }

    return 0;
}
