ALPHA [[:alpha:]]
DIGIT [0-9]
ALPHA_NUM {ALPHA}|{DIGIT}
ALPHA_PLUS {ALPHA}|[_]
ALPHA_PLUS_NUM {ALPHA}|[_]|{DIGIT}
D {DIGIT}

DOUBLE (({D}+)|({D}*\.{D}+))([eE][-+]?[0-9]+)?

BIDIR_SPECIFIER [&]
BIN_OPERATOR [,=\*/\+\-\^]
PAR_OPERATOR [\{\}\(\)\[\]<>]
OPERATOR {BIN_OPERATOR}|{PAR_OPERATOR}|{BIDIR_SPECIFIER}


WS [  \t]
WS_BREAK (?:[^{WS}])

/* EOL_DOS \r\n */
/* EOL_UNIX \n */
/* EOL_OLD_MAC \n */
EOL_PORTABLE \r?\n
EOL {EOL_PORTABLE}

%option warn
%option yylineno
/*%option noyywrap*/
%option caseless
%option reentrant
%option bison-bridge
%option stack

%{
#include <string>
#include <sstream> // for conversions of numbers from str
#include <cstring> // for strcpy
#include <vector>
#include "parse_tree.h"

#include "parser.tab.h"

using std::string;
using std::vector;

#define YY_NO_INPUT
#define YY_NO_UNPUT
#define YY_EXTRA_TYPE void*

YY_BUFFER_STATE yy_create_buffer ( FILE *file, int size, yyscan_t scanner);
void yypush_buffer_state ( YY_BUFFER_STATE buffer, yyscan_t scanner);
void yypop_buffer_state ( yyscan_t scanner );
int yyget_lineno ( yyscan_t scanner );
int yyget_column ( yyscan_t scanner );
void yyset_extra ( YY_EXTRA_TYPE user_defined, yyscan_t scanner );
YY_EXTRA_TYPE yyget_extra ( yyscan_t scanner );

int yylerror(yyscan_t scanner, const char *p)
{
    cerr << "error (line " << yyget_lineno(scanner) << "): " << p << endl;
    string * str = (string *)yyget_extra(scanner);
    if (str)
        cerr << "\twhile parsing line: " << *str << endl;
    return 3;
}

ParseTree *cur_pt = nullptr;

static int internal_subckt_count = 0;
static size_t delete_me_npos = 0;

template<typename T>
static T from_string(const string &str)
{
    std::stringstream ss(str);
    T ret;
    ss >> ret;
    return ret;
}

%}

/*** States ***/
/* %x IN_DQUOTES */
%x INCLUDE
%s SUBCKT_DECLARATION
%x SUBCKT_DEFINITION
/*Quoted string (returns char[]) (capture until next '"' and throw error on newline)*/
%x IN_DQUOTES



/* To ignore case: (?i:xxx) */

%%  /*** Rules section ***/

^\.include  {yy_push_state(INCLUDE, yyscanner);}
<INCLUDE>{WS}*      /* eat the whitespace */
<INCLUDE>[^ ;\t\n\r]+   { /* got the include file name */
        yyin = fopen( yytext, "r" );

        if ( ! yyin )
        {
            cerr << "Error: included file not found \"" << yytext << "\"" << endl;
            exit(1);
        }
        cout << "Including file: " << yytext << endl;
        yypush_buffer_state(yy_create_buffer( yyin, YY_BUF_SIZE, yyscanner), yyscanner);

        //yy_push_state(INITIAL, yyscanner);
        BEGIN(INITIAL);
        }

<IN_DQUOTES>\" {
    yy_pop_state(yyscanner);
    // cout << "leaving dquotes mode" << endl;
    return '"';
    }
<IN_DQUOTES>{EOL} { exit(yylerror(yyscanner, "Unmatched double quote")); }
<IN_DQUOTES>[^\"\r\n]* {
    // cout << "found quoted string: " << yytext << endl;
    yylval_param->s_ptr = new string(yytext);
    return T_STR;
    }

<SUBCKT_DECLARATION>{EOL} {
    // end of subcircuit declaration line (.subckt name node1 node2...)
    // go into subckt_definition state, which copies all text to a string
    // until the corresponding .ends is reached
    yylval_param->s_ptr = new string;
    internal_subckt_count = 0;
    delete_me_npos = 0;
    // cout << endl << "going to subckt definition mode" << endl;
    BEGIN(SUBCKT_DEFINITION); //needs BEGIN and not yy_push_state
}

