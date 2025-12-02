#include <algorithm>
#include <conio.h>
#include <fstream>
#include <iostream>
#include <stack>
#include <string>
#include <vector>
#include <windows.h>

using namespace std;

// Global Settings
volatile bool g_ignoreCtrlC = true;
bool g_showTitle = true;
bool g_showHelp = true;

/**
 * Snapshot structure for undo functionality
 * Stores the state of the text buffer and cursor position at a given time.
 */
struct Snapshot {
	vector<string> lines;
	int row, col;
};

static stack<Snapshot> undoStack;
static const size_t UNDO_LIMIT = 200;

/**
 * Push the current state onto the undo stack.
 * 
 * @param lines Current text buffer
 * @param row Current cursor row
 * @param col Current cursor column
 */
void push_undo(const vector<string>& lines, int row, int col) {
	Snapshot s{lines, row, col};
	undoStack.push(s);
	while (undoStack.size() > UNDO_LIMIT) undoStack.pop();
}

/**
 * Perform an undo operation by restoring the last snapshot from the undo stack.
 * 
 * @param lines Reference to the text buffer to restore
 * @param row Reference to the cursor row to restore
 * @param col Reference to the cursor column to restore
 * @return true if undo was successful, false if there was nothing to undo
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
 * Save the text buffer to a file.
 * 
 * @param filename The name of the file to save to
 * @param lines The text buffer to save
 */
void save_file(const string& filename, const vector<string>& lines) {
	ofstream ofs(filename);
	if (!ofs) return;
	for (size_t i = 0; i < lines.size(); ++i) {
		ofs << lines[i];
		if (i + 1 < lines.size()) ofs << '\n';
	}
}

/**
 * Render the text buffer to the console.
 * 
 * @param lines The text buffer to render
 * @param row The current cursor row
 * @param col The current cursor column
 * @param filename The name of the file being edited
 * @param unixMode Whether the editor is in Unix mode
 */ 
void render(const vector<string>& lines, int row, int col, const string& filename, bool unixMode, bool showLineNumbers = false, bool showGuide = false, int guideCol = 90) {
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
	if (g_showHelp) {
		if (unixMode) {
			cout << "Ctrl+K Copy Line  Ctrl+V Paste  Ctrl+D Duplicate  Ctrl+Z Undo";
		} else {
			cout << "Ctrl+C Copy Line  Ctrl+V Paste  Ctrl+D Duplicate  Ctrl+Z Undo";
		}
		if (showLineNumbers) cout << "  (Line numbers on)";
		if (showGuide) cout << "  (Guide at col " << guideCol << ")";
		cout << "\n";
	}

	int headerLines = (g_showTitle ? 1 : 0) + (g_showHelp ? 1 : 0);
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
 * Backwards-compatible wrapper for older calls that use the 4-argument render.
 * 
 * @param lines The text buffer to render
 * @param row The current cursor row
 * @param col The current cursor column
 * @param filename The name of the file being edited
 */
inline void render(const vector<string>& lines, int row, int col, const string& filename) {
	// Preserve new defaults: unixMode=false, line numbers and guide ON
	render(lines, row, col, filename, false, true, true, 90);
}

/**
 * Main function - entry point of the program.
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

	// Install Ctrl Handler to avoid process termination on Ctrl+C
	SetConsoleCtrlHandler(ConsoleHandler, TRUE);

	// Make Cursor Visible
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO cci;
	GetConsoleCursorInfo(hOut, &cci);
	cci.bVisible = TRUE;
	SetConsoleCursorInfo(hOut, &cci);

	int row = 0, col = 0;
	string clipboard;

	bool unixMode = false;
	// Defaults: line numbers and guide ON by default
	bool showLineNumbers = true;
	bool showGuide = true;
	int guideCol = 90;

	// Parse args: accept combined short flags like -thu and -g with optional value
	for (int i = 1; i < argc; ++i) {
		string a = argv[i];
		if (a.size() >= 2 && a[0] == '-') {
			// handle -g=val or -gval
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

			// Iterate short flags: e.g. -thu
			for (size_t j = 1; j < a.size(); ++j) {
				char f = a[j];
				switch (f) {
					case 'u': unixMode = true; break;
					case 'n': showLineNumbers = true; break;
					case 'h': g_showHelp = false; break; // hide help line
					case 't': g_showTitle = false; break; // hide title
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

	// Set Ctrl-C handling according to mode (Unix-like: let Ctrl+C behave normally)
	g_ignoreCtrlC = !unixMode;

	// If the user provided a filename, attempt to open and load it now
	if (!filename.empty()) {
		ifstream ifs(filename);
		if (ifs) {
			lines.clear();
			string l;
			while (getline(ifs, l)) lines.push_back(l);
			if (lines.empty()) lines.push_back("");
		}
	}

	// Initial render with selected options
	render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol);

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
			render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol);
			continue;
		}

		// Control keys
		if (c == 19) { // Ctrl+S Save
			save_file(filename.empty() ? string("untitled.txt") : filename, lines);
			render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol);
			continue;
		}

		// Copy (Mode Dependent): Default Ctrl+C, Unix mode uses Ctrl+K
		if (!unixMode && c == 3) { // Ctrl+C Copy current line
			clipboard = lines[row];
			render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol);
			continue;
		}
		if (unixMode && c == 11) { // Ctrl+K Copy current line in Unix mode
			clipboard = lines[row];
			render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol);
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
			render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol);
			continue;
		}

		if (c == 4) { // Ctrl+D Duplicate current line
			push_undo(lines, row, col);
			lines.insert(lines.begin() + row + 1, lines[row]);
			row = row + 1; col = (int)lines[row].size();
			render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol);
			continue;
		}
		if (c == 26) { // Ctrl+Z Undo
			if (do_undo(lines, row, col)) render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol);
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
			render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol);
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
			render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol);
			continue;
		}

		// Printable Characters
		if (c >= 32 && c <= 126) {
			push_undo(lines, row, col);
			lines[row].insert(lines[row].begin() + col, (char)c);
			col++;
			render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol);
			continue;
		}

		// ESC - Quit
		if (c == 27) {
			break;
		}
	}

	// Exit Text
	cout << "\nExit. (Ctrl+S to save before quitting next time)\n";
	return 0;
}

