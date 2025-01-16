# Use bash as the shell
SHELL = /bin/bash

LOCAL_CONFIG_EXISTS = $(shell (test -e config.local.mk && echo 1) || echo 0)
ifeq "${LOCAL_CONFIG_EXISTS}" "1"
  CONFIG_MK_FILE = config.local.mk
  #CONFIG_MK_FILE = config.windows.mk
else
  CONFIG_MK_FILE = config.mk
endif

include $(CONFIG_MK_FILE)

BUILD_TB ?= 0
BUILD_PATH_LIB ?= $(BUILD_PATH)-lib

CXX_LD ?= $(CXX)

CXXFLAGS += -MMD -MP
CXXFLAGS_LIB = $(CXXFLAGS) -fpic 
LDFLAGS_LIB = $(LDFLAGS) -shared -Wl,-soname,libspecs.so
BISONFLAGS += --warnings -Wall #-Wcex

# Find all source files in the source directory
SOURCES_BIN = $(shell find $(SRC_PATH) -name '*.cpp' -not -ipath '*/tb/*')
SOURCES_LIB = $(shell find $(SRC_PATH) -name '*.cpp' -not -ipath '*/tb/*' -not -name 'main.cpp' -not -ipath '*/parser/*.cpp')
SOURCES_TB = $(shell find $(SRC_PATH) -name '*.cpp' -ipath '*/tb/*')
SOURCES_TB_MAIN = $(shell find $(SRC_PATH) -name 'alltestbenches.cpp' -ipath '*/tb/*')
SOURCES_ALLFILES = $(shell find $(SRC_PATH) -name '*.cpp' -or -name '*.h')

# Add the whole source directory tree to INCLUDES
INCLUDES += $(addprefix -I,$(shell find $(SRC_PATH) -type d))
INCLUDES += -iquote"$(BUILD_PATH)/parser"

# Find all bison/flex source files
# - Note: bison/flex sources must have a matching filename
FLEX_SOURCES = $(shell find $(SRC_PATH) -name '*.l')
BISON_SOURCES = $(shell find $(SRC_PATH) -name '*.y')

# Set the object filenames from the cpp sources
OBJECTS_BIN = $(SOURCES_BIN:$(SRC_PATH)/%.cpp=$(BUILD_PATH)/%.o)
ifeq ($(BUILD_TB), 1)
  OBJECTS_BIN += $(SOURCES_TB:$(SRC_PATH)/%.cpp=$(BUILD_PATH)/%.o)
  CXXFLAGS += -DBUILD_TB
else
  OBJECTS_BIN += $(SOURCES_TB_MAIN:$(SRC_PATH)/%.cpp=$(BUILD_PATH)/%.o)
endif

# Add the objects filenames from the bison/flex sources
GEN_PARSE     =  $(BUILD_PATH)/parser/scanner.cpp $(BUILD_PATH)/parser/parser.cpp
OBJECTS_PARSE =  $(BUILD_PATH)/parser/scanner.o
OBJECTS_PARSE += $(BUILD_PATH)/parser/parser.o

#OBJECTS_BIN   += $(OBJECTS_PARSE)

# Same without main (without testbenches)
OBJECTS_LIB = $(SOURCES_LIB:$(SRC_PATH)/%.cpp=$(BUILD_PATH_LIB)/%.o)

# Set the dependency files that will be used to add header dependencies
DEPS = $(OBJECTS_BIN:.o=.d)
DEPS_PARSE = $(OBJECTS_PARSE:.o=.d)
DEPS_LIB = $(OBJECTS_LIB:.o=.d)

# Set other dependencies that should trigger rebuilding .o files
ADDITIONAL_DEPS = Makefile $(CONFIG_MK_FILE)

# Find the latest trace according to modification time
LATEST_TRACE_NAME = $(shell ls -1rt --group-directories-first traces | tail -1)

# Be verbose
V ?= 0

# Set/Unset the Q flag according to the V flag
ifeq ($(V),1)
  Q = 
  QQ = @
else
  Q = @
  QQ = @
endif

# Clear known suffixes rules
.SUFFIXES:

# Define phony targets
.PHONY: todos format waves newwaves print-% help cleandoc cleanoldtraces \
	cleantraces cleanall clean compiledb view-doc upload-doc doc readme \
	all bin lib

# Instruct make not to remove intermediate files from bison/flex compilation
# TODO: update
.SECONDARY: $(BUILD_PATH)/parser.h)
.SECONDARY: $(BUILD_PATH)/parser.cpp
.SECONDARY: $(BUILD_PATH)/scanner.h
.SECONDARY: $(BUILD_PATH)/scanner.cpp

# Default rule
all: bin

# Build executable
bin: $(BIN_NAME)

# Build shared library
lib: $(LIB_NAME)

# Link all objects together into executable
$(BIN_NAME): $(OBJECTS_PARSE) $(OBJECTS_BIN)
	@echo "Linking binary"
	$(Q)$(CCACHE) $(CXX_LD) $(OBJECTS_BIN) $(OBJECTS_PARSE) $(LDFLAGS) -o $@
	@echo "Done"

