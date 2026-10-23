#pragma once

#include <systemc.h>

#include <memory>
#include <string>
#include <vector>

#include "optical_output_port.h"
#include "optical_signal.h"
#include "specs.h"
#include "devices/spx_module.h"
#include "io/sparam_file.h"
#include "utils/transmission_cache.h"

/** F-03: N-port device given by S-parameters read from a file (SParamData, doc/sparam.md).
 *
 * State-holding, per wavelength: the device keeps the last field of each input port and wavelength;
 * on an input event it recomputes, at that wavelength, every output the input reaches,
 * b_j = sum_i S(j, i) a_i, from those memories, and writes it (absolute value) after the delay of that
 * output. S(lambda) and the delays are computed once per wavelength (cache). The S-matrix is the
 * one of the carrier wavelength: no dispersion within a pulse's bandwidth.
 *
 * Nets are numbered 0..N-1 in the order of the netlist element; net k is port m_port_map[k] of the data.
 * Unidirectional variant: nets 0..N/2-1 are inputs, N/2..N-1 outputs, and only the paths from an
 * input to an output are used. Bidirectional variant: every net is an input and an output. */
class SParamDeviceBase : public spx_module {
public:
    enum DelayMode {
        DELAY_AUTO,  // per output and wavelength: group delay of the largest path into the output
        DELAY_NONE,  // 0
        DELAY_FIXED, // m_delay_s on every path
    };

    // Parameters (set by the parser before the simulation)
    std::shared_ptr<const SParamData> m_data;
    /** Data port of each net */
    std::vector<size_t> m_port_map;
    DelayMode m_delay_mode = DELAY_AUTO;
    /** DELAY_FIXED: the delay (s) */
    double m_delay_s = 0;
    /** passivity=enforce: scale S(lambda) where it is not passive */
    bool m_enforce_passivity = false;
    /** B-014: a fixed delay written in the netlist that rounds to 0 is an error (as WG) */
    bool m_from_netlist = false;

    size_t nports() const { return m_nports; }

    // Ports, in net order of their variant (Uni: N/2 inputs, N/2 outputs; Bi: N of each)
    std::vector<std::unique_ptr<spx::oa_port_in_type>> p_in;
    std::vector<std::unique_ptr<spx::oa_port_out_type>> p_out;
    std::vector<std::unique_ptr<OpticalOutputPort>> m_out_writers;

    virtual void start_of_simulation();

    /** Main process: every input with an event in this delta cycle, in net order (an SC_METHOD
     * sensitive to all the inputs) */
    void on_inputs_changed();
    /** --vci only: prints the parameters at initialization */
    void print_initialization();

    /** S(out, in) between two nets at a wavelength, from the data (with passivity enforcement) */
    OpticalSignal::field_type transmission(size_t out_net, size_t in_net, double wavelength_m) const;

protected:
    SParamDeviceBase(sc_module_name name, size_t nports, bool bidirectional);

    size_t m_nports;
    bool m_bidirectional;
    /** Net of each input port p_in[k] and output port p_out[k] */
    std::vector<size_t> m_input_nets, m_output_nets;
    /** Outputs (indices into p_out) reached from each input (indices into p_in): S not 0 in the data */
    std::vector<std::vector<size_t>> m_reach;
    /** Inputs (indices into p_in) that reach each output */
    std::vector<std::vector<size_t>> m_sources;

    /** Per wavelength: S(output k, input i) at [k * inputs + i], and the delay of each output */
    struct WavelengthEntry {
        std::vector<OpticalSignal::field_type> S;
        std::vector<sc_time> delay;
    };
    WavelengthCache<WavelengthEntry> m_cache;
    /** Last field of each input and wavelength id */
    std::vector<std::vector<OpticalSignal::field_type>> m_memory;

    sc_time m_fixed_delay;
    // One report per device (B-014 rounding, negative group delays)
    bool m_delay_rounding_reported = false;
    bool m_negative_delay_reported = false;

    void onInput(size_t input, const OpticalSignal &s);
    WavelengthEntry computeEntry(const OpticalSignal &s);
    sc_time autoDelay(double tau_s);
};

class SParamDeviceUni : public SParamDeviceBase {
public:
    SParamDeviceUni(sc_module_name name, size_t nports)
        : SParamDeviceBase(name, nports, false)
    {}
};

class SParamDeviceBi : public SParamDeviceBase {
public:
    SParamDeviceBi(sc_module_name name, size_t nports)
        : SParamDeviceBase(name, nports, true)
    {}
};

typedef SParamDeviceUni SParamDevice;
