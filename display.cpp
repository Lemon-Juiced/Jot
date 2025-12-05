#include "display.h"
#include <iostream>

using namespace std;

/**
 * Render the text buffer to the console.
 * 
 * @param lines The text buffer to render
 * @param row The current cursor row
 * @param col The current cursor column
 * @param filename The name of the file being edited
 * @param unixMode Whether the editor is in Unix mode
 * @param showLineNumbers Whether line numbers are shown
 * @param showGuide Whether the column guide is shown
 * @param guideCol The column number of the guide
 * @param reservePromptLines Number of prompt lines to reserve between header and text
 */
void render(const vector<string>& lines, int row, int col, const string& filename, bool unixMode, bool showLineNumbers, bool showGuide, int guideCol, int reservePromptLines) {
    // Simple clear + print
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD home = {0,0};
    DWORD written;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hOut, &csbi);
    int width = csbi.dwSize.X;
    int height = csbi.dwSize.Y;

    // Fill with spaces
    DWORD cells = csbi.dwSize.X * csbi.dwSize.Y;
    FillConsoleOutputCharacter(hOut, ' ', cells, home, &written);
    FillConsoleOutputAttribute(hOut, csbi.wAttributes, cells, home, &written);
    SetConsoleCursorPosition(hOut, home);

    if (g_showTitle) {
        cout << "Jot - " << (filename.empty() ? "untitled" : filename) << "\n";
    }
    if (g_showInfo) {
        if (unixMode) {
            cout << "Ctrl+K Copy Line  Ctrl+V Paste  Ctrl+D Duplicate  Ctrl+Z Undo  Ctrl+F Find  Ctrl+R Replace Ctrl+X Delete Line";
        } else {
            cout << "Ctrl+C Copy Line  Ctrl+V Paste  Ctrl+D Duplicate  Ctrl+Z Undo  Ctrl+F Find  Ctrl+R Replace Ctrl+X Delete Line";
        }
        if (showLineNumbers) cout << "  (Line numbers on)";
        if (showGuide) cout << "  (Guide at col " << guideCol << ")";
        cout << "\n";
    }

    // If requested, reserve N prompt lines between header and text (print blank lines)
    if (reservePromptLines > 0) {
        for (int r = 0; r < reservePromptLines; ++r) cout << "\n";
    }

        int headerLines = (g_showTitle ? 1 : 0) + (g_showInfo ? 1 : 0);
    if (reservePromptLines > 0) headerLines += reservePromptLines;
    int maxLines = height - headerLines - 1; // reserve one bottom line
    if (maxLines < 1) maxLines = 1;
    int start = 0;
    if (row >= maxLines) start = row - maxLines + 1;

    // Compute prefix width for line numbers
    int prefixWidth = 0;
    int totalLines = (int)lines.size();
    if (showLineNumbers) {
        int digits = 1;
        int tmp = max(1, totalLines);
        while (tmp >= 10) { digits++; tmp /= 10; }
        // Number + Dot + space
        prefixWidth = digits + 2; // e.g. " 10. " -> digits + ". " (Print as "<num>. ")
    }

    for (int i = 0; i < maxLines && (start + i) < (int)lines.size(); ++i) {
        string ln = lines[start + i];
        int avail = width - prefixWidth;
        if (avail < 0) avail = 0;
        if ((int)ln.size() > avail) ln = ln.substr(0, avail);
        if (showLineNumbers) {
            int lineNo = start + i + 1;
            string num = to_string(lineNo);
            int digits = (int)to_string(totalLines).size();
            // Right Align
            for (int s = 0; s < digits - (int)num.size(); ++s) cout << ' ';
            cout << num << ". ";
        }
        cout << ln << "\n";
    }

    // Draw guideline (by changing cell attributes) if requested
    if (showGuide) {
        // Set a subtle background intensity
        WORD guideAttr = csbi.wAttributes | BACKGROUND_INTENSITY;
        COORD pos;
        for (int i = 0; i < maxLines && (start + i) < (int)lines.size(); ++i) {
            int screenX = prefixWidth + guideCol;
            if (screenX >= 0 && screenX < width) {
                pos.X = (SHORT)screenX;
                pos.Y = (SHORT)(i + headerLines);
                FillConsoleOutputAttribute(hOut, guideAttr, 1, pos, &written);
            }
        }
    }

    // Position cursor (Account for line number prefix)
    COORD cursorPos;
    cursorPos.X = (SHORT)(prefixWidth + col);
    cursorPos.Y = (SHORT)(row - start + headerLines);
    SetConsoleCursorPosition(hOut, cursorPos);
}

