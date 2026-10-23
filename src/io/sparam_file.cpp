#include "io/sparam_file.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <regex>
#include <sstream>

using std::string;
using std::vector;

namespace {

string trim(const string &s)
{
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b])))
        ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
        --e;
    return s.substr(b, e - b);
}

string lower(string s)
{
    for (auto &ch : s)
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return s;
}

// Whole string is a number (strtod); value in x
bool to_double(const string &s, double &x)
{
    const string t = trim(s);
    if (t.empty())
        return false;
    char *end = nullptr;
    x = std::strtod(t.c_str(), &end);
    return end == t.c_str() + t.size();
}

bool to_size(const string &s, size_t &n)
{
    const string t = trim(s);
    if (t.empty() || !std::all_of(t.begin(), t.end(), [](char ch) { return std::isdigit(static_cast<unsigned char>(ch)); }))
        return false;
    n = std::stoul(t);
    return true;
}

// Items of "(a, 'b', "c", 1)" or "[...]": split at the commas outside quotes, quotes removed.
// quoted[k]: item k was quoted
vector<string> tuple_items(const string &line, vector<bool> &quoted)
{
    vector<string> items;
    quoted.clear();
    string s = trim(line);
    if (s.size() >= 2)
        s = s.substr(1, s.size() - 2);
    string cur;
    char quote = 0;
    bool was_quoted = false;
    for (char ch : s)
    {
        if (quote)
        {
            if (ch == quote)
                quote = 0;
            else
                cur += ch;
        }
        else if (ch == '\'' || ch == '"')
        {
            quote = ch;
            was_quoted = true;
        }
        else if (ch == ',')
        {
            items.push_back(was_quoted ? cur : trim(cur));
            quoted.push_back(was_quoted);
            cur.clear();
            was_quoted = false;
        }
        else if (!was_quoted)
            cur += ch;
    }
    items.push_back(was_quoted ? cur : trim(cur));
    quoted.push_back(was_quoted);
    return items;
}

// CSV fields of one line (RFC 4180 quotes, "" inside quotes is a quote)
vector<string> csv_fields(const string &line)
{
    vector<string> fields;
    string cur;
    bool in_quotes = false;
    for (size_t k = 0; k < line.size(); ++k)
    {
        const char ch = line[k];
        if (in_quotes)
        {
            if (ch == '"' && k + 1 < line.size() && line[k + 1] == '"')
            {
                cur += '"';
                ++k;
            }
            else if (ch == '"')
                in_quotes = false;
            else
                cur += ch;
        }
        else if (ch == '"')
            in_quotes = true;
        else if (ch == ',')
        {
            fields.push_back(trim(cur));
            cur.clear();
        }
        else
            cur += ch;
    }
    fields.push_back(trim(cur));
    return fields;
}

vector<string> whitespace_tokens(const string &line)
{
    vector<string> tokens;
    std::istringstream ss(line);
    string tok;
    while (ss >> tok)
        tokens.push_back(tok);
    return tokens;
}

// Line without the comment starting at the first `c` (none: the whole line)
string strip_comment(const string &line, char c)
{
    const size_t pos = line.find(c);
    return pos == string::npos ? line : line.substr(0, pos);
}

} // namespace

/** ******************************************* **/
/**               Common                        **/
/** ******************************************* **/

void SParamData::fail(const string &message, int line) const
{
    std::cerr << "Error (" << m_source;
    if (line > 0)
        std::cerr << ":" << line;
    std::cerr << "): " << message << std::endl;
    exit(1);
}

SParamData::Format SParamData::formatFromExtension(const string &path)
{
    const string p = lower(path);
    auto ends_with = [&p](const string &suffix) {
        return p.size() >= suffix.size() && p.compare(p.size() - suffix.size(), suffix.size(), suffix) == 0;
    };
    if (ends_with(".sparam") || ends_with(".dat"))
        return FORMAT_SPARAM;
    if (ends_with(".ts") || std::regex_search(p, std::regex("\\.s[0-9]+p$")))
        return FORMAT_TOUCHSTONE;
    if (ends_with(".csv"))
        return FORMAT_CSV;
    if (ends_with(".taylor"))
        return FORMAT_TAYLOR;
    return FORMAT_AUTO;
}

const char *SParamData::formatName(Format format)
{
    switch (format)
    {
        case FORMAT_SPARAM: return "sparam";
        case FORMAT_TOUCHSTONE: return "touchstone";
        case FORMAT_CSV: return "csv";
        case FORMAT_TAYLOR: return "taylor";
        default: return "auto";
    }
}

std::shared_ptr<SParamData> SParamData::load(const string &path, Format format, Convention convention, Interp interp)
{
    std::ifstream in(path);
    if (!in)
    {
        std::cerr << "Error: cannot open S-parameter file '" << path << "'" << std::endl;
        exit(1);
    }
    return parse(in, path, format, convention, interp);
}

