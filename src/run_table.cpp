#include "run_table.h"

#include <bit>
#include <cerrno>
#include <charconv>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

using std::string;
using std::vector;
using std::cerr;
using std::endl;

namespace fs = std::filesystem;

// The .npy columns and the chunk files hold the host's bytes, declared little-endian
static_assert(std::endian::native == std::endian::little, "the run table assumes a little-endian host");

namespace {

const size_t NPY_HEADER_SIZE = 128;
const char *const MANIFEST_FILE = "table.manifest.json";
const char *const INDEX_FILE = "runs.csv";
const char *const CHUNK_MAGIC = "SPECS-RUN-CHUNK-1\n";

[[noreturn]] void fail(const string &msg)
{
    cerr << "Error: " << msg << endl;
    exit(1);
}

bool pwrite_all(int fd, const void *data, size_t size, uint64_t offset)
{
    const char *p = static_cast<const char *>(data);
    while (size > 0)
    {
        const ssize_t n = ::pwrite(fd, p, size, static_cast<off_t>(offset));
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            return false;
        p += n;
        size -= n;
        offset += n;
    }
    return true;
}

bool write_all(int fd, const string &s)
{
    const char *p = s.data();
    size_t size = s.size();
    while (size > 0)
    {
        const ssize_t n = ::write(fd, p, size);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            return false;
        p += n;
        size -= n;
    }
    return true;
}

bool read_file(const string &path, string &content)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    content = ss.str();
    return !f.bad();
}

// Content cut after its last '\n' (a partial last line from an interrupted append is dropped)
size_t complete_lines_size(const string &content)
{
    const size_t last = content.rfind('\n');
    return last == string::npos ? 0 : last + 1;
}

bool parse_u64(const string &s, uint64_t &v)
{
    const auto res = std::from_chars(s.data(), s.data() + s.size(), v, 10);
    return !s.empty() && res.ec == std::errc() && res.ptr == s.data() + s.size();
}

// --- chunk file: magic, then little-endian u64 counts, strings as (u64 length, bytes), arrays raw
void put_u64(string &b, uint64_t v)
{
    b.append(reinterpret_cast<const char *>(&v), sizeof(v));
}

void put_str(string &b, const string &s)
{
    put_u64(b, s.size());
    b += s;
}

template <typename T>
void put_array(string &b, const vector<T> &v)
{
    put_u64(b, v.size());
    b.append(reinterpret_cast<const char *>(v.data()), v.size() * sizeof(T));
}

struct Reader {
    const string &b;
    size_t pos = 0;
    bool ok = true;

    bool get_u64(uint64_t &v)
    {
        if (!ok || b.size() - pos < sizeof(v))
            return ok = false;
        std::memcpy(&v, b.data() + pos, sizeof(v));
        pos += sizeof(v);
        return true;
    }
    bool get_str(string &s)
    {
        uint64_t n = 0;
        if (!get_u64(n) || b.size() - pos < n)
            return ok = false;
        s.assign(b.data() + pos, n);
        pos += n;
        return true;
    }
    template <typename T>
    bool get_array(vector<T> &v)
    {
        uint64_t n = 0;
        if (!get_u64(n) || n > (b.size() - pos) / sizeof(T))
            return ok = false;
        v.resize(n);
        std::memcpy(v.data(), b.data() + pos, n * sizeof(T));
        pos += n * sizeof(T);
        return true;
    }
};

}

bool RunChunk::write(const string &path) const
{
    string b = CHUNK_MAGIC;
    put_u64(b, signal_names.size());
    for (const auto &s : signal_names)
        put_str(b, s);
    put_array(b, time_s);
    put_array(b, signal);
    put_array(b, wavelength_m);
    put_array(b, power_W);
    put_array(b, field_re);
    put_array(b, field_im);
    put_u64(b, draw_names.size());
    for (const auto &s : draw_names)
        put_str(b, s);
    put_array(b, draw_values);
    put_u64(b, drawn_params.size());
    for (const auto &p : drawn_params)
    {
        put_str(b, p.first);
        put_str(b, p.second);
    }
    // written under another name, then renamed: the chunk exists only once complete
    const string tmp = path + ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f)
            return false;
        f.write(b.data(), b.size());
        f.close();
        if (!f)
            return false;
    }
    return std::rename(tmp.c_str(), path.c_str()) == 0;
}