# Link all objects together into library
$(LIB_NAME): $(OBJECTS_LIB)
	@echo "Linking library"
	$(Q)$(CCACHE) $(CXX_LD) $(OBJECTS_LIB) $(LDFLAGS_LIB) -o $@
	@echo "Done"

# Objects depend on their directories
$(OBJECTS_PARSE): $(ADDITIONAL_DEPS) | $(dir $(OBJECTS_PARSE))

# Objects depend on their directories
$(OBJECTS_BIN): $(ADDITIONAL_DEPS) | $(dir $(OBJECTS_BIN))

# Objects depend on their directories
$(OBJECTS_LIB): $(ADDITIONAL_DEPS) | $(dir $(OBJECTS_LIB))

# Rule to create the $(BUILD_PATH) directory
$(BUILD_PATH)/:
	$(Q)mkdir -p $@

# Rule to create a subdirectory in $(BUILD_PATH)
$(BUILD_PATH)/%/:
	$(Q)mkdir -p $@

# Rule to create the $(BUILD_PATH_LIB) directory
$(BUILD_PATH_LIB)/:
	$(Q)mkdir -p $@

# Rule to create a subdirectory in $(BUILD_PATH_LIB)
$(BUILD_PATH_LIB)/%/:
	$(Q)mkdir -p $@

