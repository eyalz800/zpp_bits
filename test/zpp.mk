#!/usr/bin/make -f
.SUFFIXES:
.SECONDARY:
.PHONY: \
	all \
	build \
	build_init \
	rebuild \
	clean_mode \
	clean

mode ?= debug
assembly ?= false
target_type ?=
projects ?=

ifeq ($(filter $(mode), debug release), )
$(error Mode must either be debug or release)
endif

ZPP_CONFIGURATION := $(mode)
ZPP_GENERATE_ASSEMBLY := $(assembly)
ZPP_TARGET_TYPE := $(target_type)

all: build
ZPP_THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))
ZPP_OUTPUT_DIRECTORY_ROOT := out
ZPP_INTERMEDIATE_DIRECTORY_ROOT := obj

ifeq ($(projects), )
ZPP_PROJECT_SETTINGS := true
include zpp_project.mk
ZPP_PROJECT_SETTINGS := false
endif

ifeq ($(ZPP_INCLUDE_PROJECTS), )
ZPP_INCLUDE_PROJECTS := $(projects)
endif

ifneq ($(ZPP_INCLUDE_PROJECTS), )
ZPP_PROJECTS_DIRECTORIES := $(ZPP_INCLUDE_PROJECTS)
ZPP_INCLUDE_PROJECTS :=

build:
	@set -e ; \
	for project in $(ZPP_PROJECTS_DIRECTORIES); do \
		echo "Entering '$$project'." ; \
		$(MAKE) projects= -s -f `realpath $(ZPP_THIS_MAKEFILE) --relative-to $$project` -C $$project; \
		echo "Leaving '$$project'." ; \
	done
clean:
	@set -e ; \
	for project in $(ZPP_PROJECTS_DIRECTORIES); do \
		echo "Entering '$$project'." ; \
		$(MAKE) projects= -s -f `realpath $(ZPP_THIS_MAKEFILE) --relative-to $$project` -C $$project clean ZPP_CLEANING=true ; \
		echo "Leaving '$$project'." ; \
	done
rebuild:
	@set -e ; \
	for project in $(ZPP_PROJECTS_DIRECTORIES); do \
		echo "Entering '$$project'." ; \
		$(MAKE) projects= -s -f `realpath $(ZPP_THIS_MAKEFILE) --relative-to $$project` -C $$project rebuild ZPP_CLEANING=true ; \
		echo "Leaving '$$project'." ; \
	done

else # ifneq ($(ZPP_INCLUDE_PROJECTS), )
ifeq ($(ZPP_TARGET_TYPE), )
build:
	@for target_type in $(ZPP_TARGET_TYPES); do \
		$(MAKE) -s -f $(ZPP_THIS_MAKEFILE) ZPP_TARGET_TYPE=$$target_type; \
	done
clean:
	@for target_type in $(ZPP_TARGET_TYPES); do \
		$(MAKE) -s -f $(ZPP_THIS_MAKEFILE) clean ZPP_CLEANING=true ZPP_TARGET_TYPE=$$target_type; \
	done
rebuild:
	@for target_type in $(ZPP_TARGET_TYPES); do \
		$(MAKE) -s -f $(ZPP_THIS_MAKEFILE) rebuild ZPP_CLEANING=true ZPP_TARGET_TYPE=$$target_type; \
	done
else # ifeq ($(ZPP_TARGET_TYPE), )

ifeq ($(filter $(ZPP_TARGET_TYPE), $(ZPP_TARGET_TYPES)), )
$(error Invalid target type)
endif # ($(filter $(ZPP_TARGET_TYPE), $(ZPP_TARGET_TYPES)), )

ifeq ($(ZPP_SOURCE_DIRECTORIES), )
ZPP_SOURCE_FILES := $(ZPP_SOURCE_FILES)
else
ZPP_SOURCE_FILES := $(ZPP_SOURCE_FILES) \
	$(shell find $(ZPP_SOURCE_DIRECTORIES) -type f -name "*.S") \
	$(shell find $(ZPP_SOURCE_DIRECTORIES) -type f -name "*.c") \
	$(shell find $(ZPP_SOURCE_DIRECTORIES) -type f -name "*.cpp") \
	$(shell find $(ZPP_SOURCE_DIRECTORIES) -type f -name "*.cxx") \
	$(shell find $(ZPP_SOURCE_DIRECTORIES) -type f -name "*.cc") \
	$(shell find $(ZPP_SOURCE_DIRECTORIES) -type f -name "*.cppm")
endif

ZPP_CPP_SOURCE_FILES := \
	$(filter %.cpp, $(ZPP_SOURCE_FILES)) \
	$(filter %.cxx, $(ZPP_SOURCE_FILES)) \
	$(filter %.cc, $(ZPP_SOURCE_FILES)) \
	$(filter %.cppm, $(ZPP_SOURCE_FILES))
ZPP_C_SOURCE_FILES := $(filter %.c, $(ZPP_SOURCE_FILES))
ZPP_ASSEMBLY_SOURCE_FILES := $(filter %.S, $(ZPP_SOURCE_FILES))

ifeq ($(strip $(ZPP_SOURCE_FILES)), )
$(error No source files)
endif

ZPP_INTERMEDIATE_DIRECTORY := $(ZPP_INTERMEDIATE_DIRECTORY_ROOT)/$(ZPP_CONFIGURATION)/$(ZPP_TARGET_TYPE)
ZPP_OUTPUT_DIRECTORY := $(ZPP_OUTPUT_DIRECTORY_ROOT)/$(ZPP_CONFIGURATION)/$(ZPP_TARGET_TYPE)
ZPP_COMPILE_COMMANDS_PATHS := $(ZPP_INTERMEDIATE_DIRECTORY)