bool RunChunk::read(const string &path)
{
    string b;
    if (!read_file(path, b) || b.compare(0, strlen(CHUNK_MAGIC), CHUNK_MAGIC) != 0)
        return false;
    Reader r { b, strlen(CHUNK_MAGIC) };
    uint64_t n = 0;
    r.get_u64(n);
    signal_names.clear();
    for (uint64_t i = 0; r.ok && i < n; ++i)
    {
        string s;
        if (r.get_str(s))
            signal_names.push_back(s);
    }
    r.get_array(time_s);
    r.get_array(signal);
    r.get_array(wavelength_m);
    r.get_array(power_W);
    r.get_array(field_re);
    r.get_array(field_im);
    r.get_u64(n);
    draw_names.clear();
    for (uint64_t i = 0; r.ok && i < n; ++i)
    {
        string s;
        if (r.get_str(s))
            draw_names.push_back(s);
    }
    r.get_array(draw_values);
    r.get_u64(n);
    drawn_params.clear();
    for (uint64_t i = 0; r.ok && i < n; ++i)
    {
        string name, value;
        if (r.get_str(name) && r.get_str(value))
            drawn_params.emplace_back(name, value);
    }
    if (!r.ok || r.pos != b.size())
        return false;
    // consistent sizes and signal indices
    const size_t rows = time_s.size();
    if (signal.size() != rows || wavelength_m.size() != rows || power_W.size() != rows || field_re.size() != rows
        || field_im.size() != rows || draw_values.size() != draw_names.size())
        return false;
    for (int32_t s : signal)
    {
        if (s < 0 || static_cast<size_t>(s) >= signal_names.size())
            return false;
    }
    return true;
}

string RunTable::npy_header_fixed(const string &descr, uint64_t n)
{
    string dict = "{'descr': '" + descr + "', 'fortran_order': False, 'shape': (" + std::to_string(n) + ",), }";
    // magic (6) + version (2) + header length (2) + dict, padded with spaces, + '\n': 128 bytes
    const size_t dict_size = NPY_HEADER_SIZE - 10;
    dict.append(dict_size - 1 - dict.size(), ' ');
    dict += '\n';
    string out = string("\x93") + "NUMPY";
    out += char(1);
    out += char(0);
    out += char(dict_size & 0xff);
    out += char(dict_size >> 8);
    return out + dict;
}

string RunTable::csv_field(const string &s)
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

vector<string> RunTable::csv_split(const string &line)
{
    vector<string> fields(1);
    bool quoted = false;
    for (size_t i = 0; i < line.size(); ++i)
    {
        const char c = line[i];
        if (quoted)
        {
            if (c == '"' && i + 1 < line.size() && line[i + 1] == '"')
            {
                fields.back() += '"';
                ++i;
            }
            else if (c == '"')
                quoted = false;
            else
                fields.back() += c;
        }
        else if (c == '"')
            quoted = true;
        else if (c == ',')
            fields.emplace_back();
        else
            fields.back() += c;
    }
    return fields;
}

string RunTable::chunk_path(const string &dir, uint64_t run)
{
    return (fs::path(dir) / "chunks" / ("run" + std::to_string(run) + ".chunk")).string();
}

string RunTable::log_path(const string &dir, uint64_t run)
{
    return (fs::path(dir) / "logs" / ("run" + std::to_string(run) + ".log")).string();
}

string RunTable::header_line() const
{
    string h = "run,status,exit_code,seed,mc_run";
    for (const auto &p : m_param_columns)
        h += "," + csv_field(p);
    for (const auto &p : m_drawn_columns)
        h += "," + csv_field(p);
    return h + ",row_start,row_count,draw_start,draw_count,message\n";
}

RunTable::RunTable(const string &dir, const string &manifest_json, const vector<string> &param_columns,
                   const vector<string> &drawn_columns, bool overwrite)
    : m_dir(dir)
    , m_param_columns(param_columns)
    , m_drawn_columns(drawn_columns)
{
    m_data = {
        { "run", "<i4", 4 },
        { "time_s", "<f8", 8 },
        { "signal", "<i4", 4 },
        { "wavelength_m", "<f8", 8 },
        { "power_W", "<f8", 8 },
        { "field_re", "<f8", 8 },
        { "field_im", "<f8", 8 },
    };
    m_draw = {
        { "draw_run", "<i4", 4 },
        { "draw_name", "<i4", 4 },
        { "draw_value", "<f8", 8 },
    };
    open_or_create(manifest_json, overwrite);
}

