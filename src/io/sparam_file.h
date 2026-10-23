#pragma once

#include <complex>
#include <istream>
#include <memory>
#include <string>
#include <vector>

// F-03: S-parameter data of an N-port device, read from a file: Lumerical/SiEPIC .sparam, Touchstone 1.x
// and 2.x, a CSV form and a Taylor-coefficient form (formats, phase convention, interpolation and group
// delay: doc/sparam.md). S(out, in) is the field transmission from port `in` to port `out` (0-based here,
// 1-based in the files), in SPECS's phase convention (a waveguide applies exp(+j beta L)): the files are
// converted when they are read. Errors (unreadable file, syntax, unsupported content) print a message on
// stderr and exit(1), like the netlist parser.
class SParamData {
public:
    typedef std::complex<double> complex_type;

    enum Format { FORMAT_AUTO, FORMAT_SPARAM, FORMAT_TOUCHSTONE, FORMAT_CSV, FORMAT_TAYLOR };
    // Time convention of the file's phases: physics, exp(-i omega t) (a delay line is exp(+i beta L), as in
    // SPECS: kept), or engineering, exp(+j omega t) (exp(-j beta L): conjugated). AUTO: the format's own
    enum Convention { CONVENTION_AUTO, CONVENTION_PHYSICS, CONVENTION_ENGINEERING };
    enum Interp { INTERP_LINEAR, INTERP_PCHIP };

    // Speed of light (m/s), as the waveguides use it
    static constexpr double c = 299792458.0;
    // The data range is extended by this relative tolerance at both ends, as wavelength matching (B-010)
    static constexpr double RANGE_RELATIVE_TOLERANCE = 1e-12;

    // Reads a file (format AUTO: from its extension)
    static std::shared_ptr<SParamData> load(const std::string &path, Format format = FORMAT_AUTO,
            Convention convention = CONVENTION_AUTO, Interp interp = INTERP_LINEAR);
    // Reads from a stream; source_name is used in the messages, and for FORMAT_AUTO (its extension)
    static std::shared_ptr<SParamData> parse(std::istream &in, const std::string &source_name,
            Format format = FORMAT_AUTO, Convention convention = CONVENTION_AUTO, Interp interp = INTERP_LINEAR);

    // Format of a file name from its extension; FORMAT_AUTO if not recognised
    static Format formatFromExtension(const std::string &path);
    static const char *formatName(Format format);

    size_t ports() const { return m_nports; }
    const std::vector<std::string> &portNames() const { return m_port_names; }
    // Index of a port given by name or by 1-based index (as a string); -1 if none
    int portIndex(const std::string &name_or_index) const;
    const std::string &source() const { return m_source; }
    Format format() const { return m_format; }
    // The phases were conjugated when read
    bool conjugated() const { return m_conjugated; }

    // S(out, in) is not identically 0 in the file
    bool active(size_t out, size_t in) const { return m_active[out * m_nports + in]; }

    // Wavelength range of the data (m)
    double minWavelength() const { return m_min_wavelength_m; }
    double maxWavelength() const { return m_max_wavelength_m; }
    // Within the range, ends extended by RANGE_RELATIVE_TOLERANCE
    bool inRange(double wavelength_m) const;

    // S(out, in) and its group delay (s) at a wavelength in the range (SPECS convention); 0 if not active
    complex_type S(size_t out, size_t in, double wavelength_m) const;
    double groupDelay(size_t out, size_t in, double wavelength_m) const;

    // Passivity: the largest power leaving the device for a unit input, max over inputs `in` of
    // sum_out |S(out, in)|^2, over the grid points of the file (Taylor form: 201 points over its range)
    struct PassivityReport {
        double max_power_sum = 0;
        size_t input = 0; // 0-based
        double wavelength_m = 0;
    };
    PassivityReport passivity() const;
    // Largest |S(out, in)| over the same wavelengths as passivity()
    double maxMagnitude(size_t out, size_t in) const;
    // max over inputs of sum_out |S(out, in)|^2 of a full matrix (row-major, S[out * N + in])
    static double maxPowerSum(const std::vector<complex_type> &S, size_t nports, size_t *input = nullptr);

    // Unwraps phases in place (steps within [-pi, pi), +pi for a step of exactly -pi going up, as numpy.unwrap)
    static void unwrap(std::vector<double> &phase);

private:
    // One entry S(out, in) on a frequency grid
    struct Table {
        std::vector<double> f_Hz; // ascending
        std::vector<double> mag;
        std::vector<double> phase; // unwrapped, SPECS convention
        std::vector<double> tau_s; // group delay at the grid points
        std::vector<double> dmag, dphase; // PCHIP slopes (INTERP_PCHIP)
    };
    // One entry S(out, in) of the Taylor form: polynomials in d = lambda - lambda0
    struct Taylor {
        std::vector<double> amp, phase, delay; // delay empty: derived from phase
    };

    size_t m_nports = 0;
    std::vector<std::string> m_port_names;
    std::string m_source;
    Format m_format = FORMAT_AUTO;
    bool m_conjugated = false;
    Interp m_interp = INTERP_LINEAR;
    std::vector<char> m_active; // row-major, [out * N + in]
    double m_min_wavelength_m = 0;
    double m_max_wavelength_m = 0;

    // Tabulated formats
    std::vector<Table> m_tables; // row-major
    // Taylor form
    bool m_is_taylor = false;
    double m_lambda0_m = 0;
    std::vector<Taylor> m_taylor; // row-major

    SParamData() {}

    // Parsers (fill the raw data of the format), then finish() for the tables
    void parseSparam(std::istream &in);
    void parseTouchstone(std::istream &in);
    void parseCsv(std::istream &in);
    void parseTaylor(std::istream &in);

    // A raw sample of a tabulated entry: frequency, complex value in the file's convention
    struct Sample {
        double f_Hz;
        complex_type value;
    };
    // Builds the tables from the raw samples (row-major, one vector per entry), conjugating if asked
    void buildTables(std::vector<std::vector<Sample>> &raw, bool conjugate);
    void computeRange();

    [[noreturn]] void fail(const std::string &message, int line = 0) const;

    double clampFrequency(double f_Hz) const;
    // Wavelengths of passivity() and maxMagnitude()
    std::vector<double> checkWavelengths() const;
    static double evalTable(const std::vector<double> &x, const std::vector<double> &y, const std::vector<double> *slopes, double xi);
    static std::vector<double> pchipSlopes(const std::vector<double> &x, const std::vector<double> &y);
    static double polyval(const std::vector<double> &coeffs, double d);
    static double polyder(const std::vector<double> &coeffs, double d);
};