ZPP_PATH_FROM_ROOT := $(shell echo $(ZPP_SOURCE_FILES) | grep -o "\(\.\./\)*" | sort --unique | tail -n 1)
ifneq ($(ZPP_PATH_FROM_ROOT), )
ZPP_INTERMEDIATE_SUBDIRECTORY := $(shell realpath . --relative-to $(ZPP_PATH_FROM_ROOT))
ZPP_INTERMEDIATE_DIRECTORY := $(ZPP_INTERMEDIATE_DIRECTORY)/$(ZPP_INTERMEDIATE_SUBDIRECTORY)
endif

ifeq ($(ZPP_COMPILE_COMMANDS_JSON), intermediate)
ZPP_COMPILE_COMMANDS_JSON := $(ZPP_INTERMEDIATE_DIRECTORY)/compile_commands.json
endif

ZPP_TOOLCHAIN_SETTINGS := true
include zpp_project.mk
ZPP_TOOLCHAIN_SETTINGS := false

ZPP_PROJECT_FLAGS := true
include zpp_project.mk
ZPP_PROJECT_FLAGS := false

ZPP_COMMA := ,
ZPP_EMPTY :=
ZPP_SPACE := $(ZPP_EMPTY) $(ZPP_EMPTY)

ifeq ($(ZPP_CONFIGURATION), debug)
ZPP_FLAGS := $(ZPP_FLAGS) $(ZPP_FLAGS_DEBUG)
ZPP_CFLAGS := $(ZPP_CFLAGS) $(ZPP_CFLAGS_DEBUG)
ZPP_CXXFLAGS := $(ZPP_CXXFLAGS) $(ZPP_CXXFLAGS_DEBUG)
ZPP_CXXMFLAGS := $(ZPP_CXXMFLAGS) $(ZPP_CXXMFLAGS_DEBUG)
ZPP_ASFLAGS := $(ZPP_ASFLAGS) $(ZPP_ASFLAGS_DEBUG)
ZPP_LFLAGS := $(ZPP_LFLAGS) $(ZPP_LFLAGS_DEBUG)
else ifeq ($(ZPP_CONFIGURATION), release)
ZPP_FLAGS := $(ZPP_FLAGS) $(ZPP_FLAGS_RELEASE)
ZPP_CFLAGS := $(ZPP_CFLAGS) $(ZPP_CFLAGS_RELEASE)
ZPP_CXXFLAGS := $(ZPP_CXXFLAGS) $(ZPP_CXXFLAGS_RELEASE)
ZPP_CXXMFLAGS := $(ZPP_CXXMFLAGS) $(ZPP_CXXMFLAGS_RELEASE)
ZPP_ASFLAGS := $(ZPP_ASFLAGS) $(ZPP_ASFLAGS_RELEASE)
ZPP_LFLAGS := $(ZPP_LFLAGS) $(ZPP_LFLAGS_RELEASE)
endif

ifeq ($(ZPP_CPP_MODULES_TYPE), )
ZPP_COMPILED_MODULE_FILES :=
else ifeq ($(ZPP_CPP_MODULES_TYPE), clang)
ZPP_COMPILED_MODULE_EXTENSION := pcm
else
$(error ZPP_CPP_MODULES_TYPE=$(ZPP_CPP_MODULES_TYPE) is unrecognized and not supported)
endif

ifneq ($(ZPP_CPP_MODULES_TYPE), )
ZPP_MODULE_PROPERTIES_FILES := $(patsubst %, $(ZPP_INTERMEDIATE_DIRECTORY)/%.props, $(ZPP_CPP_SOURCE_FILES))
ZPP_MODULE_DEPENDENCY_FILES := $(patsubst %, $(ZPP_INTERMEDIATE_DIRECTORY)/%.deps, $(ZPP_CPP_SOURCE_FILES))
ifeq ($(ZPP_CLEANING), )
-include $(ZPP_MODULE_PROPERTIES_FILES)
-include $(ZPP_MODULE_DEPENDENCY_FILES)
ZPP_MODULE_INTERFACE_DECLARATION_FLAGS := $(sort $(ZPP_MODULE_INTERFACE_DECLARATION_FLAGS))
endif

ZPP_COMPILED_MODULE_FILES := \
	$(ZPP_CPP_COMPILED_MODULE_FILES) \
	$(ZPP_CXX_COMPILED_MODULE_FILES) \
	$(ZPP_CC_COMPILED_MODULE_FILES) \
	$(ZPP_CPPM_COMPILED_MODULE_FILES)

define ZPP_CREATE_MODULE_DEPENDENCIES_SCRIPT
import os
import sys
def first(l): return l[0] if l else None
dependencies_file, source_file = sys.argv[1], sys.argv[2]
source_file_type = os.path.splitext(source_file)[1][1:]
module_properties_file = os.path.join('$(ZPP_INTERMEDIATE_DIRECTORY)', source_file + '.props')
intermediate_extension = '.S' if '$(ZPP_GENERATE_ASSEMBLY)' == 'true' else '.o'
dependency_directives = '\n'.join([ \
	line.strip() for line in sys.stdin.read().strip().replace('\r', '').split('\n') \
	if not line.strip().startswith('#')])
