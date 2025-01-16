%define api.pure full
%param { yyscan_t scanner }


%code requires{
    typedef void * yyscan_t;
    // char *yyget_text ( yyscan_t yyscanner );
    // int yyget_lineno ( yyscan_t yyscanner );
    // int yylex(YYSTYPE * yylval, yyscan_t scanner);

    //extern int yylineno;
    //extern char *yytext;

    string yyget_filename ( yyscan_t scanner );
    string yyget_current_line ( yyscan_t scanner );
}

%{
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cmath>
#include <memory>
#include "parse_tree.h"
#include "parse_element.h"
#include "parse_analysis.h"
#include "parse_directive.h"

#include "parser.h"
#include "scanner.h"

using std::string;
using std::vector;
using std::pow;

extern ParseTree *cur_pt;

/*int yyerror(yyscan_t scanner, ParseTree *pt, const char *err)
{
    (void)pt;
    (void)p;
    printf("Error in the netlist (line %u:\"%s\")\n", yylineno, yytext);
    printf("-- Error at line %u:\n\t%s\n", yylineno, err);
    return 0;
}
*/
int yyerror(yyscan_t scanner, ParseTree *pt, const char *err);
%}

%union {
    double                  d_val;
    int                     i_val;
    char                    c_val;
    string                 *s_ptr;
    char                   *c_ptr;
    vector<double>         *vec_d_ptr;
    pair<string, Variable> *assig_ptr;
    Variable               *variable_ptr;
    vector<Variable>       *vec_var_ptr;
    pair<string, string>   *elemattr_ptr;
    pair<string, pair<string, Variable>> *netval_assign_ptr;
    pair<pair<string, string>, vector<double>> *cw_sweep_order_ptr;
}

// %define api.value.type {struct YYSTYPE}

%token <d_val> T_NUM
%token <i_val> T_INT
%token <i_val> T_NONE
%token <s_ptr> T_STR
%token <s_ptr> T_ASSIGN_ID
%token <s_ptr> T_ELEM_CWSRC T_ELEM_VLSRC T_ELEM_EVLSRC
%token <s_ptr> T_ELEM_WG T_ELEM_COUPLER T_ELEM_MERGER T_ELEM_SPLITTER
%token <s_ptr> T_ELEM_PSHIFT T_ELEM_MZI T_ELEM_CROSSING T_ELEM_PCMCELL
%token <s_ptr> T_ELEM_PROBE T_ELEM_MLPROBE T_ELEM_PDET T_ELEM_PWR_METER
%token <s_ptr> T_ELEM_X
%token <i_val> T_ANALYSIS_OP T_ANALYSIS_DC T_ANALYSIS_TRAN
%token <i_val> T_DIRECTIVE_OPTIONS T_DIRECTIVE_NODESET
%token <i_val> T_DIRECTIVE_SUBCKT
%token <s_ptr> T_DIRECTIVE_ENDS
%token <s_ptr> T_DIRECTIVE_SAVE
%token <i_val> T_LOCAL_ASSIGNMENT
%type <c_val> '='


// Available circuit elements
%type <i_val> element.wg
%type <i_val> element.merger
%type <i_val> element.splitter
%type <i_val> element.coupler
%type <i_val> element.pshift
%type <i_val> element.mzi
%type <i_val> element.crossing
%type <i_val> element.pcm_cell
%type <i_val> element.cwsrc
%type <i_val> element.vlsrc
%type <i_val> element.evlsrc
%type <i_val> element.probe
%type <i_val> element.mlprobe
%type <i_val> element.power_meter
%type <i_val> element.pdet
%type <i_val> element.x

// Available analysis
%type <i_val> analysis.op
%type <i_val> analysis.dc
%type <i_val> analysis.tran
%type <cw_sweep_order_ptr> cw_sweep_order

// Available directives
%type <i_val> directive.options
%type <i_val> directive.nodeset

// Rules for circuit nets (value is index in PT elements)
%type <s_ptr> net_name.base.str
%type <s_ptr> net_name_base
%type <s_ptr> net_name
%type <s_ptr> net.oanalog net.eanalog net.edigital
%type <s_ptr> net.oa.explicit_bidir
%type <s_ptr> net.oa.bidir
%type <s_ptr> net.oa.in
%type <s_ptr> net.oa.out

