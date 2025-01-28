#pragma once

#include "specs.h"
#include "utils/strutils.h"

#include <initializer_list>
#include <utility>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <iostream>

using std::pair;
using std::make_pair;
using std::string;
using std::vector;
using std::map;
using std::cout;
using std::cerr;
using std::endl;
using std::pair;
using std::unique_ptr;

struct ParseTree;
struct SUBCKTDirective;

/* Struct to hold a typed variable */
struct Variable {
    enum Type {
        TYPE_MIN = -1,
        DOUBLE = 0,
        COMPLEX_DOUBLE,
        INTEGER,
        BOOLEAN,
        STRING,
        LIST,
        NONE,
        TYPE_MAX,
    };
    Type type;
    double num;
    complex<double> cnum;
    int inum;
    bool bnum;
    string str;
    vector<Variable> vec;

    Variable(const Variable &v) { *this = v; }
    Variable(Type type): type(type) {}
    Variable(double num = 0): type(DOUBLE), num(num) {}
    explicit Variable(const complex<double> &cnum): type(COMPLEX_DOUBLE), cnum(cnum) {}
    explicit Variable(int inum): type(INTEGER), inum(inum) {}
    explicit Variable(bool bnum): type(BOOLEAN), bnum(bnum) {}
    explicit Variable(const string &str): type(STRING), str(str) {}
    explicit Variable(const vector<Variable> &vec): type(LIST), vec(vec) {}
    explicit Variable(const vector<double> &dvec): type(LIST)
    {
        vec.resize(dvec.size());
        for (size_t i = 0; i < dvec.size(); ++i)
        {
            vec[i] = Variable(dvec[i]);
        }
    }

    bool type_is_valid() const {
        return TYPE_MIN < type && type < TYPE_MAX;
    }

    bool is_none() const {
        return type == NONE;
    }

    bool is_list() const {
        return type == LIST;
    }

    bool is_number() const {
        return type == DOUBLE || type == INTEGER || type == BOOLEAN;
    }

    string get_str() const
    {
        stringstream ss;
        switch (type)
        {
            case STRING:
                return str;
            case DOUBLE:
                ss << num;
                return ss.str();
            case COMPLEX_DOUBLE:
                ss << cnum.real() << " + " << cnum.imag() << "i";
                return ss.str();
            case INTEGER:
                ss << inum;
                return ss.str();
            case BOOLEAN:
                ss << bnum;
                return ss.str();
            case LIST:
                ss << "[";
                for (size_t i = 0; i < vec.size(); ++i)
                {
                    ss << vec[i].to_json();
                    if (i != vec.size() - 1)
                        ss << ", ";
                }
                ss << "]";
                return ss.str();
            case NONE:
                return "None";
            default:
                cerr << "error: cannot convert variable to string" << endl;
                exit(1);
        }
    }

    string to_json() const
    {
        stringstream ss;
        ss << '{';
        ss << "\"type\":" << '"' << kind() << '"' << ',';
        ss << "\"value\":";
        {
            stringstream tss;
            switch (type) {
            case STRING:
                tss << '"' << str << '"';
                break;
            case DOUBLE:
                tss << scientific << num;
                break;
            case COMPLEX_DOUBLE:
                tss << "{\"real\":";
                tss << scientific << cnum.real();
                tss << ",\"imag\":" << cnum.imag();
                tss << '}';
                break;
            case INTEGER:
                tss << inum;
                break;
            case BOOLEAN:
                tss << (bnum ? "true" : "false");
                break;
            case LIST:
                tss << "[";
                for (size_t i = 0; i < vec.size(); ++i) {
                    tss << vec[i].to_json();
                    if (i != vec.size() - 1)
                        tss << ", ";
                }
                tss << "]";
                break;
            case NONE:
                tss << "null";
                break;
            default:
                cerr << "error: cannot convert variable to json representation" << endl;
                exit(1);
            }
            ss << tss.str();
        }
        ss << '}';
        return ss.str();
    }

    double as_double() const
    {
        if (type == DOUBLE)
            return num;
        if (type == INTEGER)
            return inum;
        if (type == BOOLEAN)
            return bnum;
        cerr << "Variable cannot be converted to a double value: " << get_str() << endl;
        exit(1);
    }

    int as_integer() const
    {
        if (type == DOUBLE)
            return num;
        if (type == INTEGER)
            return inum;
        if (type == BOOLEAN)
            return bnum;
        cerr << "Variable cannot be converted to an integer value: " << get_str() << endl;
        exit(1);
    }