dependency_directives = [s.strip().split() for s in dependency_directives.split(';') if '<' not in s and '"' not in s]
dependency_directives = [s for s in dependency_directives if len(s) > 1 and s[0] in ['import', 'export', 'module']]
module_interface = first([m[2] for m in dependency_directives if len(m) == 3 and m[0] == 'export' and m[1] == 'module'])
module_implementation = first([m[1] for m in dependency_directives if len(m) == 2 and m[0] == 'module'])
needed_modules = [m[1] if m[0] in ['import', 'module'] else m[2] for m in dependency_directives \
				 if len(m) > 1 and (m[0] in ['import', 'module'] or (m[0], m[1]) == ('export', 'import'))]
translated_file = os.path.join('$(ZPP_INTERMEDIATE_DIRECTORY)', os.path.splitext(source_file)[0]) \
	+ ('.$(ZPP_COMPILED_MODULE_EXTENSION)' if module_interface else intermediate_extension)
needed_files = ['$$(ZPP_MODULE_FILE_{0})'.format(needed_module) for needed_module in needed_modules]
with open(module_properties_file, 'w') as f:
	f.write(''.join([
		'ZPP_MODULE_FILE_{0} := {1}\n'.format(module_interface, translated_file) if module_interface else '',
		''.join(['ZPP_MODULE_INTERFACE_DECLARATION_FLAGS += ',
			'-fmodule-file=', module_interface, '=', translated_file, '\n']) if module_interface else '',
		'ZPP_{0}_COMPILED_MODULE_FILES += {1}\n'.format(source_file_type.upper(), translated_file) if module_interface else '',
		'ZPP_{0}_COMPILED_NONMODULE_FILES += {1}\n'.format(source_file_type.upper(), translated_file) if not module_interface else '',
	]))
with open(dependencies_file, 'w') as f:
	f.write(''.join([
		''.join([translated_file, ': ', ' \\\n\t'.join([f for f in needed_files]), '\n\n']) if needed_files else '',
		''.join(['ZPP_MODULE_IMPLEMENTATION_FLAGS_', source_file, ' := ',
			'-fmodule-file=', '$$(ZPP_MODULE_FILE_{0})'.format(module_implementation), '\n']) if module_implementation else '',
		''.join(['ZPP_MODULE_IMPLEMENTATION_FLAGS_', source_file[2:], ' := ',
			'-fmodule-file=', '$$(ZPP_MODULE_FILE_{0})'.format(module_implementation), '\n']) \
				if module_implementation and source_file.startswith('./') else '',
	]))
endef
export ZPP_CREATE_MODULE_DEPENDENCIES_SCRIPT
ZPP_CREATE_MODULE_DEPENDENCIES := $(ZPP_PYTHON) -c "$$ZPP_CREATE_MODULE_DEPENDENCIES_SCRIPT"
endif # ifneq ($(ZPP_CPP_MODULES_TYPE), )

ZPP_C_ASSEMBLY_FILES := $(patsubst %.c, $(ZPP_INTERMEDIATE_DIRECTORY)/%.S, $(filter %.c, $(ZPP_C_SOURCE_FILES)))
ZPP_CPP_ASSEMBLY_FILES := $(patsubst %.cpp, $(ZPP_INTERMEDIATE_DIRECTORY)/%.S, $(filter %.cpp, $(ZPP_CPP_SOURCE_FILES)))
ZPP_CXX_ASSEMBLY_FILES := $(patsubst %.cxx, $(ZPP_INTERMEDIATE_DIRECTORY)/%.S, $(filter %.cxx, $(ZPP_CPP_SOURCE_FILES)))
ZPP_CC_ASSEMBLY_FILES := $(patsubst %.cc, $(ZPP_INTERMEDIATE_DIRECTORY)/%.S, $(filter %.cc, $(ZPP_CPP_SOURCE_FILES)))
ZPP_CPPM_ASSEMBLY_FILES := $(patsubst %.cppm, $(ZPP_INTERMEDIATE_DIRECTORY)/%.S, $(filter %.cppm, $(ZPP_CPP_SOURCE_FILES)))
ZPP_S_ASSEMBLY_FILES := $(patsubst %.S, $(ZPP_INTERMEDIATE_DIRECTORY)/%.S, $(filter %.S, $(ZPP_ASSEMBLY_SOURCE_FILES)))

ZPP_C_OBJECT_FILES := $(patsubst %.c, $(ZPP_INTERMEDIATE_DIRECTORY)/%.o, $(filter %.c, $(ZPP_C_SOURCE_FILES)))
ZPP_CPP_OBJECT_FILES := $(patsubst %.cpp, $(ZPP_INTERMEDIATE_DIRECTORY)/%.o, $(filter %.cpp, $(ZPP_CPP_SOURCE_FILES)))
ZPP_CXX_OBJECT_FILES := $(patsubst %.cxx, $(ZPP_INTERMEDIATE_DIRECTORY)/%.o, $(filter %.cxx, $(ZPP_CPP_SOURCE_FILES)))
ZPP_CC_OBJECT_FILES := $(patsubst %.cc, $(ZPP_INTERMEDIATE_DIRECTORY)/%.o, $(filter %.cc, $(ZPP_CPP_SOURCE_FILES)))
ZPP_CPPM_OBJECT_FILES := $(patsubst %.cppm, $(ZPP_INTERMEDIATE_DIRECTORY)/%.o, $(filter %.cppm, $(ZPP_CPP_SOURCE_FILES)))
ZPP_S_OBJECT_FILES := $(patsubst %.S, $(ZPP_INTERMEDIATE_DIRECTORY)/%.o, $(filter %.S, $(ZPP_ASSEMBLY_SOURCE_FILES)))

