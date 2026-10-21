# Core sources, shared by `specs` and `libspx.so`. Keep the lists explicit and sorted.
core_sources = files(
  'optical_output_port.cpp',
  'optical_signal.cpp',
  'specs.cpp',
  'utils/rng.cpp',
  'utils/sysc_utils.cpp',
)
main_source = files('main.cpp')

subdir('devices')  # core_sources += devices
subdir('parser')   # parser_dep
subdir('tb')       # tb_sources, tb_args
