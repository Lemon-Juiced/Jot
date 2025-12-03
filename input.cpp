#include "input.h"
#include <iostream>
#include <conio.h>
#include "undo.h"

using namespace std;

/**
 * Single-line input with basic editing
 * 
 * @param out The string to store the input
 * @param startCoord The starting coordinate for input
 * @return True if input was confirmed (Enter), false if cancelled (ESC)
 */
bool input_line(string &out, const COORD &startCoord) {
    out.clear();
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return false;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return false;
    COORD cur = startCoord;
    SetConsoleCursorPosition(hOut, cur);
    while (true) {
        int ch = _getch();
        if (ch == 0 || ch == 224) {
            int s = _getch();
            // Ignore arrows while editing
            continue;
        }
        if (ch == 27) { // ESC
            return false;
        }
        if (ch == 13) { // Enter
            return true;
        }
        if (ch == 8) { // Backspace
            if (!out.empty()) {
                out.pop_back();
                // Move cursor back and erase
                if (cur.X > 0) cur.X = (SHORT)(cur.X - 1);
                SetConsoleCursorPosition(hOut, cur);
                cout << ' ';
                SetConsoleCursorPosition(hOut, cur);
            }
            continue;
        }
        if (ch >= 32 && ch <= 126) {
            char c = (char)ch;
            out.push_back(c);
            cout << c;
            cur.X = (SHORT)(cur.X + 1);
        }
    }
}

/**
 * Find mode: prompt for a search query, highlight matches, allow navigation, exit on ESC or Enter.
 * 
 * @param lines The text buffer to search
 * @param row The current cursor row (updated on selection)
 * @param col The current cursor column (updated on selection)
 * @param filename The name of the file being edited
 * @param unixMode Whether the editor is in Unix mode
 * @param showLineNumbers Whether line numbers are shown
 * @param showGuide Whether the column guide is shown
 * @param guideCol The column number of the guide
 */
void find_mode(vector<string> &lines, int &row, int &col, const string &filename, bool unixMode, bool showLineNumbers, bool showGuide, int guideCol) {
    string query;
    vector<Match> matches;
    int sel = -1;
    while (true) {
        int baseHeaderLines = (g_showTitle ? 1 : 0) + (g_showInfo ? 1 : 0);
        int reserveLines = 1;
        render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol, reserveLines);
        int headerLines = baseHeaderLines;
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hOut, &csbi);
        COORD promptStart = {0, (SHORT)headerLines};
        SetConsoleCursorPosition(hOut, promptStart);
        cout << "Find: " << query;
        DWORD written=0; COORD after = promptStart; after.X = (SHORT)(6 + query.size());
        FillConsoleOutputCharacter(hOut, ' ', csbi.dwSize.X - after.X, after, &written);

        matches = find_all(lines, query);
        if (!matches.empty() && sel < 0) sel = 0;
        if (matches.empty()) sel = -1;
        highlight_matches_overlay(matches, lines, row, showLineNumbers, reserveLines, sel);

        COORD inputPos = { (SHORT)(6 + query.size()), (SHORT)headerLines };
        SetConsoleCursorPosition(hOut, inputPos);

        int ch = _getch();
        if (ch == 0 || ch == 224) {
            int s = _getch();
            if (s == 72) { // Up arrow -> Prev Match
                if (!matches.empty()) {
                    if (sel <= 0) sel = (int)matches.size() - 1; else sel--;
                    row = matches[sel].line; col = matches[sel].start;
                }
            } else if (s == 80) { // Down -> Next
                if (!matches.empty()) {
                    sel = (sel + 1) % (int)matches.size();
                    row = matches[sel].line; col = matches[sel].start;
                }
            }
            continue;
        }
        if (ch == 27) { // ESC cancel
            render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol, 0);
            break;
        }
        if (ch == 13) { // Enter - Leave Find with cursor at selection (if any)
            if (!matches.empty()) {
                if (sel < 0) sel = 0;
                row = matches[sel].line; col = matches[sel].start;
            }
            render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol, 0);
            break;
        }
        if (ch == 8) { // Backspace while editing query
            if (!query.empty()) query.pop_back();
            continue;
        }
        if (ch >= 32 && ch <= 126) {
            query.push_back((char)ch);
            sel = -1;
            continue;
        }
    }
}

/**
 * Replace mode: prompt for search and replacement strings, highlight matches, allow navigation and replacement.
 * 
 * @param lines The text buffer to edit
 * @param row The current cursor row (updated on selection/replacement)
 * @param col The current cursor column (updated on selection/replacement)
 * @param filename The name of the file being edited
 * @param unixMode Whether the editor is in Unix mode
 * @param showLineNumbers Whether line numbers are shown
 * @param showGuide Whether the column guide is shown
 * @param guideCol The column number of the guide
 */