std::shared_ptr<SParamData> SParamData::parse(std::istream &in, const string &source_name, Format format,
        Convention convention, Interp interp)
{
    std::shared_ptr<SParamData> data(new SParamData());
    data->m_source = source_name;
    data->m_interp = interp;
    if (format == FORMAT_AUTO)
        format = formatFromExtension(source_name);
    if (format == FORMAT_AUTO)
        data->fail("unknown S-parameter file extension (expected .sparam, .dat, .sNp, .ts, .csv or .taylor), "
                   "give format=sparam|touchstone|csv|taylor");
    data->m_format = format;

    // Engineering convention (Touchstone, by default): conjugated, doc/sparam.md
    if (convention == CONVENTION_AUTO)
        convention = (format == FORMAT_TOUCHSTONE) ? CONVENTION_ENGINEERING : CONVENTION_PHYSICS;
    data->m_conjugated = convention == CONVENTION_ENGINEERING;
    if (format == FORMAT_TAYLOR && data->m_conjugated)
        data->fail("the Taylor form is written in the SPECS convention (convention=physics)");

    switch (format)
    {
        case FORMAT_SPARAM: data->parseSparam(in); break;
        case FORMAT_TOUCHSTONE: data->parseTouchstone(in); break;
        case FORMAT_CSV: data->parseCsv(in); break;
        case FORMAT_TAYLOR: data->parseTaylor(in); break;
        default: break;
    }
    data->computeRange();
    return data;
}

int SParamData::portIndex(const string &name_or_index) const
{
    const string key = trim(name_or_index);
    for (size_t k = 0; k < m_nports; ++k)
        if (m_port_names[k] == key)
            return static_cast<int>(k);
    size_t idx;
    if (to_size(key, idx) && idx >= 1 && idx <= m_nports)
        return static_cast<int>(idx - 1);
    return -1;
}

void SParamData::unwrap(vector<double> &phase)
{
    // as numpy.unwrap: steps taken in [-pi, pi), a step of -pi taken as +pi when the raw step is positive
    double offset = 0;
    for (size_t k = 1; k < phase.size(); ++k)
    {
        const double raw_prev = phase[k - 1] - offset; // value before correction
        const double raw = phase[k];
        const double dd = raw - raw_prev;
        double ddmod = std::fmod(dd + M_PI, 2 * M_PI);
        if (ddmod < 0)
            ddmod += 2 * M_PI;
        ddmod -= M_PI;
        if (ddmod == -M_PI && dd > 0)
            ddmod = M_PI;
        const double correction = ddmod - dd;
        if (std::fabs(dd) >= M_PI)
            offset += correction;
        phase[k] = raw + offset;
    }
}

/** ******************************************* **/
/**               Tables                        **/
/** ******************************************* **/

void SParamData::buildTables(vector<vector<Sample>> &raw, bool conjugate)
{
    const size_t n = m_nports;
    m_tables.assign(n * n, Table());
    m_active.assign(n * n, 0);
    for (size_t k = 0; k < n * n; ++k)
    {
        auto &samples = raw[k];
        if (samples.empty())
            continue;
        std::stable_sort(samples.begin(), samples.end(), [](const Sample &a, const Sample &b) { return a.f_Hz < b.f_Hz; });
        Table &t = m_tables[k];
        bool nonzero = false;
        for (size_t s = 0; s < samples.size(); ++s)
        {
            if (!(samples[s].f_Hz > 0) || !std::isfinite(samples[s].f_Hz))
                fail("frequency " + std::to_string(samples[s].f_Hz) + " Hz is not positive");
            if (s > 0 && samples[s].f_Hz == samples[s - 1].f_Hz)
            {
                std::ostringstream msg;
                msg.precision(17);
                msg << "S" << k / n + 1 << "," << k % n + 1 << ": frequency " << samples[s].f_Hz << " Hz appears twice";
                fail(msg.str());
            }
            const complex_type v = conjugate ? std::conj(samples[s].value) : samples[s].value;
            if (!std::isfinite(v.real()) || !std::isfinite(v.imag()))
                fail("S-parameter value is not finite");
            t.f_Hz.push_back(samples[s].f_Hz);
            t.mag.push_back(std::abs(v));
            t.phase.push_back(std::arg(v));
            t.re.push_back(v.real());
            t.im.push_back(v.imag());
            t.max_mag = std::max(t.max_mag, t.mag.back());
            nonzero |= std::abs(v) != 0;
        }
        unwrap(t.phase);
        // Group delay dphi/domega at the grid points: 3 points (non-uniform) inside, 2 at the ends
        const size_t m = t.f_Hz.size();
        t.tau_s.assign(m, 0);
        for (size_t s = 0; s < m && m > 1; ++s)
        {
            const double w = 2 * M_PI * t.f_Hz[s];
            if (s == 0)
                t.tau_s[s] = (t.phase[1] - t.phase[0]) / (2 * M_PI * t.f_Hz[1] - w);
            else if (s == m - 1)
                t.tau_s[s] = (t.phase[s] - t.phase[s - 1]) / (w - 2 * M_PI * t.f_Hz[s - 1]);
            else
            {
                const double h0 = w - 2 * M_PI * t.f_Hz[s - 1];
                const double h1 = 2 * M_PI * t.f_Hz[s + 1] - w;
                t.tau_s[s] = -h1 / (h0 * (h0 + h1)) * t.phase[s - 1]
                             + (h1 - h0) / (h0 * h1) * t.phase[s]
                             + h0 / (h1 * (h0 + h1)) * t.phase[s + 1];
            }
        }
        if (m_interp == INTERP_PCHIP)
        {
            t.dmag = pchipSlopes(t.f_Hz, t.mag);
            t.dphase = pchipSlopes(t.f_Hz, t.phase);
            t.dre = pchipSlopes(t.f_Hz, t.re);
            t.dim = pchipSlopes(t.f_Hz, t.im);
        }
        m_active[k] = nonzero;
    }
}