ZPP_OBJECT_FILES := \
	$(ZPP_C_OBJECT_FILES) \
	$(ZPP_CPP_OBJECT_FILES) \
	$(ZPP_CXX_OBJECT_FILES) \
	$(ZPP_CC_OBJECT_FILES) \
	$(ZPP_CPPM_OBJECT_FILES) \
	$(ZPP_S_OBJECT_FILES)

ZPP_OBJECT_FILES_DIRECTORIES := $(dir $(ZPP_OBJECT_FILES))

ZPP_DEPENDENCY_FILES := $(patsubst %.o, %.d, $(ZPP_OBJECT_FILES))

ifeq ($(ZPP_LINK_TYPE), default)
	ZPP_LINK_COMMAND := $(ZPP_LINK) -o $(ZPP_OUTPUT_DIRECTORY)/$(ZPP_TARGET_NAME) $(ZPP_OBJECT_FILES) $(ZPP_LFLAGS)
else ifeq ($(ZPP_LINK_TYPE), ld)
	ZPP_LINK_COMMAND := $(ZPP_LINK) -o $(ZPP_OUTPUT_DIRECTORY)/$(ZPP_TARGET_NAME) $(ZPP_OBJECT_FILES) $(ZPP_LFLAGS)
else ifeq ($(ZPP_LINK_TYPE), link)
	ZPP_LINK_COMMAND := $(ZPP_LINK) $(ZPP_LFLAGS) /out:$(ZPP_OUTPUT_DIRECTORY)/$(ZPP_TARGET_NAME) $(ZPP_OBJECT_FILES)
else ifeq ($(ZPP_LINK_TYPE), ar)
	ZPP_LINK_COMMAND := $(ZPP_AR) rcs $(ZPP_OUTPUT_DIRECTORY)/$(ZPP_TARGET_NAME) $(ZPP_OBJECT_FILES)
else
$(error ZPP_LINK_TYPE must either be default, ld, link, or ar)
endif

define ZPP_GENERATE_COMPILE_COMMANDS_SCRIPT
import os
import json
compile_commands = []
for root, _, files in os.walk('$(ZPP_COMPILE_COMMANDS_PATHS)'):
	for file in files:
		if not file.endswith('.zppcmd'):
			continue
		full_file = os.path.join(root, file)
		with open(full_file, 'r') as f:
			command, source_file = f.read().split('\n')[:2]
		compile_commands.append(
			{
				'directory': os.path.abspath('.'),
				'file': os.path.abspath(source_file),
				'command': command,
			}
		)
	with open('$(ZPP_COMPILE_COMMANDS_JSON)', 'w') as f:
		json.dump(compile_commands, f, indent=2, sort_keys=True)
endef
export ZPP_GENERATE_COMPILE_COMMANDS_SCRIPT
ZPP_CALL_GENERATE_COMPILE_COMMANDS_SCRIPT := $(ZPP_PYTHON) -c "$$ZPP_GENERATE_COMPILE_COMMANDS_SCRIPT"

ifeq ($(ZPP_GENERATE_ASSEMBLY), false)
ZPP_INTERMEDIATE_EXTENSION := o
ZPP_COMPILE_INTERMEDIATE_FLAG := -c
ZPP_C_COMPILED_FILES := $(ZPP_C_OBJECT_FILES)
ifeq ($(ZPP_CPP_MODULES_TYPE), )
ZPP_CPP_COMPILED_NONMODULE_FILES := $(ZPP_CPP_OBJECT_FILES)
ZPP_CXX_COMPILED_NONMODULE_FILES := $(ZPP_CXX_OBJECT_FILES)
ZPP_CC_COMPILED_NONMODULE_FILES := $(ZPP_CC_OBJECT_FILES)
ZPP_CPPM_COMPILED_NONMODULE_FILES := $(ZPP_CPPM_OBJECT_FILES)
endif
else ifeq ($(ZPP_GENERATE_ASSEMBLY), true)
ZPP_INTERMEDIATE_EXTENSION := S
ZPP_COMPILE_INTERMEDIATE_FLAG := -S
ZPP_C_COMPILED_FILES := $(ZPP_C_ASSEMBLY_FILES)
ifeq ($(ZPP_CPP_MODULES_TYPE), )
ZPP_CPP_COMPILED_NONMODULE_FILES := $(ZPP_CPP_ASSEMBLY_FILES)
ZPP_CXX_COMPILED_NONMODULE_FILES := $(ZPP_CXX_ASSEMBLY_FILES)
ZPP_CC_COMPILED_NONMODULE_FILES := $(ZPP_CC_ASSEMBLY_FILES)
ZPP_CPPM_COMPILED_NONMODULE_FILES := $(ZPP_CPPM_ASSEMBLY_FILES)
endif
else
$(error ZPP_GENERATE_ASSEMBLY must either be true or false)
endif

build: $(ZPP_COMPILE_COMMANDS_JSON) $(ZPP_OUTPUT_DIRECTORY)/$(ZPP_TARGET_NAME)
	@echo "Built '$(ZPP_TARGET_TYPE)/$(ZPP_TARGET_NAME)'."

build_init:
	@echo "Building '$(ZPP_TARGET_TYPE)/$(ZPP_TARGET_NAME)' in '$(ZPP_CONFIGURATION)' mode..."; \
	mkdir -p $(ZPP_INTERMEDIATE_DIRECTORY); \
	mkdir -p $(ZPP_OUTPUT_DIRECTORY); \
	mkdir -p $(ZPP_OBJECT_FILES_DIRECTORIES)

build_dep_init:
	@echo "Building dependencies for '$(ZPP_TARGET_TYPE)/$(ZPP_TARGET_NAME)'..."; \
	mkdir -p $(ZPP_INTERMEDIATE_DIRECTORY); \
	mkdir -p $(ZPP_OUTPUT_DIRECTORY); \
	mkdir -p $(ZPP_OBJECT_FILES_DIRECTORIES)

