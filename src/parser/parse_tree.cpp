#include "parse_tree.h"
#include "parse_directive.h"
#include "parse_element.h"
#include "parser_state.h"
#include "specs.h"

#include <sstream>
#include <iomanip>

using std::ostringstream;

using spx::oa_value_type;
using spx::ea_value_type;
using spx::ed_value_type;

using spx::ed_bus_type;
using spx::oa_signal_type;
using spx::ea_signal_type;
using spx::ed_signal_type;
using spx::ed_bus_type;

#include "../build/parser/parser.tab.h"
#include "../build/parser/parser.yy.h"

vector<shared_ptr<sc_object>> ParseNet::create(const string &name, bool force_bidir) const
{
    if ( !bidirectional() && !force_bidir )
        return { create_uni(name) };
    else
    {
        assert(type() == OANALOG && "Only optical nets can be bidirectional for now");
        return { create_uni((name + "_0").c_str()), create_uni((name + "_1").c_str()) };
    }
}

shared_ptr<sc_object> ParseNet::create_uni(const string &name) const
{
    switch (m_type)
    {
        case OANALOG:
            return { make_shared<spx::oa_signal_type>(name.c_str()) };
        case EANALOG:
            return { make_shared<spx::ea_signal_type>(name.c_str()) };
        case EDIGITAL:
            if (m_size == 1)
                return { make_shared<spx::ed_signal_type>(name.c_str()) };
            else
                return { make_shared<spx::ed_bus_type>(name.c_str(), sc_lv_base(m_size)) };
        default:
            cerr << "Unknown net type: " << name << endl;
            exit(1);
    }
    return { nullptr };
}

string ParseNet::name_from_id(int id)
{
    assert(id >= 0);
    ostringstream ss;
    ss << "N" << std::setfill('0') << std::setw(5) << id;
    return ss.str();
}

// return next bidir net which doesn't have a corresponding signal in circuit_signals
map<string, ParseNet>::iterator ParseTreeCreationHelper::next_fresh_bidir_net()
{
    for (auto it = pt->nets.begin(); it != pt->nets.end(); ++it)
    {
        const auto &net = *it;

        // check if net is bidirectional
        if (!net.second.bidirectional())
            continue;

        // check if net has a corresponding signal instanciated
        auto it_sig = find_if(circuit_signals.cbegin(), circuit_signals.cend(), [&it](const auto &p)
        { return p.first == it->first; }
        );
        if (it_sig != circuit_signals.cend())
            continue;

        // check if net is unbound
        auto it_elem = next_fresh_element_bound_to(it->first);
        if (it_elem == pt->elements.cend())
            continue;

        return it;
    }
    return pt->nets.end();
}

// return next bidir net which doesn't have a corresponding signal in circuit_signals
map<string, ParseNet>::iterator ParseTreeCreationHelper::next_fresh_net()
{
    for (auto it = pt->nets.begin(); it != pt->nets.end(); ++it)
    {
        // check if net has a corresponding signal
        auto it_sig = find_if(circuit_signals.cbegin(), circuit_signals.cend(), [&it](const auto &p)
        { return p.first == it->first; }
        );
        if (it_sig != circuit_signals.cend())
            continue;

        // check if net is unbound
        auto it_elem = next_fresh_element_bound_to(it->first);
        if (it_elem == pt->elements.cend())
            continue;

        return it;
    }
    return pt->nets.end();
}

// return next element which is connected to "net_name" and doesnt have a corresponding
// module in circuit_modules and isn't already in the backlog
vector<ParseElement *>::iterator ParseTreeCreationHelper::next_fresh_element_bound_to(const string &net_name, const set<const ParseElement *> &excludes)
{
    // Loop over elements
    for (auto it = pt->elements.begin(); it != pt->elements.end(); ++it)
    {
        const auto &elem = *it;

        // Check if element is part of exlude set
        if (excludes.find(elem) != excludes.cend())
            continue;

        // Check if element is part of backlog set
        if (elements_backlog.find(elem) != elements_backlog.cend())
            continue;

        // Check if net_name is in element nets
        if (find(elem->nets.cbegin(), elem->nets.cend(), net_name) == elem->nets.cend())
            continue;

        // Check if element has a corresponding module instantiated
        auto it_mod = find_if(circuit_modules.cbegin(), circuit_modules.cend(), [&it](const auto &p)
        { return p.first == (*it)->name; }
        );

        // if not, return current iterator `it`
        if (it_mod == circuit_modules.cend())
            return it;
    }
    return pt->elements.end();
}