void SParamData::computeRange()
{
    if (m_is_taylor)
        return; // given by the file
    double fmin = 0, fmax = std::numeric_limits<double>::infinity();
    bool any = false;
    // Intersection of the grids of the active entries; with none, of all the entries
    for (int pass = 0; pass < 2 && !any; ++pass)
    {
        for (size_t k = 0; k < m_tables.size(); ++k)
        {
            const Table &t = m_tables[k];
            if (t.f_Hz.empty() || (pass == 0 && !m_active[k]))
                continue;
            fmin = std::max(fmin, t.f_Hz.front());
            fmax = std::min(fmax, t.f_Hz.back());
            any = true;
        }
    }
    if (!any)
        fail("no S-parameter data");
    if (fmin > fmax)
        fail("the frequency grids of the S-parameters do not overlap");
    m_min_wavelength_m = c / fmax;
    m_max_wavelength_m = c / fmin;
}

bool SParamData::inRange(double wavelength_m) const
{
    return wavelength_m >= m_min_wavelength_m * (1 - RANGE_RELATIVE_TOLERANCE)
        && wavelength_m <= m_max_wavelength_m * (1 + RANGE_RELATIVE_TOLERANCE);
}

double SParamData::clampFrequency(double f_Hz) const
{
    // inside the range up to the tolerance (inRange()): onto the grid
    return std::min(std::max(f_Hz, c / m_max_wavelength_m), c / m_min_wavelength_m);
}

vector<double> SParamData::pchipSlopes(const vector<double> &x, const vector<double> &y)
{
    // Fritsch-Carlson monotone slopes, with the end conditions of scipy's PchipInterpolator
    const size_t n = x.size();
    vector<double> d(n, 0);
    if (n < 2)
        return d;
    vector<double> h(n - 1), delta(n - 1);
    for (size_t k = 0; k + 1 < n; ++k)
    {
        h[k] = x[k + 1] - x[k];
        delta[k] = (y[k + 1] - y[k]) / h[k];
    }
    if (n == 2)
    {
        d[0] = d[1] = delta[0];
        return d;
    }
    auto sign = [](double v) { return (v > 0) - (v < 0); };
    for (size_t k = 1; k + 1 < n; ++k)
    {
        if (sign(delta[k - 1]) * sign(delta[k]) <= 0)
            d[k] = 0;
        else
        {
            const double w1 = 2 * h[k] + h[k - 1];
            const double w2 = h[k] + 2 * h[k - 1];
            d[k] = (w1 + w2) / (w1 / delta[k - 1] + w2 / delta[k]);
        }
    }
    auto edge = [&sign](double h0, double h1, double m0, double m1) {
        double dd = ((2 * h0 + h1) * m0 - h0 * m1) / (h0 + h1);
        if (sign(dd) != sign(m0))
            dd = 0;
        else if (sign(m0) != sign(m1) && std::fabs(dd) > 3 * std::fabs(m0))
            dd = 3 * m0;
        return dd;
    };
    d[0] = edge(h[0], h[1], delta[0], delta[1]);
    d[n - 1] = edge(h[n - 2], h[n - 3], delta[n - 2], delta[n - 3]);
    return d;
}

size_t SParamData::segment(const vector<double> &x, double xi)
{
    const size_t n = x.size();
    size_t k = std::upper_bound(x.begin(), x.end(), xi) - x.begin();
    if (k == 0)
        k = 1;
    if (k >= n)
        k = n - 1;
    return k - 1;
}

double SParamData::evalSegment(const vector<double> &x, const vector<double> &y, const vector<double> *slopes, size_t k, double xi)
{
    if (xi == x[k])
        return y[k];
    if (xi == x[k + 1])
        return y[k + 1];
    const double h = x[k + 1] - x[k];
    const double t = (xi - x[k]) / h;
    if (!slopes)
        return (1 - t) * y[k] + t * y[k + 1];
    // cubic Hermite
    const double t2 = t * t, t3 = t2 * t;
    return (2 * t3 - 3 * t2 + 1) * y[k] + (t3 - 2 * t2 + t) * h * (*slopes)[k]
         + (-2 * t3 + 3 * t2) * y[k + 1] + (t3 - t2) * h * (*slopes)[k + 1];
}

double SParamData::evalTable(const vector<double> &x, const vector<double> &y, const vector<double> *slopes, double xi)
{
    // xi is within [x.front(), x.back()]
    if (x.size() == 1)
        return y[0];
    return evalSegment(x, y, slopes, segment(x, xi), xi);
}