//%type <s_ptr> bus_name bus bus.oanalog bus.eanalog bus.edigital

// Rules related to subcircuits
%type <i_val> subcircuit
%type <i_val> subcircuit.with_ports
%type <i_val> subcircuit.with_assignments
%type <s_ptr> port_name

// Rules for circuit elements (value is index in PT elements arrays)
%type <i_val> atomelement element
%type <i_val> element.with_args element.with_kwargs
%type <i_val> element.x.with_args element.x.with_kwargs

// Rules for circuit analysis (value is ptr to ParseAnalysis)
%type <i_val> atomanalysis analysis
%type <i_val> analysis.with_args analysis.with_kwargs

// Rules for simulator directives (value is ptr to ParseDirective)
%type <i_val> atomdirective directive
%type <i_val> directive.with_args directive.with_kwargs

// For argument parsing
%type <assig_ptr> assignment
%type <variable_ptr> variable variable_base variable_or_empty
%type <variable_ptr> variable.free_expr variable.delimited_expr
//%type <variable_ptr> variable.str_without_quotes
%type <netval_assign_ptr> netval_assignment
%type <elemattr_ptr> element_attribute

// For expression parsing
%type <variable_ptr> expr term fact pow constant
%type <variable_ptr> listvar.tail listvar.unbound listvar.bound //TODO: check

// Add parse tree parameter to yyparse signature
%parse-param {ParseTree *pt}
//%verbose
//%define parse.error detailed
//%define parse.trace

// On start, set current parse tree to the one passed in call to yyparse
%initial-action {
    cur_pt = pt;
    if (!pt)
        yyerror(scanner, pt, "Invalid parse tree pointer");
}


%%

input: %empty
     | input line '\n'
;

line: %empty
    | element { /* cout << "Inserted element: " << cur_pt->elements[$1]->name << endl; */ }
    | analysis { /* cout << "Registered analysis [...]" << endl; */ }
    | directive { /* cout << "Registered directive [...]" << endl; */ }
    | subcircuit { /* cout << "Parsed subcircuit [...]" << endl; */ }
    | local_assignment { /* cout << "Processed local assignment [...]" << endl; */ }
;

local_assignment: T_LOCAL_ASSIGNMENT assignment
                    {
                        cur_pt->set_local_variable($2->first, $2->second);
                    }
                | local_assignment assignment
                    {
                        cur_pt->set_local_variable($2->first, $2->second);
                    }
;

/* --------- Directives definitions parsing ----------- */

directive.options: T_DIRECTIVE_OPTIONS { $$ = cur_pt->register_directive(new OPTIONSDirective); }
;

directive.nodeset: T_DIRECTIVE_NODESET { $$ = cur_pt->register_directive(new NODESETDirective); }
                 | directive.nodeset netval_assignment
                    {
                        static_cast<NODESETDirective *>(cur_pt->directives[$1])->net_assignments[$2->first].push_back($2->second);
                        $$ = $1;
                    }
;
subcircuit.with_ports: T_DIRECTIVE_SUBCKT T_STR
                    {
                        // Register a new subcircuit in current parse tree
                        $$ = cur_pt->register_subcircuit(*$2);
                        delete $2;
                    }
                | subcircuit.with_ports port_name
                    {
                        if (!cur_pt->subcircuits[$1]->kwargs.empty())
                        {
                            cerr << "Expected keyword assignment in subcircuit declaration but got: " << *$2 << endl;
                            exit(1);
                        }
                        // cout << *$2 << endl;
                        cur_pt->subcircuits[$1]->register_port(*$2);
                        $$ = $1;
                        delete $2;
                    }
;
subcircuit.with_assignments:
  subcircuit.with_assignments assignment
                        {
                            cur_pt->subcircuits[$1]->kwargs[$2->first] = $2->second;

                            $$ = $1;
                            delete $2;
                        }
| subcircuit.with_ports
;