<SUBCKT_DEFINITION>^\.SUBCKT {
    // cout << "hello2" << endl;
    // increase internal count for keeping track of included subcircuits
    internal_subckt_count++;
    // but reject the rule and go to the next matching pattern
    // (i.e. save the line to the string)
    REJECT;
}

<SUBCKT_DEFINITION>^\.ENDS {
    if (internal_subckt_count)
    {
        // found .ends of included subcircuit
        // decrease internal count
        --internal_subckt_count;
        // but reject the rule and go to the next matching pattern
        // (i.e. save the line to the string)
        REJECT;
    }
    else
    {
        // found .ends of current subcircuit
        // go back to normal parsing
        //BEGIN(INITIAL);
        yy_pop_state(yyscanner);
        // return .ends token so that the subcircuit string is recorded
        return T_DIRECTIVE_ENDS;
    }
}

<SUBCKT_DEFINITION>. {
    /* save characters to buffer as-is */
    *yylval_param->s_ptr += yytext;
}

<SUBCKT_DEFINITION>{EOL} {
    /* save characters to buffer */
    *yylval_param->s_ptr += yytext;

    // cout << "found subckt line: " << yylval_param->s_ptr->substr(delete_me_npos);
    // delete_me_npos = yylval_param->s_ptr->size();
}

<SUBCKT_DEFINITION><<EOF>> {
    cerr << "Reached end of file before end of subcircuit definition" << endl;
    exit(1);
}

\" {
    yy_push_state(IN_DQUOTES, yyscanner);
    // cout << "entering dquotes mode" << endl;
    return '"';
    }

[+-]?{D}+ {
    /* signed integer number */
    yylval_param->i_val = from_string<int>(yytext);
    return T_INT;
}

[+-]?{DOUBLE} {
    /* signed floating point number */
    yylval_param->d_val = from_string<double>(yytext);
    return T_NUM;
}

(;[^\n]*) {
    /* inline comment starts with ; */
    continue;
}

^(\*[^\n]*) {
    /* full line comment starts with **/
    continue;
}

^X({ALPHA_PLUS_NUM})+ {
    /* subcircuit instance name */
    yylval_param->s_ptr = new string(yytext);
    return T_ELEM_X;
}

^WG({ALPHA_PLUS_NUM})+ {
    /* waveguide instance name */
    yylval_param->s_ptr = new string(yytext);
    return T_ELEM_WG;
}

^MERGER({ALPHA_PLUS_NUM})+ {
    /* merger instance name */
    yylval_param->s_ptr = new string(yytext);
    return T_ELEM_MERGER;
}

^SPLITTER({ALPHA_PLUS_NUM})+ {
    /* merger instance name */
    yylval_param->s_ptr = new string(yytext);
    return T_ELEM_SPLITTER;
}

^COUPLER({ALPHA_PLUS_NUM})+ {
    /* DC instance name */
    yylval_param->s_ptr = new string(yytext);
    return T_ELEM_COUPLER;
}

^PSHIFT({ALPHA_PLUS_NUM})+ {
    /* PHASESHIFTER instance name */
    yylval_param->s_ptr = new string(yytext);
    return T_ELEM_PSHIFT;
}

^MZI({ALPHA_PLUS_NUM})+ {
    /* MZI instance name */
    yylval_param->s_ptr = new string(yytext);
    return T_ELEM_MZI;
}

^CROSSING({ALPHA_PLUS_NUM})+ {
    /* CROSSING instance name */
    yylval_param->s_ptr = new string(yytext);
    return T_ELEM_CROSSING;
}

^CWSRC({ALPHA_PLUS_NUM})+ {
    /* Continuous-wave source instance name */
    yylval_param->s_ptr = new string(yytext);
    return T_ELEM_CWSRC;
}

^VLSRC({ALPHA_PLUS_NUM})+ {
    /* Value-list source instance name */
    yylval_param->s_ptr = new string(yytext);
    return T_ELEM_VLSRC;
}

^EVLSRC({ALPHA_PLUS_NUM})+ {
    /* Electric Value-list source instance name */
    yylval_param->s_ptr = new string(yytext);
    return T_ELEM_EVLSRC;
}

^PROBE({ALPHA_PLUS_NUM})+ {
    /* Probe instance */
    yylval_param->s_ptr = new string(yytext);
    return T_ELEM_PROBE;
}