SParamData::complex_type SParamData::interpolate(const Table &t, double f_Hz) const
{
    const bool pchip = m_interp == INTERP_PCHIP;
    if (t.f_Hz.size() == 1)
        return std::polar(t.mag[0], t.phase[0]);
    const size_t k = segment(t.f_Hz, f_Hz);
    // at a grid point (and wherever |S| is not small) from magnitude and phase, as the file's value
    if (f_Hz != t.f_Hz[k] && f_Hz != t.f_Hz[k + 1]
            && std::min(t.mag[k], t.mag[k + 1]) < MIXED_INTERP_THRESHOLD * t.max_mag)
    {
        // near a null: real and imaginary parts (S is continuous at the grid points either way)
        return complex_type(evalSegment(t.f_Hz, t.re, pchip ? &t.dre : nullptr, k, f_Hz),
                            evalSegment(t.f_Hz, t.im, pchip ? &t.dim : nullptr, k, f_Hz));
    }
    return std::polar(evalSegment(t.f_Hz, t.mag, pchip ? &t.dmag : nullptr, k, f_Hz),
                      evalSegment(t.f_Hz, t.phase, pchip ? &t.dphase : nullptr, k, f_Hz));
}

double SParamData::polyval(const vector<double> &coeffs, double d)
{
    double v = 0;
    for (size_t k = coeffs.size(); k-- > 0;)
        v = v * d + coeffs[k];
    return v;
}

double SParamData::polyder(const vector<double> &coeffs, double d)
{
    double v = 0;
    for (size_t k = coeffs.size(); k-- > 1;)
        v = v * d + static_cast<double>(k) * coeffs[k];
    return v;
}

SParamData::complex_type SParamData::S(size_t out, size_t in, double wavelength_m) const
{
    const size_t k = out * m_nports + in;
    if (!m_active[k])
        return 0;
    if (m_is_taylor)
    {
        const Taylor &t = m_taylor[k];
        const double d = wavelength_m - m_lambda0_m;
        return std::polar(polyval(t.amp, d), polyval(t.phase, d));
    }
    return interpolate(m_tables[k], clampFrequency(c / wavelength_m));
}

double SParamData::groupDelay(size_t out, size_t in, double wavelength_m) const
{
    const size_t k = out * m_nports + in;
    if (!m_active[k])
        return 0;
    if (m_is_taylor)
    {
        const Taylor &t = m_taylor[k];
        const double d = wavelength_m - m_lambda0_m;
        if (!t.delay.empty())
            return polyval(t.delay, d);
        // tau = dphi/domega = -(lambda^2 / (2 pi c)) dphi/dlambda
        return -(wavelength_m * wavelength_m / (2 * M_PI * c)) * polyder(t.phase, d);
    }
    const Table &t = m_tables[k];
    return evalTable(t.f_Hz, t.tau_s, nullptr, clampFrequency(c / wavelength_m));
}

double SParamData::maxPowerSum(const vector<complex_type> &S, size_t nports, size_t *input)
{
    double worst = 0;
    for (size_t in = 0; in < nports; ++in)
    {
        double sum = 0;
        for (size_t out = 0; out < nports; ++out)
            sum += std::norm(S[out * nports + in]);
        if (sum > worst || in == 0)
        {
            worst = sum;
            if (input)
                *input = in;
        }
    }
    return worst;
}

vector<double> SParamData::checkWavelengths() const
{
    // the grid points within the range, or 201 points over the Taylor range
    vector<double> wavelengths;
    if (m_is_taylor)
    {
        for (int k = 0; k <= 200; ++k)
            wavelengths.push_back(m_min_wavelength_m + (m_max_wavelength_m - m_min_wavelength_m) * k / 200.0);
        return wavelengths;
    }
    vector<double> f;
    for (size_t k = 0; k < m_tables.size(); ++k)
        if (m_active[k])
            for (double fk : m_tables[k].f_Hz)
                if (inRange(c / fk))
                    f.push_back(fk);
    std::sort(f.begin(), f.end());
    f.erase(std::unique(f.begin(), f.end()), f.end());
    for (double fk : f)
        wavelengths.push_back(c / fk);
    return wavelengths;
}

double SParamData::maxMagnitude(size_t out, size_t in) const
{
    double m = 0;
    if (!active(out, in))
        return 0;
    for (double wl : checkWavelengths())
        m = std::max(m, std::abs(S(out, in, wl)));
    return m;
}

SParamData::PassivityReport SParamData::passivity() const
{
    const vector<double> wavelengths = checkWavelengths();
    PassivityReport report;
    vector<complex_type> mat(m_nports * m_nports);
    bool first = true;
    for (double wl : wavelengths)
    {
        for (size_t out = 0; out < m_nports; ++out)
            for (size_t in = 0; in < m_nports; ++in)
                mat[out * m_nports + in] = S(out, in, wl);
        size_t input = 0;
        const double sum = maxPowerSum(mat, m_nports, &input);
        if (first || sum > report.max_power_sum)
        {
            report.max_power_sum = sum;
            report.input = input;
            report.wavelength_m = wl;
            first = false;
        }
    }
    return report;
}

/** ******************************************* **/
/**       Lumerical/SiEPIC .sparam              **/
/** ******************************************* **/