subcircuit: subcircuit.with_assignments T_DIRECTIVE_ENDS
            {
                // cout << "Parsed subcircuit " << cur_pt->subcircuits[$1]->name << endl;
                cur_pt->subcircuits[$1]->netlist = *$2;
                delete $2;
            }
;

/* --------- Elements definitions parsing ----------- */

element.wg: T_ELEM_WG net.oa.in net.oa.out
            {
                $$ = cur_pt->register_element(new WGElement(*$1, {*$2, *$3}));
                delete $1;
                delete $2;
                delete $3;
            }
;

element.merger: T_ELEM_MERGER net.oa.in net.oa.in net.oa.out
            {
                $$ = cur_pt->register_element(new MergerElement(*$1, {*$2, *$3, *$4}));
                delete $1;
                delete $2;
                delete $3;
                delete $4;
            }
;

element.splitter: T_ELEM_SPLITTER net.oa.in net.oa.out net.oa.out
            {
                $$ = cur_pt->register_element(new SplitterElement(*$1, {*$2, *$3, *$4}));
                delete $1;
                delete $2;
                delete $3;
                delete $4;
            }
;

element.coupler: T_ELEM_COUPLER net.oa.in net.oa.in net.oa.out net.oa.out
            {
                $$ = cur_pt->register_element(new DCElement(*$1, {*$2, *$3, *$4, *$5}));
                delete $1;
                delete $2;
                delete $3;
                delete $4;
                delete $5;
            }
;

element.pshift: T_ELEM_PSHIFT net.oa.in net.oa.out net.eanalog
            {
                $$ = cur_pt->register_element(new PhaseShifterElement(*$1, {*$2, *$3, *$4}));
                delete $1;
                delete $2;
                delete $3;
                delete $4;
            }
;

element.mzi: T_ELEM_MZI net.oa.in net.oa.in net.oa.out net.oa.out net.eanalog
            {
                $$ = cur_pt->register_element(new MZIElement(*$1, {*$2, *$3, *$4, *$5, *$6}));
                delete $1;
                delete $2;
                delete $3;
                delete $4;
                delete $5;
                delete $6;
            }
;

element.crossing: T_ELEM_CROSSING net.oa.in net.oa.in net.oa.out net.oa.out
            {
                $$ = cur_pt->register_element(new CrossingElement(*$1, {*$2, *$3, *$4, *$5}));
                delete $1;
                delete $2;
                delete $3;
                delete $4;
                delete $5;
            }
;

element.pcm_cell: T_ELEM_PCMCELL net.oa.in net.oa.out
            {
                $$ = cur_pt->register_element(new PCMCellElement(*$1, {*$2, *$3}));
                delete $1;
                delete $2;
                delete $3;
            }
;

element.cwsrc: T_ELEM_CWSRC net.oa.out
            {
                $$ = cur_pt->register_element(new CWSourceElement(*$1, {*$2}));
                delete $1;
                delete $2;
            }
;

element.vlsrc: T_ELEM_VLSRC net.oa.out
            {
                $$ = cur_pt->register_element(new VLSourceElement(*$1, {*$2}));
                delete $1;
                delete $2;
            }
;

element.evlsrc: T_ELEM_EVLSRC net.eanalog
            {
                $$ = cur_pt->register_element(new EVLSourceElement(*$1, {*$2}));
                delete $1;
                delete $2;
            }
;

element.probe: T_ELEM_PROBE net.oa.in
            {
                $$ = cur_pt->register_element(new ProbeElement(*$1, {*$2}));
                delete $1;
                delete $2;
            }
;

element.mlprobe: T_ELEM_MLPROBE net.oa.in
            {
                $$ = cur_pt->register_element(new MLProbeElement(*$1, {*$2}));
                delete $1;
                delete $2;
            }
;

element.pdet: T_ELEM_PDET net.oa.in net.eanalog
            {
                $$ = cur_pt->register_element(new PhotodetectorElement(*$1, {*$2, *$3}));
                delete $1;
                delete $2;
                delete $3;
            }
;

element.power_meter: T_ELEM_PWR_METER net.oa.in
            {
                $$ = cur_pt->register_element(new PowerMeterElement(*$1, {*$2}));
                delete $1;
                delete $2;
            }
;

