// Functions of netlist expressions (F-02): math functions, random functions drawing from per-name
// streams of the master seed (each subcircuit instance on its own, shared=1 / shared="name"), the
// draw records, and .options seed applied when its line is parsed.
#include <gtest/gtest.h>

#include <cmath>
#include <string>

#include "parser/parse_functions.h"
#include "parser/parse_tree.h"
#include "parser/parser_state.h"
#include "test/utils.h"
#include "utils/rng.h"

#include "parser.h"
#include "scanner.h"

namespace {

// Parses netlist lines (each ending with '\n') into tree; returns yyparse()'s result
int parse_netlist(ParseTree &tree, const std::string &text)
{
    ParserState state;
    state.current_filename = "test.cir";
    yyscan_t scanner;
    yylex_init_extra(&state, &scanner);
    YY_BUFFER_STATE buf = yy_scan_string(text.c_str(), scanner);
    yy_switch_to_buffer(buf, scanner);
    yyset_lineno(1, scanner);
    const int result = yyparse(scanner, &tree);
    yylex_destroy(scanner);
    return result;
}

double param(ParseTree &tree, std::string name)
{
    return tree.get_variable(name).as_double();
}

double gauss_of(uint64_t seed, const std::string &stream, double mu, double sigma)
{
    spx::Rng rng = spx::rng_for(seed, stream);
    return mu + sigma * rng.normal();
}

}

TEST(ExpressionFunctions, MathFunctions)
{
    SPECS_TEST_HEADER()
    {
        ParseTree tree;
        ASSERT_EQ(parse_netlist(tree, ".param a=sqrt(16) b=exp(0) c=log(1) d=log10(1000) e=abs(-2.5)\n"
                                      ".param f=floor(2.7) g=ceil(2.2) h=min(3, 1, 2) i=max(3, {a}, 2)\n"
                                      ".param j=SIN(0) k=cos(0) l=2*sqrt(9)+1\n"), 0);
        EXPECT_EQ(param(tree, "a"), 4);
        EXPECT_EQ(param(tree, "b"), 1);
        EXPECT_EQ(param(tree, "c"), 0);
        EXPECT_EQ(param(tree, "d"), std::log10(1000.0));
        EXPECT_EQ(param(tree, "e"), 2.5);
        EXPECT_EQ(param(tree, "f"), 2);
        EXPECT_EQ(param(tree, "g"), 3);
        EXPECT_EQ(param(tree, "h"), 1);
        EXPECT_EQ(param(tree, "i"), 4);
        EXPECT_EQ(param(tree, "j"), 0);
        EXPECT_EQ(param(tree, "k"), 1);
        EXPECT_EQ(param(tree, "l"), 7);
        // math functions do not draw
        EXPECT_TRUE(tree.draws.empty());
        EXPECT_TRUE(tree.drawn_params.empty());
    }
    SPECS_TEST_FOOTER();
}

