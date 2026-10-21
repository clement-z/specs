#pragma once

#include <algorithm>
#include <string>
#include <vector>

using std::string;

struct ParserState {
    string current_filename;
    string current_line;
    // Netlist files opened by the scanner (-f files and .include), for the run manifest (F-01).
    // Points to the list of the root ParseTree (ParseTree::root_input_files()); nullptr: not recorded
    std::vector<string> *input_files = nullptr;

    // Record path, as used to open it, unless it was already recorded (first-open order)
    void record_input_file(const string &path)
    {
        if (input_files && std::find(input_files->begin(), input_files->end(), path) == input_files->end())
            input_files->push_back(path);
    }
};
