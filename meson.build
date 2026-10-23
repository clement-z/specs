project('specs', 'cpp',
  version: '0.3.0',
  license: 'LGPL-3.0-or-later',
  meson_version: '>=1.3.0',
  default_options: [
    # gnu++20 (not c++20): same as the former CMake build (CMAKE_CXX_STANDARD 20, extensions ON).
    # GNU mode enables -ffp-contract=fast on GCC, so ISO mode could change results in the last bits.
    # SystemC must be built with the same standard (it encodes it in a link-time API symbol).
    'cpp_std=gnu++20',
    'buildtype=release',
    'warning_level=2',
    # Keep assert() on: the code validates user input with asserts (see discover/meson/DECISIONS.md).
    'b_ndebug=false',
    # SystemC and taywee/args are built from their wraps (subprojects/) unless an installed copy is
    # requested with -Dforce_fallback_for= or --wrap-mode=nofallback (see Dependencies below).
    'force_fallback_for=systemc,args',
  ],
)

cxx = meson.get_compiler('cpp')
fs = import('fs')
cmake = import('cmake')
src_root = meson.project_source_root() / 'src'

# Project includes are quoted and relative to src/ ("devices/waveguide.h").
add_project_arguments('-Wfatal-errors', '-iquote', src_root, language: 'cpp')
if get_option('native')
  add_project_arguments('-march=native', language: 'cpp')
endif
# F-15: debug ids of optical signals (OpticalSignal::m_id), off by default
if get_option('signal_ids')
  add_project_arguments('-DSPECS_SIGNAL_IDS', language: 'cpp')
endif

# ---- Dependencies -------------------------------------------------------------------------------

# SystemC and taywee/args come from subprojects/*.wrap by default (force_fallback_for in the
# default_options). An installed copy is only looked up on explicit request, with Meson's own
# options: -Dforce_fallback_for= (installed if found, else the wrap) or --wrap-mode=nofallback
# (installed or an error; it wins over the project's default force_fallback_for, which Meson
# cannot tell apart from a user value). --wrap-mode=forcefallback always uses the wraps.
use_installed = {}
foreach name : ['systemc', 'args']
  use_installed += {name: get_option('wrap_mode') == 'nofallback' or
                          (get_option('wrap_mode') != 'forcefallback' and
                           name not in get_option('force_fallback_for'))}
endforeach

# An installed SystemC: pkg-config (distro/autotools installs), then its CMake package (honours
# -Dcmake_prefix_path=...). It is only accepted if it was built with our C++ standard, otherwise
# linking fails late with "undefined reference to sc_api_version_..._cxx<std>".
systemc_abi_check = '''
#include <systemc.h>
int sc_main(int, char *[]) { return 0; }
'''
# SystemC's CMake build registers its build trees in the CMake user package registry
# (~/.cmake/packages), which may point to stale scratch builds: never search it.
cmake_no_registry = ['-DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF',
                     '-DCMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=ON']
systemc_dep = dependency('', required: false)
# Where an installed specs/libspx.so finds a shared SystemC outside the default linker paths
# (Meson drops build-tree rpaths on install).
systemc_libdir = ''
if use_installed['systemc']
  systemc_dep = dependency('systemc', method: 'pkg-config', required: false, allow_fallback: false,
                           include_type: 'system')
  if systemc_dep.found() and not cxx.links(systemc_abi_check, dependencies: systemc_dep,
                                           name: 'pkg-config SystemC matches cpp_std')
    warning('Ignoring pkg-config SystemC: it was built with another C++ standard than', get_option('cpp_std'))
    systemc_dep = dependency('', required: false)
  endif
  if not systemc_dep.found()
    systemc_dep = dependency('SystemCLanguage', method: 'cmake', modules: ['SystemC::systemc'],
                             cmake_args: cmake_no_registry, required: false, include_type: 'system')
    if systemc_dep.found() and not cxx.links(systemc_abi_check, dependencies: systemc_dep,
                                             name: 'CMake SystemC matches cpp_std')
      warning('Ignoring CMake SystemC package: it was built with another C++ standard than',
              get_option('cpp_std'))
      systemc_dep = dependency('', required: false)
    endif
  endif
  if systemc_dep.found() and systemc_dep.type_name() == 'pkgconfig'
    systemc_libdir = systemc_dep.get_variable(pkgconfig: 'libdir', default_value: '')
  elif systemc_dep.found()
    foreach prefix : get_option('cmake_prefix_path')
      foreach libdir : ['lib64', 'lib']
        if systemc_libdir == '' and fs.exists(prefix / libdir / 'libsystemc.so')
          systemc_libdir = prefix / libdir
        endif
      endforeach
    endforeach
  endif
  if not systemc_dep.found() and get_option('wrap_mode') == 'nofallback'
    error('No usable installed SystemC (built as C++20), and --wrap-mode=nofallback forbids '
          + 'building it from subprojects/systemc.wrap. Pass -Dcmake_prefix_path=<SystemC prefix> '
          + 'or drop --wrap-mode=nofallback.')
  endif