element.x: T_ELEM_X
            {
                $$ = cur_pt->register_element(new XElement(*$1));
                delete $1;
            }
;

/* --------- Analyses definitions parsing ----------- */

analysis.op: T_ANALYSIS_OP
                { $$ = cur_pt->register_analysis(new OPAnalysis()); }
;

analysis.dc: T_ANALYSIS_DC
                { $$ = cur_pt->register_analysis(new DCAnalysis()); }
           | analysis.dc cw_sweep_order
                {
                    auto ana = dynamic_cast<DCAnalysis *>(cur_pt->analyses[$1]);
                    if (ana->sweep_orders.size())
                    {
                        cerr << "Registering more than one DC sweep is not supported yet." << endl;
                        exit(1);
                    }
                    ana->register_sweep_order(*$2);
                    delete $2;
                }
;

analysis.tran: T_ANALYSIS_TRAN
                { $$ = cur_pt->register_analysis(new TRANAnalysis()); }
;


/* ---------- Directives arguments parsing ----------- */

atomdirective: directive.options { $$ = $1; }
             | directive.nodeset { $$ = $1; }
;

directive.with_args:
  directive.with_args variable.delimited_expr
            {
                // cout << "found positional arg: " << $2->get_str() << endl;
                if (!cur_pt->directives[$1]->kwargs.empty())
                    yyerror(scanner, pt, "positional arguments cannot follow keyword arguments");
                cur_pt->directives[$1]->args.emplace_back(*$2);
                $$ = $1;
                delete $2;
            }
| atomdirective { $$ = $1; }
;

directive.with_kwargs:
  directive.with_kwargs assignment
            {
                // cout << "found keyword arg: " << $2->second.get_str() << endl;
                cur_pt->directives[$1]->kwargs[$2->first] = $2->second;
                $$ = $1;
                delete $2;
            }
| directive.with_args
;

directive: directive.with_kwargs
;

/* --------- Elements arguments parsing ----------- */

atomelement: element.wg { $$ = $1; }
           | element.merger { $$ = $1; }
           | element.splitter { $$ = $1; }
           | element.coupler { $$ = $1; }
           | element.pshift { $$ = $1; }
           | element.mzi { $$ = $1; }
           | element.crossing { $$ = $1; }
           | element.pcm_cell { $$ = $1; }
           | element.cwsrc { $$ = $1; }
           | element.vlsrc { $$ = $1; }
           | element.evlsrc { $$ = $1; }
           | element.probe { $$ = $1; }
           | element.mlprobe { $$ = $1; }
           | element.power_meter { $$ = $1; }
           | element.pdet { $$ = $1; }
;

element.with_args:
  element.with_args variable.delimited_expr
            {
                // cout << "found positional arg: " << $2->get_str() << endl;
                if (!cur_pt->elements[$1]->kwargs.empty())
                    yyerror(scanner, pt, "positional arguments cannot follow keyword arguments");
                cur_pt->elements[$1]->args.emplace_back(*$2);
                $$ = $1;

                // TODO: for X element only, nets are stored as args
                // Xnnnn n1 n2 n3 ... sub_name **sub_kwargs
                // because the number of nets is not known when reading n1 n2 n3...
                // [n1 n2 n3 ... subname] are also args
                // when the  time an arg is found, the previous one (if it exists), can be considered a net
                // and therefore added to the element's nets array and discarded from args
                // this will ensure that args will contain only [sub_name]
                // for this to work, the X element cannot have args beyond sub_name (only kwargs)
                if (cur_pt->elements[$1]->args.size() == 2)
                {
                    // check if the args[0] is a valid net name
                    // create net with name args[0] (by default with type NONE)
                    // add args[0] to nets
                    // delete args[0] from args
                }

                delete $2;
            }
| atomelement { $$ = $1; }
;

element.with_kwargs:
  element.with_kwargs assignment
            {
                // cout << "found keyword arg: " << $2->second.get_str() << endl;
                cur_pt->elements[$1]->kwargs[$2->first] = $2->second;
                $$ = $1;
                delete $2;
            }
| element.with_args
;

