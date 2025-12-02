#include "editor.h"
#include "fileio.h"
#include "undo.h"
#include "input.h"
#include <conio.h>

using namespace std;

/**
 * Run the main editor loop. Parameters are passed by reference so the caller can observe final cursor/clipboard state if desired.
 * 
 * @param lines The text buffer being edited
 * @param row The current cursor row (updated during editing)
 * @param col The current cursor column (updated during editing)
 * @param filename The name of the file being edited
 * @param unixMode Whether the editor is in Unix mode
 * @param showLineNumbers Whether line numbers are shown
 * @param showGuide Whether the column guide is shown
 * @param guideCol The column number of the guide
 * @param clipboard The clipboard string for copy/paste operations
 */
void run_editor(vector<string>& lines, int& row, int& col, string& filename, bool& unixMode, bool& showLineNumbers, bool& showGuide, int& guideCol, string& clipboard) {
    // Initial render should have been called by main.
    while (true) {
        int c = _getch();

        if (c == 0 || c == 224) {
            int s = _getch();
            // Arrow Keys
            if (s == 72) { // Up
                if (row > 0) {
                    row--; if (col > (int)lines[row].size()) col = (int)lines[row].size();
                }
            } else if (s == 80) { // Down
                if (row + 1 < (int)lines.size()) {
                    row++; if (col > (int)lines[row].size()) col = (int)lines[row].size();
                }
            } else if (s == 75) { // Left
                if (col > 0) col--; else if (row > 0) { row--; col = (int)lines[row].size(); }
            } else if (s == 77) { // Right
                if (col < (int)lines[row].size()) col++; else if (row + 1 < (int)lines.size()) { row++; col = 0; }
            }
            render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol, false);
            continue;
        }

        // Control keys
        if (c == 19) { // Ctrl+S Save
            save_file(filename.empty() ? string("untitled.txt") : filename, lines);
            render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol, false);
            continue;
        }

        if (c == 6) { // Ctrl+F Find
            find_mode(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol);
            render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol, false);
            continue;
        }

        if (c == 18) { // Ctrl+R Replace
            replace_mode(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol);
            render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol, false);
            continue;
        }

        // Copy (Mode Dependent): Default Ctrl+C, Unix mode uses Ctrl+K
        if (!unixMode && c == 3) { // Ctrl+C Copy current line
            clipboard = lines[row];
            render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol, false);
            continue;
        }
        if (unixMode && c == 11) { // Ctrl+K Copy current line in Unix mode
            clipboard = lines[row];
            render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol, false);
            continue;
        }

        if (c == 22) { // Ctrl+V Paste
            push_undo(lines, row, col);
            // Paste clipboard at cursor position (Insert, do not overwrite)
            if (row >= 0 && row < (int)lines.size()) {
                lines[row].insert(col, clipboard);
                col += (int)clipboard.size();
            } else {
                // If Somehow Empty, Create a New Line
                lines.insert(lines.begin() + row + 1, clipboard);
                row = min(row + 1, (int)lines.size() - 1);
                col = (int)clipboard.size();
            }
            render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol, false);
            continue;
        }

        if (c == 4) { // Ctrl+D Duplicate current line
            push_undo(lines, row, col);
            lines.insert(lines.begin() + row + 1, lines[row]);
            row = row + 1; col = (int)lines[row].size();
            render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol, false);
            continue;
        }
        if (c == 26) { // Ctrl+Z Undo
            if (do_undo(lines, row, col)) render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol, false);
            continue;
        }

        if (c == 13) { // Enter
            push_undo(lines, row, col);
            string cur = lines[row];
            string left = cur.substr(0, col);
            string right = cur.substr(col);
            lines[row] = left;
            lines.insert(lines.begin() + row + 1, right);
            row++; col = 0;
            render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol, false);
            continue;
        }

        if (c == 8) { // Backspace
            if (col > 0) {
                push_undo(lines, row, col);
                lines[row].erase(col - 1, 1);
                col--;
            } else if (row > 0) {
                push_undo(lines, row, col);
                int prevLen = (int)lines[row-1].size();
                lines[row-1] += lines[row];
                lines.erase(lines.begin() + row);
                row--; col = prevLen;
            }
            render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol, false);
            continue;
        }

        // Printable Characters
        if (c >= 32 && c <= 126) {
            push_undo(lines, row, col);
            lines[row].insert(lines[row].begin() + col, (char)c);
            col++;
            render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol, false);
            continue;
        }

        // ESC - Quit
        if (c == 27) {
            break;
        }
    }
}