$(ZPP_OUTPUT_DIRECTORY)/$(ZPP_TARGET_NAME): $(ZPP_OBJECT_FILES)
	@echo "Linking '$(ZPP_OUTPUT_DIRECTORY)/$(ZPP_TARGET_NAME)'..."; \
	set -e; \
	$(ZPP_LINK_COMMAND); \
	$(ZPP_POSTLINK_COMMANDS)

clean:
	@echo "Cleaning '$(ZPP_TARGET_TYPE)/$(ZPP_TARGET_NAME)'..."; \
	rm -rf $(ZPP_INTERMEDIATE_DIRECTORY_ROOT)/debug/$(ZPP_TARGET_TYPE); \
	rm -rf $(ZPP_INTERMEDIATE_DIRECTORY_ROOT)/release/$(ZPP_TARGET_TYPE); \
	rm -f $(ZPP_OUTPUT_DIRECTORY_ROOT)/debug/$(ZPP_TARGET_TYPE)/$(ZPP_TARGET_NAME); \
	rm -f $(ZPP_OUTPUT_DIRECTORY_ROOT)/release/$(ZPP_TARGET_TYPE)/$(ZPP_TARGET_NAME); \
	rm -f $(ZPP_COMPILE_COMMANDS_JSON); \
	find $(ZPP_INTERMEDIATE_DIRECTORY_ROOT) -type d -empty -delete 2> /dev/null; \
	find $(ZPP_OUTPUT_DIRECTORY_ROOT) -type d -empty -delete 2> /dev/null; \
	echo "Cleaned '$(ZPP_TARGET_TYPE)/$(ZPP_TARGET_NAME)'."

rebuild: clean_mode
	@$(MAKE) -s -f $(ZPP_THIS_MAKEFILE) build ZPP_CLEANING=

clean_mode:
	@rm -rf $(ZPP_INTERMEDIATE_DIRECTORY); \
	rm -rf $(ZPP_OUTPUT_DIRECTORY)/$(ZPP_TARGET_NAME)

ifneq ($(ZPP_COMPILE_COMMANDS_JSON), )
$(ZPP_COMPILE_COMMANDS_JSON): $(patsubst %, %.zppcmd, $(ZPP_OBJECT_FILES)) $(patsubst %, %.zppcmd, $(ZPP_COMPILED_MODULE_FILES))
	@echo "Building '$@'..."; \
	$(ZPP_CALL_GENERATE_COMPILE_COMMANDS_SCRIPT)
endif

$(ZPP_C_COMPILED_FILES): $(ZPP_INTERMEDIATE_DIRECTORY)/%.$(ZPP_INTERMEDIATE_EXTENSION): %.c | build_init $(ZPP_COMPILE_COMMANDS_JSON)
	@echo "Compiling '$<'..."; \
	$(ZPP_CC) $(ZPP_COMPILE_INTERMEDIATE_FLAG) $(ZPP_CFLAGS) -o $@ $< \
		-MD -MP -MF `dirname $@`/`basename $@ .$(ZPP_INTERMEDIATE_EXTENSION)`.d

$(patsubst %.$(ZPP_INTERMEDIATE_EXTENSION), %.o.zppcmd, $(ZPP_C_COMPILED_FILES)): \
		$(ZPP_INTERMEDIATE_DIRECTORY)/%.o.zppcmd: %.c | build_init
	@echo '$(ZPP_CC) -c $(ZPP_CFLAGS) -o '`dirname $@`/`basename $@ .zppcmd`' $< ' \
		'-MD -MP -MF '`dirname $@`/`basename $@ .o.zppcmd`.d > $@; \
	echo $< >> $@

$(ZPP_CPP_COMPILED_NONMODULE_FILES): $(ZPP_INTERMEDIATE_DIRECTORY)/%.$(ZPP_INTERMEDIATE_EXTENSION): %.cpp | build_init $(ZPP_COMPILE_COMMANDS_JSON)
	@echo "Compiling '$<'..."; \
	$(ZPP_CXX) $(ZPP_COMPILE_INTERMEDIATE_FLAG) $(ZPP_CXXFLAGS) -o $@ $< \
		$(ZPP_MODULE_INTERFACE_DECLARATION_FLAGS) $(ZPP_MODULE_IMPLEMENTATION_FLAGS_$<) \
		-MD -MP -MF `dirname $@`/`basename $@ .$(ZPP_INTERMEDIATE_EXTENSION)`.d

$(patsubst %.$(ZPP_INTERMEDIATE_EXTENSION), %.o.zppcmd, $(ZPP_CPP_COMPILED_NONMODULE_FILES)): \
		$(ZPP_INTERMEDIATE_DIRECTORY)/%.o.zppcmd: %.cpp | build_init
	@echo '$(ZPP_CXX) $(ZPP_COMPILE_INTERMEDIATE_FLAG) $(ZPP_CXXFLAGS) -o '`dirname $@`/`basename $@ .zppcmd`' $< ' \
		'$(ZPP_MODULE_INTERFACE_DECLARATION_FLAGS) $(ZPP_MODULE_IMPLEMENTATION_FLAGS_$<) ' \
		'-MD -MP -MF '`dirname $@`/`basename $@ .o.zppcmd`.d > $@; \
	echo $< >> $@