void ParseTreeCreationHelper::create_signals(const ParseElement *elem)
{
    // element should be in the parsetree
    assert(find(pt->elements.begin(), pt->elements.end(), elem) != pt->elements.end());

    // for all nets connected to elements
    for (const auto &net : elem->nets)
    {
        // find whether it has already been instantiated
        if (circuit_signals.find(net) == circuit_signals.end())
        {
            // if not, do it
            circuit_signals.emplace(net, pt->nets.at(net).create(net));

            // verify the net had no connection (it would be a bug otherwise,
            // since it hadn't been instantiated)
            assert(pt->nets.at(net).m_connect_count == 0);

            // find all elements connected to net and add them to the elements backlog
            // FIXME: potential bug here if an element can have both bidir and unidir nets
            auto it = next_fresh_element_bound_to(net, {elem});
            while (it != pt->elements.end())
            {
                elements_backlog.insert(*it);
                it = next_fresh_element_bound_to(net, {elem});
            }
        }
    }
}

void ParseTreeCreationHelper::upgrade_signal(const string &net_name)
{
    // Upgrade even if: net.bidirectional() is false

    // The signal should exist because it has been created by pt_helper.create_signals()
    // but it doesn't hurt to check once more time
    assert(circuit_signals.find(net_name) != circuit_signals.end());

    // Set the bidirectional flag to true
    pt->nets[net_name].m_bidirectional = true;

    // Check if the net was created as unidirectional (only one signal)
    if (circuit_signals.at(net_name).size() == 1)
    {
        // if the existing signal is unidirectional

        // check if a port is already connected to the signal
        if (pt->nets[net_name].m_connect_count)
        {
            // if yes, that means a port is already connected to the signal
            // we cannot easily re-instantiate it as it may be bound to a port;
            // instead, we add a second signal to the vector to make it bidirectional
            circuit_signals[net_name].push_back(pt->nets[net_name].create_uni(net_name + "_1"));

            // For bidirectional signals, first port to connect binds to signal_0
            // for writing and signal_1 for reading
            // Therefore we have to "re-create" this scenario, as if the first device
            // connected to a bidirectional net. But for this we need to know which
            // if the first device was a writer or a reader.

            // first determine if the connected port was writing or reading the signal
            bool has_writer = pt->nets[net_name].m_connect_writer_count;
            bool has_reader = pt->nets[net_name].m_connect_reader_count;

            // only one should be true since the net was unidirectional
            assert(has_writer ^ has_reader);

            // if a writer, then it's as expected
            if (has_writer)
            {
                // if a writer, then it's as expected:
                // the next port will connect to signal_1 for writing
                // we just need to update the number of "readers" connected to the net
                pt->nets[net_name].m_readers_count++;
            }
            else
            {
                // if a writer, then the next port needs to reverse its order.
                // for this we swap signal_0 and signal_1
                swap(circuit_signals[net_name][0], circuit_signals[net_name][1]);
                // then we need to update the number of "writers" connected to the net
                pt->nets[net_name].m_writers_count++;
            }
        }
        else
        {
            // otherwise, we can recreate the first signal as well
            circuit_signals[net_name].clear(); // will delete the net through the constructor
            circuit_signals[net_name] = pt->nets.at(net_name).create(net_name);
        }
    }
}

string ParseElement::to_json() const
{
    // name
    // kind()
    // nets
    // args
    // kwargs

    stringstream ss;
    ss << "{";
    ss << "\"name\":" << '"' << name << '"';
    ss << ',';
    ss << "\"type\":" << '"' << kind() << '"';
    ss << ',';
    ss << "\"nets\":";
    {
        ss << '[';
        for (size_t i = 0; i < nets.size(); ++i)
        {
            ss << '"' << nets[i] << '"';
            if (i != nets.size() - 1)
                ss << ", ";
        }
        ss << ']';
    }
    ss << ',';
    ss << "\"args\":";
    {
        ss << '[';
        for (size_t i = 0; i < args.size(); ++i)
        {
            ss << args[i].to_json();
            if (i != args.size() - 1)
                ss << ", ";
        }
        ss << ']';
    }
    ss << ',';
    ss << "\"kwargs\":";
    {
        ss << '{';
        for (auto it = kwargs.begin(); it != kwargs.end(); ++it)
        {
            if (it != kwargs.begin())
                ss << ',';
            ss << '"' << it->first << '"' << ':';
            ss << it->second.to_json();
        }
        ss << '}';
    }
    ss << '}';
    return ss.str();
}

