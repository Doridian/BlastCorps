VERSION  := us.v11

BUILD_DIR = build

TOOLS_DIR := ../tools
ASM_PROCESSOR_DIR := $(TOOLS_DIR)/asm-processor

# One source tree for every version.  The objects are us.v11's, named by
# their us.v11 offsets, and each version's config lists the ones it has
# (tools/gen_code_yaml.py --like).  C that differs between versions is
# #if'd on VERSION_US_V10, VERSION_US_V11, VERSION_JP or VERSION_EU.
SRC_DIR = src
VERSION_DEFINE := VERSION_$(subst .,_,$(shell echo $(VERSION) | tr a-z A-Z))
# the C files of a module: its config's `c` subsegments
c_files = $(shell sed -n 's/^ *- \[0x[0-9A-Fa-f]*, c, \([^]]*\)\].*/$(SRC_DIR)\/\1.c/p' $(1).$(VERSION).yaml 2>/dev/null)

# gzip mtimes, taken from each ROM's own gzip headers.  They are part of the
# compressed stream, so getting one wrong makes the rebuilt section differ from
# the original even when the code inside it is identical.
ifeq ($(VERSION),us.v10)
HD_CODE_TEXT_TIMESTAMP      = 855502464
HD_CODE_DATA_TIMESTAMP      = 855502465
HD_FRONT_END_TEXT_TIMESTAMP = 855502466
HD_FRONT_END_DATA_TIMESTAMP = 855502466
else ifeq ($(VERSION),us.v11)
HD_CODE_TEXT_TIMESTAMP      = 855919890
HD_CODE_DATA_TIMESTAMP      = 855919891
HD_FRONT_END_TEXT_TIMESTAMP = 855919891
HD_FRONT_END_DATA_TIMESTAMP = 855919891
else ifeq ($(VERSION),jp)
HD_CODE_TEXT_TIMESTAMP      = 855921039
HD_CODE_DATA_TIMESTAMP      = 855921040
HD_FRONT_END_TEXT_TIMESTAMP = 855921040
HD_FRONT_END_DATA_TIMESTAMP = 855921040
else ifeq ($(VERSION),eu)
HD_CODE_TEXT_TIMESTAMP      = 862230153
HD_CODE_DATA_TIMESTAMP      = 862230153
HD_FRONT_END_TEXT_TIMESTAMP = 862230154
HD_FRONT_END_DATA_TIMESTAMP = 862230154
else
$(error Unknown VERSION '$(VERSION)'; expected one of us.v10 us.v11 jp eu)
endif

# init
INIT_ASM_DIRS  = asm/init asm/data/init
INIT_BIN_DIRS  = assets/init
INIT_SRC_DIR   = $(SRC_DIR)/init
INIT_SRC_DIRS  = $(INIT_SRC_DIR)

