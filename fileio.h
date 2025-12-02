#pragma once

#include <string>
#include <vector>

using std::string;
using std::vector;
using namespace std;

void save_file(const string& filename, const vector<string>& lines);
bool load_file(const string& filename, vector<string>& lines);