// each draw takes the stream "<scope><owner>/<key>#<n>" of the master seed
TEST(ExpressionFunctions, DrawsFromNamedStreamsOfTheSeed)
{
    SPECS_TEST_HEADER()
    {
        specsGlobalConfig.seed = 42;
        ParseTree tree;
        ASSERT_EQ(parse_netlist(tree, ".param a=gauss(1, 0.1)\n"
                                      ".param a=gauss(1, 0.1) u=uniform(2, 4)\n"
                                      ".param ln=lognormal(0, 0.5) n=10\n"
                                      "WG1 x y length=gauss(1e-6, 1e-7) neff=2\n"), 0);
        ASSERT_EQ(tree.draws.size(), 5u);
        EXPECT_EQ(tree.draws[0].first, "ROOT/.param/a#0");
        EXPECT_EQ(tree.draws[1].first, "ROOT/.param/a#1");
        EXPECT_EQ(tree.draws[2].first, "ROOT/.param/u#0");
        EXPECT_EQ(tree.draws[3].first, "ROOT/.param/ln#0");
        EXPECT_EQ(tree.draws[4].first, "ROOT/WG1/length#0");
        // the second assignment of a wins
        EXPECT_EQ(param(tree, "a"), gauss_of(42, "ROOT/.param/a#1", 1, 0.1));
        EXPECT_EQ(tree.draws[0].second, gauss_of(42, "ROOT/.param/a#0", 1, 0.1));
        spx::Rng rng_u = spx::rng_for(42, "ROOT/.param/u#0");
        EXPECT_EQ(param(tree, "u"), 2 + 2 * rng_u.uniform());
        EXPECT_EQ(param(tree, "ln"), std::exp(gauss_of(42, "ROOT/.param/ln#0", 0, 0.5)));
        EXPECT_EQ(tree.elements.at(0)->kwargs.at("length").as_double(),
                  gauss_of(42, "ROOT/WG1/length#0", 1e-6, 1e-7));
        // the .param names whose value drew, in first-assignment order
        EXPECT_EQ(tree.drawn_params, (vector<string>{ "a", "u", "ln" }));
        EXPECT_TRUE(tree.draw_seed_conflict.empty());

        // the same seed gives the same values, another seed other values
        ParseTree same, other;
        ASSERT_EQ(parse_netlist(same, ".param a=gauss(1, 0.1)\n"), 0);
        specsGlobalConfig.seed = 43;
        ASSERT_EQ(parse_netlist(other, ".param a=gauss(1, 0.1)\n"), 0);
        EXPECT_EQ(same.draws[0].second, tree.draws[0].second);
        EXPECT_NE(other.draws[0].second, tree.draws[0].second);
    }
    SPECS_TEST_FOOTER();
}

TEST(ExpressionFunctions, ChoiceAndSharedNames)
{
    SPECS_TEST_HEADER()
    {
        ParseTree tree;
        ASSERT_EQ(parse_netlist(tree, ".param c1=choice(5, 6, 7) c2=choice([\"a\", \"b\"]) c3=choice(9)\n"
                                      ".param w1=gauss(0, 1, shared=\"wafer\") w2=gauss(0, 1, SHARED=\"wafer\")\n"), 0);
        const double c1 = param(tree, "c1");
        EXPECT_TRUE(c1 == 5 || c1 == 6 || c1 == 7);
        const std::string c2 = tree.local_assignments.at("c2").as_string();
        EXPECT_TRUE(c2 == "a" || c2 == "b");
        EXPECT_EQ(param(tree, "c3"), 9);
        // a named shared stream: one value for every use of the name
        EXPECT_EQ(param(tree, "w1"), param(tree, "w2"));
        EXPECT_EQ(param(tree, "w1"), gauss_of(0, "SHARED/wafer", 0, 1));
        ASSERT_EQ(tree.draws.size(), 5u);
        EXPECT_TRUE(std::isnan(tree.draws[1].second)); // a string has no numeric value
    }
    SPECS_TEST_FOOTER();
}

// a subcircuit body is parsed once per instance: each instance draws on its own, shared=1 once for all
TEST(ExpressionFunctions, SubcircuitInstancesDrawIndependently)
{
    SPECS_TEST_HEADER()
    {
        specsGlobalConfig.seed = 7;
        ParseTree tree;
        ASSERT_EQ(parse_netlist(tree, ".subckt RING i o L=1\n"
                                      ".param Lr=gauss({L}, 0.1)\n"
                                      ".param Ls=gauss({L}, 0.1, shared=1)\n"
                                      "WG1 i o length={Lr} loss={Ls}\n"
                                      ".ends\n"
                                      "X1 a b RING L=10\n"
                                      "X2 b c RING L=10\n"), 0);
        tree.flatten();
        ASSERT_EQ(tree.elements.size(), 2u);
        const auto &kw1 = tree.elements[0]->kwargs;
        const auto &kw2 = tree.elements[1]->kwargs;
        EXPECT_EQ(kw1.at("length").as_double(), gauss_of(7, "ROOT/X1/.param/Lr#0", 10, 0.1));
        EXPECT_EQ(kw2.at("length").as_double(), gauss_of(7, "ROOT/X2/.param/Lr#0", 10, 0.1));
        EXPECT_NE(kw1.at("length").as_double(), kw2.at("length").as_double());
        EXPECT_EQ(kw1.at("loss").as_double(), kw2.at("loss").as_double());
        EXPECT_EQ(kw1.at("loss").as_double(), gauss_of(7, "SUBCKT:RING/.param/Ls#0", 10, 0.1));
        // the draws of the instances are recorded in the root tree
        ASSERT_EQ(tree.draws.size(), 4u);
        EXPECT_EQ(tree.draws[0].first, "ROOT/X1/.param/Lr#0");
        EXPECT_EQ(tree.draws[3].first, "SUBCKT:RING/.param/Ls#0");
        // only top-level .param names are drawn parameters
        EXPECT_TRUE(tree.drawn_params.empty());
    }
    SPECS_TEST_FOOTER();
}