element.x.with_args:
  element.x.with_args net_name_base
            {
                // cout << "found positional arg: " << $2->get_str() << endl;
                if (!cur_pt->elements[$1]->kwargs.empty())
                    yyerror(scanner, pt, "positional arguments cannot follow keyword arguments");
                cur_pt->elements[$1]->args.emplace_back(Variable(*$2));
                $$ = $1;

                // TODO: for X element only, nets are stored as args
                // Xnnnn n1 n2 n3 ... sub_name **sub_kwargs
                // because the number of nets is not known when reading n1 n2 n3...
                // [n1 n2 n3 ... subname] are also args
                // when the  time an arg is found, the previous one (if it exists), can be considered a net
                // and therefore added to the element's nets array and discarded from args
                // this will ensure that args will contain only [sub_name]
                // for this to work, the X element cannot have args beyond sub_name (only kwargs)
                if (cur_pt->elements[$1]->args.size() == 2)
                {
                    // check if the args[0] is a valid net name
                    // create net with name args[0] (by default with type NONE)
                    // add args[0] to nets
                    // delete args[0] from args
                }

                delete $2;
            }
| element.x { $$ = $1; }
;

element.x.with_kwargs:
  element.x.with_kwargs assignment
            {
                // cout << "found keyword arg: " << $2->second.get_str() << endl;
                cur_pt->elements[$1]->kwargs[$2->first] = $2->second;
                $$ = $1;
                delete $2;
            }
| element.x.with_args
;

element:
  element.with_kwargs
| element.x.with_kwargs
;

/* ---------- Analysis arguments parsing ----------- */

atomanalysis: analysis.op { $$ = $1; }
            | analysis.dc { $$ = $1; }
            | analysis.tran { $$ = $1; }
;

analysis.with_args:
  analysis.with_args variable.delimited_expr
            {
                // cout << "found positional arg: " << $2->get_str() << endl;
                if (!cur_pt->analyses[$1]->kwargs.empty())
                    yyerror(scanner, pt, "positional arguments cannot follow keyword arguments");
                cur_pt->analyses[$1]->args.emplace_back(*$2);
                $$ = $1;
                delete $2;
            }
| atomanalysis { $$ = $1; }
;

analysis.with_kwargs:
  analysis.with_kwargs assignment
            {
                // cout << "found keyword arg: " << $2->second.get_str() << endl;
                cur_pt->analyses[$1]->kwargs[$2->first] = $2->second;
                $$ = $1;
                delete $2;
            }
| analysis.with_args
;

analysis: analysis.with_kwargs

/* --------- NET type parsing ----------- */

net.oanalog: net_name
                {
                    bool ok = cur_pt->nets[*$1].setType(ParseNet::OANALOG);
                    if (!ok)
                    {
                        stringstream ret;
                        ret << "Error creating net '" + *$1 + "': net already exists with a different type" << endl;
                        ret << "\t (new type: " << "OANALOG" << ", old type: " << cur_pt->nets[*$1].type_str() << ")" << endl;
                        yyerror(scanner, pt, ret.str().c_str());
                    }
                    cur_pt->nets[*$1].m_ports_count++;
                    $$ = $1;
                }
;

net.oa.explicit_bidir:
  net.oanalog '&'
    {
        // this net will be both written to and read from by the new device
        // so increment both reader and writer count
        // and set the explicit "bidirectional" flag to true
        cur_pt->nets[*$1].m_bidirectional = true;
        cur_pt->nets[*$1].m_readers_count++;
        cur_pt->nets[*$1].m_writers_count++;
        $$ = $1;
    }
;

net.oa.bidir:
  net.oa.explicit_bidir
| net.oanalog
    {
        // this net will be both written to and read from by the new device
        // so increment both reader and writer count
        cur_pt->nets[*$1].m_readers_count++;
        cur_pt->nets[*$1].m_writers_count++;
        $$ = $1;
    }
;

net.oa.in:
  net.oanalog
    {
    // this net will only be read from by the new device
    // so increment only reader count
    cur_pt->nets[*$1].m_readers_count++;
    $$ = $1;
    }
