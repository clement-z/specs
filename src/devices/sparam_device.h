#pragma once

#include <systemc.h>

#include <memory>
#include <queue>
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
 * State-holding, per wavelength and path: the device keeps a delayed copy m_ji of the field of each
 * path (input i to output j) and wavelength. An input event at t with field a_i sets m_ji = a_i at
 * t + tau_ji for every output j the input reaches (an arrival in the device's timed queue; at once for a
 * path without delay); on an arrival the output is recomputed at that wavelength from the copies,
 * b_j = sum_i S(j, i) m_ji, and written (absolute value) without delay, so the writes to an output are
 * in time order. S(lambda) and the delays are computed once per wavelength (cache). The S-matrix is the
 * one of the carrier wavelength: no dispersion within a pulse's bandwidth.
 *
 * delay_per="output" (m_per_path_delays false; F-03 phase 1, before PF03b): one delay per output and
 * wavelength, the one of the path with the largest |S| into that output; an input event sets the copies
 * of its paths at once (m_ji = a_i) and writes each output it reaches, b_j = sum_i S(j, i) m_ji, after
 * that output's delay.
 *
 * Nets are numbered 0..N-1 in the order of the netlist element; net k is port m_port_map[k] of the data.
 * Unidirectional variant: nets 0..N/2-1 are inputs, N/2..N-1 outputs, and only the paths from an
 * input to an output are used. Bidirectional variant: every net is an input and an output. */
class SParamDeviceBase : public spx_module {
public:
    enum DelayMode {
        DELAY_AUTO,  // per path (or output) and wavelength: the group delay of the path (0 where negative)
        DELAY_NONE,  // 0
        DELAY_FIXED, // m_delay_s on every path
    };

    // Parameters (set by the parser before the simulation)
    std::shared_ptr<const SParamData> m_data;
    /** Data port of each net */
    std::vector<size_t> m_port_map;
    DelayMode m_delay_mode = DELAY_AUTO;
    /** delay_per="path" (default): a delay per path, delayed copies; "output": a delay per output */
    bool m_per_path_delays = true;
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

    /** Main process (an SC_METHOD sensitive to all the inputs and to the timed queue): the arrivals due
     * now, then every input with an event in this delta cycle, in net order */
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

    /** Per wavelength: S(output k, input i) and the delay of that path, at [k * inputs + i] (the path index);
     * the group delay of the data where it was negative (0 was used), else 0. delay_per="output": every
     * path into output k has the delay of the output, and only the path it comes from has a negative delay */
    struct WavelengthEntry {
        std::vector<OpticalSignal::field_type> S;
        std::vector<sc_time> delay;
        std::vector<double> negative_delay_s;
    };
    WavelengthCache<WavelengthEntry> m_cache;
    /** Delayed copy of each path (index k * inputs + i) and wavelength id: the input field that has
     * arrived at the end of the path */
    std::vector<std::vector<OpticalSignal::field_type>> m_copy;

    /** Timed queue of the delayed paths: the input field reaches the end of the path at t. Taken in
     * the order (t, seq): arrivals at the same time in the order of their input events */
    struct Arrival {
        sc_time t;
        uint64_t seq;
        uint32_t path;
        uint32_t wavelength_id;
        OpticalSignal::field_type field;
        bool operator>(const Arrival &other) const
        {
            return t != other.t ? t > other.t : seq > other.seq;
        }
    };
    std::priority_queue<Arrival, std::vector<Arrival>, std::greater<Arrival>> m_arrivals;
    uint64_t m_arrival_seq = 0;
    /** Notified at the time of the first arrival */
    sc_event m_arrival_event;

    sc_time m_fixed_delay;
    // One report per device (B-014 rounding) and per path (negative group delays)
    bool m_delay_rounding_reported = false;
    std::vector<char> m_negative_delay_reported;

    void onInput(size_t input, const OpticalSignal &s);
    /** Copies the arrivals due now and writes the outputs they change */
    void processArrivals();
    /** Writes output k at a wavelength, from the copies of its paths, after a delay (delay_per="output";
     * without delay for the delayed copies) */
    void writeOutput(size_t k, uint32_t wavelength_id, const WavelengthEntry &e, const sc_time &delay = SC_ZERO_TIME);
    WavelengthEntry computeEntry(const OpticalSignal &s);
    sc_time autoDelay(double tau_s);
    /** .tran: reports a negative group delay of a path (0 used) once per path, when the path is first used
     * (delay_per="output": the path that gives the output its delay, when the output is first written) */
    void reportNegativeDelay(size_t k, size_t i, double wavelength_m, double tau_s);
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
