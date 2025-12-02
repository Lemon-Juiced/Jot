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
void render(const vector<string>& lines, int row, int col, const string& filename, bool unixMode, bool showLineNumbers = false, bool showGuide = false, int guideCol = 90, int reservePromptLines = 0) {
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
			cout << "Ctrl+K Copy Line  Ctrl+V Paste  Ctrl+D Duplicate  Ctrl+Z Undo  Ctrl+F Find  Ctrl+R Replace";
		} else {
			cout << "Ctrl+C Copy Line  Ctrl+V Paste  Ctrl+D Duplicate  Ctrl+Z Undo  Ctrl+F Find  Ctrl+R Replace";
		}
		if (showLineNumbers) cout << "  (Line numbers on)";
		if (showGuide) cout << "  (Guide at col " << guideCol << ")";
		cout << "\n";
	}

	// If requested, reserve N prompt lines between header and text (print blank lines)
	if (reservePromptLines > 0) {
		for (int r = 0; r < reservePromptLines; ++r) cout << "\n";
	}

	int headerLines = (g_showTitle ? 1 : 0) + (g_showHelp ? 1 : 0);
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
 * Clear the console screen (Windows) and reset cursor to home.
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

// Match Descriptor for Find/Replace
struct Match {
	int line;
	int start;
	int len;
};

/**
 * Find all occurrences (non-overlapping) of `q` in `lines`
 * 
 * @param lines The text buffer to search
 * @param q The query string to find
 * @return A vector of Match structures representing all found occurrences
 */
static vector<Match> find_all(const vector<string>& lines, const string& q) {
	vector<Match> out;
	if (q.empty()) return out;
	for (int i = 0; i < (int)lines.size(); ++i) {
		const string &ln = lines[i];
		size_t pos = 0;
		while (pos < ln.size()) {
			size_t f = ln.find(q, pos);
			if (f == string::npos) break;
			out.push_back(Match{i, (int)f, (int)q.size()});
			pos = f + (q.empty() ? 1 : q.size());
		}
	}
	return out;
}

/**
 * Compute prefix width used for rendering line numbers
 * 
 * @param showLineNumbers Whether line numbers are shown
 * @param totalLines Total number of lines in the text buffer
 * @return The width in characters of the line number prefix
 */
static int compute_prefix_width(bool showLineNumbers, int totalLines) {
	if (!showLineNumbers) return 0;
	int digits = 1;
	int tmp = max(1, totalLines);
	while (tmp >= 10) { digits++; tmp /= 10; }
	return digits + 2;
}

/**
 * Overlay highlight for matches that are visible in the current viewport
 * 
 * @param matches The vector of Match structures to highlight
 * @param lines The text buffer
 * @param curRow The current cursor row
 * @param showLineNumbers Whether line numbers are shown
 */
static void highlight_matches_overlay(const vector<Match>& matches, const vector<string>& lines, int curRow, bool showLineNumbers, int headerOffset = 0, int selectedIndex = -1) {
	if (matches.empty()) return;
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE) return;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;
	int width = csbi.dwSize.X;
	int height = csbi.dwSize.Y;
	int headerLines = (g_showTitle ? 1 : 0) + (g_showHelp ? 1 : 0);
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
 */
static COORD draw_prompt(const string &promptText) {
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE) return COORD{0,0};
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return COORD{0,0};
	int headerLines = (g_showTitle ? 1 : 0) + (g_showHelp ? 1 : 0);
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

/**
 * Input a single-line string with basic editing: printable chars, backspace, ESC to cancel, Enter to accept.
 * While typing, the caller can re-render highlights by calling external functions. Returns true if accepted, false if cancelled.
 * 
 * @param out Reference to string to store the input
 * @param startCoord The coordinate where input should start
 * @return true if input was accepted (Enter), false if cancelled (ESC)
 */
static bool input_line(string &out, const COORD &startCoord) {
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
 * Find-mode UI invoked by Ctrl+F. 
 * Highlights matches and allows arrow navigation through matches.
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
static void find_mode(vector<string> &lines, int &row, int &col, const string &filename, bool unixMode, bool showLineNumbers, bool showGuide, int guideCol) {
	string query;
	vector<Match> matches;
	int sel = -1;
	while (true) {
		// Reserve one prompt line between header and text for Find
		int baseHeaderLines = (g_showTitle ? 1 : 0) + (g_showHelp ? 1 : 0);
		int reserveLines = 1;
		// Render Current Editor State (reserve one prompt line)
		render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol, reserveLines);
		// Draw Find Prompt directly below the header section
		int headerLines = baseHeaderLines;
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(hOut, &csbi);
		COORD promptStart = {0, (SHORT)headerLines};
		SetConsoleCursorPosition(hOut, promptStart);
		cout << "Find: " << query;
		// Clear Rest of Prompt Line
		DWORD written=0; COORD after = promptStart; after.X = (SHORT)(6 + query.size());
		FillConsoleOutputCharacter(hOut, ' ', csbi.dwSize.X - after.X, after, &written);

		// Compute Matches and Highlight (account for reserved prompt line)
		matches = find_all(lines, query);
		// Auto-select first match so the selected (red) highlight appears immediately
		if (!matches.empty() && sel < 0) sel = 0;
		if (matches.empty()) sel = -1;
		highlight_matches_overlay(matches, lines, row, showLineNumbers, reserveLines, sel);

		// Place cursor at end of prompt for further typing
		COORD inputPos = { (SHORT)(6 + query.size()), (SHORT)headerLines };
		SetConsoleCursorPosition(hOut, inputPos);

		// Wait for keystroke for prompt: allow typing, arrows for navigation, ESC to exit, Enter to accept and leave cursor at selected
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
			// Re-render to clear prompts/highlights (no longer reserving)
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
			// Reset selection
			sel = -1;
			continue;
		}
	}
}