| net.oa.explicit_bidir
    {
    // bidir already got a writer and reader increment, but
    // this net will only be read from by the new device
    // so decrement writer count
    cur_pt->nets[*$1].m_writers_count--;
    $$ = $1;
    }
;

net.oa.out:
  net.oanalog
    {
    // this net will only be written to by the new device
    // so increment only writer count
    cur_pt->nets[*$1].m_writers_count++;
    $$ = $1;
    }
| net.oa.explicit_bidir
    {
    // bidir already got a writer and reader increment, but
    // this net will only be written to by the new device
    // so decrement reader count
    cur_pt->nets[*$1].m_readers_count--;
    $$ = $1;
    }
;

net.eanalog: net_name
                {
                    bool ok = cur_pt->nets[*$1].setType(ParseNet::EANALOG);
                    if (!ok)
                    {
                        stringstream ret;
                        ret << "Error creating net '" + *$1 + "': net already exists with a different type" << endl;
                        ret << "\t (new type: " << "EANALOG" << ", old type: " << cur_pt->nets[*$1].type_str() << ")" << endl;
                        yyerror(scanner, pt, ret.str().c_str());
                    }
                    $$ = $1;
                }

net.edigital: net_name
                {
                    bool ok = cur_pt->nets[*$1].setType(ParseNet::EDIGITAL);
                    if (!ok)
                    {
                        stringstream ret;
                        ret << "Error creating net '" + *$1 + "': net already exists with a different type" << endl;
                        ret << "\t (new type: " << "EDIGITAL" << ", old type: " << cur_pt->nets[*$1].type_str() << ")";
                        yyerror(scanner, pt, ret.str().c_str());
                    }
                    $$ = $1;
                }
;

/* --------- BUS type parsing ----------- */

/*
bus.oanalog: bus_name
                {
                    cur_pt->nets[*$1].m_type = ParseNet::OANALOG;
                    $$ = $1;
                }
;

bus.eanalog: bus_name
                {
                    cur_pt->nets[*$1].m_type = ParseNet::EANALOG;
                    $$ = $1;
                }
;

bus.edigital: bus_name
                {
                    cur_pt->nets[*$1].m_type = ParseNet::EDIGITAL;
                    $$ = $1;
                }
;
bus: bus.oanalog { $$ = $1; }
   | bus.eanalog { $$ = $1; }
   | bus.edigital { $$ = $1; }
;
*/
/* --------- NET and BUS name parsing ----------- */

net_name.base.str:
              T_STR { $$ = $1; }
            | '/' T_STR
                {
                    $$ = $2;
                    *$2 = "/" + *$2;
                }
            | '/' T_INT
                {
                    stringstream ss;
                    ss << "/" << $2;
                    $$ = new string(ss.str());
                }
            | net_name.base.str '-' T_STR
                {
                    // FIXME: this rule is problematic (overlap with -expr)
                    $$ = $1;
                    *$1 += "-" + *$3;
                    delete $3;
                }
;

net_name_base: T_INT { $$ = new string(ParseNet::name_from_id($1)); }
             | net_name.base.str { $$ = $1; }
;

net_name: net_name_base
            {
                $$ = new string(cur_pt->get_or_create_net(*$1));
                delete $1;
            }
;

/*
bus_name: net_name { $$ = $1; }
        | net_name_base '<' T_INT '>'
            {
                $$ = new string(cur_pt->get_or_create_net(*$1, $3));
                delete $1;
            }
;
*/
port_name: net_name_base { $$ = $1; }
;

/* --------- Assignment and variable ----------- */

netval_assignment: '/' net_name '/' T_STR '=' variable.delimited_expr
                    {
                        $$ = new pair<string, pair<string, Variable>>(
                                *$2,
                                pair<string, Variable>(*$4, *$6)
                                );
                        delete $2;
                        delete $4;
                        delete $6;
                    }
;

cw_sweep_order: element_attribute variable_base variable_base variable_base
                {
                    // cout << $1->first << "->" << $1->second;
                    // cout << " = [" << $2->as_double();
                    // cout << ":" << $4->as_double();
                    // cout << ":" << $3->as_double() << "]" << endl;
                    $$ = new pair<pair<string, string>, vector<double>>(*$1, {$2->as_double(), $3->as_double(), $4->as_double()});
                    delete $1;
                    delete $2;
                    delete $3;
                    delete $4;
                }
