#pragma once

/** ******************************************* **/
/**       Elementary passive devices            **/
/** ******************************************* **/
#include "devices/waveguide.h"
#include "devices/directional_coupler.h"
#include "devices/merger.h"
#include "devices/splitter.h"
#include "devices/crossing.h"
#include "devices/pcm_device.h"

/** ******************************************* **/
/**            Active devices                   **/
/** ******************************************* **/
#include "devices/detector.h"
#include "devices/phaseshifter.h"
#include "devices/mzi_active.h"
#include "devices/mzi.h"

/** ******************************************* **/
/**            Sources                          **/
/** ******************************************* **/
#include "devices/bitstream_source.h"
#include "devices/cw_source.h"
#include "devices/value_list_source.h"
#include "devices/electrical_value_list_source.h"

/** ******************************************* **/
/**              Utilities                      **/
/** ******************************************* **/
#include "devices/probe.h"
#include "devices/ring.h"
#include "devices/time_monitor.h"
#include "devices/power_meter.h"
#include "devices/subcircuit_instance.h"

/** ******************************************* **/
/**           Circuits                          **/
/** ******************************************* **/
#include "devices/crow.h"
#include "devices/octane_cell.h"
#include "devices/octane_segment.h"
#include "devices/octane_matrix.h"
#include "devices/mesh_col.h"
#include "devices/clements.h"
