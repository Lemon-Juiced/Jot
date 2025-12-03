#include <algorithm>
#include <conio.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

#include "fileio.h"
#include "undo.h"
#include "util.h"
#include "display.h"
#include "input.h"
#include "editor.h"

using namespace std;

// Global Settings 
volatile bool g_ignoreCtrlC = true;
bool g_showTitle = true;
bool g_showInfo = true;

/**
 * Console control handler to manage Ctrl+C behavior.
 *
 * @param signal The control signal received
 * @return TRUE to ignore the signal, FALSE to allow default handling
 */
BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        // Ignore default termination so Ctrl+C can be used as editor command
        if (g_ignoreCtrlC) {
            return TRUE;
        } else {
            return FALSE;
        }
    }
    return FALSE;
}

/**
 * Main function - entry point of Jot.
 * 
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code
 * @author LemonJuiced
 */
int main(int argc, char** argv) {
    string filename;

    // Initial content (single empty line by default)
    vector<string> lines(1, "");

    // Make Cursor Visible
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cci;
    GetConsoleCursorInfo(hOut, &cci);
    cci.bVisible = TRUE;
    SetConsoleCursorInfo(hOut, &cci);

    int row = 0, col = 0;
    string clipboard;

    // Defaults: unixMode off by default, line numbers and guide ON by default, guide column at 90
    bool unixMode = false;
    bool showLineNumbers = true;
    bool showGuide = true;
    int guideCol = 90;

    // Parse args: accept combined short flags like -itu and -g with optional value

    // PASS 1: Detect immediate-exit special flags (-h, -v) and handle them.
    bool wantHelp = false;
    bool wantVersion = false;
    for (int i = 1; i < argc; ++i) {
        string a = argv[i];
        if (a.empty()) continue;

        // If token is exactly "-g", skip it and its value (next token) entirely
        if (a == "-g") { if (i + 1 < argc) ++i; continue; }

        // If token starts with -g (like -g80 or -g=80), skip this token
        if (a.size() > 1 && a[0] == '-' && a[1] == 'g') continue;

        // Only consider tokens that start with '-'
        if (a.size() >= 2 && a[0] == '-') {
            for (size_t j = 1; j < a.size(); ++j) {
                char ch = a[j];
                if (ch == 'h') wantHelp = true;
                else if (ch == 'v') wantVersion = true;
            }
        }
    }

    if (wantVersion) {
        cout << "Jot Version: " << __DATE__ << "\n";
    }
    if (wantHelp) {
        if(wantVersion) cout << "\n";
        cout << "Jot - Minimal Terminal Text Editor for Windows\n";
        cout << "Usage: jot.exe [-u] [-n] [-g <col>] [-i] [-t] [-h] [-v] [filename]\n\n";
        cout << "Flags:\n";
        cout << "  -g <col> | -g=<col>   Enable vertical guide at column <col> (default 90)\n";
        cout << "  -i                    Show the info/keybindings line\n";
        cout << "  -n                    Enable line numbers\n";
        cout << "  -t                    Hide the title line\n";
        cout << "  -u                    Unix Mode (Ctrl+C acts like SIGINT; copy becomes Ctrl+K)\n";
        cout << "Special Flags:\n";
        cout << "  -h                    Show this help and exit\n";
        cout << "  -v                    Show version (build date) and exit\n";
    }
    if (wantHelp || wantVersion) {
        return 0;
    }

    // PASS 2: Early-effect flags (things that should be applied before other setup).
    // Currently: -u (Unix mode) affects Ctrl-C handling.
    for (int i = 1; i < argc; ++i) {
        string a = argv[i];
        if (a.size() >= 2 && a[0] == '-') {
            for (size_t j = 1; j < a.size(); ++j) {
                char ch = a[j];
                if (ch == 'g') break; // -g consumes rest
                if (ch == 'u') unixMode = true;
            }
        }
    }

    // PASS 3: Full parsing (flags, -g values, filename)
    for (int i = 1; i < argc; ++i) {
        string a = argv[i];
        if (a.size() >= 2 && a[0] == '-') {
            // Handle -g=val or -gval
            if (a.rfind("-g=", 0) == 0) {
                showGuide = true;
                string num = a.substr(3);
                if (!num.empty()) guideCol = stoi(num);
                continue;
            }
            if (a.size() > 2 && a[1] == 'g') {
                // -g90 (no '=')
                showGuide = true;
                string num = a.substr(2);
                if (!num.empty()) guideCol = stoi(num);
                continue;
            }

            // Iterate short flags: e.g. -tiu
            for (size_t j = 1; j < a.size(); ++j) {
                char f = a[j];
                switch (f) {
                    case 'u': unixMode = true; break; // already applied in pass 2 too
                    case 'n': showLineNumbers = true; break;
                    case 'i': g_showInfo = true; break; // Show info line
                    case 't': g_showTitle = false; break; // Hide title
                    case 'g': {
                        // -g Followed By Number in same token? Check Rest
                        string rest = a.substr(j+1);
                        showGuide = true;
                        if (!rest.empty()) {
                            guideCol = stoi(rest);
                            j = a.size(); // Consume Rest
                        } else if (i + 1 < argc) {
                            // Value in next arg
                            guideCol = stoi(argv[++i]);
                        }
                        // Done processing this token
                        j = a.size();
                        break;
                    }
                    default:
                        // Unknown Flag - Ignore
                        break;
                }
            }
            continue;
        }

        // Not an option, treat as filename if not set
        if (filename.empty()) filename = a;
    }

    // Install Ctrl Handler to avoid process termination on Ctrl+C
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    // Set Ctrl-C handling according to mode (Unix-like: let Ctrl+C behave normally)
    g_ignoreCtrlC = !unixMode;

    // If the user provided a filename, attempt to open and load it now
    if (!filename.empty()) {
        load_file(filename, lines);
    }

    // Initial render with selected options
    render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol);

    // Run main editor loop
    run_editor(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol, clipboard);

    // Clear the console so it appears as if `cls` or `clear` was run after exit.
    clear_console();
    return 0;
}
