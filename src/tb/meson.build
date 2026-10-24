# Legacy testbenches (`specs -t <name>`, listed by `specs -p`). alltestbenches.cpp defines the
# (possibly empty) tb_map; BUILD_TB=1 fills it and is also needed by src/main.cpp.
tb_sources = files('alltestbenches.cpp')
tb_args = []
if get_option('old_tb')
  tb_sources += files(
    'detector_tb.cpp',
    'directional_coupler_tb.cpp',
    'freqsweep_tb.cpp',
    'lambda_tb.cpp',
    'merger_tb.cpp',
    'mesh_tb.cpp',
    'mzi_tb.cpp',
    'pcm_device_tb.cpp',
    'phase_shifter_tb.cpp',
    'ring_tb.cpp',
    'splitter_tb.cpp',
    'wg_tb.cpp',
  )
  tb_args += '-DBUILD_TB=1'
endif