RunTable::~RunTable()
{
    // closes only: a forked run process destroys its copy too
    for (auto *columns : { &m_data, &m_draw })
    {
        for (auto &c : *columns)
        {
            if (c.fd >= 0)
                ::close(c.fd);
        }
    }
    for (int fd : { m_runs_fd, m_signal_names_fd, m_draw_names_fd, m_lock_fd })
    {
        if (fd >= 0)
            ::close(fd);
    }
}

void RunTable::open_or_create(const string &manifest_json, bool overwrite)
{
    const fs::path d(m_dir);
    std::error_code ec;
    if (!fs::exists(d, ec))
    {
        if (::mkdir(m_dir.c_str(), 0777) != 0)
            fail("cannot create the run table directory \"" + m_dir + "\": " + strerror(errno));
    }
    else if (!fs::is_directory(d, ec))
        fail("the run table \"" + m_dir + "\" exists and is not a directory");

    // one writer at a time
    m_lock_fd = ::open((d / ".lock").c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0666);
    if (m_lock_fd < 0)
        fail("cannot open \"" + (d / ".lock").string() + "\": " + strerror(errno));
    if (::flock(m_lock_fd, LOCK_EX | LOCK_NB) != 0)
        fail("the run table \"" + m_dir + "\" is being written by another specs process");

    const fs::path manifest = d / MANIFEST_FILE;
    bool resume = false;
    if (fs::exists(manifest, ec))
    {
        string old;
        if (!read_file(manifest.string(), old))
            fail("cannot read \"" + manifest.string() + "\"");
        if (old == manifest_json && !overwrite)
            resume = true;
        else if (!overwrite)
            fail("the run table \"" + m_dir + "\" was made by another netlist, plan, build or command line "
                 "(see its " + MANIFEST_FILE + "); use --overwrite to replace it, or another -o");
    }
    else
    {
        // an empty directory (or one holding only the lock) becomes a table; nothing else is touched
        for (const auto &entry : fs::directory_iterator(d, ec))
        {
            if (entry.path().filename() != ".lock")
                fail("\"" + m_dir + "\" exists and is not a run table (no " + MANIFEST_FILE + ")");
        }
    }

    if (!resume)
    {
        // a new table: only the files of a run table are removed (the manifest last)
        for (const char *f : { INDEX_FILE, "signal_names.txt", "draw_names.txt" })
            fs::remove(d / f, ec);
        for (const auto *columns : { &m_data, &m_draw })
            for (const auto &c : *columns)
                fs::remove(d / (c.name + ".npy"), ec);
        fs::remove_all(d / "logs", ec);
        fs::remove_all(d / "chunks", ec);
        fs::remove(manifest, ec);
    }
    else
    {
        // chunks of runs not committed by an interrupted plan
        fs::remove_all(d / "chunks", ec);
    }
    for (const char *sub : { "logs", "chunks" })
    {
        if (::mkdir((d / sub).c_str(), 0777) != 0 && errno != EEXIST)
            fail("cannot create \"" + (d / sub).string() + "\": " + strerror(errno));
    }

    const string index_path = (d / INDEX_FILE).string();
    if (!resume)
    {
        std::ofstream f(index_path, std::ios::binary | std::ios::trunc);
        f << header_line();
        f.close();
        if (!f)
            fail("cannot write \"" + index_path + "\"");
    }
    else
        load_index();
    m_runs_fd = ::open(index_path.c_str(), O_WRONLY | O_APPEND | O_CLOEXEC);
    if (m_runs_fd < 0)
        fail("cannot open \"" + index_path + "\": " + strerror(errno));

    open_columns(m_data, m_rows, !resume);
    open_columns(m_draw, m_draws, !resume);
    m_signal_names_fd = open_names("signal_names.txt", m_signal_names, m_signal_index, !resume);
    m_draw_names_fd = open_names("draw_names.txt", m_draw_names, m_draw_index, !resume);

    if (!resume)
    {
        std::ofstream f(manifest, std::ios::binary | std::ios::trunc);
        f << manifest_json;
        f.close();
        if (!f)
            fail("cannot write \"" + manifest.string() + "\"");
    }
}

