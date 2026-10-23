# Core sources, shared by `specs` and `libspx.so`. Keep the lists explicit and sorted.
core_sources = files(
  'io/sparam_file.cpp',
  'optical_output_port.cpp',
  'optical_signal.cpp',
  'run_manifest.cpp',
  'run_plan.cpp',
  'run_progress.cpp',
  'run_stats.cpp',
  'run_table.cpp',
  'specs.cpp',
  'trace_table.cpp',
  'utils/rng.cpp',
  'utils/sha256.cpp',
  'utils/sysc_utils.cpp',
)
main_source = files('main.cpp')

subdir('devices')  # core_sources += devices
subdir('parser')   # parser_dep
subdir('tb')       # tb_sources, tb_args