ParseTree::ParseTree(const string &name, const ParseSubcircuit &subcircuit, const map<string, Variable> &kwargs)
: ParseTree(name)
{
    parent = subcircuit.parent;
    is_subcircuit = true;

    // take kwargs as local assignments
    local_assignments = subcircuit.kwargs;

    for (const auto &p : kwargs)
    {
        if (local_assignments.count(p.first) == 0)
        {
            cerr << name << ": Unknown subcircuit parameter " << p.first << endl;
            exit(1);
        }
        local_assignments[p.first] = p.second;
    }

    yyscan_t scanner;
    ParserState *parser_state = new ParserState;
    YY_BUFFER_STATE buf;

    yylex_init_extra(parser_state, &scanner);
    buf = yy_scan_string(subcircuit.netlist.c_str(), scanner);
    yy_switch_to_buffer(buf, scanner);

    int parsing_result = yyparse(scanner, this);

    //yy_delete_buffer(buf, scanner);
    yylex_destroy(scanner);
    delete parser_state;

    // Return if unsuccessful
    if (parsing_result != 0) {
        cerr << name << ": failed parsing subcircuit netlist" << endl;
        exit(1);
    }

    if (directives.size() > 0)
    {
        cerr << name << ": found a directive in subcircuit, ignoring." << endl;
        directives.clear();
    }

    if (analyses.size() > 0)
    {
        cerr << name << ": found an analysis in subcircuit, ignoring." << endl;
        analyses.clear();
    }
    // print();
}

int ParseTree::register_directive(ParseDirective *directive)
{
    directive->parent = this;
    directives.push_back(directive);
    // cout << "Created directive " << directive->kind() << endl;
    // cout << "current number of directives: " << directives.size() << endl;
    return directives.size() - 1;

}

int ParseTree::register_element(ParseElement *element)
{
    element->name = name_prefix() + element->name;
    strutils::toupper(element->name);
    element->parent = this;
    elements.push_back(element);
    // cout << "Created element " << element->name << endl;
    // cout << "current number of elements: " << elements.size() << endl;
    return elements.size() - 1;
}

int ParseTree::register_analysis(ParseAnalysis *analysis)
{
    if ( !analyses.empty() )
    {
        cerr << "Attempted to register a new analysis (" << analysis->kind() << ")" << endl;
        cerr << "But another one was already specified (" << analyses[0]->kind() << ")" << endl;
        exit(1);
    }
    analysis->parent = this;
    analyses.push_back(analysis);
    // cout << "Created analysis " << analysis->kind() << endl;
    // cout << "current number of analysis: " << analyses.size() << endl;
    return analyses.size() - 1;
}

int ParseTree::register_subcircuit(string name)
{
    strutils::toupper(name);
    //name = name_prefix() + name;

    auto it = find_if(subcircuits.cbegin(), subcircuits.cend(),
        [&name](const ParseSubcircuit *p){ return p->name == name;});
    if (it != subcircuits.cend())
    {
        cerr << "Attempted to register a new subcircuit with name " << name << endl;
        cerr << "But another subcircuit with this name already exists" << endl;
        exit(1);
    }

    subcircuits.push_back(new ParseSubcircuit(name, this));
    int i = subcircuits.size() - 1;

    // TODO: Copy variables definitions ?

    // cout << "Created subcircuit " << subcircuits[i]->name << endl;
    // cout << "(current number of subcircuits: " << subcircuits.size() << ")" << endl;

    return i;
}

const ParseSubcircuit *ParseTree::find_subcircuit(const string &name) const
{
    auto pred = [&name](const ParseSubcircuit *p) {
        return p->name == name;
    };
    auto it = find_if(subcircuits.begin(), subcircuits.end(), pred);
    if (it != subcircuits.end())
        return *it;

    cerr << "Subcircuit definition not found: " << name << endl;
    exit(1);
}

