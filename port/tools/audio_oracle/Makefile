# The libaudio oracle (docs/PORT.md, "The libaudio oracle"): the replay and
# the two libraries it replays into, for i386 (the N64's layouts).
#
#   make -C port/tools/audio_oracle VERSION=us.v10 [OUT=build/audio_oracle]
#
# orig.so is the decompiled original (blastcorps/src/libultra/audio, through
# the hd_code objects that include it), port.so the port's own
# (port/libaudio); both compiled as the port compiles the game's C, less
# BEPass.

ROOT := $(abspath ../../..)
BLAST := $(ROOT)/blastcorps
PORT := $(ROOT)/port
VERSION ?= us.v10
OUT ?= $(ROOT)/build/audio_oracle/$(VERSION)
CC := clang
VDEF := VERSION_$(shell echo $(VERSION) | tr a-z. A-Z_)
RESOURCE := $(shell $(CC) -print-resource-dir)/include

ARCH := -m32 -msse2 -mfpmath=sse -malign-double
N64_CFLAGS := $(ARCH) -O2 -g -fPIC -std=gnu89 -fsigned-char -fno-strict-aliasing -fwrapv \
    -fno-delete-null-pointer-checks -ftrivial-auto-var-init=zero -ffp-contract=off -fno-common \
    -fno-builtin -nostdinc -isystem $(RESOURCE) -include $(PORT)/include/port_game.h \
    -D_LANGUAGE_C -D_FINALROM -DTARGET_PC -D$(VDEF) -D_MIPS_SZLONG=32 -D_MIPS_SZINT=32 \
    -DPORT_AUDIO_ORACLE -w -MMD -MP
ORIG_INC := -I$(PORT)/include -I$(BLAST) -I$(BLAST)/include -I$(BLAST)/include/2.0I \
    -I$(BLAST)/include/2.0I/PR
PORT_INC := -I$(PORT)/libaudio -I$(PORT)/include -I$(PORT)/include/sdk -I$(PORT)/include/sdk/PR \
    -I$(BLAST) -I$(BLAST)/include -I$(BLAST)/include/2.0I -I$(BLAST)/include/2.0I/PR

# the objects that hold libaudio (port/CMakeLists.txt's LIBAUDIO_ORIG)
ORIG_OBJS := 90390 92350 923B0 926F0 927B0 92800 92A30 92AD0 92B60 92BF0 92C80 92D20 92DA0 \
    92F00 92F30 939F0 93A30 93AA0 93B50 93B60 93BA0 93DF0 95020 95460 99620 996A0 996B0 9AFF0 \
    9B470 9BBF0 9BC90 9BDE0 9C930 9CC30 9D810 9D920 9F7D0 9FDF0 9FE00 9FE20
ORIG_O := $(ORIG_OBJS:%=$(OUT)/orig/%.o)
PORT_SRC := $(wildcard $(PORT)/libaudio/*.c)
PORT_O := $(PORT_SRC:$(PORT)/libaudio/%.c=$(OUT)/port/%.o)

all: $(OUT)/replay $(OUT)/orig.so $(if $(PORT_SRC),$(OUT)/port.so $(OUT)/banktest.so)

$(OUT)/replay: replay.c $(PORT)/include/audio_log.h
	@mkdir -p $(dir $@)
	$(CC) $(ARCH) -O2 -g -Wall -rdynamic -o $@ $< -ldl

$(OUT)/orig/%.o: $(BLAST)/src/hd_code/%.c
	@mkdir -p $(dir $@)
	$(CC) $(N64_CFLAGS) $(ORIG_INC) -c -o $@ $<

$(OUT)/orig.so: $(ORIG_O)
	$(CC) $(ARCH) -shared -o $@ $^

$(OUT)/port/%.o: $(PORT)/libaudio/%.c $(wildcard $(PORT)/libaudio/*.h) $(wildcard $(PORT)/include/sdk/PR/libaudio.h)
	@mkdir -p $(dir $@)
	$(CC) $(N64_CFLAGS) $(PORT_INC) -c -o $@ $<

$(OUT)/port.so: $(PORT_O)
	$(CC) $(ARCH) -shared -o $@ $^

# port.so with its banks' objects copied out of the file (banktest.c): the
# replay's bank-object translation at work, while port/libaudio relocates
# in place
$(OUT)/banktest/core.o: $(OUT)/port/core.o
	@mkdir -p $(dir $@)
	objcopy --redefine-sym alBnkfNew=port_alBnkfNew $< $@

$(OUT)/banktest/banktest.o: banktest.c $(PORT)/include/audio_log.h
	@mkdir -p $(dir $@)
	$(CC) $(N64_CFLAGS) $(PORT_INC) -c -o $@ $<

$(OUT)/banktest.so: $(filter-out $(OUT)/port/core.o,$(PORT_O)) $(OUT)/banktest/core.o $(OUT)/banktest/banktest.o
	$(CC) $(ARCH) -shared -o $@ $^

clean:
	rm -rf $(OUT)

# the headers each object read
-include $(ORIG_O:.o=.d) $(PORT_O:.o=.d) $(OUT)/banktest/banktest.d

.PHONY: all clean