endif
if not systemc_dep.found()
  message('Building SystemC from subprojects/systemc.wrap (CMake subproject)')
  systemc_opts = cmake.subproject_options()
  systemc_opts.add_cmake_defines({
    'CMAKE_CXX_STANDARD': '20',
    'CMAKE_CXX_STANDARD_REQUIRED': true,
    # Shared: a static SystemC would be copied into both libspx.so and unit_tests.
    'BUILD_SHARED_LIBS': true,
    'ENABLE_EXAMPLES': false,
    'DISABLE_COPYRIGHT_MESSAGE': true,
    'CMAKE_EXPORT_NO_PACKAGE_REGISTRY': true,
    'ENABLE_REGRESSION': false,
    'BUILD_SOURCE_DOCUMENTATION': false,
  })
  systemc_opts.set_override_option('cpp_std', get_option('cpp_std'))
  systemc_opts.set_override_option('warning_level', '0')
  # Meson cannot evaluate SystemC's $<COMPILE_LANGUAGE:ASM> generator expression, so its
  # -xassembler-with-cpp (meant for the QuickThreads .s file) lands on every C file, qt.c
  # included. -xnone restores suffix-based languages; the .s file then skips cpp, whose only
  # directive there (#if __linux__ && __ELF__ around .note.GNU-stack) gas reads as a comment.
  systemc_opts.append_compile_args('c', '-xnone')
  # libsystemc.so calls the user's sc_main: it must link with undefined symbols.
  systemc_opts.set_override_option('b_lundef', 'false')
  # Not sanitized, like the installed SystemC sanitizer builds used before: SystemC's process
  # dispatch (sc_process.h, member-function pointer on sc_process_host) trips UBSan's vptr check.
  systemc_opts.set_override_option('b_sanitize', 'none')
  # Installed next to specs (library only, no headers).
  systemc_opts.set_install(true)
  systemc_dep = cmake.subproject('systemc', options: systemc_opts).dependency('systemc',
                                                                             include_type: 'system')
  systemc_libdir = get_option('prefix') / get_option('libdir')
endif
if systemc_libdir in ['/usr/lib', '/usr/lib64'] or systemc_libdir.startswith('/usr/lib/')
  systemc_libdir = ''
endif

# taywee/args is header-only: an installed CMake package on request (same rule as SystemC), else
# subprojects/args.wrap through its CMake build (it only defines the include directory; nothing
# is built or installed).
args_dep = dependency('', required: false)
if use_installed['args']
  args_dep = dependency('args', method: 'cmake', modules: ['taywee::args'],
                        cmake_args: cmake_no_registry, required: get_option('wrap_mode') == 'nofallback',
                        allow_fallback: false, include_type: 'system')
endif
if not args_dep.found()
  args_opts = cmake.subproject_options()
  args_opts.add_cmake_defines({
    'ARGS_BUILD_EXAMPLE': false,
    'ARGS_BUILD_UNITTESTS': false,
    'CMAKE_EXPORT_NO_PACKAGE_REGISTRY': true,
  })
  args_opts.set_install(false)
  args_dep = cmake.subproject('args', options: args_opts).dependency('args', include_type: 'system')
