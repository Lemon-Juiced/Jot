#pragma once

#include <vector>
#include <string>

using namespace std;

void push_undo(const vector<string>& lines, int row, int col);
bool do_undo(vector<string>& lines, int& row, int& col);