;

element_attribute: '/' T_STR '/' T_STR
                {
                    // cout << *$2 << "->" << *$4 << endl;
                    $$ = new pair<string, string>(*$2,*$4);
                    delete $2;
                    delete $4;
                }
;

assignment: T_ASSIGN_ID '=' variable
            {
                $$ = new pair<string, Variable>(*$1, *$3);
                delete $1;
                delete $3;
            }
;

variable_base:
          T_INT
            {
                $$ = new Variable($1);
            }
        | T_NUM
            {
                $$ = new Variable($1);
            }
        | T_NONE
            {
                $$ = new Variable(Variable::NONE);
            }
        | '{' T_STR '}'
            {
                $$ = new Variable(cur_pt->get_variable(*$2));
                delete $2;
            }
        | '"' T_STR '"'
            {
                $$ = new Variable(*$2);
                delete $2;
            }
        | listvar.bound
            {
                $$ = $1;
            }
;

variable.delimited_expr:
  '{' expr '}'
    {
        $$ = $2;
    }
| variable_base;
;

variable.free_expr:
  expr
    {
        $$ = $1;
    }
;

variable:
  variable.free_expr
;

/*
variable.str_without_quotes:
  T_STR
    {
        if (*$1 == "None")
            $$ = new Variable(Variable::NONE);
        else
            $$ = new Variable(*$1);
        delete $1;
    }
| variable;
;
*/

variable_or_empty:
  variable
  | %empty
    {
        $$ = new Variable(Variable::NONE);
    }
;

/* ------------ Expression parsing ------------- */

constant: variable_base { $$ = $1; }
;

expr:
  expr '+' term { $$ = $1; *$$ += *$3; delete $3; }
| expr '-' term { $$ = $1; *$$ -= *$3; delete $3; }
| '+' term { $$ = $2; }
| '-' term { $$ = $2; *$$ *= -1; }
| term { $$ = $1; }
;

term:
  term '*' fact { $$ = $1; *$$ *= *$3; delete $3; }
| term '/' fact { $$ = $1; *$$ /= *$3; delete $3; }
| fact { $$ = $1; }
;

fact:
  fact '^' pow { $$ = $1; *$$ ^= *$3; delete $3; }
| pow { $$ = $1; }
;

pow:
  constant
| '(' expr ')' { $$ = $2; }
;

// --------------- variables list (elements can have different types)
listvar.tail: variable_or_empty
                {
                    $$ = new Variable(Variable::LIST);
                    $$->vec.insert($$->vec.begin(), *$1);
                    delete $1;
                }
;

listvar.unbound: variable_or_empty ',' listvar.tail
                    {
                        $3->vec.insert($3->vec.begin(), *$1);
                        $$ = $3;
                        delete $1;
                    }
               | variable_or_empty ',' listvar.unbound
                    {
                        $3->vec.insert($3->vec.begin(), *$1);
                        $$ = $3;
                        delete $1;
                    }
;

listvar.bound: '[' listvar.tail ']' { $$ = $2; }
             | '[' listvar.unbound ']' { $$ = $2; }
;

%%

int yyerror(yyscan_t scanner, ParseTree *pt, const char *err)
{
    (void)pt;
    (void)scanner;
    string token = yyget_text(scanner);
    stringstream ss;
    for (char c : token)
    {
        switch (c)
        {
            case '\r':
                ss << "\\r";
                break;
            case '\n':
                ss << "\\n";
                break;
            case '\t':
                ss << "\\t";
                break;
            case '\b':
                ss << "\\b";
                break;
            default:
                ss << c;
                break;
        }
    }
    string token_escaped = ss.str();
    
    cerr << "Parsing error (" << yyget_filename(scanner) << ":" << yyget_lineno(scanner) << "): " << err << endl;
    cerr << "-- offending token: \"" << token_escaped.c_str() << "\"" << endl;
    string str = yyget_current_line(scanner);
    if (!str.empty())
        cerr << "-- while parsing line: \"" << str << "\"" << endl;
    exit(1);
}