endif

m_dep = cxx.find_library('m', required: false)

# ---- Sources ------------------------------------------------------------------------------------

# Defines core_sources, main_source, parser_dep, tb_sources and tb_args.
subdir('src')
# Defines version_dep (the generated <utils/version.h> for `specs --version` and the run manifest).
subdir('src/utils')

# ---- Targets ------------------------------------------------------------------------------------

specs_exe = executable('specs',
  core_sources, main_source, tb_sources,
  cpp_args: tb_args,
  dependencies: [systemc_dep, args_dep, m_dep, parser_dep, version_dep],
  implicit_include_directories: false,
  install: true,
  install_rpath: systemc_libdir,
)

# libspx.so: core + devices, without main(), the parser and the testbenches (sc_main is left to
# the consumer). Not built by default, like the former CMake target (EXCLUDE_FROM_ALL), unless
# -Dspx=true; install: true would put it into `all`, so both follow the option. (unit_tests links
# it, which builds it anyway when the tests are enabled.)
spx_lib = shared_library('spx',
  core_sources,
  dependencies: [systemc_dep, version_dep],
  implicit_include_directories: false,
  build_by_default: get_option('spx'),
  install: get_option('spx'),
  install_rpath: systemc_libdir,
)

# ---- Tests --------------------------------------------------------------------------------------

# Defines unit_tests_exe (or a disabler when the tests are disabled or gtest is missing).
subdir('src/test')

if get_option('old_tb')
  # One smoke test per testbench, as in the old `make test`. `specs -t` exits 0 even when a
  # testbench prints "Failure!", so these only catch crashes and errors.
  # Disabled in `make test` too: ring, ac_add_drop, ac_crow. (mesh was disabled there because it
  # segfaulted; it runs since B-047.)
  sh = find_program('sh')
  foreach tb : ['waveguide', 'splitter', 'merger', 'detector', 'directional_coupler',
                'phaseshifter', 'mzi', 'pcm', 'mesh']
    # Testbenches write traces/<name>.vcd relative to the cwd.
    test(tb, sh,
      args: ['-c', 'mkdir -p traces && exec "$0" -t "$1"', specs_exe.full_path(), tb],
      depends: specs_exe,
      workdir: meson.project_build_root(),
      suite: 'tb',
    )
  endforeach
endif

# Sanitizer builds: LeakSanitizer suppressions for the two accepted kinds of reports (see the file);
# print_suppressions=0 keeps stderr unchanged for the golden comparisons.
if get_option('b_sanitize').contains('address')
  add_test_setup('lsan',
    env: {'LSAN_OPTIONS': 'suppressions=' + (meson.project_source_root() / 'tests/sanitizers/lsan.supp')
                          + ':print_suppressions=0'},
    is_default: true,
  )
endif

if get_option('golden')
  subdir('tests/golden')
endif
subdir('tests/sparam')

# Accuracy suite (oracles) and benchmark ratios (F-06)
subdir('tests/bench')
# Tests of the Python tools in utils/ (activity map, F-31)
subdir('tests/tools')

# ---- Helper targets -----------------------------------------------------------------------------

doxygen = find_program('doxygen', required: get_option('docs'))
if doxygen.found()
  # Output: <builddir>/doc/html (the Doxyfile paths are relative to the repo root).
  run_target('doc',
    command: [find_program('utils/doc.sh'), doxygen, meson.project_source_root(),
              meson.project_build_root() / 'doc'],
  )
endif

# Trace helpers; they act on <repo>/traces, where `build/specs ...` run from the repo root writes.
traces_sh = find_program('utils/traces.sh')
traces_dir = meson.project_source_root() / 'traces'
foreach action : ['cleantraces', 'cleanoldtraces', 'waves', 'newwaves']
  run_target(action, command: [traces_sh, action, traces_dir])
endforeach

run_target('todos', command: [find_program('utils/todos.sh'), meson.project_source_root()])