    bool as_boolean() const
    {
        if (type == DOUBLE)
            return num;
        if (type == INTEGER)
            return inum;
        if (type == BOOLEAN)
            return bnum;
        cerr << "Variable cannot be converted to a boolean value: " << get_str() << endl;
        exit(1);
    }

    complex<double> as_complex() const
    {
        if (type == DOUBLE)
            return num;
        if (type == COMPLEX_DOUBLE)
            return cnum;
        if (type == INTEGER)
            return inum;
        if (type == BOOLEAN)
            return bnum;
        cerr << "Variable cannot be converted to a complex value: " << get_str() << endl;
        exit(1);
    }

    string as_string() const
    {
        if (type == STRING)
            return str;
        cerr << "Variable is not a string: " << get_str() << endl;
        exit(1);
        return 0;
    }

    string kind() const
    {
        if (type == DOUBLE)
            return "DOUBLE";
        if (type == COMPLEX_DOUBLE)
            return "COMPLEX_DOUBLE";
        if (type == INTEGER)
            return "INTEGER";
        if (type == BOOLEAN)
            return "BOOLEAN";
        if (type == STRING)
            return "STRING";
        if (type == LIST)
            return "LIST";
        if (type == NONE)
            return "NONE";

        return "UNDEFINED";
    }

    Variable &operator+=(const Variable &rhs)
    {
        num = as_double() + rhs.as_double();
        type = DOUBLE;
        return *this;
    }
    Variable &operator-=(const Variable &rhs)
    {
        num = as_double() - rhs.as_double();
        type = DOUBLE;
        return *this;
    }
    Variable &operator*=(const Variable &rhs)
    {
        num = as_double() * rhs.as_double();
        type = DOUBLE;
        return *this;
    }
    Variable &operator/=(const Variable &rhs)
    {
        num = as_double() / rhs.as_double();
        type = DOUBLE;
        return *this;
    }
    Variable &operator^=(const Variable &rhs)
    {
        num = pow(as_double(), rhs.as_double());
        type = DOUBLE;
        return *this;
    }
    Variable &operator=(const Variable &rhs) = default;
    operator string() const { return get_str(); }
};

/* Dummy circuit directive struct */
struct ParseDirective {
    ParseTree *parent;
    vector<Variable> args; // list of non-keyword arguments found on netlist
    map<string, Variable> kwargs; // list of kw arguments found on netlist

    ParseDirective()
    {}

    ParseDirective(const ParseDirective& other) = default;

    virtual void print() const
    {
        cout << "DIRECTIVE[" << kind() << "] ";
        cout << "{";
        for (const auto &x : args)
            cout << x.get_str() << " (" << x.kind() << "), ";
        if ( !args.empty() )
            cout << "\b\b";
        cout << "} ";
        cout << "{";
        for (const auto &x : kwargs)
            cout << x.first << " (" << x.second.kind() << ") = " << x.second.get_str() << ", ";
        if ( !kwargs.empty() )
            cout << "\b\b";
        cout << "}" << endl;
    }
    virtual string to_json() const;

    virtual ParseDirective *clone() const = 0;
    virtual void create() const = 0;
    virtual string kind() const { return "UNDEFINED"; }
    virtual ~ParseDirective() {}
};

/* Dummy circuit analysis struct */
struct ParseAnalysis {
    const ParseTree *parent;
    vector<Variable> args; // list of non-keyword arguments found on netlist
    map<string, Variable> kwargs; // list of kw arguments found on netlist

    ParseAnalysis()
    {}

    ParseAnalysis(const ParseAnalysis& other) = default;

    void print() const
    {
        cout << "ANALYSIS[" << kind() << "] ";
        cout << "{";
        for (const auto &x : args)
            cout << x.get_str() << " (" << x.kind() << "), ";
        if ( !args.empty() )
            cout << "\b\b";
        cout << "} ";
        cout << "{";
        for (const auto &x : kwargs)
            cout << x.first << " (" << x.second.kind() << ") = " << x.second.get_str() << ", ";
        if ( !kwargs.empty() )
            cout << "\b\b";
        cout << "}" << endl;
    }
    virtual string to_json() const;

    virtual ParseAnalysis *clone() const = 0;
    virtual void create() const = 0;
    virtual string kind() const { return "UNDEFINED"; }
    virtual ~ParseAnalysis() {}
};

