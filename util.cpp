#include "util.h"

using namespace std;

/**
 * Find all occurrences (non-overlapping) of `q` in `lines`
 * 
 * @param lines The text buffer to search
 * @param q The query string to find
 * @return A vector of Match structures representing all found occurrences
 */
vector<Match> find_all(const vector<string>& lines, const string& q) {
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
int compute_prefix_width(bool showLineNumbers, int totalLines) {
    if (!showLineNumbers) return 0;
    int digits = 1;
    int tmp = max(1, totalLines);
    while (tmp >= 10) { digits++; tmp /= 10; }
    return digits + 2;
}