void ParseTree::print() const
{
    cout << "-----------" << endl;
    cout << global_assignments.size() << " global assignments" << endl;
    for (const auto &x : global_assignments)
        cout << " - " << x.first << " (" << x.second.kind()
             << ") = " << x.second.get_str() << endl;

    cout << "-----------" << endl;
    cout << local_assignments.size() << " local assignments" << endl;
    for (const auto &x : local_assignments)
        cout << " - " << x.first << " (" << x.second.kind()
             << ") = " << x.second.get_str() << endl;

    cout << "-----------" << endl;
    cout << elements.size() << " elements" << endl;
    for (const auto &x : elements)
    {
        cout << " - ";
        x->print();
    }

    cout << "-----------" << endl;
    cout << nets.size() << " named nets" << endl;
    for (const auto &x : nets)
        cout << " - " << x.first << ": " << x.second.type_str()
        << "<" << x.second.size() << ">"
        << (x.second.bidirectional() ? "b" : "u")
        << " (" << x.second.m_writers_count << "|" << x.second.m_readers_count << ")"
        << endl;

    cout << "-----------" << endl;
    cout << directives.size() << " directives" << endl;
    for (const auto &x : directives)
    {
        cout << " - ";
        x->print();
    }

    cout << "-----------" << endl;
    cout << subcircuits.size() << " subcircuits" << endl;
    for (const auto &x : subcircuits)
    {
        cout << " - ";
        x->print();
    }

    cout << "-----------" << endl;
    cout << analyses.size() << " analyses" << endl;
    for (const auto &x : analyses)
    {
        cout << " - ";
        x->print();
    }
}

string ParseDirective::to_json() const
{
    stringstream ss;
    ss << "{";
    ss << "\"type\":" << '"' << kind() << '"';
    ss << ',';
    ss << "\"args\":";
    {
        ss << '[';
        for (size_t i = 0; i < args.size(); ++i)
        {
            ss << args[i].to_json();
            if (i != args.size() - 1)
                ss << ", ";
        }
        ss << ']';
    }
    ss << ',';
    ss << "\"kwargs\":";
    {
        ss << '{';
        for (auto it = kwargs.begin(); it != kwargs.end(); ++it)
        {
            if (it != kwargs.begin())
                ss << ',';
            ss << '"' << it->first << '"' << ':';
            ss << it->second.to_json();
        }
        ss << '}';
    }
    ss << '}';
    return ss.str();
}

string ParseAnalysis::to_json() const
{
    stringstream ss;
    ss << "{";
    ss << "\"type\":" << '"' << kind() << '"';
    ss << ',';
    ss << "\"args\":";
    {
        ss << '[';
        for (size_t i = 0; i < args.size(); ++i)
        {
            ss << args[i].to_json();
            if (i != args.size() - 1)
                ss << ", ";
        }
        ss << ']';
    }
    ss << ',';
    ss << "\"kwargs\":";
    {
        ss << '{';
        for (auto it = kwargs.begin(); it != kwargs.end(); ++it)
        {
            if (it != kwargs.begin())
                ss << ',';
            ss << '"' << it->first << '"' << ':';
            ss << it->second.to_json();
        }
        ss << '}';
    }
    ss << '}';
    return ss.str();
}

string ParseSubcircuit::to_json() const
{
    stringstream ss;
    ss << "TODO";
    return ss.str();
}

string ParseNet::to_json() const
{
    stringstream ss;
    ss << "{";
    ss << "\"type\":" << '"' << type_str() << '"';
    ss << ',';
    ss << "\"size\":" << m_size;
    ss << ',';
    ss << "\"bidirectional\":" << bidirectional();
    ss << ',';
    ss << "\"readers\":" << m_readers_count;
    ss << ',';
    ss << "\"writers\":" << m_readers_count;
    ss << '}';
    return ss.str();
}

