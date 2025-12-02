#pragma once

#include <string>
#include <vector>

using namespace std;

// Match Descriptor for Find/Replace
struct Match {
    int line;
    int start;
    int len;
};

// Find all occurrences (non-overlapping) of `q` in `lines`
vector<Match> find_all(const vector<string>& lines, const string& q);

// Compute prefix width used for rendering line numbers
int compute_prefix_width(bool showLineNumbers, int totalLines);
