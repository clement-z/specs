#pragma once

/** ******************************************* **/
/**       Elementary passive devices            **/
/** ******************************************* **/
#include <waveguide.h>
#include <directional_coupler.h>
#include <merger.h>
#include <splitter.h>
#include <crossing.h>
#include <pcm_device.h>

/** ******************************************* **/
/**            Active devices                   **/
/** ******************************************* **/
#include <detector.h>
#include <phaseshifter.h>
#include <mzi_active.h>
#include <mzi.h>

/** ******************************************* **/
/**            Sources                          **/
/** ******************************************* **/
//#include <bitstream_source.h>
#include <cw_source.h>
#include <value_list_source.h>
#include <electrical_value_list_source.h>

/** ******************************************* **/
/**              Utilities                      **/
/** ******************************************* **/
#include <probe.h>
//#include <ring.h>
#include <time_monitor.h>
#include <power_meter.h>

/** ******************************************* **/
/**           Circuits                          **/
/** ******************************************* **/
#include <crow.h>
#include <octane_cell.h>
#include <octane_segment.h>
#include <octane_matrix.h>
#include <mesh_col.h>
#include <clements.h>