string ParseTree::to_json() const
{
    stringstream ss;
    ss << "{";
    ss << "\"name\":" << '"' << name << '"';
    ss << ',';
    ss << "\"parent\":";
    if (parent)
        ss << '"' << parent->name << '"';
    else
        ss << "null";
    ss << ',';
    ss << "\"is_subcircuit\":" << (is_subcircuit ? "true" : "false");
    ss << ',';
    ss << "\"unnamed_nets\":" << unnamed_net_count;
    ss << ',';
    ss << "\"nets\":";
    {
        ss << '{';
        for (auto it = nets.begin(); it != nets.end(); ++it)
        {
            if (it != nets.begin())
                ss << ',';
            ss << '"' << it->first << '"' << ':';
            ss << it->second.to_json();
        }
        ss << '}';
    }
    ss << ',';
    ss << "\"elements\":";
    {
        ss << '[';
        for (size_t i = 0; i < elements.size(); ++i)
        {
            ss << elements[i]->to_json();
            if (i != elements.size() - 1)
                ss << ", ";
        }
        ss << ']';
    }
    ss << ',';
    ss << "\"directives\":";
    {
        ss << '[';
        for (size_t i = 0; i < directives.size(); ++i)
        {
            ss << directives[i]->to_json();
            if (i != directives.size() - 1)
                ss << ", ";
        }
        ss << ']';
    }
    ss << ',';
    ss << "\"analyses\":";
    {
        ss << '[';
        for (size_t i = 0; i < analyses.size(); ++i)
        {
            ss << analyses[i]->to_json();
            if (i != analyses.size() - 1)
                ss << ", ";
        }
        ss << ']';
    }
    ss << ',';
    ss << "\"subcircuits\":";
    {
        ss << "\"TODO\"";
        if (false)
        {
            ss << '[';
            for (size_t i = 0; i < subcircuits.size(); ++i)
            {
                ss << subcircuits[i]->to_json();
                if (i != subcircuits.size() - 1)
                    ss << ", ";
            }
            ss << ']';
        }
    }
    ss << ',';
    ss << "\"local_variables\":";
    {
        ss << '{';
        for (auto it = local_assignments.begin(); it != local_assignments.end(); ++it)
        {
            if (it != local_assignments.begin())
                ss << ',';
            ss << '"' << it->first << '"' << ':';
            ss << it->second.to_json();
        }
        ss << '}';
    }
    ss << ',';
    ss << "\"global_variables\":";
    {
        ss << '{';
        for (auto it = global_assignments.begin(); it != global_assignments.end(); ++it)
        {
            if (it != global_assignments.begin())
                ss << ',';
            ss << '"' << it->first << '"' << ':';
            ss << it->second.to_json();
        }
        ss << '}';
    }
    ss << '}';
    return ss.str();
}

