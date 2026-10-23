#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

using std::string;

struct ParserState {
    string current_filename;
    string current_line;
    // Netlist files opened by the scanner (-f files and .include), for the run manifest (F-01).
    // Points to the list of the root ParseTree (ParseTree::root_input_files()); nullptr: not recorded
    std::vector<string> *input_files = nullptr;

    // Directory of the file of each buffer on the scanner's buffer stack, for .include (B-143):
    // the bottom one (-f file, generated string, subcircuit body: the file that defines the
    // subcircuit), then one per open .include. "" for content that is not a file.
    std::vector<string> include_dirs;

    // Line of the current buffer where the body of the subcircuit being read starts (the line after
    // ".subckt name ..."), for the parse errors in that body (B-182)
    int subckt_body_line = 1;

    // Record path, as used to open it, unless it was already recorded (first-open order)
    void record_input_file(const string &path)
    {
        if (input_files && std::find(input_files->begin(), input_files->end(), path) == input_files->end())
            input_files->push_back(path);
    }

    string current_include_dir() const
    {
        return include_dirs.empty() ? "" : include_dirs.back();
    }

    // "a/b.cir" -> "a", "b.cir" -> ""
    static string dir_of(const string &path)
    {
        return std::filesystem::path(path).parent_path().string();
    }

    // Path to open for ".include path" in the current buffer (B-143): a relative path is looked up
    // in the directory of the including file first, then as given (relative to the working
    // directory). Warns when both exist and are different files with different contents.
    string resolve_include(const string &path) const
    {
        namespace fs = std::filesystem;
        const string dir = current_include_dir();
        if (fs::path(path).is_absolute() || dir.empty() || dir == ".")
            return path;
        const string local_path = (fs::path(dir) / path).string();
        // stat only: no file descriptor (B-156)
        std::error_code ec;
        if (!fs::exists(local_path, ec) || fs::is_directory(local_path, ec))
            return path;
        if (fs::exists(path, ec) && !fs::equivalent(local_path, path, ec) && !same_contents(local_path, path))
        {
            std::cerr << "Warning: .include " << path << ": using " << local_path
                      << " (relative to the including file); " << path
                      << " relative to the working directory is a different file" << std::endl;
        }
        return local_path;
    }

    // Same bytes (false if one of them cannot be read)
    static bool same_contents(const string &path_a, const string &path_b)
    {
        std::error_code ec_a, ec_b;
        const auto size_a = std::filesystem::file_size(path_a, ec_a);
        const auto size_b = std::filesystem::file_size(path_b, ec_b);
        if (ec_a || ec_b || size_a != size_b)
            return false;
        std::ifstream file_a(path_a, std::ios::binary), file_b(path_b, std::ios::binary);
        if (!file_a || !file_b)
            return false;
        return std::equal(std::istreambuf_iterator<char>(file_a), std::istreambuf_iterator<char>(),
                          std::istreambuf_iterator<char>(file_b), std::istreambuf_iterator<char>());
    }
};