INIT_S_FILES   = $(foreach dir,$(INIT_ASM_DIRS),$(wildcard $(dir)/*.s))
INIT_C_FILES   = $(call c_files,init)
INIT_BIN_FILES = $(foreach dir,$(INIT_BIN_DIRS),$(wildcard $(dir)/*.bin))

INIT_O_FILES := $(foreach file,$(INIT_S_FILES),$(BUILD_DIR)/$(file).o) \
                $(foreach file,$(INIT_C_FILES),$(BUILD_DIR)/$(file).o) \
                $(foreach file,$(INIT_BIN_FILES),$(BUILD_DIR)/$(file).o)

#asm/hd_code asm/hd_front_end
HD_CODE_ASM_DIRS  = asm/hd_code asm/data/hd_code
HD_CODE_BIN_DIRS  = assets/hd_code
HD_CODE_SRC_DIR   = $(SRC_DIR)/hd_code
HD_CODE_SRC_DIRS  = $(HD_CODE_SRC_DIR)

HD_CODE_S_FILES   = $(foreach dir,$(HD_CODE_ASM_DIRS),$(wildcard $(dir)/*.s))
HD_CODE_C_FILES   = $(call c_files,hd_code)
HD_CODE_BIN_FILES = $(foreach dir,$(HD_CODE_BIN_DIRS),$(wildcard $(dir)/*.bin))

HD_CODE_O_FILES := $(foreach file,$(HD_CODE_S_FILES),$(BUILD_DIR)/$(file).o) \
                   $(foreach file,$(HD_CODE_C_FILES),$(BUILD_DIR)/$(file).o) \
                   $(foreach file,$(HD_CODE_BIN_FILES),$(BUILD_DIR)/$(file).o)

# asm/hd_front_end
HD_FRONT_END_ASM_DIRS  = asm/hd_front_end asm/data/hd_front_end
HD_FRONT_END_BIN_DIRS  = assets/hd_front_end
HD_FRONT_END_SRC_DIR   = $(SRC_DIR)/hd_front_end
HD_FRONT_END_SRC_DIRS  = $(HD_FRONT_END_SRC_DIR)

HD_FRONT_END_S_FILES   = $(foreach dir,$(HD_FRONT_END_ASM_DIRS),$(wildcard $(dir)/*.s))
HD_FRONT_END_C_FILES   = $(call c_files,hd_front_end)
HD_FRONT_END_BIN_FILES = $(foreach dir,$(HD_FRONT_END_BIN_DIRS),$(wildcard $(dir)/*.bin))

HD_FRONT_END_O_FILES := $(foreach file,$(HD_FRONT_END_S_FILES),$(BUILD_DIR)/$(file).o) \
                        $(foreach file,$(HD_FRONT_END_C_FILES),$(BUILD_DIR)/$(file).o) \
                        $(foreach file,$(HD_FRONT_END_BIN_FILES),$(BUILD_DIR)/$(file).o)


# Tooling
CROSS   = mips-linux-gnu-
AS      = $(CROSS)as
CPP     = cpp
LD      = $(CROSS)ld
OBJDUMP = $(CROSS)objdump
OBJCOPY = $(CROSS)objcopy
PYTHON  = python3
GZIP    = gzip


CC      = $(TOOLS_DIR)/ido5.3_recomp/cc

OBJCOPYFLAGS = -O binary

OPT_FLAGS := -O1
MIPSISET := -mips2 -o32

INCLUDE_CFLAGS := -I . -I include -I include/2.0I -I include/2.0I/PR

# --no-pad-sections: handwritten code between the data islands in hd_code
# starts and ends at odd offsets, and padding its .text out to 16 would shift
# everything after it.
ASFLAGS = -EB -mtune=vr4300 -march=vr4300 -mabi=32 -I include --no-pad-sections

CFLAGS := -G 0 -Xfullwarn -Xcpluscomm -signed -nostdinc -non_shared -Wab,-r4300_mul
CFLAGS += -D_LANGUAGE_C -D_FINALROM -D$(VERSION_DEFINE)
# ignore compiler warnings about anonymous structs
CFLAGS += -woff 649,838
CFLAGS += $(INCLUDE_CFLAGS)

# Linker config
INIT_TARGET = $(BUILD_DIR)/init.$(VERSION)
INIT_LD_SCRIPT = init.$(VERSION).ld

HD_CODE_TARGET = $(BUILD_DIR)/hd_code.$(VERSION)
HD_CODE_LD_SCRIPT = hd_code.$(VERSION).ld

HD_FRONT_END_TARGET = $(BUILD_DIR)/hd_front_end.$(VERSION)
HD_FRONT_END_LD_SCRIPT = hd_front_end.$(VERSION).ld

# The symbol files splat and we write give every name its address in the
# original ROM.  tools/link_syms.py turns them into what each link uses (see
# there): names the module's objects define are dropped, addresses inside
# the module become relative to its symbols, addresses in another module are
# looked up in that module's layout link, ROM offsets of segments become the
# top-level ROM's symbols, and the fixed memory map stays as it is.
SYMFILES = undefined_syms_auto.$(1).$(VERSION).txt module_syms_auto.$(1).$(VERSION).txt undefined_funcs_auto.$(1).$(VERSION).txt undefined_syms.$(1).$(VERSION).txt
LINK_SYMS = $(PYTHON) $(TOOLS_DIR)/link_syms.py

# ROM positions: of the assets (fixed), and of everything including the
# compressed modules as built (for init's boot code).
ROM_SYMS_FIXED = $(BUILD_DIR)/rom_fixed.$(VERSION).ld
ROM_SYMS       = $(BUILD_DIR)/rom.$(VERSION).ld
GZ_FILES       = $(BUILD_DIR)/hd_code_text.$(VERSION).gz $(BUILD_DIR)/hd_code_data.$(VERSION).gz \
                 $(BUILD_DIR)/hd_front_end_text.$(VERSION).gz $(BUILD_DIR)/hd_front_end_data.$(VERSION).gz

# SHIFT=1: a build that is laid out differently from the original, to show
# that nothing depends on absolute addresses.  SHIFT_PAD bytes of padding
# (zeros, which are nops) go at the start of hd_code's and hd_front_end's
# .text and .data; everything after moves.  Nothing is sha1-checked.
# (--defsym has to come before the scripts that test it with DEFINED().)
SHIFT ?= 0
ifneq ($(SHIFT),0)
SHIFT_PAD     ?= 0x10
SHIFT_LDFLAGS := --defsym SHIFT_PAD=$(SHIFT_PAD)
endif

INIT_LDFLAGS    = --emit-relocs -T $(INIT_LD_SCRIPT) -Map $(INIT_TARGET).map -T $(BUILD_DIR)/init.$(VERSION).syms.ld -T $(ROM_SYMS) --no-check-sections
HD_CODE_LDFLAGS = $(SHIFT_LDFLAGS) --emit-relocs -T $(HD_CODE_LD_SCRIPT) -Map $(HD_CODE_TARGET).map -T $(BUILD_DIR)/hd_code.$(VERSION).syms.ld -T $(ROM_SYMS_FIXED) --no-check-sections
HD_FRONT_END_LDFLAGS = $(SHIFT_LDFLAGS) --emit-relocs -T $(HD_FRONT_END_LD_SCRIPT) -Map $(HD_FRONT_END_TARGET).map -T $(BUILD_DIR)/hd_front_end.$(VERSION).syms.ld -T $(ROM_SYMS_FIXED) --no-check-sections


### Optimisation Overrides

# Files not built at the default -O1 (libultra's gu and audio code, for
# example), as paths under $(SRC_DIR).  The same in every version.
O2_C_FILES :=
O3_C_FILES := hd_code/90760.c hd_code/90800.c hd_code/909C0.c hd_code/90E40.c hd_code/91050.c hd_code/912C0.c hd_code/91360.c hd_code/91830.c hd_code/92130.c hd_code/92350.c hd_code/923B0.c hd_code/926F0.c hd_code/927B0.c hd_code/92800.c hd_code/92A30.c hd_code/92AD0.c hd_code/92B60.c hd_code/92BF0.c hd_code/92C80.c hd_code/92D20.c hd_code/92DA0.c hd_code/92F00.c hd_code/92F30.c hd_code/939F0.c hd_code/93A30.c hd_code/93AA0.c hd_code/93B50.c hd_code/93B60.c hd_code/93BA0.c hd_code/93DF0.c hd_code/95020.c hd_code/95070.c hd_code/950E0.c hd_code/95460.c hd_code/96800.c hd_code/97140.c hd_code/99580.c hd_code/99620.c hd_code/996A0.c hd_code/996B0.c hd_code/9AFF0.c hd_code/9B470.c hd_code/9BBF0.c hd_code/9BC90.c hd_code/9BDE0.c hd_code/9C930.c hd_code/9CC30.c hd_code/9D810.c hd_code/9D920.c hd_code/9F7D0.c hd_code/9FDF0.c hd_code/9FE00.c hd_code/9FE20.c hd_code/A0AA0.c
$(foreach f,$(O2_C_FILES),$(BUILD_DIR)/$(SRC_DIR)/$(f).o): OPT_FLAGS := -O2
$(foreach f,$(O3_C_FILES),$(BUILD_DIR)/$(SRC_DIR)/$(f).o): OPT_FLAGS := -O3

# libultra's ll.c (the long long helpers) is built -mips3 -32, as in the SDK.
MIPS3_C_FILES := init/2D60.c hd_code/90390.c hd_code/95C40.c
# ld won't mix it with the -mips2 objects, so it's relabelled mips2 after.
$(foreach f,$(MIPS3_C_FILES),$(BUILD_DIR)/$(SRC_DIR)/$(f).o): MIPSISET := -mips3 -32
$(foreach f,$(MIPS3_C_FILES),$(BUILD_DIR)/$(SRC_DIR)/$(f).o): POSTCC := $(PYTHON) $(TOOLS_DIR)/elf_mips2.py


### Targets

default: all

# asm/ and assets/ hold one version at a time, so leftovers from a previous
# VERSION would quietly be linked into this one.  Refuse to build on a tree
# that was last used for something else.
stamp:
	@if [ -f .version ] && [ "$$(cat .version)" != "$(VERSION)" ]; then \
		echo "error: tree holds $$(cat .version) output; run 'make clean' first" >&2; \
		exit 1; \
	fi
	@echo $(VERSION) > .version

all: verify_init verify_hd_code verify_hd_front_end

dirs:
	$(foreach dir,$(INIT_SRC_DIR) $(INIT_ASM_DIRS) $(INIT_BIN_DIRS) $(HD_CODE_SRC_DIR) $(HD_CODE_ASM_DIRS) $(HD_CODE_BIN_DIRS) $(HD_FRONT_END_SRC_DIR) $(HD_FRONT_END_ASM_DIRS) $(HD_FRONT_END_BIN_DIRS),$(shell mkdir -p $(BUILD_DIR)/$(dir)))


init: dirs $(INIT_TARGET).bin

hd_code: dirs $(HD_CODE_TARGET).bin

hd_front_end: dirs $(HD_FRONT_END_TARGET).bin

ifeq ($(SHIFT),0)
VERIFY = echo "$$(cat $(1).$(VERSION).sha1)  $(BUILD_DIR)/$(1).$(VERSION).bin" | sha1sum --check
else
VERIFY = echo "$(BUILD_DIR)/$(1).$(VERSION).bin: shifted by $(SHIFT_PAD), not checked"
endif

verify_init: init
	@$(call VERIFY,init)

verify_hd_code: hd_code
	@$(call VERIFY,hd_code)

verify_hd_front_end: hd_front_end
	@$(call VERIFY,hd_front_end)

extract: stamp
	$(PYTHON) $(TOOLS_DIR)/split.py init.$(VERSION).yaml
	$(PYTHON) $(TOOLS_DIR)/split.py hd_code.$(VERSION).yaml
	$(PYTHON) $(TOOLS_DIR)/split.py hd_front_end.$(VERSION).yaml

compress: $(GZ_FILES) $(INIT_TARGET).bin
	@echo "Replacing extracted init..."
	cp $(INIT_TARGET).bin ../assets/init.$(VERSION).bin
	@echo "Replacing extracted hd_code..."
	cp $(BUILD_DIR)/hd_code_text.$(VERSION).gz ../assets/hd_code_text.$(VERSION).bin
	cp $(BUILD_DIR)/hd_code_data.$(VERSION).gz ../assets/hd_code_data.$(VERSION).bin
	@echo "Replacing extracted hd_front_end..."
	cp $(BUILD_DIR)/hd_front_end_text.$(VERSION).gz ../assets/hd_front_end_text.$(VERSION).bin
	cp $(BUILD_DIR)/hd_front_end_data.$(VERSION).gz ../assets/hd_front_end_data.$(VERSION).bin

clean:
	rm -rf asm
	rm -rf assets
	rm -rf build
	rm -f *auto.txt
	rm -f *.ld
	rm -f .version


### Recipes

# Relink when SHIFT changes, though no object does.
$(BUILD_DIR)/shift.stamp: FORCE
	@mkdir -p $(BUILD_DIR)
	@if [ "$$(cat $@ 2>/dev/null)" != "$(SHIFT) $(SHIFT_LDFLAGS)" ]; then echo "$(SHIFT) $(SHIFT_LDFLAGS)" > $@; fi

# The assets' positions come from their built sizes, so the top level's
# asset build (tools/assets.py, shifted too with SHIFT=1) goes first.  A tree
# without the top level's assets extracted gets the config's positions.
../build/assets/.stamp: FORCE
	@if [ -f ../assets/layout.yaml ]; then \
		$(MAKE) --no-print-directory -C .. VERSION=$(VERSION) SHIFT=$(SHIFT) assets; \
	elif [ ! -f $@ ]; then mkdir -p $(@D) && touch $@; fi

$(ROM_SYMS_FIXED): ../blastcorps.$(VERSION).yaml $(TOOLS_DIR)/rom_syms.py ../build/assets/.stamp
	@mkdir -p $(BUILD_DIR)
	cd .. && $(PYTHON) tools/rom_syms.py $(VERSION) --fixed-only --out blastcorps/$@

$(ROM_SYMS): ../blastcorps.$(VERSION).yaml $(TOOLS_DIR)/rom_syms.py $(GZ_FILES) ../build/assets/.stamp
	cd .. && $(PYTHON) tools/rom_syms.py $(VERSION) --out blastcorps/$@ \
		$(foreach m,hd_code_text hd_code_data hd_front_end_text hd_front_end_data,--module $(m)=blastcorps/$(BUILD_DIR)/$(m).$(VERSION).gz)

# Each module is linked twice.  The layout link places everything with other
# modules' symbols at their original addresses; the final link takes them
# from the other modules' layout links.  (A module's layout doesn't depend on
# those values, so there is no cycle.)  Outside SHIFT, every name must still
# be at its original address.
LINK_DEPS = $(TOOLS_DIR)/link_syms.py $(TOOLS_DIR)/modmap.py $(BUILD_DIR)/shift.stamp
ifeq ($(SHIFT),0)
CHECK_SYMS = $(LINK_SYMS) --check $@ $(1) $(VERSION) $(call SYMFILES,$(1))
endif

$(BUILD_DIR)/init.$(VERSION).syms.ld: $(call SYMFILES,init) $(INIT_O_FILES) $(ROM_SYMS) $(LINK_DEPS)
	@$(LINK_SYMS) init $(VERSION) -o $@ --rom $(ROM_SYMS) --report $(@:.ld=.txt) $(call SYMFILES,init)

$(BUILD_DIR)/%.$(VERSION).layout.ld: $(ROM_SYMS_FIXED) $(LINK_DEPS)
	@$(LINK_SYMS) $* $(VERSION) -o $@ --rom $(ROM_SYMS_FIXED) $(call SYMFILES,$*)

$(BUILD_DIR)/%.$(VERSION).layout.elf: $(BUILD_DIR)/%.$(VERSION).layout.ld
	@$(LD) $(SHIFT_LDFLAGS) -T $*.$(VERSION).ld -T $< -T $(ROM_SYMS_FIXED) --no-check-sections -o $@

$(BUILD_DIR)/hd_code.$(VERSION).layout.ld: $(call SYMFILES,hd_code) $(HD_CODE_O_FILES)
$(BUILD_DIR)/hd_front_end.$(VERSION).layout.ld: $(call SYMFILES,hd_front_end) $(HD_FRONT_END_O_FILES)
$(BUILD_DIR)/hd_code.$(VERSION).layout.elf: $(HD_CODE_O_FILES)
$(BUILD_DIR)/hd_front_end.$(VERSION).layout.elf: $(HD_FRONT_END_O_FILES)

$(BUILD_DIR)/hd_code.$(VERSION).syms.ld: $(call SYMFILES,hd_code) $(HD_CODE_O_FILES) $(ROM_SYMS_FIXED) \
                                        $(BUILD_DIR)/hd_front_end.$(VERSION).layout.elf $(LINK_DEPS)
	@$(LINK_SYMS) hd_code $(VERSION) -o $@ --rom $(ROM_SYMS_FIXED) --report $(@:.ld=.txt) \
		--layout hd_front_end=$(BUILD_DIR)/hd_front_end.$(VERSION).layout.elf $(call SYMFILES,hd_code)

$(BUILD_DIR)/hd_front_end.$(VERSION).syms.ld: $(call SYMFILES,hd_front_end) $(HD_FRONT_END_O_FILES) $(ROM_SYMS_FIXED) \
                                             $(BUILD_DIR)/hd_code.$(VERSION).layout.elf $(LINK_DEPS)
	@$(LINK_SYMS) hd_front_end $(VERSION) -o $@ --rom $(ROM_SYMS_FIXED) --report $(@:.ld=.txt) \
		--layout hd_code=$(BUILD_DIR)/hd_code.$(VERSION).layout.elf $(call SYMFILES,hd_front_end)

$(BUILD_DIR)/init.$(VERSION).elf: $(INIT_O_FILES) $(BUILD_DIR)/init.$(VERSION).syms.ld
	@$(LD) $(INIT_LDFLAGS) -o $@
	@$(call CHECK_SYMS,init)

$(BUILD_DIR)/hd_code.$(VERSION).elf: $(HD_CODE_O_FILES) $(BUILD_DIR)/hd_code.$(VERSION).syms.ld
	@$(LD) $(HD_CODE_LDFLAGS) -o $@
	@$(call CHECK_SYMS,hd_code)

$(BUILD_DIR)/hd_front_end.$(VERSION).elf: $(HD_FRONT_END_O_FILES) $(BUILD_DIR)/hd_front_end.$(VERSION).syms.ld
	@$(LD) $(HD_FRONT_END_LDFLAGS) -o $@
	@$(call CHECK_SYMS,hd_front_end)

# IDO can't write dependency files, so list the headers with the host cpp and
# add the .s files each C file pulls in through GLOBAL_ASM.  Otherwise
# re-extracting (which rewrites the .s) leaves stale objects behind.
# Each prerequisite also gets an empty rule (as with -MP), so one that goes
# away, like a .s renamed by re-extracting, doesn't stop the build.
define write_deps
	@{ $(CPP) -MM -MP -MT $@ $(INCLUDE_CFLAGS) -D_LANGUAGE_C -D_FINALROM -D$(VERSION_DEFINE) $< 2>/dev/null || echo "$@: $<"; \
	   { grep -o 'GLOBAL_ASM("[^"]*")' $< || true; } | sed 's/GLOBAL_ASM("\(.*\)")/$(subst /,\/,$@): \1\n\1:/'; } > $(@:.o=.d)
endef

# A C file goes through tools/version_ifs.py first, which decides its
# VERSION_* conditionals (asm-processor would otherwise open the .s of every
# version's GLOBAL_ASM).  If a GLOBAL_ASM is left, asm-processor builds it;
# else IDO compiles the file as it is.  PERMUTER=1 (tools/permute.sh) shows
# the plain command.
ifndef PERMUTER
$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(@D)
	@$(PYTHON) $(TOOLS_DIR)/version_ifs.py $(VERSION_DEFINE) $< -o $(BUILD_DIR)/$*.ver.c
	@if grep -q GLOBAL_ASM $(BUILD_DIR)/$*.ver.c; then \
		echo "asm-processor $<"; \
		$(PYTHON) $(ASM_PROCESSOR_DIR)/asm_processor.py $(OPT_FLAGS) $(BUILD_DIR)/$*.ver.c > $(BUILD_DIR)/$< && \
		$(CC) -c -32 $(CFLAGS) $(OPT_FLAGS) $(MIPSISET) -o $@ $(BUILD_DIR)/$< && \
		$(PYTHON) $(ASM_PROCESSOR_DIR)/asm_processor.py $(OPT_FLAGS) $(BUILD_DIR)/$*.ver.c --post-process $@ \
			--assembler "$(AS) $(ASFLAGS)" --asm-prelude $(ASM_PROCESSOR_DIR)/prelude.s; \
	else \
		echo "$(CC) -c $(CFLAGS) $(OPT_FLAGS) $(MIPSISET) -o $@ $<"; \
		$(CC) -c $(CFLAGS) $(OPT_FLAGS) $(MIPSISET) -o $@ $<; \
	fi
	$(if $(POSTCC),$(POSTCC) $@)
	$(write_deps)
else
$(BUILD_DIR)/%.c.o: %.c
	$(CC) -c $(CFLAGS) $(OPT_FLAGS) $(MIPSISET) -o $@ $<
	$(if $(POSTCC),$(POSTCC) $@)
	$(write_deps)
endif

$(BUILD_DIR)/%.s.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/%.bin.o: %.bin
	$(LD) -r -b binary -o $@ $<

%.bin: %.elf
	$(OBJCOPY) $(OBJCOPYFLAGS) $< $@

# compression

$(BUILD_DIR)/hd_code_text.$(VERSION).gz: $(BUILD_DIR)/hd_code_text.$(VERSION).bin
	$(PYTHON) $(TOOLS_DIR)/rarezip.py $< $@ --name hd_code_text.raw --time $(HD_CODE_TEXT_TIMESTAMP) --level 6

$(BUILD_DIR)/hd_code_data.$(VERSION).gz: $(BUILD_DIR)/hd_code_data.$(VERSION).bin
	$(PYTHON) $(TOOLS_DIR)/rarezip.py $< $@ --name hd_code_data.raw --time $(HD_CODE_DATA_TIMESTAMP) --level 6

$(BUILD_DIR)/hd_front_end_text.$(VERSION).gz: $(BUILD_DIR)/hd_front_end_text.$(VERSION).bin
	$(PYTHON) $(TOOLS_DIR)/rarezip.py $< $@ --name hd_front_end_text.raw --time $(HD_FRONT_END_TEXT_TIMESTAMP) --level 6

$(BUILD_DIR)/hd_front_end_data.$(VERSION).gz: $(BUILD_DIR)/hd_front_end_data.$(VERSION).bin
	$(PYTHON) $(TOOLS_DIR)/rarezip.py $< $@ --name hd_front_end_data.raw --time $(HD_FRONT_END_DATA_TIMESTAMP) --level 6

# split out sections
$(BUILD_DIR)/hd_code_text.$(VERSION).bin: $(BUILD_DIR)/hd_code.$(VERSION).elf
	$(OBJCOPY) -O binary --only-section .hd_code $< $@

$(BUILD_DIR)/hd_code_data.$(VERSION).bin: $(BUILD_DIR)/hd_code.$(VERSION).elf
	$(OBJCOPY) -O binary --only-section .hd_code_data $< $@

$(BUILD_DIR)/hd_front_end_text.$(VERSION).bin: $(BUILD_DIR)/hd_front_end.$(VERSION).elf
	$(OBJCOPY) -O binary --only-section .hd_front_end $< $@

$(BUILD_DIR)/hd_front_end_data.$(VERSION).bin: $(BUILD_DIR)/hd_front_end.$(VERSION).elf
	$(OBJCOPY) -O binary --only-section .hd_front_end_data $< $@


-include $(foreach file,$(INIT_C_FILES) $(HD_CODE_C_FILES) $(HD_FRONT_END_C_FILES),$(BUILD_DIR)/$(file).d)

### Settings
.SECONDARY:
.PHONY: all clean default dirs extract compress stamp FORCE \
        init hd_code hd_front_end \
        verify_init verify_hd_code verify_hd_front_end
SHELL = /bin/bash -e -o pipefail


#
# import datetime
# d = datetime.datetime.strptime("1997-02-14 11:31:31", '%Y-%m-%d %H:%M:%S')
# d.timestamp()