void RunTable::load_index()
{
    const string path = (fs::path(m_dir) / INDEX_FILE).string();
    string content;
    if (!read_file(path, content))
        fail("cannot read \"" + path + "\"");
    const size_t size = complete_lines_size(content);
    if (size != content.size())
    {
        // a partial line of an interrupted append
        content.resize(size);
        if (::truncate(path.c_str(), static_cast<off_t>(size)) != 0)
            fail("cannot repair \"" + path + "\": " + strerror(errno));
    }
    const string header = header_line();
    if (content.compare(0, header.size(), header) != 0)
        fail("\"" + path + "\" does not have the expected header");

    const size_t n_fields = csv_split(header.substr(0, header.size() - 1)).size();
    std::map<uint64_t, string> last_status;
    std::istringstream lines(content.substr(header.size()));
    string line;
    while (std::getline(lines, line))
    {
        const vector<string> f = csv_split(line);
        uint64_t run = 0, row_start = 0, row_count = 0, draw_start = 0, draw_count = 0;
        // run ... row_start, row_count, draw_start, draw_count, message
        if (f.size() != n_fields || !parse_u64(f[0], run) || !parse_u64(f[n_fields - 5], row_start)
            || !parse_u64(f[n_fields - 4], row_count) || !parse_u64(f[n_fields - 3], draw_start)
            || !parse_u64(f[n_fields - 2], draw_count) || row_start != m_rows || draw_start != m_draws)
            fail("\"" + path + "\": invalid line " + std::to_string(m_lines + 2) + ": " + line);
        m_rows += row_count;
        m_draws += draw_count;
        last_status[run] = f[1];
        ++m_lines;
    }
    for (const auto &p : last_status)
    {
        if (p.second == "ok")
            m_completed.insert(p.first);
    }
}

void RunTable::open_columns(vector<Column> &columns, uint64_t n, bool create)
{
    for (auto &c : columns)
    {
        const string path = (fs::path(m_dir) / (c.name + ".npy")).string();
        c.fd = ::open(path.c_str(), O_RDWR | O_CLOEXEC | (create ? O_CREAT | O_TRUNC : 0), 0666);
        if (c.fd < 0)
            fail("cannot open \"" + path + "\": " + strerror(errno));
        const uint64_t size = NPY_HEADER_SIZE + n * c.itemsize;
        struct stat st;
        if (!create && (::fstat(c.fd, &st) != 0 || static_cast<uint64_t>(st.st_size) < size))
            fail("\"" + path + "\" holds fewer rows than runs.csv accounts for");
        // rows past the committed ones are from an interrupted commit
        if (::ftruncate(c.fd, static_cast<off_t>(size)) != 0 || !set_count(c, n))
            fail("cannot write \"" + path + "\": " + strerror(errno));
    }
}

int RunTable::open_names(const string &file, vector<string> &names, std::map<string, int32_t> &index, bool create)
{
    const string path = (fs::path(m_dir) / file).string();
    if (!create)
    {
        string content;
        if (!read_file(path, content))
            fail("cannot read \"" + path + "\"");
        const size_t size = complete_lines_size(content);
        if (size != content.size() && ::truncate(path.c_str(), static_cast<off_t>(size)) != 0)
            fail("cannot repair \"" + path + "\": " + strerror(errno));
        std::istringstream lines(content.substr(0, size));
        string line;
        while (std::getline(lines, line))
        {
            index.emplace(line, static_cast<int32_t>(names.size()));
            names.push_back(line);
        }
    }
    const int fd = ::open(path.c_str(), O_WRONLY | O_APPEND | O_CLOEXEC | (create ? O_CREAT | O_TRUNC : 0), 0666);
    if (fd < 0)
        fail("cannot open \"" + path + "\": " + strerror(errno));
    return fd;
}

int32_t RunTable::name_index(const string &name, vector<string> &names, std::map<string, int32_t> &index, int fd,
                             bool &ok)
{
    const auto it = index.find(name);
    if (it != index.end())
        return it->second;
    const int32_t i = static_cast<int32_t>(names.size());
    ok = write_all(fd, name + "\n") && ok;
    index.emplace(name, i);
    names.push_back(name);
    return i;
}

