# Netlist parser: flex/bison generated scanner and parser plus the parse tree.
# Generated headers land in <builddir>/src/parser and are included as "parser.h"/"scanner.h"
# (by the parser sources and by src/main.cpp, through parser_dep).
flex = find_program('flex')
bison = find_program('bison', version: '>=3.0')  # %define api.pure full

scanner_gen = custom_target('scanner',
  input: 'parser.l',
  output: ['scanner.cpp', 'scanner.h'],
  command: [flex, '--header-file=@OUTPUT1@', '-o', '@OUTPUT0@', '@INPUT@'],
)
parser_gen = custom_target('parser',
  input: 'parser.y',
  output: ['parser.cpp', 'parser.h'],
  command: [bison, '--warnings', '-Wall', '--defines=@OUTPUT1@', '-o', '@OUTPUT0@', '@INPUT@'],
)

# Generated headers only (include_directories('.') would also add the source dir).
parser_inc_args = ['-I' + meson.current_build_dir()]

# A separate (non-PIC) archive so that, as with the former CMake object library, the parser is
# compiled without the testbench define. link_whole keeps every object, like an object library.
parser_lib = static_library('specs_parser',
  files(
    'parse_analysis.cpp',
    'parse_directive.cpp',
    'parse_element.cpp',
    'parse_tree.cpp',
  ),
  scanner_gen, parser_gen,
  dependencies: [systemc_dep],
  cpp_args: parser_inc_args,
  implicit_include_directories: false,
  pic: false,
)
parser_dep = declare_dependency(
  link_whole: parser_lib,
  compile_args: parser_inc_args,
  sources: [scanner_gen[1], parser_gen[1]],
)
