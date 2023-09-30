# Quickstart guide {#quickstart}

[TOC]

# Dependencies

## Building toolchain

* `git`
* `bash`
* `find`
* `make`
* a c++ compiler that supports c++14 or c++17 (`g++` or `clang++`)
* GNU `flex` (`lex` is untested)
* GNU `bison` (`yacc` is untested)

Optional:

* `python3` for generating the sources using the module definitions
* ...

## External build dependencies

* `systemc` (v2.3.3)

## Documentation generation

* `doxygen`
* `dot` (via `graphviz` for example)

## Traces visualization

* GTKWave
* `python3`
* `python3-matplotlib`
* ...

# Building

* First, make sure to install all the dependencies on your system, either locally or globally.
* Then, fetch the code:

      % git clone git@gitlab.inl90.ec-lyon.fr:czrounba/systemc-pcm-matrix-multiplication-simulator.git
      % git submodule update --init --recursive

* Copy `config.mk` to `config.local.mk` and update the compiler/library information there
* If you installed systemc in a non-default path, you might need to update the
`INCLUDES` and `LD_FLAGS` make variables in `config.mk`.
* Finally, build `specs`
      % make -j$(nproc)
* Run a test circuits
      % ./sim -t {waveguide,splitter,pcm,...}

# Running

Simulating a circuit from a netlist is straightforward:

    % ./sim -f <my-circuit>.cir

Look at the [cheatsheet](doc/resources/cheatsheet.pdf) for more information on the syntax to use.
[](TODO: add a page for info on syntax of circuit files)

After running, you can find results in the VCD tracefile (the file name can be specified in the circuit file with option `-o`).

# Generating documentation (OUTDATED)

First make sure `doxygen` and `graphviz` are installed, then:

* To generate the doc, run:

      % make doc

* To open it in your default browser, run:

      % make view-doc

The generated documentation is available as HTML under `doc/html/` and compressed
into a `tar.xz` archive under `doc/`. Other ouput formats are supported by `doxygen`.

# Troubleshooting

<!--
If you cannot manage to build the simulator, you can check the `.gitlab-ci.yml`
file which contains all the instructions to run to successfully build the code
in a docker:

* Install docker ([docker.io](https://docker.io)) and start it
* Start an alpine container:

      % docker run -it alpine:latest

* Run all commands from the `before_script` section
* Clone the git repo and update submodules:

      % git clone <url> <local-folder>
      % cd <local-folder>
	  % git submodule update --init --recursive

* Run all commands from the `build:script` section
* Run all commands from the `test:script` section
* Optional: Run all commands from the `doc:script` section
-->
