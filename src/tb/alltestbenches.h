#pragma once

#include "tb/detector_tb.h"
#include "tb/directional_coupler_tb.h"
#include "tb/merger_tb.h"
#include "tb/pcm_device_tb.h"
#include "tb/splitter_tb.h"
#include "tb/wg_tb.h"
#include "tb/mzi_tb.h"
#include "tb/ring_tb.h"
#include "tb/freqsweep_tb.h"
#include "tb/crow_tb.h"
#include "tb/lambda_tb.h"
#include "tb/phase_shifter_tb.h"
#include "tb/mesh_tb.h"

#include <map>
#include <string>

typedef void (*tb_func_t)();

extern std::map<std::string, tb_func_t> tb_map;