bool RunTable::append(Column &c, const void *data, size_t n, uint64_t at)
{
    return pwrite_all(c.fd, data, n * c.itemsize, NPY_HEADER_SIZE + at * c.itemsize);
}

bool RunTable::set_count(Column &c, uint64_t n)
{
    const string h = npy_header_fixed(c.descr, n);
    return pwrite_all(c.fd, h.data(), h.size(), 0);
}

bool RunTable::commit(const Result &result, const RunChunk *chunk)
{
    bool ok = true;
    const uint64_t row_start = m_rows, draw_start = m_draws;
    uint64_t n_rows = 0, n_draws = 0;
    if (chunk)
    {
        n_rows = chunk->time_s.size();
        n_draws = chunk->draw_names.size();
        vector<int32_t> signal_map(chunk->signal_names.size());
        for (size_t i = 0; i < signal_map.size(); ++i)
            signal_map[i] = name_index(chunk->signal_names[i], m_signal_names, m_signal_index, m_signal_names_fd, ok);
        vector<int32_t> run(n_rows, static_cast<int32_t>(result.run));
        vector<int32_t> signal(n_rows);
        for (size_t i = 0; i < n_rows; ++i)
            signal[i] = signal_map.at(chunk->signal[i]);
        ok = append(m_data[0], run.data(), n_rows, row_start) && ok;
        ok = append(m_data[1], chunk->time_s.data(), n_rows, row_start) && ok;
        ok = append(m_data[2], signal.data(), n_rows, row_start) && ok;
        ok = append(m_data[3], chunk->wavelength_m.data(), n_rows, row_start) && ok;
        ok = append(m_data[4], chunk->power_W.data(), n_rows, row_start) && ok;
        ok = append(m_data[5], chunk->field_re.data(), n_rows, row_start) && ok;
        ok = append(m_data[6], chunk->field_im.data(), n_rows, row_start) && ok;

        vector<int32_t> draw_run(n_draws, static_cast<int32_t>(result.run));
        vector<int32_t> draw_name(n_draws);
        for (size_t i = 0; i < n_draws; ++i)
            draw_name[i] = name_index(chunk->draw_names[i], m_draw_names, m_draw_index, m_draw_names_fd, ok);
        ok = append(m_draw[0], draw_run.data(), n_draws, draw_start) && ok;
        ok = append(m_draw[1], draw_name.data(), n_draws, draw_start) && ok;
        ok = append(m_draw[2], chunk->draw_values.data(), n_draws, draw_start) && ok;
        for (auto &c : m_data)
            ok = set_count(c, row_start + n_rows) && ok;
        for (auto &c : m_draw)
            ok = set_count(c, draw_start + n_draws) && ok;
    }

    string line = std::to_string(result.run) + "," + result.status + "," + std::to_string(result.exit_code) + ","
                  + std::to_string(result.seed) + "," + result.mc_run;
    for (const auto &v : result.param_values)
        line += "," + csv_field(v);
    for (const auto &name : m_drawn_columns)
    {
        string value;
        if (chunk)
        {
            for (const auto &p : chunk->drawn_params)
            {
                if (p.first == name)
                    value = p.second;
            }
        }
        line += "," + csv_field(value);
    }
    line += "," + std::to_string(row_start) + "," + std::to_string(n_rows) + "," + std::to_string(draw_start) + ","
            + std::to_string(n_draws) + "," + csv_field(result.message) + "\n";
    if (!ok)
    {
        cerr << "Error: cannot write the run table \"" << m_dir << "\": " << strerror(errno) << endl;
        return false;
    }
    // the commit point: once this line is written, the run's rows are part of the table
    if (!write_all(m_runs_fd, line))
    {
        cerr << "Error: cannot write \"" << (fs::path(m_dir) / INDEX_FILE).string() << "\": " << strerror(errno) << endl;
        return false;
    }
    m_rows += n_rows;
    m_draws += n_draws;
    ++m_lines;
    if (result.status == "ok")
        m_completed.insert(result.run);
    else
        m_completed.erase(result.run);
    return true;
}
