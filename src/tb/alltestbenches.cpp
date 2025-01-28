#include <map>
#include "tb/alltestbenches.h"

#if defined(BUILD_TB) && BUILD_TB == 1
std::map<std::string, tb_func_t> tb_map = {
    { "wg", wg_tb_run },
    { "waveguide", wg_tb_run },
    { "merg", Merger_tb_run },
    { "merger", Merger_tb_run },
    { "dc", DirectionalCoupler_tb_run },
    { "directional_coupler", DirectionalCoupler_tb_run },
    { "spli", Splitter_tb_run },
    { "splitter", Splitter_tb_run },
    { "det", Detector_tb_run },
    { "detector", Detector_tb_run },
    { "pcm", PCMElement_tb_run },
    { "mzi", MZI_tb_run },
    { "ring", Ring_tb_run },
    { "ac_add_drop", freqsweep_tb_run_add_drop },
    { "ac_crow", crow_tb_run },
    { "lambda", lambda_tb_run },
    { "phaseshifter", ps_tb_run },
    { "ps", ps_tb_run },
    { "mesh", mesh_tb_run },
};
#else
std::map<std::string, tb_func_t> tb_map = {};
#endif