/* SUBCKT: subcircuit definition */
struct ParseSubcircuit {
    string name; // name of the subcircuit
    string netlist; // subcircuit netlist (extracted from parent netlist)
    vector<string> ports; // name of ports
    map<string, Variable> kwargs; // list of kw arguments found on netlist
    const ParseTree *parent;

    ParseSubcircuit(const string &name, ParseTree *parent = nullptr)
    : name(name)
    , parent(parent)
    {}

    ParseSubcircuit(const ParseSubcircuit& other) = default;

    void register_port(const string &port_name)
    {
        auto it = find(ports.cbegin(), ports.cend(), port_name);
        if (it == ports.cend())
        {
            ports.push_back(port_name);
        }
        else
        {
            cerr << "Subcircuit cannot have duplicate port names";
            cerr << "(" << port_name << " was already defined)" << endl;
            exit(1);
        }
    }
    //ParseTree *instantiate(const string &instance_name, const map<string,Variable> &kwargs) const;
    void print() const
    {
        cout << name << " (" << kind() << ") ";
        for (const auto &x : ports)
            cout << x << " ";
        cout << "{";
        for (const auto &x : kwargs)
            cout << x.first << " (" << x.second.kind() << ") = " << x.second.get_str() << ", ";
        if ( !kwargs.empty() )
            cout << "\b\b";
        cout << "}" << endl;
        if (false)
        {
            cout << "###########################" << endl;
            cout << "## Start netlist of " << name << endl;
            cout << "###########################" << endl;
            cout << netlist;
            cout << "###########################" << endl;
            cout << "## End netlist of " << name << endl;
            cout << "###########################" << endl;
        }
    }
    string to_json() const;
    string kind() const
    { return "SUBCIRCUIT DEFINITION"; }
    ParseSubcircuit *clone() const
    { return new ParseSubcircuit(*this); }
};

/* Dummy circuit net struct (although close to what it should look like) */
struct ParseNet {
    /* Possible net types */
    enum Type {
        NONE,
        OANALOG,
        EANALOG,
        EDIGITAL,
    };

    /* Member variables */
    Type m_type;
    unsigned int m_size; // number of "wires (more than 1 for buses)
    bool m_bidirectional = false; // whether the signal should be bidirectional (for oanalog and eanalog representing forward/backward waves)
    unsigned int m_writers_count = 0; // number of writers
    unsigned int m_readers_count = 0; // number of readers
    unsigned int m_ports_count = 0; // number of individual ports connected (in total)
    unsigned int m_connect_count = 0; // number of ports actually connected (updated during creation phase only)
    unsigned int m_connect_writer_count = 0; // number of writer ports actually connected (updated during creation phase only)
    unsigned int m_connect_reader_count = 0; // number of reader ports actually connected (updated during creation phase only)

    /* Constructor */
    ParseNet(Type type = NONE, unsigned int size = 1): m_type(type), m_size(size) {}
    ParseNet(const ParseNet &n) = default;
    inline ParseNet& operator=(const ParseNet &n) = default;
    bool combine_with(const ParseNet &n)
    {
        if (m_type != n.m_type)
            return false;
        if (m_size != n.m_size)
            return false;
        if (m_connect_count || n.m_connect_count)
            return false;

        m_bidirectional |= n.m_bidirectional;
        m_writers_count += n.m_writers_count;
        m_readers_count += n.m_readers_count;
        m_ports_count += n.m_ports_count;

        return true;
    }

    /* const Getters */
    unsigned int size() const { return m_size; }
    Type type() const { return m_type; }
    bool bidirectional() const
    {
        return m_bidirectional || (m_writers_count == 2);
        //return m_bidirectional || (m_writers_count == 2) || (m_readers_count == 2);
    }
    bool validate() const
    {
        return m_writers_count <= 2 && m_ports_count <= 2;
    }

    /* setters */
    inline bool setType(Type type, bool force = false) {
        //cout << type_str() << " - " << ParseNet(type).type_str() << endl;
        if (m_type == type)
            // no need to do anything
            return true;
        if (m_type == NONE || force)
        {
            m_type = type;
            return true;
        }
        // cerr << "Incompatible net type assignment" << endl;
        return false;
    }

    string type_str() const
    {
        switch (m_type) {
        case Type::NONE:
            return "NONE";
        case Type::OANALOG:
            return "OANALOG";
        case Type::EANALOG:
            return "EANALOG";
        case Type::EDIGITAL:
            return "EDIGITAL";
        default:
            cerr << "Unexpected type" << endl;
            exit(1);
        }
    }