// .options seed applies to the draws after it; one after a draw that changes the seed is a conflict
TEST(ExpressionFunctions, OptionsSeedAppliesWhenParsed)
{
    SPECS_TEST_HEADER()
    {
        ParseTree before;
        ASSERT_EQ(parse_netlist(before, ".options seed=5\n.param a=gauss(0, 1)\n"), 0);
        EXPECT_EQ(specsGlobalConfig.seed, 5u);
        EXPECT_EQ(param(before, "a"), gauss_of(5, "ROOT/.param/a#0", 0, 1));
        EXPECT_TRUE(before.draw_seed_conflict.empty());

        specsGlobalConfig.seed = 0;
        ParseTree after;
        ASSERT_EQ(parse_netlist(after, ".param a=gauss(0, 1)\n.options seed=5\n"), 0);
        EXPECT_NE(after.draw_seed_conflict.find(".options seed=5 at test.cir:2"), std::string::npos)
            << after.draw_seed_conflict;

        // the same seed again is not a conflict
        specsGlobalConfig.seed = 5;
        ParseTree same;
        ASSERT_EQ(parse_netlist(same, ".param a=gauss(0, 1)\n.options seed=5\n"), 0);
        EXPECT_TRUE(same.draw_seed_conflict.empty());

        // --seed wins: .options seed is not applied
        specsGlobalConfig.seed = 3;
        specsGlobalConfig.seed_explicit = true;
        ParseTree explicit_seed;
        ASSERT_EQ(parse_netlist(explicit_seed, ".param a=gauss(0, 1)\n.options seed=5\n"), 0);
        EXPECT_EQ(specsGlobalConfig.seed, 3u);
        EXPECT_TRUE(explicit_seed.draw_seed_conflict.empty());
    }
    SPECS_TEST_FOOTER();
}

TEST(ExpressionFunctionsDeathTest, InvalidCalls)
{
    SPECS_TEST_HEADER()
    {
        const auto failed = testing::ExitedWithCode(1);
        ParseTree t1, t2, t3, t4, t5, t6, t7, t8;
        EXPECT_EXIT(parse_netlist(t1, ".param a=foo(1)\n"), failed, "unknown function foo\\(\\)");
        EXPECT_EXIT(parse_netlist(t2, ".param a=sqrt(-1)\n"), failed, "sqrt\\(-1\\) is not a finite number");
        EXPECT_EXIT(parse_netlist(t3, ".param a=gauss(1)\n"), failed, "gauss\\(\\) takes two finite numbers");
        EXPECT_EXIT(parse_netlist(t4, ".param a=gauss(1, -1)\n"), failed, "needs sigma >= 0");
        EXPECT_EXIT(parse_netlist(t5, ".param a=uniform(2, 1)\n"), failed, "needs a <= b");
        EXPECT_EXIT(parse_netlist(t6, ".param a=gauss(0, 1, seed=1)\n"), failed, "unknown keyword seed");
        EXPECT_EXIT(parse_netlist(t7, ".subckt S i o L=gauss(1, 0.1)\n.ends\n"), failed,
                    "cannot be used in the parameter defaults of a .subckt line");
        EXPECT_EXIT(parse_netlist(t8, ".param a=gauss(0, 1, shared=1, shared=0)\n"), failed, "given twice");
    }
    SPECS_TEST_FOOTER();
}