$(ZPP_CXX_COMPILED_NONMODULE_FILES): $(ZPP_INTERMEDIATE_DIRECTORY)/%.$(ZPP_INTERMEDIATE_EXTENSION): %.cxx | build_init $(ZPP_COMPILE_COMMANDS_JSON)
	@echo "Compiling '$<'..."; \
	$(ZPP_CXX) $(ZPP_COMPILE_INTERMEDIATE_FLAG) $(ZPP_CXXFLAGS) -o $@ $< \
		$(ZPP_MODULE_INTERFACE_DECLARATION_FLAGS) $(ZPP_MODULE_IMPLEMENTATION_FLAGS_$<) \
		-MD -MP -MF `dirname $@`/`basename $@ .$(ZPP_INTERMEDIATE_EXTENSION)`.d

$(patsubst %.$(ZPP_INTERMEDIATE_EXTENSION), %.o.zppcmd, $(ZPP_CXX_COMPILED_NONMODULE_FILES)): \
		$(ZPP_INTERMEDIATE_DIRECTORY)/%.o.zppcmd: %.cxx | build_init
	@echo '$(ZPP_CXX) $(ZPP_COMPILE_INTERMEDIATE_FLAG) $(ZPP_CXXFLAGS) -o '`dirname $@`/`basename $@ .zppcmd`' $< ' \
		'$(ZPP_MODULE_INTERFACE_DECLARATION_FLAGS) $(ZPP_MODULE_IMPLEMENTATION_FLAGS_$<) ' \
		'-MD -MP -MF '`dirname $@`/`basename $@ .o.zppcmd`.d > $@; \
	echo $< >> $@

$(ZPP_CC_COMPILED_NONMODULE_FILES): $(ZPP_INTERMEDIATE_DIRECTORY)/%.$(ZPP_INTERMEDIATE_EXTENSION): %.cc | build_init $(ZPP_COMPILE_COMMANDS_JSON)
	@echo "Compiling '$<'..."; \
	$(ZPP_CXX) $(ZPP_COMPILE_INTERMEDIATE_FLAG) $(ZPP_CXXFLAGS) -o $@ $< \
		$(ZPP_MODULE_INTERFACE_DECLARATION_FLAGS) $(ZPP_MODULE_IMPLEMENTATION_FLAGS_$<) \
		-MD -MP -MF `dirname $@`/`basename $@ .$(ZPP_INTERMEDIATE_EXTENSION)`.d

$(patsubst %.$(ZPP_INTERMEDIATE_EXTENSION), %.o.zppcmd, $(ZPP_CC_COMPILED_NONMODULE_FILES)): \
		$(ZPP_INTERMEDIATE_DIRECTORY)/%.o.zppcmd: %.cc | build_init
	@echo '$(ZPP_CXX) -c $(ZPP_CXXFLAGS) -o '`dirname $@`/`basename $@ .zppcmd`' $< ' \
		'$(ZPP_MODULE_INTERFACE_DECLARATION_FLAGS) $(ZPP_MODULE_IMPLEMENTATION_FLAGS_$<) ' \
		'-MD -MP -MF '`dirname $@`/`basename $@ .o.zppcmd`.d > $@; \
	echo $< >> $@

$(ZPP_CPPM_COMPILED_NONMODULE_FILES): $(ZPP_INTERMEDIATE_DIRECTORY)/%.$(ZPP_INTERMEDIATE_EXTENSION): %.cppm | build_init $(ZPP_COMPILE_COMMANDS_JSON)
	@echo "Compiling '$<'..."; \
	$(ZPP_CXX) $(ZPP_COMPILE_INTERMEDIATE_FLAG) $(ZPP_CXXFLAGS) -o $@ $< \
		$(ZPP_MODULE_INTERFACE_DECLARATION_FLAGS) $(ZPP_MODULE_IMPLEMENTATION_FLAGS_$<) \
		-MD -MP -MF `dirname $@`/`basename $@ .$(ZPP_INTERMEDIATE_EXTENSION)`.d

$(patsubst %.$(ZPP_INTERMEDIATE_EXTENSION), %.o.zppcmd, $(ZPP_CPPM_COMPILED_NONMODULE_FILES)): \
		$(ZPP_INTERMEDIATE_DIRECTORY)/%.o.zppcmd: %.cppm | build_init
	@echo '$(ZPP_CXX) -c $(ZPP_CXXFLAGS) -o '`dirname $@`/`basename $@ .zppcmd`' $< ' \
		'$(ZPP_MODULE_INTERFACE_DECLARATION_FLAGS) $(ZPP_MODULE_IMPLEMENTATION_FLAGS_$<) ' \
		'-MD -MP -MF '`dirname $@`/`basename $@ .o.zppcmd`.d > $@; \
	echo $< >> $@

$(patsubst %.$(ZPP_COMPILED_MODULE_EXTENSION), %.$(ZPP_INTERMEDIATE_EXTENSION), $(ZPP_COMPILED_MODULE_FILES)): \
		%.$(ZPP_INTERMEDIATE_EXTENSION): %.$(ZPP_COMPILED_MODULE_EXTENSION) | build_init $(ZPP_COMPILE_COMMANDS_JSON)
	@echo "Compiling '$<'..."; \
	$(ZPP_CXX) $(ZPP_COMPILE_INTERMEDIATE_FLAG) $(ZPP_CXXMFLAGS) -o $@ $<

