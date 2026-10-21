#include "trace_table.h"

#include <array>
#include <bit>
#include <charconv>
#include <cstring>
#include <iostream>

using std::string;
using std::vector;
using std::cerr;
using std::endl;

// The .npy arrays are written in the host's byte order and declared little-endian ('<f8', '<i4', '<U')
static_assert(std::endian::native == std::endian::little, "the .npz writer assumes a little-endian host");

namespace {

const char *const CSV_HEADER = "time_s,signal,wavelength_m,power_W,field_re,field_im\n";

// Shortest decimal form that reads back to the same double ("nan", "inf" and "-inf" as such)
void appendDouble(string &line, double v)
{
    char buf[32];
    const auto res = std::to_chars(buf, buf + sizeof(buf), v);
    line.append(buf, res.ptr);
}

// A CSV field: quoted (RFC 4180) only if it contains a separator, a quote or a line break
string csvField(const string &s)
{
    if (s.find_first_of(",\"\r\n") == string::npos)
        return s;
    string out = "\"";
    for (char c : s)
    {
        if (c == '"')
            out += '"';
        out += c;
    }
    return out + '"';
}

void putU16(string &b, uint16_t v)
{
    b += char(v & 0xff);
    b += char(v >> 8);
}

void putU32(string &b, uint32_t v)
{
    for (int i = 0; i < 4; ++i)
        b += char((v >> (8 * i)) & 0xff);
}

// One .npy member of the .npz
struct NpyMember {
    string name;
    string header; // npy_header()
    const char *data;
    size_t size;
    uint32_t crc = 0;
    uint64_t offset = 0; // of its local file header
};

template <typename T>
NpyMember npyMember(const string &name, const string &descr, const vector<T> &v)
{
    return { name + ".npy", TraceTable::npy_header(descr, v.size()),
             reinterpret_cast<const char *>(v.data()), v.size() * sizeof(T) };
}

}

TraceTable::TraceTable(const string &csv_filename, const string &npz_filename)
    : m_csv_filename(csv_filename)
    , m_npz_filename(npz_filename)
{
    if (!m_csv_filename.empty())
    {
        m_csv.open(m_csv_filename, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!m_csv)
        {
            cerr << "Error: cannot open trace file \"" << m_csv_filename << "\"" << endl;
            exit(1);
        }
        m_csv << CSV_HEADER;
    }
    if (!m_npz_filename.empty())
    {
        m_npz.open(m_npz_filename, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!m_npz)
        {
            cerr << "Error: cannot open trace file \"" << m_npz_filename << "\"" << endl;
            exit(1);
        }
    }
}

TraceTable::~TraceTable()
{
    close();
}

uint32_t TraceTable::signal_index(const string &name)
{
    for (size_t i = 0; i < m_signal_names.size(); ++i)
    {
        if (m_signal_names[i] == name)
            return i;
    }
    m_signal_names.push_back(name);
    m_csv_fields.push_back(csvField(name));
    return m_signal_names.size() - 1;
}

void TraceTable::append(double time_s, uint32_t signal, const OpticalSignal &s)
{
    if (m_closed)
        return;
    if (time_s != m_pending_time_s)
    {
        writePending();
        m_pending_time_s = time_s;
    }
    const Row row = { time_s, signal, s.getWavelength(), s.power(), s.m_field.real(), s.m_field.imag() };
    const uint64_t key = (uint64_t(signal) << 32) | s.m_wavelength_id;
    const auto it = m_pending_index.find(key);
    if (it == m_pending_index.end())
    {
        m_pending_index.emplace(key, m_pending.size());
        m_pending.push_back(row);
    }
    else
        m_pending[it->second] = row; // the last sample of the time step wins
}

void TraceTable::writePending()
{
    for (const Row &row : m_pending)
    {
        ++m_rows;
        if (m_csv.is_open())
        {
            m_line.clear();
            appendDouble(m_line, row.time_s);
            m_line += ',';
            m_line += m_csv_fields.at(row.signal);
            m_line += ',';
            appendDouble(m_line, row.wavelength_m);
            m_line += ',';
            appendDouble(m_line, row.power_W);
            m_line += ',';
            appendDouble(m_line, row.field_re);
            m_line += ',';
            appendDouble(m_line, row.field_im);
            m_line += '\n';
            m_csv.write(m_line.data(), m_line.size());
        }
        if (m_npz.is_open())
        {
            m_time_s.push_back(row.time_s);
            m_signal.push_back(row.signal);
            m_wavelength_m.push_back(row.wavelength_m);
            m_power_W.push_back(row.power_W);
            m_field_re.push_back(row.field_re);
            m_field_im.push_back(row.field_im);
        }
    }
    m_pending.clear();
    m_pending_index.clear();
}

bool TraceTable::close()
{
    if (m_closed)
        return true;
    writePending();
    m_closed = true;
    bool ok = true;
    if (m_csv.is_open())
    {
        m_csv.close();
        if (!m_csv)
        {
            cerr << "Error: cannot write trace file \"" << m_csv_filename << "\"" << endl;
            ok = false;
        }
    }
    if (m_npz.is_open())
    {
        ok = writeNpz() && ok;
        m_npz.close();
        if (!m_npz)
        {
            cerr << "Error: cannot write trace file \"" << m_npz_filename << "\"" << endl;
            ok = false;
        }
    }
    return ok;
}