    string to_json() const;

    // create the net
    vector<shared_ptr<sc_object>> create(const string &name, bool force_bidir = false) const;
    shared_ptr<sc_object> create_uni(const string &name) const;

    /* Get a generic net name from net number (different from id) */
    static string name_from_id(int net_number);
};

struct ParseTreeCreationHelper;

/* Dummy circuit element struct (although close to )*/
struct ParseElement {
    typedef sc_module element_type_base;

    string name;
    const ParseTree *parent;
    vector<string> nets; // name of parent nets connected to element
    vector<Variable> args; // list of non-keyword arguments found on netlist
    map<string, Variable> kwargs; // list of kw arguments found on netlist

    const int n_nets = 0;
    const bool bidirectionalable = false;

    ParseElement(const string &name)
    : name(name)
    {}

    ParseElement(const string &name, const vector<string> &nets)
    : name(name)
    , nets(nets)
    {}

    ParseElement(const ParseElement& other) = default;

    void print() const
    {
        cout << name << " (" << kind() << ") ";
        for (const auto &x : nets)
            cout << x << " ";
        cout << "{";
        for (const auto &x : args)
            cout << x.get_str() << " (" << x.kind() << "), ";
        if ( !args.empty() )
            cout << "\b\b";
        cout << "} ";
        cout << "{";
        for (const auto &x : kwargs)
            cout << x.first << " (" << x.second.kind() << ") = " << x.second.get_str() << ", ";
        if ( !kwargs.empty() )
            cout << "\b\b";
        cout << "}" << endl;
    }

    virtual ParseElement *clone() const = 0;
    virtual sc_module *create(ParseTreeCreationHelper &pt_helper) const = 0;
    virtual element_type_base *instantiate_and_connect_uni(ParseTreeCreationHelper &pt_helper) const = 0;
    virtual element_type_base *instantiate_and_connect_bi(ParseTreeCreationHelper &pt_helper) const
    {
        (void) pt_helper;
        cerr << "Elements of type " << kind() << " cannot be bidirectional" << endl;
        exit(1);
    }

    virtual string kind() const { return "UNDEFINED"; }
    virtual bool bidir_capable() const { return false; }                            \

    virtual ~ParseElement() {}

    virtual string to_json() const;
};

struct ParseTree {
    string name;
    const ParseTree *parent = nullptr;
    bool is_subcircuit = false;
    size_t unnamed_net_count = 0;

    map<string, ParseNet> nets;
    vector<string> ports; // only for subcircuits
    vector<ParseElement *> elements;
    vector<ParseAnalysis *> analyses;
    vector<ParseDirective *> directives;
    vector<ParseSubcircuit *> subcircuits;
    map<string, Variable> local_assignments;
    static map<string, Variable> global_assignments;

    ParseTree(const string &name = "ROOT")
    : name(name)
    {}

    ParseTree(const string &name, const ParseSubcircuit &subcircuit, const map<string, Variable> &kwargs);

    int register_directive(ParseDirective *directive);
    int register_element(ParseElement *element);
    int register_analysis(ParseAnalysis *analysis);
    int register_subcircuit(string name);
    const ParseSubcircuit *find_subcircuit(const string &name) const;

    string get_or_create_net(string net_name, unsigned int size = 1,
                             ParseNet::Type type = ParseNet::NONE)
    {
        if (net_name == "_")
        {
            net_name = "_" + to_string(unnamed_net_count++);
        }

        string full_net_name = name_prefix() + net_name;
        if (nets.count(full_net_name) == 0)
        {
            nets.emplace(full_net_name, ParseNet(type, size));
            return full_net_name;
        }
        if (nets[full_net_name].size() != size)
        {
            cerr << "Incompatible size: " << net_name << "<" << size << ">"
                 << " (previously declared size was: " << nets[full_net_name].size()
                 << ")" << endl;
            exit(1);
        }
        if (nets[full_net_name].type() == ParseNet::NONE && type != ParseNet::NONE)
            nets[full_net_name].m_type = type;
        if (type == ParseNet::NONE && nets[full_net_name].type() != ParseNet::NONE)
            type = nets[full_net_name].m_type;
        if (nets[full_net_name].type() != type)
        {
            cerr << "Incompatible type: " << net_name << "[" << ParseNet(type).type_str() << "]"
                 << " (previously declared type was: " << nets[full_net_name].type_str()
                 << ")" << endl;
            exit(1);
        }
        return full_net_name;
    }