# Default .cpp → .o compilation rule
$(BUILD_PATH)/%.o: $(SRC_PATH)/%.cpp | $(GEN_PARSE)
	@echo "Compiling $<"
	$(Q)$(CCACHE) $(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Default .cpp → .o compilation rule for library
$(BUILD_PATH_LIB)/%.o: $(SRC_PATH)/%.cpp
	@echo "Compiling $<"
	$(Q)$(CCACHE) $(CXX) $(CXXFLAGS_LIB) $(INCLUDES) -MMD -MP -c $< -o $@

# Default .cpp → .o compilation rule
$(BUILD_PATH)/parser/%.o: $(BUILD_PATH)/parser/%.cpp
	@echo "Compiling $<"
	$(Q)$(CCACHE) $(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

## Special .cpp compilation rule for flex generated sources
#%.yy.o: %.yy.cpp %.tab.h
#	@echo "Compiling $<"
#	$(Q)$(CCACHE) $(CXX) $(CXXFLAGS) $(INCLUDES) -iquote"$(dir $@)" -c $< -o $@
#
## Special .cpp compilation rule for bison generated sources
#%.tab.o: %.tab.cpp %.yy.h
#	@echo "Compiling $<"
#	$(Q)$(CCACHE) $(CXX) $(CXXFLAGS) $(INCLUDES) -iquote"$(dir $@)" -c $< -o $@

$(BUILD_PATH)/parser/scanner.cpp: $(SRC_PATH)/parser/parser.l | $(dir $($@))
	@echo "Lexing $<"
	$(Q)flex --header-file="$(@:.cpp=.h)" -o $@ $<

$(BUILD_PATH)/parser/parser.cpp: $(SRC_PATH)/parser/parser.y $(BUILD_PATH)/parser/scanner.cpp | $(dir $($@))
	@echo "Bisoning $<"
	$(Q)bison $(BISONFLAGS) --defines="$(@:.cpp=.h)" -o $@ -d $<

# Dependency rule for parser headers
$(BUILD_PATH)/parser/%.h: | $(BUILD_PATH)/parser/%.cpp
	@:

# # Default .l → .yy.cpp compilation rule
# $(BUILD_PATH)/%.yy.cpp: $(SRC_PATH)/%.l | $(dir $($@))
# 	@echo "Lexing $<"
# 	$(Q)flex --header-file="$(@:.cpp=.h)" -o $@ $<
# 
# # Default .y → .tab.{h,cpp} compilation rule
# $(BUILD_PATH)/%.tab.cpp: $(SRC_PATH)/%.y $(BUILD_PATH)/%.yy.cpp | $(dir $($@))
# 	@echo "Bisoning $<"
# 	$(Q)bison $(BISONFLAGS) --defines="$(@:.cpp=.h)" -o $@ -d $<
# 
# # Dependency rule for .tab.h (bison output, needed to compile flex output)
# %.tab.h: | %.tab.cpp
# 	@:
# 
# # Dependency rule for .yy.h (flex output, needed to compile bison output)
# %.yy.h: | %.yy.cpp
# 	@:

#$(ADDITIONAL_DEPS):
#@:

# Add dependency files, if they exist
-include $(DEPS)
-include $(DEPS_LIB)
-include $(DEPS_PARSE)

# Generation of documentation using Doxygen
doc:
	@echo "Generating documentation"
	@mkdir -p doc/html
	@cp -r doc/resources/ doc/html/
	$(Q)doxygen doc/Doxyfile
	#$(Q)tar -c -f doc/pcm-doc.tar.xz -C doc/html .

# Regenerate compile_commands.json for YouCompleteMe
compiledb: compile_commands.json

# Regenerate compile_commands.json for YouCompleteMe
compile_commands.json: clean
	@echo "Generating compile_commands.json"
	$(Q)compiledb --verbose make -j8
	$(Q)sed -i '/"-MMD"\|"-MP"/d' $@

# Clean build artifacts
clean:
	@echo "Removing build artifacts"
	$(QQ)#[ ! -d 'build' ] || rm -rf build/
	$(QQ)#[ ! -d 'build-lib' ] || rm -rf build-lib/
	$(Q)[ ! -d "$(BUILD_PATH)" ] || rm -rf "$(BUILD_PATH)"
	$(Q)[ ! -d "$(BUILD_PATH_LIB)" ] || rm -rf "$(BUILD_PATH_LIB)"

# Remove any product of the build (build, trace, doc, bin)
cleanall: clean cleantraces cleandoc
	@echo "Removing binary"
	$(Q)rm -f Pout.obj Pout.png detector_trace.txt
	$(Q)rm -f sim specs libspecs.so

# Clean all traces
cleantraces:
	@echo "Removing all traces"
	$(Q)find traces/ -maxdepth 1 -type f -and -iname '*.vcd' -delete

# Clean all traces but the newest
cleanoldtraces:
	@echo "Removing all traces but newest"
	$(Q)find traces/ -maxdepth 1 -iname '*.vcd' -and -type f -and ! -iname "$(LATEST_TRACE_NAME)" -delete

# Clean documentation output
cleandoc:
	@echo "Removing generated documentation"
	$(Q)rm -rf doc/html doc/pcm-doc.tar.xz

# Show the latest trace with a blank GTKWave config
newwaves:
	@echo "Opening latest trace file"
	@mkdir -p traces/configs
	@bash utils/openwave.sh "traces/$(LATEST_TRACE_NAME)"

# Show the latest trace with a menu to select which GTKWave config to use
waves:
	@echo "Opening latest trace file"
	@mkdir -p traces/configs
	@bash utils/openwave.sh "traces/$(LATEST_TRACE_NAME)" traces/configs

test:
	@echo "Running all tests"
	$(Q)./specs -t waveguide
	$(Q)./specs -t splitter
	$(Q)./specs -t merger
	$(Q)./specs -t detector
	$(Q)./specs -t directional_coupler
	$(Q)./specs -t phaseshifter
	$(Q)./specs -t mzi
	$(Q)./specs -t pcm
	$(Q)#./specs -t ring
	$(Q)#./specs -t ac_add_drop
	$(Q)#./specs -t ac_crow
	$(Q)#./specs -t mesh

rc: all
	./${BIN_NAME} -fcircuits/rc_mohab.cir


# TODO: add a gate so we don't accidentally "mess up" the working 
#       directory by shadowing real changes. Maybe do a commit or something
#       like this ? or check the git status whether there are changes ?
#       Automatic formatting should be automatically run on write, but if
#       modifying a file with a different format and re-formatting, changes
#       seen by git could be far from obvious to re-read later
# Run clang-format on the source (in place)
format:
	@echo "Formatting source"
	@clang-format -i $(SOURCES_ALLFILES)

# Print all TODOs found in the source and other files
todos:
	@echo "TODO (from code):"
	@find $(SRC_PATH) -type f | xargs awk '/TODO:/ {split($$0, arr, "TODO:"); content = arr[2]; print "- " content, "("FILENAME":"FNR")"}' || true
	@#find module_definitions/ -type f | xargs awk '/TODO:/ {split($$0, arr, "TODO:"); content = arr[2]; print "- " content, "("FILENAME":"FNR")"}' || true
	@find utils/ -type f | xargs awk '/TODO:/ {split($$0, arr, "TODO:"); content = arr[2]; print "- " content, "("FILENAME":"FNR")"}' || true
	@cat todo.txt || true

# Print a given variable
print-%:
	$(Q)echo $* = $($*)

# FIXME: cannot have ":" in rule descriptions
# Show this help prompt
help:
	@ echo
	@ echo '  Usage:'
	@ echo ''
	@ echo '    make <target> [flags...]'
	@ echo ''
	@ echo '  Targets:'
	@ echo ''
	@ awk '/^#/{ comment = substr($$0,3) } comment && /^[a-zA-Z][a-zA-Z0-9_-]+ ?:/{ print "   ", $$1, comment }' $(MAKEFILE_LIST) | column -t -s ':' | sort
	@ echo ''
	@ echo '  Flags:'
	@ echo ''
	@awk '/^#/{ comment = substr($$0,3) } comment && /^[a-zA-Z][a-zA-Z0-9_-]*/ && /\??=/ && /[01]$$/{ print "   " $$1 " (default = " $$3 ")",":",comment }' $(MAKEFILE_LIST) | column -t -s ':' | sort
	@ echo ''