void ParseTree::build_circuit()
{
    cout << "Flattening..." << endl;
    // flatten current parse tree by expanding subcircuits
    flatten();
    cout << "Done (flattening)" << endl;

    auto pt_helper = ParseTreeCreationHelper(this);

    auto &elements_backlog = pt_helper.elements_backlog;
    auto &circuit_signals = pt_helper.circuit_signals;
    auto &circuit_modules = pt_helper.circuit_modules;

    // Find first bidirectional net in nets that is not in circuit_nets
    auto next_net = pt_helper.next_fresh_bidir_net();
    while(next_net != nets.end())
    {
        cout << "Elaborating network of " << next_net->first << "..." << endl;

        // Find elements which are connected
        auto elem = pt_helper.next_fresh_element_bound_to(next_net->first);
        while (elem != elements.end())
        {
            cout << "Creating " << (*elem)->name << " (reason: connection to bidir net)" << endl;
            auto mod = (*elem)->create(pt_helper);
            cout << "Done creating " << (*elem)->name << endl;
            if (mod->name() != (*elem)->name)
            {
                cerr << "Error: a module with name '" << (*elem)->name << "' already exists or";
                cerr << " it was renamed due to a bad naming." << endl;
                cerr << "Make sure there is no naming conflict in the netlist" << endl;
                exit(1);
            }
            circuit_modules.emplace(mod->name(), mod);

            // Process backlog elements (elements that were connected to the nets)
            while ( !elements_backlog.empty() )
            {
                auto it = elements_backlog.begin();
                auto &backlog_elem = *it;
                cout << "Creating " << backlog_elem->name << " (reason: backlog)" << endl;
                auto mod = backlog_elem->create(pt_helper);
                cout << "Done creating " << backlog_elem->name << endl;

                if (mod->name() != (*it)->name)
                {
                    cerr << "Error: a module with name '" << (*it)->name << "' already exists or";
                    cerr << " it was renamed due to a bad naming." << endl;
                    cerr << "Make sure there is no naming conflict in the netlist" << endl;
                    exit(1);
                }

                circuit_modules.emplace(mod->name(), mod);
                elements_backlog.erase(*it);
            }
            elem = pt_helper.next_fresh_element_bound_to(next_net->first);
        }

        cout << "Done (elaborating network of " << next_net->first << ")" << endl;

        // At this point all elements connected to the bidirectional net have been created
        // move on to the next one
        next_net = pt_helper.next_fresh_bidir_net();
    }
    cout << "Done with bidirectional nets" << endl;
    // At this point all bidirectional nets have been created, and all devices
    // connected to them as well we can proceed with unidirectional nets
    next_net = pt_helper.next_fresh_net();
    while(next_net != nets.end())
    {
        cout << "Elaborating network of " << next_net->first << "..." << endl;

        // Find elements which are connected
        auto elem = pt_helper.next_fresh_element_bound_to(next_net->first);
        while (elem != elements.end())
        {
            cout << "Creating " << (*elem)->name << " (reason: connection to unidir net)" << endl;
            auto mod = (*elem)->create(pt_helper);
            cout << "Done creating " << (*elem)->name << endl;
            if (mod->name() != (*elem)->name)
            {
                cerr << "Error: a module with name '" << (*elem)->name << "' already exists or";
                cerr << " it was renamed due to a bad naming." << endl;
                cerr << "Make sure there is no naming conflict in the netlist" << endl;
                exit(1);
            }
            circuit_modules.emplace(mod->name(), mod);

            // Process backlog elements (elements that were connected to the nets)
            while ( !elements_backlog.empty() )
            {
                auto it = elements_backlog.begin();
                auto &backlog_elem = *it;
                cout << "Creating " << backlog_elem->name << " (reason: backlog)" << endl;
                auto mod = backlog_elem->create(pt_helper);
                cout << "Done creating " << backlog_elem->name << endl;

                if (mod->name() != (*it)->name)
                {
                    cerr << "Error: a module with name '" << (*it)->name << "' already exists or";
                    cerr << " it was renamed due to a bad naming." << endl;
                    cerr << "Make sure there is no naming conflict in the netlist" << endl;
                    exit(1);
                }

                circuit_modules.emplace(mod->name(), mod);
                elements_backlog.erase(*it);
            }
            elem = pt_helper.next_fresh_element_bound_to(next_net->first);
        }

        cout << "Done (elaborating network of " << next_net->first << ")" << endl;

        // At this point all elements connected to the bidirectional net have been created
        // move on to the next one
        next_net = pt_helper.next_fresh_net();
    }
    for (const auto &x : directives)
        x->create();

    if (! analyses.empty())
        analyses.at(0)->create();

    // Add all nets and elements to SPECSGlobalConfig
    for (const auto &x: circuit_modules)
        specsGlobalConfig.register_object(x.second);
    for (const auto &x: circuit_signals)
        for (const auto &sig: x.second)
            specsGlobalConfig.register_object(sig);
}