// Zip archive of the .npy members: local file headers and data, then the central directory. Stored
// (method 0), no data descriptor, all dates 1980-01-01 00:00 so that the bytes only depend on the data
bool TraceTable::writeNpz()
{
    // signal_names: fixed-width unicode, one UTF-32 code unit per byte of the (ASCII) names
    size_t width = 1;
    for (const auto &name : m_signal_names)
        width = std::max(width, name.size());
    vector<uint32_t> names_utf32(width * m_signal_names.size(), 0);
    for (size_t i = 0; i < m_signal_names.size(); ++i)
    {
        for (size_t j = 0; j < m_signal_names[i].size(); ++j)
            names_utf32[i * width + j] = static_cast<unsigned char>(m_signal_names[i][j]);
    }

    vector<NpyMember> members = {
        npyMember("time_s", "<f8", m_time_s),
        npyMember("signal", "<i4", m_signal),
        npyMember("wavelength_m", "<f8", m_wavelength_m),
        npyMember("power_W", "<f8", m_power_W),
        npyMember("field_re", "<f8", m_field_re),
        npyMember("field_im", "<f8", m_field_im),
        { "signal_names.npy", npy_header("<U" + std::to_string(width), m_signal_names.size()),
          reinterpret_cast<const char *>(names_utf32.data()), names_utf32.size() * sizeof(uint32_t) },
    };

    const uint16_t dos_time = 0;
    const uint16_t dos_date = (0 << 9) | (1 << 5) | 1; // 1980-01-01
    const uint64_t zip32_max = 0xFFFFFFFFu;

    uint64_t offset = 0;
    for (auto &m : members)
    {
        m.crc = crc32(m.header.data(), m.header.size());
        m.crc = crc32(m.data, m.size, m.crc);
        m.offset = offset;
        offset += 30 + m.name.size() + m.header.size() + m.size;
    }
    if (offset + members.size() * (46 + 32) + 22 > zip32_max)
    {
        cerr << "Error: the table of \"" << m_npz_filename << "\" exceeds 4 GiB, which the .npz writer does not "
             << "support (no ZIP64): use --format csv" << endl;
        return false;
    }

    for (const auto &m : members)
    {
        const uint32_t size = m.header.size() + m.size;
        string h;
        putU32(h, 0x04034b50); // local file header
        putU16(h, 20);         // version needed to extract (2.0)
        putU16(h, 0);          // flags
        putU16(h, 0);          // stored
        putU16(h, dos_time);
        putU16(h, dos_date);
        putU32(h, m.crc);
        putU32(h, size);       // compressed size
        putU32(h, size);       // uncompressed size
        putU16(h, m.name.size());
        putU16(h, 0);          // extra field length
        h += m.name;
        h += m.header;
        m_npz.write(h.data(), h.size());
        m_npz.write(m.data, m.size);
    }

    string cd;
    for (const auto &m : members)
    {
        const uint32_t size = m.header.size() + m.size;
        putU32(cd, 0x02014b50); // central directory file header
        putU16(cd, 20);         // version made by
        putU16(cd, 20);         // version needed to extract
        putU16(cd, 0);          // flags
        putU16(cd, 0);          // stored
        putU16(cd, dos_time);
        putU16(cd, dos_date);
        putU32(cd, m.crc);
        putU32(cd, size);
        putU32(cd, size);
        putU16(cd, m.name.size());
        putU16(cd, 0);          // extra field length
        putU16(cd, 0);          // comment length
        putU16(cd, 0);          // disk number
        putU16(cd, 0);          // internal attributes
        putU32(cd, 0);          // external attributes
        putU32(cd, m.offset);
        cd += m.name;
    }
    string eocd;
    putU32(eocd, 0x06054b50); // end of central directory
    putU16(eocd, 0);
    putU16(eocd, 0);
    putU16(eocd, members.size());
    putU16(eocd, members.size());
    putU32(eocd, cd.size());
    putU32(eocd, offset);
    putU16(eocd, 0);          // comment length
    m_npz.write(cd.data(), cd.size());
    m_npz.write(eocd.data(), eocd.size());
    return true;
}

uint32_t TraceTable::crc32(const void *data, size_t size, uint32_t crc)
{
    static const auto table = [] {
        std::array<uint32_t, 256> t {};
        for (uint32_t i = 0; i < 256; ++i)
        {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k)
                c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            t[i] = c;
        }
        return t;
    }();
    const unsigned char *p = static_cast<const unsigned char *>(data);
    crc = ~crc;
    for (size_t i = 0; i < size; ++i)
        crc = table[(crc ^ p[i]) & 0xff] ^ (crc >> 8);
    return ~crc;
}

string TraceTable::npy_header(const string &descr, size_t n)
{
    string dict = "{'descr': '" + descr + "', 'fortran_order': False, 'shape': (" + std::to_string(n) + ",), }";
    // magic (6) + version (2) + header length (2) + dict + '\n', padded with spaces to a multiple of 64
    const size_t unpadded = 10 + dict.size() + 1;
    dict.append((64 - unpadded % 64) % 64, ' ');
    dict += '\n';
    string out = string("\x93") + "NUMPY";
    out += char(1);
    out += char(0);
    putU16(out, dict.size());
    return out + dict;
}
