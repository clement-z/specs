// Integer literals of the netlist (B-180): the lexer reads them as 64-bit signed integers (they were
// clamped to 2147483647), a literal beyond 9223372036854775807 is a parse error with file:line and the
// limit; Variable stores them in 64 bits and as_integer() range-checks the conversion of a double.
#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

#include "parser/parse_tree.h"
#include "parser/parser_state.h"
#include "test/utils.h"

#include "parser.h"
#include "scanner.h"

namespace {
const int64_t int64_max = std::numeric_limits<int64_t>::max();

// Parses one netlist line (as specs --param does) into a scratch tree; returns yyparse()'s result
int parse_line(ParseTree &tree, const std::string &line)
{
    ParserState state;
    state.current_filename = "test.cir";
    yyscan_t scanner;
    yylex_init_extra(&state, &scanner);
    YY_BUFFER_STATE buf = yy_scan_string(line.c_str(), scanner);
    yy_switch_to_buffer(buf, scanner);
    yyset_lineno(1, scanner);
    const int result = yyparse(scanner, &tree);
    yylex_destroy(scanner);
    return result;
}
}

TEST(IntegerLiterals, VariableStores64Bits)
{
    SPECS_TEST_HEADER()
    {
        const Variable v(int64_max);
        EXPECT_EQ(v.type, Variable::INTEGER);
        EXPECT_EQ(v.inum, int64_max);
        EXPECT_EQ(v.as_integer(), int64_max);
        EXPECT_EQ(v.get_str(), "9223372036854775807");
        EXPECT_EQ(v.to_json(), "{\"type\":\"INTEGER\",\"value\":9223372036854775807}");
        // a copy keeps every bit
        const Variable copy = v;
        EXPECT_EQ(copy.inum, int64_max);

        // an int is still an INTEGER (not a double or a bool)
        const Variable one(1);
        EXPECT_EQ(one.type, Variable::INTEGER);
        EXPECT_EQ(one.as_integer(), 1);
    }
    SPECS_TEST_FOOTER();
}

// a DOUBLE is truncated toward zero, over the whole 64-bit range
TEST(IntegerLiterals, AsIntegerOfDouble)
{
    SPECS_TEST_HEADER()
    {
        EXPECT_EQ(Variable(3e9).as_integer(), 3000000000);
        EXPECT_EQ(Variable(-2.7).as_integer(), -2);
        EXPECT_EQ(Variable(2.7).as_integer(), 2);
        EXPECT_EQ(Variable(-9223372036854775808.0).as_integer(), std::numeric_limits<int64_t>::min());
        // the largest double below 2^63
        EXPECT_EQ(Variable(std::nextafter(9223372036854775808.0, 0.0)).as_integer(), int64_max - 1023);
    }
    SPECS_TEST_FOOTER();
}

// out of the 64-bit range (or NaN): a clean error, not an undefined cast
TEST(IntegerLiteralsDeathTest, AsIntegerOfDoubleOutOfRange)
{
    SPECS_TEST_HEADER()
    {
        const auto failed = testing::ExitedWithCode(1);
        EXPECT_EXIT(Variable(9223372036854775808.0).as_integer(), failed,
                    "^Variable cannot be converted to an integer value: 9\\.22337e\\+18 \\(outside the 64-bit integer range\\)\n$");
        EXPECT_EXIT(Variable(-1e19).as_integer(), failed,
                    "^Variable cannot be converted to an integer value: -1e\\+19 \\(outside the 64-bit integer range\\)\n$");
        EXPECT_EXIT(Variable(std::nan("")).as_integer(), failed, "outside the 64-bit integer range");
        EXPECT_EXIT(Variable(std::numeric_limits<double>::infinity()).as_integer(), failed,
                    "outside the 64-bit integer range");
    }
    SPECS_TEST_FOOTER();
}

// the lexer reads digit strings as 64-bit integers, exactly
TEST(IntegerLiterals, LexerReads64Bits)
{
    SPECS_TEST_HEADER()
    {
        ParseTree tree;
        ASSERT_EQ(parse_line(tree, ".param a=9223372036854775807 b=3000000000 c=2147483648 d=007\n"), 0);
        ASSERT_EQ(tree.local_assignments.size(), 4u);
        for (const auto &p : tree.local_assignments)
            EXPECT_EQ(p.second.type, Variable::INTEGER) << p.first;
        EXPECT_EQ(tree.local_assignments.at("a").inum, int64_max);
        EXPECT_EQ(tree.local_assignments.at("b").inum, 3000000000);
        EXPECT_EQ(tree.local_assignments.at("c").inum, 2147483648);
        EXPECT_EQ(tree.local_assignments.at("d").inum, 7);
        // an exponent or a scale suffix is still a double
        ParseTree doubles;
        ASSERT_EQ(parse_line(doubles, ".param e=3e9 f=3g\n"), 0);
        EXPECT_EQ(doubles.local_assignments.at("e").type, Variable::DOUBLE);
        EXPECT_EQ(doubles.local_assignments.at("f").type, Variable::DOUBLE);
        EXPECT_EQ(doubles.local_assignments.at("f").as_integer(), 3000000000);
    }
    SPECS_TEST_FOOTER();
}

// a literal beyond 2^63 - 1 is a parse error with file:line and the limit, exit 1
TEST(IntegerLiteralsDeathTest, LexerRejectsBeyond64Bits)
{
    SPECS_TEST_HEADER()
    {
        const auto failed = testing::ExitedWithCode(1);
        auto parse = [](const std::string &line) {
            ParseTree tree;
            parse_line(tree, line);
            exit(0);
        };
        EXPECT_EXIT(parse(".param n=9223372036854775808\n"), failed,
                    "^Parsing error \\(test\\.cir:1\\): integer literal 9223372036854775808 is out of range "
                    "\\(the limit is 9223372036854775807\\)\n"
                    "-- while parsing line: \"\\.param n=9223372036854775808\"\n$");
        EXPECT_EXIT(parse(".param a=1\n.param n=99999999999999999999\n"), failed,
                    "^Parsing error \\(test\\.cir:2\\): integer literal 99999999999999999999 is out of range "
                    "\\(the limit is 9223372036854775807\\)\n"
                    "-- while parsing line: \"\\.param n=99999999999999999999\"\n$");
        // the limit itself parses
        EXPECT_EXIT(parse(".param n=9223372036854775807\n"), testing::ExitedWithCode(0), "^$");
    }
    SPECS_TEST_FOOTER();
}

// numeric net names keep the full range (net 3000000000 was N2147483647)
TEST(IntegerLiterals, NumericNetNames)
{
    SPECS_TEST_HEADER()
    {
        EXPECT_EQ(ParseNet::name_from_id(7), "N00007");
        EXPECT_EQ(ParseNet::name_from_id(3000000000), "N3000000000");
        EXPECT_EQ(ParseNet::name_from_id(int64_max), "N9223372036854775807");
    }
    SPECS_TEST_FOOTER();
}