void ParseTree::flatten()
{
    cout << "Flattening " << name << endl;

    vector<ParseElement *> elements_flat;
    map<string, ParseNet> nets_flat = nets;

    // First, find all subcircuit instances (X elements) and ingest their parse-tree
    // (flattening the global parse-tree)
    for (auto &elem : elements)
    {
        // try to cast as XElement
        XElement *xelem = dynamic_cast<XElement *>(elem);

        // if unsuccessful, it means the device doesnt need to be flattened
        if (!xelem)
        {
            elements_flat.push_back(elem);
            continue;
        }

        // get subcircuit name
        string subcircuit_name = xelem->args.back().as_string();
        strutils::toupper(subcircuit_name);

        // get subcircuit definition in element's parent
        // TODO: figure out if we want global subcircuits
        auto subcircuit = xelem->parent->find_subcircuit(subcircuit_name);

        // parse the subcircuit netlist into a new parse tree
        ParseTree sub_pt(xelem->name, *subcircuit, xelem->kwargs);

        // flatten it
        sub_pt.flatten();

        // Check the number of nets (in args)
        if(xelem->args.size() - 1 != subcircuit->ports.size())
        {
            cerr << "Wrong number of nets for subcircuit instance: ";
            cerr << xelem->name << endl;
            exit(1);
        }

        // Compute net_name equivalence between internal and external nets
        map<string, string> net_names_translations;
        for (size_t i = 0; i < subcircuit->ports.size(); ++i)
        {
            Variable v = xelem->args[i]; // naming from Xnn ... line
            string internal_net_name = sub_pt.name_prefix() + subcircuit->ports[i]; // naming from .SUBCKT line
            string external_net_name = "";

            // build full exposed net_name
            string net_base;

            // extract from Variable (either string or integer)
            // normally parser should always transform it to a string variable
            // for us
            if (v.type == Variable::STRING)
                net_base = v.as_string();
            else if (v.type == Variable::INTEGER)
                net_base = ParseNet::name_from_id(v.as_integer());
            else
            {
                cerr << "Incompatible net name for " << xelem->name << endl;
                exit(1);
            }

            // if a single underscore was given, use the unnamed net count to
            // define a unique net
            // FIXME: this way is not correct; it should really increment the
            // parent's unnamed net count; I think it would be acceptable to
            // discard const for this, as the user specifically doesn't care about
            // the actual name of unnamed nets.
            if (net_base == "_")
                net_base = "_" + to_string(this->unnamed_net_count++);

            // add current prefix
            external_net_name = name_prefix() + net_base;

            // see if the net was already defined (was used by another device)
            if (nets.find(external_net_name) != nets.end())
            {
                // if so, combined the previously defined net with the internal one
                bool ok = sub_pt.nets[internal_net_name].combine_with(nets.at(external_net_name));
                if (!ok)
                {
                    cerr << "Could not reconcile nets: " << internal_net_name;
                    cerr << " and " << external_net_name << endl;
                    cerr << "During flattening of " << xelem->name << endl;
                    exit(1);
                }
            }
            // Add the (potentially combined) net to pt_helper nets
            nets_flat[external_net_name] = sub_pt.nets.at(internal_net_name);

            // Update the net name translations list
            net_names_translations[internal_net_name] = external_net_name;

            // Erase the internal net_name (replaced by the global net name)
            sub_pt.nets.erase(internal_net_name);
        } // for (size_t i = 0; i < subcircuit->ports.size(); ++i)

        // show result of translation
        if (false)
            for (const auto &p : net_names_translations)
                cout << p.first << " <==> " << p.second << endl;

        // translate internal net names by modifying the elements within the
        // subcircuit which are bound to them
        for (auto subelem : sub_pt.elements)
        {
            // loop over translation, and update if necessary
            for (const auto &p : net_names_translations)
            {
                auto it = find(subelem->nets.begin(), subelem->nets.end(), p.first);
                if (it == subelem->nets.end())
                    continue;
                *it = p.second;
            }
        } // for (auto subelem : sub_pt.elements)

        // insert the (globally named) internal-only nets into flattened netlist
        nets_flat.insert(sub_pt.nets.cbegin(), sub_pt.nets.cend());
        // insert the (potentially updated) elements to flattened netlist
        elements_flat.insert(elements_flat.end(), sub_pt.elements.cbegin(), sub_pt.elements.cend());

        // finally, free the memory of the elem object
        // note that the pointer itself is still in the elements vector!
        // we cannot remove it due to the for loop
        delete elem;
        elem = nullptr;
    } // for (auto elem : elements)

    // Print new list of elements and nets
    if (false)
    {
        cout << "-----------" << endl;
        cout << elements_flat.size() << " elements" << endl;
        for (const auto &x : elements_flat)
        {
            cout << " - ";
            x->print();
        }

        cout << "-----------" << endl;
        cout << nets_flat.size() << " named nets" << endl;
        for (const auto &x : nets_flat)
            cout << " - " << x.first << ": " << x.second.type_str()
            << "<" << x.second.size() << ">"
            << (x.second.bidirectional() ? "b" : "u")
            << " (" << x.second.m_writers_count << "|" << x.second.m_readers_count << ")"
            << endl;
    }

    // move to flattened nets and elements
    nets = nets_flat;
    elements = elements_flat;

    return;
}

map<string, Variable> ParseTree::global_assignments =
{
    { "pi", Variable(M_PI) },
    { "c",  Variable(299792458.0) },
};