$(patsubst %.$(ZPP_COMPILED_MODULE_EXTENSION), %.o.zppcmd, $(ZPP_CPP_COMPILED_MODULE_FILES)): \
		$(ZPP_INTERMEDIATE_DIRECTORY)/%.o.zppcmd: %.cpp | build_init
	@echo '$(ZPP_CXX) -c $(ZPP_CXXMFLAGS) -o ' \
		`dirname $@`/`basename $@ .zppcmd` `dirname $@`/`basename $@ .o.zppcmd`.$(ZPP_COMPILED_MODULE_EXTENSION) > $@; \
	echo `dirname $@`/`basename $@ .o.zppcmd`.$(ZPP_COMPILED_MODULE_EXTENSION) >> $@

$(patsubst %.$(ZPP_COMPILED_MODULE_EXTENSION), %.o.zppcmd, $(ZPP_CXX_COMPILED_MODULE_FILES)): \
		$(ZPP_INTERMEDIATE_DIRECTORY)/%.o.zppcmd: %.cxx | build_init
	@echo '$(ZPP_CXX) -c $(ZPP_CXXMFLAGS) -o ' \
		`dirname $@`/`basename $@ .zppcmd` `dirname $@`/`basename $@ .o.zppcmd`.$(ZPP_COMPILED_MODULE_EXTENSION) > $@; \
	echo `dirname $@`/`basename $@ .o.zppcmd`.$(ZPP_COMPILED_MODULE_EXTENSION) >> $@

$(patsubst %.$(ZPP_COMPILED_MODULE_EXTENSION), %.o.zppcmd, $(ZPP_CC_COMPILED_MODULE_FILES)): \
		$(ZPP_INTERMEDIATE_DIRECTORY)/%.o.zppcmd: %.cc | build_init
	@echo '$(ZPP_CXX) -c $(ZPP_CXXMFLAGS) -o ' \
		`dirname $@`/`basename $@ .zppcmd` `dirname $@`/`basename $@ .o.zppcmd`.$(ZPP_COMPILED_MODULE_EXTENSION) > $@; \
	echo `dirname $@`/`basename $@ .o.zppcmd`.$(ZPP_COMPILED_MODULE_EXTENSION) >> $@

$(patsubst %.$(ZPP_COMPILED_MODULE_EXTENSION), %.o.zppcmd, $(ZPP_CPPM_COMPILED_MODULE_FILES)): \
		$(ZPP_INTERMEDIATE_DIRECTORY)/%.o.zppcmd: %.cppm | build_init
	@echo '$(ZPP_CXX) -c $(ZPP_CXXMFLAGS) -o ' \
		`dirname $@`/`basename $@ .zppcmd` `dirname $@`/`basename $@ .o.zppcmd`.$(ZPP_COMPILED_MODULE_EXTENSION) > $@; \
	echo `dirname $@`/`basename $@ .o.zppcmd`.$(ZPP_COMPILED_MODULE_EXTENSION) >> $@

ifeq ($(ZPP_CPP_MODULES_TYPE), clang)
$(ZPP_CPP_COMPILED_MODULE_FILES): \
		$(ZPP_INTERMEDIATE_DIRECTORY)/%.$(ZPP_COMPILED_MODULE_EXTENSION): %.cpp | build_init $(ZPP_COMPILE_COMMANDS_JSON)
	@echo "Compiling '$<'..."; \
	$(ZPP_CXX) --precompile -x c++-module $(ZPP_CXXFLAGS) -o $@ $< \
		$(ZPP_MODULE_INTERFACE_DECLARATION_FLAGS) $(ZPP_MODULE_FLAG_$<) \
		-MD -MP -MF `dirname $@`/`basename $@ .$(ZPP_COMPILED_MODULE_EXTENSION)`.d

$(patsubst %, %.zppcmd, $(ZPP_CPP_COMPILED_MODULE_FILES)): \
		$(ZPP_INTERMEDIATE_DIRECTORY)/%.$(ZPP_COMPILED_MODULE_EXTENSION).zppcmd: %.cpp | build_init
	@echo '$(ZPP_CXX) --precompile -x c++-module $(ZPP_CXXFLAGS) -o '`dirname $@`/`basename $@ .zppcmd` ' $< ' \
		'$(ZPP_MODULE_INTERFACE_DECLARATION_FLAGS) $(ZPP_MODULE_FLAG_$<) ' \
		'-MD -MP -MF '`dirname $@`/`basename $@ .$(ZPP_COMPILED_MODULE_EXTENSION).zppcmd`.d > $@; \
	echo $< >> $@

$(ZPP_CXX_COMPILED_MODULE_FILES): \
		$(ZPP_INTERMEDIATE_DIRECTORY)/%.$(ZPP_COMPILED_MODULE_EXTENSION): %.cxx | build_init $(ZPP_COMPILE_COMMANDS_JSON)
	@echo "Compiling '$<'..."; \
	$(ZPP_CXX) --precompile -x c++-module $(ZPP_CXXFLAGS) -o $@ $< \
		$(ZPP_MODULE_INTERFACE_DECLARATION_FLAGS) $(ZPP_MODULE_FLAG_$<) \
		-MD -MP -MF `dirname $@`/`basename $@ .$(ZPP_COMPILED_MODULE_EXTENSION)`.d

$(patsubst %, %.zppcmd, $(ZPP_CXX_COMPILED_MODULE_FILES)): \
		$(ZPP_INTERMEDIATE_DIRECTORY)/%.$(ZPP_COMPILED_MODULE_EXTENSION).zppcmd: %.cxx | build_init
	@echo '$(ZPP_CXX) --precompile -x c++-module $(ZPP_CXXFLAGS) -o '`dirname $@`/`basename $@ .zppcmd` ' $< ' \
		'$(ZPP_MODULE_INTERFACE_DECLARATION_FLAGS) $(ZPP_MODULE_FLAG_$<) ' \
		'-MD -MP -MF '`dirname $@`/`basename $@ .$(ZPP_COMPILED_MODULE_EXTENSION).zppcmd`.d > $@; \
	echo $< >> $@