void SParamData::parseSparam(std::istream &in)
{
    struct Block {
        string out, in;
        int line;
        vector<Sample> samples;
    };
    vector<string> declared;     // port declarations, in order
    vector<string> appearance;   // ports in order of first appearance in the headers
    std::map<string, string> port_mode;
    vector<Block> blocks;

    auto add_port = [](vector<string> &list, const string &name) {
        if (std::find(list.begin(), list.end(), name) == list.end())
            list.push_back(name);
    };

    string line;
    int lineno = 0;
    while (std::getline(in, line))
    {
        ++lineno;
        const string t = trim(line);
        if (t.empty())
            continue;
        if (t[0] != '(' && t[0] != '[')
            fail("expected a port declaration or a block header, got '" + t + "'", lineno);
        vector<bool> quoted;
        const vector<string> items = tuple_items(t, quoted);
        if (items.size() == 2 && quoted[0])
        {
            // ["port 1","LEFT"]: port declaration
            if (!blocks.empty())
                fail("port declaration after the first block", lineno);
            add_port(declared, items[0]);
            continue;
        }
        if (items.size() == 7)
            fail("block headers with a group delay (7th field) are not supported", lineno);
        if (items.size() != 6 || !quoted[0] || !quoted[3])
            fail("expected a block header (\"out\",\"mode\",mode_id,\"in\",mode_id,\"transmission\"), got '" + t + "'", lineno);
        if (lower(items[5]) != "transmission")
            fail("block type '" + items[5] + "' is not supported (only 'transmission')", lineno);
        Block b;
        b.out = items[0];
        b.in = items[3];
        b.line = lineno;
        // one mode per port
        const string out_mode = items[1] + "/" + items[2];
        const string in_mode = items[1] + "/" + items[4];
        for (const auto &pm : {std::make_pair(b.out, out_mode), std::make_pair(b.in, in_mode)})
        {
            auto it = port_mode.find(pm.first);
            if (it == port_mode.end())
                port_mode[pm.first] = pm.second;
            else if (it->second != pm.second)
                fail("port '" + pm.first + "' appears with two modes (multimode data is not supported)", lineno);
        }
        add_port(appearance, b.out);
        add_port(appearance, b.in);

        // (rows, 3)
        if (!std::getline(in, line))
            fail("missing the (rows,columns) line after the block header", lineno);
        ++lineno;
        const vector<string> dims = tuple_items(trim(line), quoted);
        size_t rows = 0, cols = 0;
        if (dims.size() != 2 || !to_size(dims[0], rows) || !to_size(dims[1], cols))
            fail("expected (rows,columns), got '" + trim(line) + "'", lineno);
        if (cols != 3)
            fail("blocks with " + std::to_string(cols) + " columns are not supported (expected 3: frequency, |S|, phase)", lineno);
        for (size_t r = 0; r < rows; ++r)
        {
            if (!std::getline(in, line))
                fail("block ends before its " + std::to_string(rows) + " rows", lineno);
            ++lineno;
            const vector<string> tok = whitespace_tokens(line);
            double f, mag, phase;
            if (tok.size() != 3 || !to_double(tok[0], f) || !to_double(tok[1], mag) || !to_double(tok[2], phase))
                fail("expected 3 numbers (frequency, |S|, phase), got '" + trim(line) + "'", lineno);
            b.samples.push_back({f, std::polar(mag, phase)});
        }
        blocks.push_back(std::move(b));
    }
    if (blocks.empty())
        fail("no S-parameter block");

    if (!declared.empty())
    {
        for (const auto &p : appearance)
            if (std::find(declared.begin(), declared.end(), p) == declared.end())
                fail("port '" + p + "' is used in a block but not declared");
        m_port_names = declared;
    }
    else
        m_port_names = appearance;
    m_nports = m_port_names.size();

    vector<vector<Sample>> raw(m_nports * m_nports);
    for (auto &b : blocks)
    {
        const size_t o = std::find(m_port_names.begin(), m_port_names.end(), b.out) - m_port_names.begin();
        const size_t i = std::find(m_port_names.begin(), m_port_names.end(), b.in) - m_port_names.begin();
        if (!raw[o * m_nports + i].empty())
            fail("two blocks for the same pair ('" + b.out + "', '" + b.in + "')", b.line);
        raw[o * m_nports + i] = std::move(b.samples);
    }
    buildTables(raw, m_conjugated);
}

/** ******************************************* **/
/**               Touchstone                    **/
/** ******************************************* **/

