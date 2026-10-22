// .include resolution of the scanner (B-143, F-12): ParserState::resolve_include() looks a relative
// path up next to the including file first, then in the working directory, and warns when both
// exist and differ.
#include <gtest/gtest.h>

#include <dirent.h>
#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include "parser/parser_state.h"
#include "test/utils.h"

namespace {
void write_file(const std::string &path, const std::string &content)
{
    std::ofstream f(path, std::ios::binary);
    f << content;
}

// fresh working directory for the test body (which runs in a forked child), removed at the end
struct TmpWorkingDir {
    std::string dir = testing::TempDir() + "specs_include_" + std::to_string(getpid());
    TmpWorkingDir()
    {
        std::filesystem::remove_all(dir);
        std::filesystem::create_directories(dir + "/d/sub");
        if (chdir(dir.c_str()) != 0)
            ADD_FAILURE() << "cannot enter " << dir;
    }
    ~TmpWorkingDir()
    {
        if (chdir(testing::TempDir().c_str()) == 0)
            std::filesystem::remove_all(dir);
    }
};

size_t open_fd_count()
{
    size_t n = 0;
    DIR *d = opendir("/proc/self/fd");
    if (!d)
        return 0;
    while (readdir(d))
        ++n;
    closedir(d);
    return n;
}

ParserState state_in(const std::string &dir)
{
    ParserState s;
    s.include_dirs.assign(1, dir);
    return s;
}
}

TEST(ParserState, DirOf)
{
    SPECS_TEST_HEADER()
    {
        EXPECT_EQ(ParserState::dir_of("a/b/c.cir"), "a/b");
        EXPECT_EQ(ParserState::dir_of("c.cir"), "");
        EXPECT_EQ(ParserState::dir_of("./c.cir"), ".");
        EXPECT_EQ(ParserState::dir_of("/c.cir"), "/");
    }
    SPECS_TEST_FOOTER();
}

// one candidate: no directory (generated content), ".", an absolute path
TEST(ParserState, ResolveIncludeSingleCandidate)
{
    SPECS_TEST_HEADER()
    {
        TmpWorkingDir tmp;
        write_file("d/x.cir", "a\n");
        EXPECT_EQ(ParserState().resolve_include("x.cir"), "x.cir");
        EXPECT_EQ(state_in("").resolve_include("x.cir"), "x.cir");
        EXPECT_EQ(state_in(".").resolve_include("d/x.cir"), "d/x.cir");
        EXPECT_EQ(state_in("d").resolve_include("/no/such/x.cir"), "/no/such/x.cir");
    }
    SPECS_TEST_FOOTER();
}

// next to the including file first, else as given (relative to the working directory)
TEST(ParserState, ResolveIncludeFileThenWorkingDir)
{
    SPECS_TEST_HEADER()
    {
        TmpWorkingDir tmp;
        write_file("d/only_local.cir", "a\n");
        write_file("only_cwd.cir", "b\n");
        write_file("d/sub/leaf.cir", "c\n");
        EXPECT_EQ(state_in("d").resolve_include("only_local.cir"), "d/only_local.cir");
        EXPECT_EQ(state_in("d").resolve_include("only_cwd.cir"), "only_cwd.cir");
        EXPECT_EQ(state_in("d").resolve_include("sub/leaf.cir"), "d/sub/leaf.cir");
        // neither exists: the name as given, for the "not found" error
        EXPECT_EQ(state_in("d").resolve_include("none.cir"), "none.cir");
        // a directory next to the including file is not a candidate
        write_file("sub", "e\n");
        EXPECT_EQ(state_in("d").resolve_include("sub"), "sub");
        // the innermost buffer decides
        ParserState s = state_in("");
        s.include_dirs.push_back("d");
        EXPECT_EQ(s.resolve_include("only_local.cir"), "d/only_local.cir");
        s.include_dirs.pop_back();
        EXPECT_EQ(s.resolve_include("only_local.cir"), "only_local.cir");
    }
    SPECS_TEST_FOOTER();
}

// both exist: a warning only when they are different files with different contents
TEST(ParserStateDeathTest, ResolveIncludeBothExist)
{
    SPECS_TEST_HEADER()
    {
        TmpWorkingDir tmp;
        const auto ok = testing::ExitedWithCode(0);
        auto resolve = [](const std::string &path) {
            std::cerr << state_in("d").resolve_include(path) << "\n";
            exit(0);
        };

        write_file("d/differ.cir", "a\n");
        write_file("differ.cir", "b\n");
        EXPECT_EXIT(resolve("differ.cir"), ok,
                    "^Warning: .include differ.cir: using d/differ.cir \\(relative to the including file\\); "
                    "differ.cir relative to the working directory is a different file\nd/differ.cir\n$");

        write_file("d/same_bytes.cir", "same\n");
        write_file("same_bytes.cir", "same\n");
        EXPECT_EXIT(resolve("same_bytes.cir"), ok, "^d/same_bytes.cir\n$");

        write_file("linked.cir", "c\n");
        std::filesystem::create_symlink("../linked.cir", "d/linked.cir");
        EXPECT_EXIT(resolve("linked.cir"), ok, "^d/linked.cir\n$");

        // same size, different bytes
        write_file("d/same_size.cir", "ab\n");
        write_file("same_size.cir", "ba\n");
        EXPECT_EXIT(resolve("same_size.cir"), ok, "^Warning: .include same_size.cir: using d/same_size.cir [^\n]*\nd/same_size.cir\n$");
    }
    SPECS_TEST_FOOTER();
}

// existence checks and comparisons leave no file open (B-156)
TEST(ParserState, ResolveIncludeClosesFiles)
{
    SPECS_TEST_HEADER()
    {
        TmpWorkingDir tmp;
        write_file("d/x.cir", "same\n");
        write_file("x.cir", "same\n");
        const size_t n_before = open_fd_count();
        for (int i = 0; i < 100; ++i)
            EXPECT_EQ(state_in("d").resolve_include("x.cir"), "d/x.cir");
        EXPECT_EQ(open_fd_count(), n_before);
    }
    SPECS_TEST_FOOTER();
}
