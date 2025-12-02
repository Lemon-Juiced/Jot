#pragma once

#include <string>
#include <vector>
#include <windows.h>
#include "util.h"
#include "display.h"

using namespace std;

// Globals declared in main translation unit; input may read them.
extern volatile bool g_ignoreCtrlC;
extern bool g_showTitle;
extern bool g_showHelp;

// Single-line input with basic editing
bool input_line(string &out, const COORD &startCoord);

// Find and Replace modes
void find_mode(vector<string> &lines, int &row, int &col, const string &filename, bool unixMode, bool showLineNumbers, bool showGuide, int guideCol);
void replace_mode(vector<string> &lines, int &row, int &col, const string &filename, bool unixMode, bool showLineNumbers, bool showGuide, int guideCol);