    string get_or_create_net(int i, unsigned int size = 1,
                             ParseNet::Type type = ParseNet::NONE)
    {
        return get_or_create_net(ParseNet::name_from_id(i), size, type);
    }
    string get_or_create_net(Variable v, unsigned int size = 1,
                             ParseNet::Type type = ParseNet::NONE)
    {
        if (v.type == Variable::STRING)
            return get_or_create_net(v.as_string(), size, type);
        else if (v.type == Variable::INTEGER)
            return get_or_create_net(v.as_integer(), size, type);
        else
        {
            cerr << "Incompatible variable type for creating a net" << endl;
            exit(1);
        }
    }

    string name_prefix() const
    {
        string ret = "";
        if ( !is_subcircuit && parent )
        {
            ret = parent->name_prefix();
            if ( name.empty() )
            {
                cerr << "Name of child parse tree should not be empty" << endl;
                exit(1);
            }
        }
        ret += name + "/";
        strutils::toupper(ret);
        return ret;
    }

    template <typename T>
    void set_local_variable(string &name, T val)
    {
        // will throw a compile error if type T is not supported by Variable
        local_assignments[name] = val;
    }

    template <typename T>
    void set_global_variable(string &name, T val)
    {
        // will throw a compile error if type T is not supported by Variable
        global_assignments[name] = val;
    }

    Variable get_variable(string &name)
    {
        // try to find variable in locals table first
        auto it = local_assignments.find(name);
        if (it != local_assignments.end())
            return it->second;

        // try to find variable in globals table
        it = global_assignments.find(name);
        if (it != global_assignments.end())
            return it->second;

        // error
        cerr << "Variable not found: " << name << endl;
        exit(1);
    }

    void print() const;
    string to_json() const;

    ParseTree *clone() const
    {
        ParseTree *clone = new ParseTree(name);

        clone->parent = nullptr;
        clone->ports = ports;
        clone->nets = nets;
        clone->local_assignments = local_assignments;
        clone->unnamed_net_count = unnamed_net_count;

        for (const auto &x : elements)
            clone->elements.push_back(x->clone());
        for (const auto &x : analyses)
            clone->analyses.push_back(x->clone());
        for (const auto &x : directives)
            clone->directives.push_back(x->clone());
        for (const auto &x : subcircuits)
            clone->subcircuits.push_back(x->clone());

        return clone;
    }
    void build_circuit();
    void flatten();

    ~ParseTree()
    {
        if (!parent)
        {
            for (const auto &x : elements)
                if(x) delete x;
            for (const auto &x : analyses)
                if(x) delete x;
            for (const auto &x : directives)
                if(x) delete x;
            for (const auto &x : subcircuits)
                if(x) delete x;
        }
    }
};

struct ParseTreeCreationHelper {
    ParseTree *pt;

    // backlog: elements to be created in priority
    map<string, ParseNet> *nets;

    // backlog: elements to be created in priority
    set<const ParseElement *> elements_backlog;

    // Circuit elements generated by build
    map<string, vector<shared_ptr<sc_object>>> circuit_signals;
    map<string, shared_ptr<sc_module>> circuit_modules;

    ParseTreeCreationHelper()
    : pt(nullptr)
    {}

    ParseTreeCreationHelper(ParseTree *parse_tree)
    {
        resetParseTree(parse_tree);
    }

    void resetParseTree(ParseTree *parse_tree)
    {
        pt = parse_tree;
        nets = &parse_tree->nets;
        elements_backlog.clear();
        circuit_signals.clear();
        circuit_modules.clear();
    }

    // return next bidir net which doesn't have a corresponding signal in circuit_signals
    map<string, ParseNet>::iterator next_fresh_bidir_net();

    // return next net which doesn't have a corresponding signal in circuit_signals
    map<string, ParseNet>::iterator next_fresh_net();

    // return next element which is connected to "net_name" and doesnt have a corresponding module in circuit_modules
    vector<ParseElement *>::iterator next_fresh_element_bound_to(const string &net_name, const set<const ParseElement *> &excludes = {});

    // upgrade a signal to bidirectional
    void upgrade_signal(const string &name);

    // Create signals connected to a given element
    void create_signals(const ParseElement *elem);

    // Connect a port to a signal using the net name
    template <typename signal_type, typename port_type>
    void connect_uni(port_type &port, const string& net_name, bool as_writer);