void replace_mode(vector<string> &lines, int &row, int &col, const string &filename, bool unixMode, bool showLineNumbers, bool showGuide, int guideCol) {
    string query;
    string repl;
    vector<Match> matches;
    int sel = -1;

    while (true) {
        int baseHeaderLines = (g_showTitle ? 1 : 0) + (g_showInfo ? 1 : 0);
        int reserveLines = 2;
        render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol, reserveLines);
        int headerLines = baseHeaderLines;
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hOut, &csbi);
        COORD promptStart = {0, (SHORT)headerLines};
        SetConsoleCursorPosition(hOut, promptStart);
        cout << "Find: " << query;
        DWORD written=0; COORD after = promptStart; after.X = (SHORT)(6 + query.size());
        FillConsoleOutputCharacter(hOut, ' ', csbi.dwSize.X - after.X, after, &written);

        matches = find_all(lines, query);
        if (!matches.empty() && sel < 0) sel = 0;
        if (matches.empty()) sel = -1;
        highlight_matches_overlay(matches, lines, row, showLineNumbers, reserveLines, sel);

        COORD inputPos = { (SHORT)(6 + query.size()), (SHORT)headerLines };
        SetConsoleCursorPosition(hOut, inputPos);

        int ch = _getch();
        if (ch == 0 || ch == 224) {
            int s = _getch();
            if (s == 72) {
                if (!matches.empty()) { sel = (sel <= 0) ? (int)matches.size()-1 : sel-1; row = matches[sel].line; col = matches[sel].start; }
            } else if (s == 80) {
                if (!matches.empty()) { sel = (sel + 1) % (int)matches.size(); row = matches[sel].line; col = matches[sel].start; }
            }
            continue;
        }
        if (ch == 27) { render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol, 0); return; }
        if (ch == 13) { break; }
        if (ch == 8) { if (!query.empty()) query.pop_back(); continue; }
        if (ch >= 32 && ch <= 126) { query.push_back((char)ch); sel = -1; continue; }
    }

    if (query.empty()) return;

    repl.clear();
    sel = -1;
    matches = find_all(lines, query);
    while (true) {
        int baseHeaderLines = (g_showTitle ? 1 : 0) + (g_showInfo ? 1 : 0);
        int reserveLines = 2;
        render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol, reserveLines);
        int headerLines = baseHeaderLines;
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hOut, &csbi);
        COORD findPos = {0, (SHORT)headerLines};
        SetConsoleCursorPosition(hOut, findPos);
        cout << "Find: " << query;
        COORD replPos = {0, (SHORT)(headerLines + 1)};
        SetConsoleCursorPosition(hOut, replPos);
        cout << "Replace: " << repl;
        DWORD written=0; COORD after = replPos; after.X = (SHORT)(9 + repl.size());
        FillConsoleOutputCharacter(hOut, ' ', csbi.dwSize.X - after.X, after, &written);

        matches = find_all(lines, query);
        if (!matches.empty() && sel < 0) sel = 0;
        if (matches.empty()) sel = -1;
        highlight_matches_overlay(matches, lines, row, showLineNumbers, reserveLines, sel);

        COORD inputPos = { (SHORT)(9 + repl.size()), (SHORT)(headerLines + 1) };
        SetConsoleCursorPosition(hOut, inputPos);

        int ch = _getch();
        if (ch == 0 || ch == 224) {
            int s = _getch();
            if (s == 72) { // Up -> Prev Match
                if (!matches.empty()) { sel = (sel <= 0) ? (int)matches.size()-1 : sel-1; row = matches[sel].line; col = matches[sel].start; }
            } else if (s == 80) { // Down -> Next Match
                if (!matches.empty()) { sel = (sel + 1) % (int)matches.size(); row = matches[sel].line; col = matches[sel].start; }
            }
            continue;
        }
        if (ch == 27) { render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol); return; }
        if (ch == 13) {
            if (!matches.empty()) {
                if (sel < 0) sel = 0;
                Match m = matches[sel];
                push_undo(lines, row, col);
                string &ln = lines[m.line];
                string left = ln.substr(0, m.start);
                string right = ln.substr(m.start + m.len);
                ln = left + repl + right;
                row = m.line;
                col = (int)left.size() + (int)repl.size();
            }
            matches = find_all(lines, query);
            sel = -1;
            continue;
        }
        if (ch == 8) { if (!repl.empty()) repl.pop_back(); continue; }
        if (ch >= 32 && ch <= 126) { repl.push_back((char)ch); continue; }
    }
}