void SParamData::parseTouchstone(std::istream &in)
{
    size_t nports = 0;
    {
        std::smatch m;
        const string src = lower(m_source);
        if (std::regex_search(src, m, std::regex("\\.s([0-9]+)p$")))
            nports = std::stoul(m[1]);
    }
    bool v2 = false;
    bool have_option = false;
    double unit = 1e9;
    string value_format = "ma";
    string two_port_order; // v2: 12_21 or 21_12
    string matrix_format = "full";
    size_t n_freq = 0;
    bool in_network_data = false, network_data_done = false;
    std::map<size_t, string> named_ports; // ! Port[i] = name
    vector<string> listed_ports;          // ! ports: a, b
    vector<double> numbers;
    vector<int> number_lines;

    string line;
    int lineno = 0;
    while (std::getline(in, line))
    {
        ++lineno;
        // comments: port names
        const size_t bang = line.find('!');
        if (bang != string::npos)
        {
            const string comment = trim(line.substr(bang + 1));
            std::smatch m;
            if (std::regex_match(comment, m, std::regex("port\\s*\\[\\s*([0-9]+)\\s*\\]\\s*=\\s*(.*)", std::regex::icase)))
            {
                string rest = trim(m[2]);
                std::smatch q;
                if (std::regex_search(rest, q, std::regex("[\"']([^\"']*)[\"']")))
                    rest = q[1];
                named_ports[std::stoul(m[1])] = rest;
            }
            else if (std::regex_match(comment, m, std::regex("ports\\s*:\\s*(.*)", std::regex::icase)))
            {
                listed_ports.clear();
                for (const auto &p : csv_fields(m[1]))
                    listed_ports.push_back(p);
            }
            line = line.substr(0, bang);
        }
        const string t = trim(line);
        if (t.empty())
            continue;
        if (t[0] == '[')
        {
            const size_t close = t.find(']');
            if (close == string::npos)
                fail("unterminated keyword '" + t + "'", lineno);
            const string kw = lower(trim(t.substr(1, close - 1)));
            const string arg = trim(t.substr(close + 1));
            if (kw == "version")
            {
                if (arg != "2.0" && arg != "2.1")
                    fail("Touchstone version '" + arg + "' is not supported (1.x, 2.0, 2.1)", lineno);
                v2 = true;
            }
            else if (kw == "number of ports")
            {
                if (!to_size(arg, nports) || nports == 0)
                    fail("invalid [Number of Ports] '" + arg + "'", lineno);
            }
            else if (kw == "two-port data order")
            {
                two_port_order = lower(arg);
                if (two_port_order != "12_21" && two_port_order != "21_12")
                    fail("invalid [Two-Port Data Order] '" + arg + "' (12_21 or 21_12)", lineno);
            }
            else if (kw == "number of frequencies")
            {
                if (!to_size(arg, n_freq))
                    fail("invalid [Number of Frequencies] '" + arg + "'", lineno);
            }
            else if (kw == "matrix format")
            {
                matrix_format = lower(arg);
                if (matrix_format != "full" && matrix_format != "lower" && matrix_format != "upper")
                    fail("invalid [Matrix Format] '" + arg + "' (Full, Lower or Upper)", lineno);
            }
            else if (kw == "network data")
                in_network_data = true;
            else if (kw == "noise data" || kw == "end")
            {
                in_network_data = false;
                network_data_done = true;
            }
            else if (kw == "mixed-mode order")
                fail("mixed-mode Touchstone data is not supported", lineno);
            else if (kw == "reference" || kw == "number of noise frequencies" || kw == "begin information"
                     || kw == "end information")
                ; // ignored
            else
                fail("unknown Touchstone keyword [" + kw + "]", lineno);
            continue;
        }
        if (t[0] == '#')
        {
            if (have_option)
                continue; // only the first option line counts
            have_option = true;
            const vector<string> tok = whitespace_tokens(t.substr(1));
            for (size_t k = 0; k < tok.size(); ++k)
            {
                const string w = lower(tok[k]);
                if (w == "hz") unit = 1;
                else if (w == "khz") unit = 1e3;
                else if (w == "mhz") unit = 1e6;
                else if (w == "ghz") unit = 1e9;
                else if (w == "s") ;
                else if (w == "y" || w == "z" || w == "h" || w == "g")
                    fail("only S-parameters are supported (option line has '" + tok[k] + "')", lineno);
                else if (w == "ri" || w == "ma" || w == "db") value_format = w;
                else if (w == "r")
                    ++k; // reference impedance: ignored
                else
                    fail("unknown token '" + tok[k] + "' in the option line", lineno);
            }
            continue;
        }
        if (network_data_done || (v2 && !in_network_data))
        {
            if (v2 && !network_data_done)
                fail("data outside [Network Data]", lineno);
            continue; // after [End] / noise data
        }
        for (const auto &tok : whitespace_tokens(t))
        {
            double x;
            if (!to_double(tok, x))
                fail("expected a number, got '" + tok + "'", lineno);
            numbers.push_back(x);
            number_lines.push_back(lineno);
        }
    }
    if (nports == 0)
        fail(v2 ? "missing [Number of Ports]" : "Touchstone 1 file without an .sNp extension (or [Version] 2.0 with [Number of Ports])");
    if (!have_option && v2)
        fail("missing the option line (# ...)");
    m_nports = nports;
    const size_t n = nports;

    // order of the values of one frequency: (out, in) pairs
    vector<std::pair<size_t, size_t>> order;
    if (n == 2 && matrix_format == "full")
    {
        if (v2 && two_port_order.empty())
            fail("a 2-port Touchstone 2 file needs [Two-Port Data Order]");
        if (!v2 || two_port_order == "21_12")
            order = {{0, 0}, {1, 0}, {0, 1}, {1, 1}};
        else
            order = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
    }
    else
    {
        for (size_t r = 0; r < n; ++r)
            for (size_t col = 0; col < n; ++col)
            {
                if (matrix_format == "lower" && col > r)
                    continue;
                if (matrix_format == "upper" && col < r)
                    continue;
                order.push_back({r, col});
            }
    }
    const size_t per_freq = 1 + 2 * order.size();

    vector<vector<Sample>> raw(n * n);
    size_t count = 0;
    double last_f = -1;
    for (size_t pos = 0; pos < numbers.size(); pos += per_freq)
    {
        const double f = numbers[pos] * unit;
        // version 1, 2 ports: the noise data follow, from a frequency lower than the last one
        if (!v2 && n == 2 && f <= last_f)
            break;
        if (pos + per_freq > numbers.size())
            fail("incomplete data for the last frequency (" + std::to_string(numbers.size() - pos)
                 + " numbers, expected " + std::to_string(per_freq) + ")", number_lines[pos]);
        if (f <= last_f)
            fail("frequencies must increase", number_lines[pos]);
        last_f = f;
        for (size_t e = 0; e < order.size(); ++e)
        {
            const double a = numbers[pos + 1 + 2 * e], b = numbers[pos + 2 + 2 * e];
            complex_type v;
            if (value_format == "ri")
                v = complex_type(a, b);
            else if (value_format == "ma")
                v = std::polar(a, b * M_PI / 180);
            else
                v = std::polar(std::pow(10.0, a / 20), b * M_PI / 180);
            const auto [o, i] = order[e];
            raw[o * n + i].push_back({f, v});
            if (matrix_format != "full" && o != i)
                raw[i * n + o].push_back({f, v});
        }
        ++count;
    }
    if (count == 0)
        fail("no network data");
    if (v2 && n_freq && count != n_freq)
        fail("[Number of Frequencies] is " + std::to_string(n_freq) + " but the data have " + std::to_string(count));

    // port names
    m_port_names.clear();
    for (size_t k = 0; k < n; ++k)
    {
        string name = std::to_string(k + 1);
        if (listed_ports.size() == n)
            name = listed_ports[k];
        auto it = named_ports.find(k + 1);
        if (it != named_ports.end() && !it->second.empty())
            name = it->second;
        m_port_names.push_back(name);
    }
    buildTables(raw, m_conjugated);
}