    // Connect a bidirectional port to a signal pair using the net name
    template <typename signal_type, typename port_in_type, typename port_out_type>
    void connect_bi(port_in_type &p_in, port_out_type &p_out, const string& net_name);

    ~ParseTreeCreationHelper()
    {}
};

/** ******************************************* **/
/**  Non-specialized template implementations   **/
/** ******************************************* **/

template <typename signal_type, typename port_type>
void ParseTreeCreationHelper::connect_uni(port_type &port, const string& net_name, bool as_writer)
{
    assert(pt->nets.find(net_name) != pt->nets.end());
    const auto &net = pt->nets.at(net_name);

    // Get the signal to be written or read
    sc_object *sig_raw;
    signal_type *sig = nullptr;
    if ( !net.bidirectional())
    {
        sig_raw = circuit_signals.at(net_name)[0].get();
        sig = dynamic_cast<signal_type *>(sig_raw);
    }
    else
    {
        // verify no more than 2 writer in total
        if(pt->nets.at(net_name).m_connect_writer_count + as_writer > 2)
        {
            cerr << "Attempted to add a third writer to a bidirectional net ("
                 << net_name << ")." << endl;
            exit(2);
        }

        int i_write = pt->nets.at(net_name).m_connect_writer_count;
        i_write = max(i_write, 1);// If there are already 2 writers, act as a reader for the second one (imitate the second writer but only as a reader)
        int i_read = (i_write + 1) % 2;
        int i = as_writer ? i_write : i_read;

        sig_raw = circuit_signals.at(net_name)[i].get();
        sig = dynamic_cast<signal_type *>(sig_raw);
    }

    // Check corresponding signal could be found
    if (!sig_raw)
    {
        cerr << "Missing object for signal connected to "
                << net_name << endl;
        exit(1);
    }

    // Check it could correctly be cast to requested type
    if (!sig)
    {
        cerr << "Wrong signal type for " << sig << " connected to "
             << net_name << endl;
        exit(1);
    }

    // Connect the port to the signal
    port.bind(*sig);

    // Update connection count of the net
    pt->nets[net_name].m_connect_count++;
    if (as_writer)
        pt->nets[net_name].m_connect_writer_count++;
    else
        pt->nets[net_name].m_connect_reader_count++;
}

template <typename signal_type, typename port_in_type, typename port_out_type>
void ParseTreeCreationHelper::connect_bi(port_in_type &p_in, port_out_type &p_out, const string& net_name)
{
    assert(pt->nets.find(net_name) != pt->nets.end());
    const auto &net = pt->nets.at(net_name);

    if (net.bidirectional())
    {
        // verify no more than 2 writer in total
        if(pt->nets.at(net_name).m_connect_writer_count >= 2)
        {
            cerr << "Attempted to add a third writer to a bidirectional net ("
                 << net_name << ")." << endl;
            exit(1);
        }

        // First get the signal to be written or read (in the right order)
        int i_write = pt->nets.at(net_name).m_connect_writer_count;
        int i_read = (i_write + 1) % 2;

        auto sig_writeable_raw = circuit_signals.at(net_name)[i_write].get();
        auto sig_readable_raw = circuit_signals.at(net_name)[i_read].get();

        // Check corresponding signals could be found
        if (!sig_writeable_raw)
        {
            cerr << "Missing object for output signal connected to "
                 << net_name << endl;
            exit(1);
        }
        if (!sig_readable_raw)
        {
            cerr << "Missing object for input signal connected to "
                 << net_name << endl;
            exit(1);
        }

        auto sig_writeable = dynamic_cast<signal_type *>(sig_writeable_raw);
        auto sig_readable = dynamic_cast<signal_type *>(sig_readable_raw);

        // Check they could correctly be cast to requested type
        if (!sig_writeable)
        {
            cerr << "Wrong signal type for " << sig_writeable << " connected to "
                 << net_name << endl;
            exit(1);
        }
        if (!sig_readable)
        {
            cerr << "Wrong signal type for " << sig_readable << " connected to "
                 << net_name << endl;
            exit(1);
        }

        // Connect ports to both signals
        p_out.bind(*sig_writeable);
        p_in.bind(*sig_readable);

        // Update connection count of the net
        pt->nets[net_name].m_connect_count++;
        pt->nets[net_name].m_connect_writer_count++;
        pt->nets[net_name].m_connect_reader_count++;
    }
    else
    {
        cerr << "Expected a bidirectional net for " << net_name << endl;
        exit(1);
    }
}