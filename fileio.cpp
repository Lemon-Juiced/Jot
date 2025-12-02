#include "fileio.h"

#include <fstream>

using namespace std;

/**
 * Save the provided lines to `filename`. If the file cannot be written, returns false.
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
 *  Load `filename` into `lines`. Returns true if the file was successfully opened and read.
 * 
 * @param filename The name of the file to load
 * @param lines The text buffer to load into
 */
bool load_file(const string& filename, vector<string>& lines) {
    ifstream ifs(filename);
    if (!ifs) return false;
    vector<string> tmp;
    string l;
    while (getline(ifs, l)) tmp.push_back(l);
    if (tmp.empty()) tmp.push_back("");
    lines = std::move(tmp);
    return true;
}