/** ******************************************* **/
/**               CSV                           **/
/** ******************************************* **/

void SParamData::parseCsv(std::istream &in)
{
    string line;
    int lineno = 0;
    vector<string> header;
    while (std::getline(in, line))
    {
        ++lineno;
        if (!trim(line).empty())
        {
            header = csv_fields(line);
            break;
        }
    }
    if (header.empty())
        fail("empty CSV file");
    if (lower(header[0]) != "wavelength_m")
        fail("the first column must be wavelength_m (got '" + header[0] + "')", lineno);

    // columns "<in>@<mode>,<out>@<mode>_re|_im"
    struct Column {
        string in, out;
        bool re;
    };
    vector<Column> columns;
    vector<string> ports;
    auto add_port = [&ports](const string &p) {
        if (std::find(ports.begin(), ports.end(), p) == ports.end())
            ports.push_back(p);
    };
    const std::regex col_re("([^,@]+)@([0-9]+),([^,@]+)@([0-9]+)_(re|im)", std::regex::icase);
    for (size_t k = 1; k < header.size(); ++k)
    {
        std::smatch m;
        if (!std::regex_match(header[k], m, col_re))
            fail("column '" + header[k] + "' is not \"<in>@<mode>,<out>@<mode>_re\" or _im", lineno);
        if (m[2] != "0" || m[4] != "0")
            fail("column '" + header[k] + "': only mode 0 is supported", lineno);
        columns.push_back({trim(m[1]), trim(m[3]), lower(m[5]) == "re"});
        add_port(trim(m[1]));
        add_port(trim(m[3]));
    }
    m_port_names = ports;
    m_nports = ports.size();
    const size_t n = m_nports;
    if (n == 0)
        fail("no S-parameter column", lineno);
    // pair -> (re column, im column)
    std::map<size_t, std::pair<int, int>> pairs;
    for (size_t k = 0; k < columns.size(); ++k)
    {
        const size_t o = std::find(ports.begin(), ports.end(), columns[k].out) - ports.begin();
        const size_t i = std::find(ports.begin(), ports.end(), columns[k].in) - ports.begin();
        auto &p = pairs.try_emplace(o * n + i, -1, -1).first->second;
        int &slot = columns[k].re ? p.first : p.second;
        if (slot >= 0)
            fail("column '" + header[k + 1] + "' appears twice", lineno);
        slot = static_cast<int>(k);
    }
    for (const auto &p : pairs)
        if (p.second.first < 0 || p.second.second < 0)
            fail("S-parameter from '" + ports[p.first % n] + "' to '" + ports[p.first / n] + "' needs both _re and _im columns");

    vector<vector<Sample>> raw(n * n);
    while (std::getline(in, line))
    {
        ++lineno;
        if (trim(line).empty())
            continue;
        const vector<string> fields = csv_fields(line);
        if (fields.size() != header.size())
            fail("expected " + std::to_string(header.size()) + " fields, got " + std::to_string(fields.size()), lineno);
        vector<double> values(fields.size());
        for (size_t k = 0; k < fields.size(); ++k)
            if (!to_double(fields[k], values[k]))
                fail("expected a number, got '" + fields[k] + "'", lineno);
        if (!(values[0] > 0))
            fail("wavelength must be positive", lineno);
        const double f = c / values[0];
        for (const auto &p : pairs)
            raw[p.first].push_back({f, complex_type(values[1 + p.second.first], values[1 + p.second.second])});
    }
    buildTables(raw, m_conjugated);
}