$(ZPP_CC_COMPILED_MODULE_FILES): \
		$(ZPP_INTERMEDIATE_DIRECTORY)/%.$(ZPP_COMPILED_MODULE_EXTENSION): %.cc | build_init $(ZPP_COMPILE_COMMANDS_JSON)
	@echo "Compiling '$<'..."; \
	$(ZPP_CXX) --precompile -x c++-module $(ZPP_CXXFLAGS) -o $@ $< \
		$(ZPP_MODULE_INTERFACE_DECLARATION_FLAGS) $(ZPP_MODULE_FLAG_$<) \
		-MD -MP -MF `dirname $@`/`basename $@ .$(ZPP_COMPILED_MODULE_EXTENSION)`.d

$(patsubst %, %.zppcmd, $(ZPP_CC_COMPILED_MODULE_FILES)): \
		$(ZPP_INTERMEDIATE_DIRECTORY)/%.$(ZPP_COMPILED_MODULE_EXTENSION).zppcmd: %.cc | build_init
	@echo '$(ZPP_CXX) --precompile -x c++-module $(ZPP_CXXFLAGS) -o '`dirname $@`/`basename $@ .zppcmd` ' $< ' \
		'$(ZPP_MODULE_INTERFACE_DECLARATION_FLAGS) $(ZPP_MODULE_FLAG_$<) ' \
		'-MD -MP -MF '`dirname $@`/`basename $@ .$(ZPP_COMPILED_MODULE_EXTENSION).zppcmd`.d > $@; \
	echo $< >> $@

$(ZPP_CPPM_COMPILED_MODULE_FILES): \
		$(ZPP_INTERMEDIATE_DIRECTORY)/%.$(ZPP_COMPILED_MODULE_EXTENSION): %.cppm | build_init $(ZPP_COMPILE_COMMANDS_JSON)
	@echo "Compiling '$<'..."; \
	$(ZPP_CXX) --precompile -x c++-module $(ZPP_CXXFLAGS) -o $@ $< \
		$(ZPP_MODULE_INTERFACE_DECLARATION_FLAGS) $(ZPP_MODULE_FLAG_$<) \
		-MD -MP -MF `dirname $@`/`basename $@ .$(ZPP_COMPILED_MODULE_EXTENSION)`.d

$(patsubst %, %.zppcmd, $(ZPP_CPPM_COMPILED_MODULE_FILES)): \
		$(ZPP_INTERMEDIATE_DIRECTORY)/%.$(ZPP_COMPILED_MODULE_EXTENSION).zppcmd: %.cppm | build_init
	@echo '$(ZPP_CXX) --precompile -x c++-module $(ZPP_CXXFLAGS) -o '`dirname $@`/`basename $@ .zppcmd` ' $< ' \
		'$(ZPP_MODULE_INTERFACE_DECLARATION_FLAGS) $(ZPP_MODULE_FLAG_$<) ' \
		'-MD -MP -MF '`dirname $@`/`basename $@ .$(ZPP_COMPILED_MODULE_EXTENSION).zppcmd`.d > $@; \
	echo $< >> $@
endif # ifeq ($(ZPP_CPP_MODULES_TYPE), clang)

$(ZPP_S_OBJECT_FILES): $(ZPP_INTERMEDIATE_DIRECTORY)/%.o: %.S | build_init $(ZPP_COMPILE_COMMANDS_JSON)
	@echo "Assemblying '$<'..."; \
	$(ZPP_AS) -c $(ZPP_ASFLAGS) -o $@ $< -MD -MP -MF `dirname $@`/`basename $@ .o`.d

$(patsubst %, %.zppcmd, $(ZPP_S_OBJECT_FILES)): $(ZPP_INTERMEDIATE_DIRECTORY)/%.o.zppcmd: %.S | build_init
	@echo '$(ZPP_AS) -c $(ZPP_ASFLAGS) -o '`dirname $@`/`basename $@ .zppcmd`' $< \
		-MD -MP -MF '`dirname $@`/`basename $@ .o.zppcmd`.d > $@; \
	echo $< >> $@

ifeq ($(ZPP_GENERATE_ASSEMBLY), true)
$(ZPP_C_OBJECT_FILES) $(ZPP_CPP_OBJECT_FILES) $(ZPP_CXX_OBJECT_FILES) $(ZPP_CC_OBJECT_FILES) $(ZPP_CPPM_OBJECT_FILES): %.o: %.S
	@echo "Assemblying '$<'..."; \
	$(ZPP_CC) -Wno-unicode -c -o $@ $<
endif

ifeq ($(ZPP_CPP_MODULES_TYPE), clang)
$(ZPP_MODULE_DEPENDENCY_FILES): $(ZPP_INTERMEDIATE_DIRECTORY)/%.deps: % | build_dep_init
	@$(ZPP_CXX) $(ZPP_CXXFLAGS) -Wno-unused-command-line-argument -E \
		-Xclang -print-dependency-directives-minimized-source $< 2> /dev/null \
		| $(ZPP_CREATE_MODULE_DEPENDENCIES) $@ $<
endif

ifeq ($(ZPP_CLEANING), )
-include $(ZPP_DEPENDENCY_FILES)
endif

ZPP_PROJECT_RULES := true
include zpp_project.mk
ZPP_PROJECT_RULES := false

endif # ifeq ($(ZPP_TARGET_TYPE), )
endif # ifneq ($(ZPP_INCLUDE_PROJECTS), )