/**
 * Replace-mode UI invoked by Ctrl+R. Prompts Find then Replace and allows per-match replace with Enter.
 * 
 * @param lines The text buffer to search and modify
 * @param row The current cursor row (updated on replacement)
 * @param col The current cursor column (updated on replacement)
 * @param filename The name of the file being edited
 * @param unixMode Whether the editor is in Unix mode
 * @param showLineNumbers Whether line numbers are shown
 * @param showGuide Whether the column guide is shown
 * @param guideCol The column number of the guide
 */
static void replace_mode(vector<string> &lines, int &row, int &col, const string &filename, bool unixMode, bool showLineNumbers, bool showGuide, int guideCol) {
	string query;
	string repl;
	vector<Match> matches;
	int sel = -1;

	// First: Get the query (simple inline input similar to find_mode but finish with Enter)
	while (true) {
		int baseHeaderLines = (g_showTitle ? 1 : 0) + (g_showHelp ? 1 : 0);
		int reserveLines = 2; // reserve two lines for Find+Replace prompts while entering the query for replace
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
		// Auto-select first match when available
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

	// If no query, Abort
	if (query.empty()) return;

	// Now prompt for replacement text
	repl.clear();
	sel = -1;
	matches = find_all(lines, query);
	while (true) {
		int baseHeaderLines = (g_showTitle ? 1 : 0) + (g_showHelp ? 1 : 0);
		int reserveLines = 2; // reserve two lines for Find + Replace
		render(lines, row, col, filename, unixMode, showLineNumbers, showGuide, guideCol, reserveLines);
		int headerLines = baseHeaderLines;
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(hOut, &csbi);
		// Draw both prompts
		COORD findPos = {0, (SHORT)headerLines};
		SetConsoleCursorPosition(hOut, findPos);
		cout << "Find: " << query;
		COORD replPos = {0, (SHORT)(headerLines + 1)};
		SetConsoleCursorPosition(hOut, replPos);
		cout << "Replace: " << repl;
		DWORD written=0; COORD after = replPos; after.X = (SHORT)(9 + repl.size());
		FillConsoleOutputCharacter(hOut, ' ', csbi.dwSize.X - after.X, after, &written);

		// Highlight Matches (account for reserved prompt lines)
		matches = find_all(lines, query);
		// Auto-select first match so the replacement target is shown in red immediately
		if (!matches.empty() && sel < 0) sel = 0;
		if (matches.empty()) sel = -1;
		highlight_matches_overlay(matches, lines, row, showLineNumbers, reserveLines, sel);

		// Move cursor to end of replace input
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
			// Confirm replace at current selection (if any)
			if (!matches.empty()) {
				if (sel < 0) sel = 0;
				Match m = matches[sel];
				push_undo(lines, row, col);
				string &ln = lines[m.line];
				string left = ln.substr(0, m.start);
				string right = ln.substr(m.start + m.len);
				ln = left + repl + right;
				// Move cursor to end of inserted replacement
				row = m.line;
				col = (int)left.size() + (int)repl.size();
			}
			// Recompute Matches for Continued Operation
			matches = find_all(lines, query);
			sel = -1;
			continue;
		}
		if (ch == 8) { if (!repl.empty()) repl.pop_back(); continue; }
		if (ch >= 32 && ch <= 126) { repl.push_back((char)ch); continue; }
	}
}

/**
 * Backwards-compatible wrapper for older calls that use the 4-argument render.
 * 
 * @param lines The text buffer to render
 * @param row The current cursor row
 * @param col The current cursor column
 * @param filename The name of the file being edited
 */
inline void render(const vector<string>& lines, int row, int col, const string&filename) {
	// Preserve new defaults: unixMode=false, line numbers and guide ON
	render(lines, row, col, filename, false, true, true, 90, 0);
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

	// Clear the console so it appears as if `cls` or `clear` was run after exit.
	clear_console();
	return 0;
}

