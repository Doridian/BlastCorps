# Mechanical translation of the handwritten asm to C, and its differential
# test.  Needs a built stage 2 of VERSION (`make VERSION=... -C blastcorps`,
# default us.v11): the translator reads blastcorps/asm/ and the linked ELFs,
# the test the linked images.
#
#   make -C tools/recomp               translate into blastcorps/build/recomp/src
#   make -C tools/recomp port          ... and compile it as the port would
#   make -C tools/recomp test          build the test library and run difftest
#   make -C tools/recomp snapshots     RDRAM snapshots from mupen64plus (optional)
#
# PYTHON needs numpy and unicorn for the test (the translator itself only
# needs the standard library).

ROOT    := $(abspath ../..)
PYTHON  ?= python3
CC      ?= cc
JOBS    ?= $(shell nproc)
TRIALS  ?= 48
VERSION ?= us.v11

OUT     := $(ROOT)/blastcorps/build/recomp
GEN     := $(OUT)/src
HERE    := $(ROOT)/tools/recomp

# -ffp-contract=off: no fused multiply-add, the VR4300 rounds every step
CFLAGS  := -O1 -fPIC -ffp-contract=off -fno-strict-aliasing -Wall -Wno-unused-label \
           -I$(HERE)/runtime -I$(GEN)
TESTFLAGS := -DRECOMP_TEST -DRECOMP_QEMU_COMPAT

ASM     := $(wildcard $(ROOT)/blastcorps/asm/hd_code/*.s $(ROOT)/blastcorps/asm/hd_front_end/*.s)
ELFS    := $(ROOT)/blastcorps/build/hd_code.$(VERSION).elf $(ROOT)/blastcorps/build/hd_front_end.$(VERSION).elf
TOOLS   := $(wildcard $(HERE)/*.py)

all: $(GEN)/.stamp

$(GEN)/.stamp: $(ASM) $(ELFS) $(TOOLS) $(HERE)/native_sites.txt
	rm -rf $(GEN)
	$(PYTHON) $(HERE)/translate.py --version $(VERSION) $(GEN)
	touch $@

# compile every generated file plus `extra` with `flags` into `lib`
define compile
	mkdir -p $(1)
	ls $(GEN)/*.c $(3) | xargs -P$(JOBS) -I{} sh -c \
	    '$(CC) -c $(CFLAGS) $(2) {} -o $(1)/$$(basename {} .c).o'
endef

# the port build: no instrumentation; checks that it compiles cleanly
$(OUT)/librecomp.a: $(GEN)/.stamp $(HERE)/runtime/recomp.h
	rm -rf $(OUT)/obj-port
	$(call compile,$(OUT)/obj-port,-Werror,)
	ar rcs $@ $(OUT)/obj-port/*.o

$(OUT)/librecomp_test.so: $(GEN)/.stamp $(HERE)/runtime/recomp.h $(HERE)/test/harness.c
	rm -rf $(OUT)/obj-test
	$(call compile,$(OUT)/obj-test,$(TESTFLAGS),$(HERE)/test/harness.c)
	$(CC) -shared -o $@ $(OUT)/obj-test/*.o -lm

# with a call before every instruction, for difftest.py --trace
$(OUT)/librecomp_trace.so: $(GEN)/.stamp $(HERE)/runtime/recomp.h $(HERE)/test/harness.c
	rm -rf $(OUT)/obj-trace
	$(call compile,$(OUT)/obj-trace,$(TESTFLAGS) -DRECOMP_TRACE,$(HERE)/test/harness.c)
	$(CC) -shared -o $@ $(OUT)/obj-trace/*.o -lm

port: $(OUT)/librecomp.a
lib: $(OUT)/librecomp_test.so
trace-lib: $(OUT)/librecomp_trace.so

# RDRAM from real play (title, name entry, Simian Acres, the pause menu, the
# world map).  Optional: the test uses them when they are there.
SNAPSHOT_FRAMES ?= $(shell seq 1500 500 9000)
$(OUT)/snapshot: $(HERE)/test/snapshot.c
	mkdir -p $(OUT)
	$(CC) -O1 -Wall -rdynamic -o $@ $< -ldl

snapshots: $(OUT)/snapshot
	rm -rf $(OUT)/snapshots
	SDL_VIDEODRIVER=offscreen $(OUT)/snapshot $(ROOT)/baserom.us.v11.z64 $(OUT)/snapshots $(SNAPSHOT_FRAMES)

# (snapshot.c plays us.v11: its RDRAM is no use to another version's code)
SNAPS := $(if $(filter us.v11,$(VERSION)),$(if $(wildcard $(OUT)/snapshots/rdram_*.bin),--snapshots $(OUT)/snapshots))

# Two passes: calls out of the translated code run the real callee (in
# unicorn), then return a hash of their arguments (gets further in code
# whose callees fault on random state).  The second reports the combined
# block coverage.
test: $(OUT)/librecomp_test.so
	RECOMP_VERSION=$(VERSION) $(PYTHON) $(HERE)/test/difftest.py $< --trials $(TRIALS) $(SNAPS) \
	    --save-coverage $(OUT)/coverage-follow.npy
	RECOMP_VERSION=$(VERSION) $(PYTHON) $(HERE)/test/difftest.py $< --trials $(TRIALS) $(SNAPS) --stub-externs \
	    --blocks $(GEN)/blocks.tsv --merge-coverage $(OUT)/coverage-follow.npy

clean:
	rm -rf $(OUT)

.PHONY: all port lib trace-lib snapshots test clean
