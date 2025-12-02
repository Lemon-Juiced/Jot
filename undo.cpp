#include "undo.h"

#include <stack>
#include <vector>
#include <string>

using namespace std;

struct Snapshot {
    vector<string> lines;
    int row, col;
};

static stack<Snapshot> undoStack;
static const size_t UNDO_LIMIT = 200;

/**
 * Push the current state onto the undo stack.
 * 
 * @param lines The current text buffer
 * @param row The current cursor row
 * @param col The current cursor column
 */
void push_undo(const vector<string>& lines, int row, int col) {
    Snapshot s{lines, row, col};
    undoStack.push(s);
    while (undoStack.size() > UNDO_LIMIT) undoStack.pop();
}

/**
 * Perform an undo operation, restoring the last snapshot.
 * 
 * @param lines The text buffer to restore
 * @param row The cursor row to restore
 * @param col The cursor column to restore
 * @return True if an undo was performed, false if there was nothing to undo
 */
bool do_undo(vector<string>& lines, int& row, int& col) {
    if (undoStack.empty()) return false;
    Snapshot s = undoStack.top(); undoStack.pop();
    lines = s.lines;
    row = s.row; col = s.col;
    if (row < 0) row = 0;
    if (row >= (int)lines.size()) row = (int)lines.size() - 1;
    if (col < 0) col = 0;
    if (col > (int)lines[row].size()) col = (int)lines[row].size();
    return true;
}