/** ******************************************* **/
/**               Taylor form                   **/
/** ******************************************* **/

void SParamData::parseTaylor(std::istream &in)
{
    m_is_taylor = true;
    string line;
    int lineno = 0;
    bool have_magic = false, have_center = false, have_range = false;
    struct Entry {
        string out, in, what;
        vector<double> coeffs;
        int line;
    };
    vector<Entry> entries;
    while (std::getline(in, line))
    {
        ++lineno;
        const vector<string> tok = whitespace_tokens(strip_comment(line, '#'));
        if (tok.empty())
            continue;
        const string kw = lower(tok[0]);
        if (!have_magic)
        {
            if (kw != "specs-taylor" || tok.size() != 2 || tok[1] != "1")
                fail("the first line must be 'specs-taylor 1'", lineno);
            have_magic = true;
            continue;
        }
        auto numbers = [&](size_t from) {
            vector<double> v;
            for (size_t k = from; k < tok.size(); ++k)
            {
                double x;
                if (!to_double(tok[k], x))
                    fail("expected a number, got '" + tok[k] + "'", lineno);
                v.push_back(x);
            }
            return v;
        };
        if (kw == "ports")
        {
            if (m_nports)
                fail("'ports' given twice", lineno);
            size_t n;
            if (tok.size() == 2 && to_size(tok[1], n))
            {
                for (size_t k = 0; k < n; ++k)
                    m_port_names.push_back(std::to_string(k + 1));
            }
            else
                m_port_names.assign(tok.begin() + 1, tok.end());
            m_nports = m_port_names.size();
            if (m_nports == 0)
                fail("'ports' needs a count or names", lineno);
        }
        else if (kw == "center")
        {
            const auto v = numbers(1);
            if (v.size() != 1 || !(v[0] > 0))
                fail("'center' needs one positive wavelength (m)", lineno);
            m_lambda0_m = v[0];
            have_center = true;
        }
        else if (kw == "range")
        {
            const auto v = numbers(1);
            if (v.size() != 2 || !(v[0] > 0) || !(v[1] >= v[0]))
                fail("'range' needs two wavelengths (m), min <= max", lineno);
            m_min_wavelength_m = v[0];
            m_max_wavelength_m = v[1];
            have_range = true;
        }
        else if (kw == "s")
        {
            if (tok.size() < 5)
                fail("expected 'S <out> <in> amp|phase|delay c0 [c1 ...]'", lineno);
            const string what = lower(tok[3]);
            if (what != "amp" && what != "phase" && what != "delay")
                fail("expected amp, phase or delay, got '" + tok[3] + "'", lineno);
            entries.push_back({tok[1], tok[2], what, numbers(4), lineno});
        }
        else
            fail("unknown keyword '" + tok[0] + "'", lineno);
    }
    if (!have_magic)
        fail("empty file (the first line must be 'specs-taylor 1')");
    if (!m_nports)
        fail("missing 'ports'");
    if (!have_center)
        fail("missing 'center'");
    if (!have_range)
        fail("missing 'range' (the wavelength range where the expansions hold)");
    const size_t n = m_nports;
    m_taylor.assign(n * n, Taylor());
    m_active.assign(n * n, 0);
    vector<std::map<string, bool>> seen(n * n);
    for (const auto &e : entries)
    {
        const int o = portIndex(e.out), i = portIndex(e.in);
        if (o < 0)
            fail("unknown port '" + e.out + "'", e.line);
        if (i < 0)
            fail("unknown port '" + e.in + "'", e.line);
        const size_t k = o * n + i;
        if (seen[k][e.what])
            fail("'" + e.what + "' given twice for S " + e.out + " " + e.in, e.line);
        seen[k][e.what] = true;
        Taylor &t = m_taylor[k];
        if (e.what == "amp")
            t.amp = e.coeffs;
        else if (e.what == "phase")
            t.phase = e.coeffs;
        else
            t.delay = e.coeffs;
    }
    for (size_t k = 0; k < n * n; ++k)
    {
        Taylor &t = m_taylor[k];
        if (t.amp.empty())
        {
            if (!t.phase.empty() || !t.delay.empty())
                fail("S " + m_port_names[k / n] + " " + m_port_names[k % n] + ": 'phase' or 'delay' without 'amp'");
            continue;
        }
        m_active[k] = std::any_of(t.amp.begin(), t.amp.end(), [](double x) { return x != 0; });
    }
}
