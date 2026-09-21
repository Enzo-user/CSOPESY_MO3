#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <mutex> 
#include <conio.h>
#include <map>
#include <windows.h>

using namespace std;

mutex console_mtx;

//  Windows ANSI Escape Code Enabler
void enable_virtual_terminal() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    }
#endif
}

void load_ascii(map<char, vector<string>>& font_map) {
    ifstream file("ascii_big.txt");
    string line;
    
    char current_char = 33; 
    
    while (file.good()) {
        vector<string> char_art;
        bool valid_block = false;
        
        for (int i = 0; i < 8; i++) {
            if (getline(file, line)) {
                // Remove hidden Windows carriage returns just in case
                if (!line.empty() && line.back() == '\r') line.pop_back();
                char_art.push_back(line);
                valid_block = true;
            }
        }
        
        if (valid_block && char_art.size() == 8) {
            // DYNAMIC WIDTH DETECTOR 
            int max_width = 0;
            
            // Find the true width of this specific letter
            for (int r = 0; r < 8; r++) {
                int last_char_pos = char_art[r].find_last_not_of(" ");
                if (last_char_pos != string::npos && last_char_pos + 1 > max_width) {
                    max_width = last_char_pos + 1;
                }
            }
            
            // Crop the useless empty space and add exactly 1 space for kerning
            for (int r = 0; r < 8; r++) {
                if (max_width > 0 && max_width < char_art[r].length()) {
                    char_art[r] = char_art[r].substr(0, max_width) + " "; 
                } else if (max_width == 0) {
                     char_art[r] = " "; // Failsafe for completely empty blocks
                }
            }
            
            font_map[current_char] = char_art;
            current_char++; 
        }
    }
    
    // Manually define the spacebar character to be a sensible width (e.g., 5 spaces)
    font_map[' '] = vector<string>(8, "     "); 
}

void intromessage(){
    vector<string> devs = {"The Archfiend, Nulgath"};
    
    cout << "\033[2J\033[1;1H";
    
    for(int i = 0; i < 9; i++) cout << "\n"; 
    
    cout << "Welcome to CSOPESY!\n\n";

    if (devs.size() == 1){
        cout << "Developer:\n";
        cout << devs[0] << "\n";
    } else if (devs.size() > 1){
        cout << "Developers:\n";
        for (string dev : devs) 
            cout << dev << "\n";
    }

    cout << "Version Date: v2026.9.15\n";
}

void help(){
    cout << "Available commands:" << endl;
    cout << "  help                       - Show this help message" << endl;
    cout << "  start_marquee              - Starts the marquee animation" << endl;
    cout << "  stop_marquee               - Stops the marquee animation" << endl;
    cout << "  set_text <display_text>    - Sets the text for the marquee" << endl;
    cout << "  set_speed <milliseconds>   - Sets the speed of the marquee" << endl;
    cout << "  exit                       - Terminates the console" << endl;
}

void start_marquee(int& animationtrigger){
    animationtrigger = 1;
}

void stop_marquee(int& animationtrigger){
    animationtrigger = 0;

    cout << "\033[s"; 
    for(int i = 0; i < 8; i++) {
        cout << "\033[" << (i + 1) << ";1H\033[2K"; 
    }
    cout << "\033[u";
    cout.flush();
}

void set_text(string& display, string new_text){
    display = new_text + " ";
}

void set_speed(int& milliseconds, int new_speed){
    milliseconds = new_speed;
}

void trigger_exit(int& exittrigger, bool& program_running){
    exittrigger = 1;
    program_running = false;
}

void marquee(int& animationtrigger, int& milliseconds, bool& program_running, map<char, vector<string>>& font_map, string& display){
    int offset = 0;
    while (program_running) {
        if (animationtrigger == 1 && !display.empty() && !font_map.empty()){
            
            vector<string> stitched_word(8, ""); 
            
            for (char c : display) {
                if (font_map.count(c)) {
                    for (int row = 0; row < 8; row++) {
                        stitched_word[row] += font_map[c][row]; 
                    }
                } else {
                    for (int row = 0; row < 8; row++) {
                        stitched_word[row] += font_map[' '][row]; 
                    }
                }
            }
            
            console_mtx.lock(); 
            cout << "\033[s"; 
            
            for (int row = 0; row < 8; ++row) { 
                cout << "\033[" << (row + 1) << ";1H\033[2K"; 
                
                // 2. RESTORED: Bulletproof string wrapping logic
                string pattern = stitched_word[row] + "   ";
                string repeated = pattern;
                
                while (repeated.length() < pattern.length() + 100) {
                    repeated += pattern; 
                }
                
                int start = offset % pattern.length();
                cout << repeated.substr(start, 99); 
            }
            
            cout << "\033[u"; 
            cout.flush();
            console_mtx.unlock(); 
            
            offset++;
        }
        this_thread::sleep_for(chrono::milliseconds(milliseconds));
    }
}

int main(){
    enable_virtual_terminal(); // Required for Windows terminal UI!

    int animationtrigger = 0;
    int exittrigger = 0;
    bool program_running = true;
    string command;
    string input_line;
    string display = "Hello, World! ";
    int milliseconds = 100;
    
    map<char, vector<string>> font_map;
    load_ascii(font_map);
    
    thread animation(marquee, ref(animationtrigger), ref(milliseconds), ref(program_running), ref(font_map), ref(display));

    intromessage();

    do {
        console_mtx.lock(); 
        cout << "\033[15;1H\033[2KCommand> ";
        cout.flush();
        console_mtx.unlock(); 
        
        input_line = "";

        while (true) {
            if (_kbhit()) { 
                char c = _getch(); 
                
                if (c == '\r') { 
                    break;
                }
                else if (c == '\b') { 
                    if (!input_line.empty()) {
                        input_line.pop_back();
                    }
                }
                else if (c >= 32 && c <= 126) { 
                    input_line += c;
                }

                console_mtx.lock();
                cout << "\033[15;1H\033[2KCommand> " << input_line;
                cout.flush();
                console_mtx.unlock();
            }
            this_thread::sleep_for(chrono::milliseconds(10)); 
        }
        
        if (input_line.empty()) continue;
        
        stringstream ss(input_line);
        ss >> command;

        console_mtx.lock(); 
        cout << "\033[17;1H\033[0J";

        if (command == "help")
            help();
        else if (command == "start_marquee")
            start_marquee(animationtrigger);
        else if (command == "stop_marquee") 
            stop_marquee(animationtrigger);
        else if (command == "set_text"){
            string new_text;
            getline(ss >> ws, new_text); 
            
            if (!new_text.empty())
                set_text(display, new_text);
            else
                cout << "Error: Missing text. Usage: set_text <display_text>\n";
        }
        else if (command == "set_speed"){
            int new_speed;
            if (ss >> new_speed)
                set_speed(milliseconds, new_speed);
            else
                cout << "Error: Invalid speed. Usage: set_speed <milliseconds>\n";
        }
        else if (command == "exit")    
            trigger_exit(exittrigger, program_running);
        else
            cout << "Unknown command.\n";
            
        console_mtx.unlock(); 
            
    } while (exittrigger != 1);

    animation.join();
    
    return 0;
}