#pragma once

#include <string>
#include <vector>
#include <windows.h>
#include "util.h"

using namespace std;

// Globals declared in main translation unit; display/input may read them.
extern volatile bool g_ignoreCtrlC;
extern bool g_showTitle;
extern bool g_showHelp;

// Render the text buffer to the console.
void render(const vector<string>& lines, int row, int col, const string& filename, bool unixMode, bool showLineNumbers, bool showGuide, int guideCol, int reservePromptLines = 0);

// Clear the console screen (Windows) and reset cursor to home.
void clear_console();

// Overlay highlight for matches that are visible in the current viewport
void highlight_matches_overlay(const vector<Match>& matches, const vector<string>& lines, int curRow, bool showLineNumbers, int headerOffset = 0, int selectedIndex = -1);

// Draw prompt at header area and return the coordinate where user input should start
COORD draw_prompt(const string &promptText);
