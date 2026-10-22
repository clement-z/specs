#include "run_stats.h"
#include "run_manifest.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cxxabi.h>
#include <cstdlib>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <typeinfo>

void RunStats::Totals::add(const OpticalOutputPortStats &s, uint64_t pending)
{
    ++output_ports;
    scheduled += s.scheduled;
    coalesced += s.coalesced;
    processed += s.processed;
    emitted += s.emitted;
    suppressed_abstol += s.suppressed_abstol;
    suppressed_reltol += s.suppressed_reltol;
    suppressed_snap += s.suppressed_snap;
    pending_max = std::max(pending_max, s.pending_max);
    pending_sum += s.pending_sum;
    pending_end += pending;
    if (s.has_emitted)
    {
        last_emission_s = has_emitted ? std::max(last_emission_s, s.last_emission.to_seconds())
                                      : s.last_emission.to_seconds();
        has_emitted = true;
    }
}

void RunStats::collect(const std::vector<OpticalOutputPort *> &oops)
{
    totals = Totals();
    ports.clear();
    devices.clear();

    std::map<std::string, DeviceRow> by_type;
    std::map<std::string, std::set<const sc_object *>> writers_by_type;
    for (const OpticalOutputPort *oop : oops)
    {
        const OpticalOutputPortStats &s = oop->getStats();
        const uint64_t pending = oop->pending();
        totals.add(s, pending);

        const sc_object *net = dynamic_cast<const sc_object *>(oop->m_port.get_interface());
        const sc_object *writer = oop->get_parent_object();

        PortRow row;
        row.port = oop->name();
        row.net = net ? net->name() : "";
        row.writer = writer ? writer->name() : "";
        for (uint32_t wlid : s.wavelength_ids)
            row.wavelengths_m.push_back(OpticalSignal::getWavelength(wlid));
        row.counters.add(s, pending);
        ports.push_back(row);

        const std::string type = writer ? type_name(*writer) : "";
        DeviceRow &dev = by_type[type];
        dev.type = type;
        dev.counters.add(s, pending);
        writers_by_type[type].insert(writer);
    }
    for (auto &p : by_type)
    {
        p.second.instances = writers_by_type[p.first].size();
        devices.push_back(p.second);
    }
}

static std::string counters_json(const RunStats::Totals &c)
{
    std::ostringstream ss;
    ss << "\"scheduled\": " << c.scheduled
       << ", \"coalesced\": " << c.coalesced
       << ", \"processed\": " << c.processed
       << ", \"emitted\": " << c.emitted
       << ", \"suppressed\": {\"abstol\": " << c.suppressed_abstol
       << ", \"reltol\": " << c.suppressed_reltol
       << ", \"snap\": " << c.suppressed_snap << "}"
       << ", \"pending_max\": " << c.pending_max
       << ", \"pending_mean\": "
       << RunStats::json_number(c.scheduled ? static_cast<double>(c.pending_sum) / c.scheduled : 0.0)
       << ", \"pending_end\": " << c.pending_end
       << ", \"last_emission_s\": " << (c.has_emitted ? RunStats::json_number(c.last_emission_s) : "null");
    return ss.str();
}

std::string RunStats::to_json() const
{
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"format\": \"specs-run-stats\",\n";
    ss << "  \"format_version\": 1,\n";
    ss << "  \"analysis\": " << RunManifest::json_quote(analysis) << ",\n";

    ss << "  \"sections\": [\"summary\"";
    if (sections & PORTS)
        ss << ", \"ports\"";
    if (sections & DEVICES)
        ss << ", \"devices\"";
    ss << "],\n";

    ss << "  \"options\": {\"abstol\": " << json_number(abstol) << ", \"reltol\": " << json_number(reltol)
       << ", \"snapfactor\": " << json_number(snap_factor) << ", \"coalesce\": " << (coalesce ? "true" : "false")
       << ", \"maxevents\": " << max_events << "},\n";
    ss << "  \"event_cap_reached\": " << (event_cap_reached ? "true" : "false") << ",\n";
    ss << "  \"end_time_s\": " << json_number(end_time_s) << ",\n";
    // on its own line: the only value that differs between two runs of the same build
    ss << "  \"wall_time_s\": " << json_number(wall_time_s) << ",\n";
    ss << "  \"totals\": {\"output_ports\": " << totals.output_ports << ", " << counters_json(totals) << "}";

    if (sections & PORTS)
    {
        ss << ",\n  \"ports\": [";
        for (size_t i = 0; i < ports.size(); ++i)
        {
            const PortRow &r = ports[i];
            ss << (i ? "," : "") << "\n    {\"port\": " << RunManifest::json_quote(r.port)
               << ", \"net\": " << (r.net.empty() ? "null" : RunManifest::json_quote(r.net))
               << ", \"writer\": " << (r.writer.empty() ? "null" : RunManifest::json_quote(r.writer))
               << ", \"wavelengths_m\": [";
            for (size_t k = 0; k < r.wavelengths_m.size(); ++k)
                ss << (k ? ", " : "") << json_number(r.wavelengths_m[k]);
            ss << "], " << counters_json(r.counters) << "}";
        }
        ss << (ports.empty() ? "" : "\n  ") << "]";
    }

    if (sections & DEVICES)
    {
        ss << ",\n  \"devices\": [";
        for (size_t i = 0; i < devices.size(); ++i)
        {
            const DeviceRow &r = devices[i];
            ss << (i ? "," : "") << "\n    {\"type\": " << (r.type.empty() ? "null" : RunManifest::json_quote(r.type))
               << ", \"instances\": " << r.instances << ", \"output_ports\": " << r.counters.output_ports
               << ", " << counters_json(r.counters) << "}";
        }
        ss << (devices.empty() ? "" : "\n  ") << "]";
    }
    ss << "\n}\n";
    return ss.str();
}

bool RunStats::parse_sections(const std::string &value, unsigned &sections)
{
    sections = SUMMARY;
    std::stringstream ss(value);
    std::string item;
    while (std::getline(ss, item, ','))
    {
        if (item == "summary")
            sections |= SUMMARY;
        else if (item == "ports")
            sections |= PORTS;
        else if (item == "devices")
            sections |= DEVICES;
        else
            return false;
    }
    return true;
}

std::string RunStats::type_name(const sc_object &obj)
{
    const char *mangled = typeid(obj).name();
    int status = 0;
    std::unique_ptr<char, void (*)(void *)> demangled(abi::__cxa_demangle(mangled, nullptr, nullptr, &status),
                                                      std::free);
    return status == 0 && demangled ? std::string(demangled.get()) : std::string(mangled);
}

std::string RunStats::json_number(double v)
{
    // JSON has no inf/NaN (not expected here)
    if (!std::isfinite(v))
        return "null";
    char buf[32];
    const auto res = std::to_chars(buf, buf + sizeof(buf), v);
    return std::string(buf, res.ptr);
}