^MLPROBE({ALPHA_PLUS_NUM})+ {
    /* Probe instance */
    yylval_param->s_ptr = new string(yytext);
    return T_ELEM_MLPROBE;
}

^PDET({ALPHA_PLUS_NUM})+ {
    /* Probe instance */
    yylval_param->s_ptr = new string(yytext);
    return T_ELEM_PDET;
}

^PCMCELL({ALPHA_PLUS_NUM})+ {
    /* Probe instance */
    yylval_param->s_ptr = new string(yytext);
    return T_ELEM_PCMCELL;
}

^\.ASSIGN { return T_LOCAL_ASSIGNMENT; }
^\.PARAM { return T_LOCAL_ASSIGNMENT; }
^\.SAVE { return T_DIRECTIVE_SAVE; }

^\.OP { return T_ANALYSIS_OP; }
^\.DC { return T_ANALYSIS_DC; }
^\.TRAN { return T_ANALYSIS_TRAN; }

^\.OPTIONS { return T_DIRECTIVE_OPTIONS; }
^\.NODESET { return T_DIRECTIVE_NODESET; }
^\.IC { exit(1); }
^\.SUBCKT {
    /* Subcircuit declaration */
    // enter new parser state
    yy_push_state(SUBCKT_DECLARATION, yyscanner);
    // return token so bison can correctly parse the declaration
    return T_DIRECTIVE_SUBCKT;
}

<INITIAL>^\.ENDS {
    cerr << "Unexpeced '.ENDS' directive" << endl;
    exit(1);
}

{ALPHA_PLUS}{ALPHA_PLUS_NUM}*/{WS}*= {
    yylval_param->s_ptr = new string(yytext);

    return T_ASSIGN_ID;
}

{ALPHA_PLUS}{ALPHA_PLUS_NUM}* {
    yylval_param->s_ptr = new string(yytext);

    /*
    if (yylval_param->s_ptr == "null")
        return T_NONE;
    if (yylval_param->s_ptr == "true")
    {
        yylval_param->b_val = true;
        return T_BOOL;
    }
    if (yylval_param->s_ptr == "false")
    {
        yylval_param->b_val = false;
        return T_BOOL;
    }
    */
    return T_STR;
}

{OPERATOR} {
    yylval_param->c_val = yytext[0];
    return yytext[0];
}

{WS} ;

^.*/{EOL} {
    string * str = (string *)yyget_extra(yyscanner);
    if (!str)
        str = new string;
    *str = yytext;
    yyset_extra((void *)str, yyscanner);
    REJECT;
}

(?# the following lines are for compatibility with KiCad generated netlists)
(?# they were added fast with relatively no checking for potential problems if)
(?# the keywords are found in subcircuit files or included files)
^\.TITLE({WS}.*)?/{EOL} {
    // cout << "Found '.title' directive, ignoring." << endl;
}
^\.END {
    // cout << "Found '.end' directive, ignoring." << endl;
}

(?# Ignore EOL followed by '+' -- line continuation)
{EOL}\+ ;

{EOL} { return '\n'; }

<INITIAL,INCLUDE><<EOF>> {
    yypop_buffer_state(yyscanner);

        if ( !YY_CURRENT_BUFFER )
        {
        yyterminate();
        }
        return '\n';
    }

<SUBCKT_DECLARATION,IN_DQUOTES><<EOF>> {
    exit(yylerror(yyscanner, "Unexpected end of file."));
}

<*>. {
    stringstream msg;
    msg << "Unexpected character: \"";
    switch (yytext[0])
    {
        case '\r':
            msg << "\\r";
            break;
        case '\n':
            msg << "\\n";
            break;
        case '\t':
            msg << "\\t";
            break;
        case '\b':
            msg << "\\b";
            break;
        default:
            msg << yytext[0];
            break;
    }
    msg << "\"";
    exit(yylerror(yyscanner, msg.str().c_str()));
    }
%%

/* END */

/* ^.OP(?:{WS}+) { return T_DIRECTIVE_OP; } */
/* ^.DC(?:{WS}+) { return T_DIRECTIVE_DC; } */

/* Quoted string with 2 strings on the same line (returns char [])
\"[^\"\\]*\" {
    int n = strlen(yytext) - 2;
    yylval_param->sv = new char[n+1];
    strncpy(yylval_param->sv, yytext + 1, n);
    yylval_param->sv[n] = '\0';
    return T_QUOTED_STR;
}
*/


