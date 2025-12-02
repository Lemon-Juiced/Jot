#pragma once

#include <string>
#include <vector>
#include "display.h"

using namespace std;

// Run the main editor loop. Parameters are passed by reference so the callercan observe final cursor/clipboard state if desired.
void run_editor(vector<string>& lines, int& row, int& col, string& filename, bool& unixMode, bool& showLineNumbers, bool& showGuide, int& guideCol, string& clipboard);