/** 
 * Clear the console screen 
 */
void clear_console() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;
    DWORD cells = csbi.dwSize.X * csbi.dwSize.Y;
    COORD home = {0,0};
    DWORD written = 0;
    FillConsoleOutputCharacter(hOut, ' ', cells, home, &written);
    FillConsoleOutputAttribute(hOut, csbi.wAttributes, cells, home, &written);
    SetConsoleCursorPosition(hOut, home);
}

/**
 * Overlay highlight for matches that are visible in the current viewport
 * 
 * @param matches The vector of Match structures to highlight
 * @param lines The text buffer
 * @param curRow The current cursor row
 * @param showLineNumbers Whether line numbers are shown
 * @param headerOffset Additional header lines offset (e.g. for reserved prompt lines)
 * @param selectedIndex The index of the currently selected match (-1 if none)
 */
void highlight_matches_overlay(const vector<Match>& matches, const vector<string>& lines, int curRow, bool showLineNumbers, int headerOffset, int selectedIndex) {
    if (matches.empty()) return;
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;
    int width = csbi.dwSize.X;
    int height = csbi.dwSize.Y;
        int headerLines = (g_showTitle ? 1 : 0) + (g_showInfo ? 1 : 0);
    headerLines += headerOffset;
    int maxLines = height - headerLines - 1;
    if (maxLines < 1) maxLines = 1;
    int start = 0;
    if (curRow >= maxLines) start = curRow - maxLines + 1;

    int prefixWidth = compute_prefix_width(showLineNumbers, (int)lines.size());

    // Yellow-ish background highlight for normal matches
    WORD highlightAttr = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY;
    // Red-ish background for the selected match
    WORD selectedAttr = BACKGROUND_RED | BACKGROUND_INTENSITY;
    DWORD written = 0;
    for (int idx = 0; idx < (int)matches.size(); ++idx) {
        const Match &m = matches[idx];
        if (m.line < start || m.line >= start + maxLines) continue;
        int screenX = prefixWidth + m.start;
        if (screenX < 0 || screenX >= width) continue;
        COORD pos; pos.X = (SHORT)screenX; pos.Y = (SHORT)(m.line - start + headerLines);
        WORD attr = (idx == selectedIndex) ? selectedAttr : highlightAttr;
        FillConsoleOutputAttribute(hOut, attr, (DWORD)max(0, min((int)m.len, width - screenX)), pos, &written);
    }
}

/**
 * Draw prompt at header area and return the coordinate where user input should start
 * 
 * @param promptText The prompt text to display
 * @return The COORD position where user input should start
 */
COORD draw_prompt(const string &promptText) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return COORD{0,0};
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return COORD{0,0};
        int headerLines = (g_showTitle ? 1 : 0) + (g_showInfo ? 1 : 0);
    COORD p; p.X = 0; p.Y = (SHORT)headerLines;
    SetConsoleCursorPosition(hOut, p);
    // Write Prompt and Clear Rest of Line
    cout << promptText;
    // Clear Trailing Area to End of Line
    DWORD written = 0;
    COORD after = p; after.X = (SHORT)promptText.size();
    FillConsoleOutputCharacter(hOut, ' ', csbi.dwSize.X - after.X, after, &written);
    SetConsoleCursorPosition(hOut, after);
    return after;
}

// Adjust the console font size by delta (positive to increase, negative to decrease)
void change_font_size(int delta) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;

    // CONSOLE_FONT_INFOEX is supported on modern Windows. Use it to get/set font size.
    CONSOLE_FONT_INFOEX cfi;
    ZeroMemory(&cfi, sizeof(cfi));
    cfi.cbSize = sizeof(cfi);
    if (!GetCurrentConsoleFontEx(hOut, FALSE, &cfi)) return;

    // Adjust font height (Y). Keep X proportional or leave as-is.
    SHORT newY = (SHORT)max(4, (int)cfi.dwFontSize.Y + delta);
    // Optional clamp to reasonable max
    if (newY > 200) newY = 200;

    cfi.dwFontSize.Y = newY;
    // Apply new font size
    SetCurrentConsoleFontEx(hOut, FALSE, &cfi);
}
