#!/usr/bin/env bash
# Regenerate the stage-2 configs for every version: hd_code's and
# hd_front_end's for us.v11 from the binaries, and every module's for the
# other versions from us.v11's (port() below, tools/vermap.py).
#
# Run from the repo root with the module binaries decompressed
# (`make VERSION=<v> decompress` for each version).  Splat is run in blastcorps/
# and leaves that version's asm/ behind, so `make -C blastcorps clean` afterwards.
# ONLY=<version> regenerates that version's alone.
#
# The bins are the data islands inside .text.  hd_code has six in every
# version, in the same order:
#   1. u16 lookup table (sine) used by the handwritten math routines
#   2. a second u16 table, between two blocks of those routines
#   3. data with code pointers, before a block of handwritten routines
#   4. more such data, after it
#   5. per-level tables {u32 n; n x {u32 key, u32 count, u8[16]}}, read by
#      the handwritten code (func_802A1EC8, func_802BD064 in us.v11)
#   6. RSP microcode, right after libultra's ldiv
# 1, 2, 5 and 6 are byte-identical across versions; 3 and 4 hold pointers, so
# their offsets come from where code stops reaching into them.
set -euo pipefail
cd "$(dirname "$0")/../blastcorps"

GEN="python3 ../tools/gen_code_yaml.py"
EXTRA=("$@")    # e.g. --c

T1="u16 lookup table (sine) for the handwritten math routines around it"
T2="second u16 lookup table of the handwritten math routines"
D1="data with code pointers, ahead of a block of handwritten routines"
D2="more data with code pointers, after that block"
D3="per-level tables the handwritten code reads (func_802A1EC8 in us.v11)"
UC="RSP microcode; its entry sets sp=0x110 and jumps into IMEM"
FE="RSP microcode (0xFB0 bytes, fits IMEM): sets sp=0x110 and loads from DMEM 0xFC4, so it is not r4300 code"
FE_NOTE="hd_front_end_text is inflated to 0x801E7000 by func_8028B3E0 in hd_code"

UD1="RSP microcode data (DMEM image), used by the graphics task setup in 405F0"
UD2="RSP microcode data (DMEM image), used by the audio task setup in 20460"
UD3="RSP microcode data (DMEM image), used by the handwritten code at 5FD50"

hd_code() {  # version data end t1 t2 d1 d2 d3 uc [rodata ucode-data] [--option ...]
    local v=$1
    [ -z "${ONLY:-}" ] || [ "$ONLY" = "$v" ] || return 0
    local split=("${@:10}")
    if [ -n "${10:-}" ] && [[ ${10} != --* ]]; then
        # .data/.rodata per object; the three microcode DMEM images follow .rodata
        local u=$((${11}))
        split=(--rodata "${10}"
               --tail "$(printf %X $u):800:$UD1"
               --tail "$(printf %X $((u + 0x800))):2D0:$UD2"
               --tail "$(printf %X $((u + 0xAD0))):800:$UD3" "${@:12}")
    fi
    $GEN hd_code "$v" --vram 0x802447C0 --data "$2" --end "$3" \
        --bin "$4:804:$T1" --bin "$5:884:$T2" --bin "$6:1EE0:$D1" \
        --bin "$7:1A4:$D2" --bin "$8:F50:$D3" --bin "$9:37E0:$UC" \
        ${SYMS:+--symbols symbol_addrs.hd_code.$v.txt} "${split[@]}" "${EXTRA[@]}"
}

FD="RSP microcode data (DMEM image) for the microcode at the end of .text"

hd_front_end() {  # version data end ucode [rodata ucode-data [--option ...]]
    local v=$1
    [ -z "${ONLY:-}" ] || [ "$ONLY" = "$v" ] || return 0
    local split=()
    if [ -n "${5:-}" ]; then
        # .data/.rodata per object; the microcode's DMEM image follows .rodata
        split=(--rodata "$5" --tail "$(printf %X $(($6))):800:$FD" "${@:7}")
    fi
    $GEN hd_front_end "$v" --vram 0x801E7000 --data "$2" --end "$3" \
        --bin "$4:FB0:$FE" --note "$FE_NOTE" \
        ${SYMS:+--symbols symbol_addrs.hd_front_end.$v.txt} "${split[@]}" "${EXTRA[@]}"
}

# us.v11 is the reference: its configs come from the binaries, as above.
# The other versions' are us.v11's, moved to where tools/vermap.py finds
# each object (gen_code_yaml.py --like): the same objects, names and kinds,
# so that one source tree builds all four.  port() writes the version's map
# (blastcorps/vermap.<v>.txt, and its undefined_syms from us.v11's) and the
# three configs; its arguments are the version's own layout, for the map.
port() {  # version hd_code-layout hd_front_end-layout [module:option ...] [vermap:option ...]
    local v=$1 hc=$2 fe=$3
    shift 3
    [ -z "${ONLY:-}" ] || [ "$ONLY" = "$v" ] || return 0
    local o vm=()
    for o in "$@"; do
        [[ $o == "vermap:"* ]] && vm+=(${o#"vermap:"})
    done
    python3 ../tools/vermap.py "$v" --layout "hd_code:$hc" --layout "hd_front_end:$fe" "${vm[@]}"
    local m data end opts
    for m in init hd_code hd_front_end; do
        case $m in
            init) data=3A40 end=3CE0 ;;
            hd_code) IFS=: read -r data end _ <<< "$hc" ;;
            hd_front_end) IFS=: read -r data end _ <<< "$fe" ;;
        esac
        opts=()
        for o in "$@"; do
            [[ $o == "$m:"* ]] && opts+=(${o#"$m:"})
        done
        $GEN "$m" "$v" --like us.v11 --data "$data" --end "$end" "${opts[@]}"
    done
}

# us.v11 also splits .data/.rodata: .rodata starts after reverb.c's L_INC
# (the last .data), and the microcode data after the last .rodata.  No
# reference shows where three blocks start: the data after gzip's tables
# (reached from 26570 and 39050, and from the dialogue table's pointers),
# thread.c's queues (only exceptasm and other objects use them), and reverb.c's L_INC,
# which nothing uses.  .bss runs from the end of .data to 0x803FF600, where
# the heap starts (00000 points D_8035806C there); the boot code clears on up
# to 0x80400000, whose last two words hold hd_front_end's ROM range.
# 0x2B3F0 and 0x4B450 start objects (after yoshi.c and 48D00): eu pads the
# .text before each to 16.  0x2B3F0's .bss starts at 0x8036BBB0: the 0x60
# bytes before it, which nothing names, are the rest of yoshi.c's last
# variable, D_8036BB48, the u16 text buffer func_8026F004 copies whole
# window texts into.
hd_code us.v11 0xA4410 0xCAEA0 0x68810 0x690C0 0x7D9D0 0x800DC 0x8E910 0xA0C30 0xC33D0 0xC9BD0 \
    --data-split BAF70 --data-split C2FA0:97BB0 --data-split C33C0:9FE20 \
    --split 2B3F0 --split 4B450 --bss-split 8036BBB0:2B3F0 \
    --bin "0x91D50:40:0x40 zero bytes after createmesgqueue.o; no C object ends like that" \
    --bss 8030F660:803FF600
# us.v11 also splits .data/.rodata: .rodata starts at "SELECT VEHICLE!",
# the microcode data ("RSP SW Version: 2.0D") after the last .rodata.
# 0x10850 is where bestTimes.c starts after pfsHandler.c: pfsHandler's
# .rodata ends in func_801F58E8's jump tables and the menu strings follow.
# 0xC450 (found by missed_boundaries) is another object start: 9570's .rodata
# ends in 12 bytes of padding before C450's.  .bss follows .data; its last
# variable is libultra's at 0x8021AC2C.  func_8028B3E0 clears everything up
# to 0x8021ED00 after inflating the module.  9570's .data starts at 0x214F0,
# not where its code's references put it (0x21FA0): its first texture runs
# from there to 0x80208FA8 (the display list after it loads 0x55C texels),
# so it can't be 7800's.  Its .bss starts at 0x802159F0, with 0x80 bytes
# nothing refers to: in us.v10, where 9570 comes after 17990 in the link,
# those bytes still come right before 9570's variables.  0x8380 starts an
# object after 7800 (jp pads 7800's .text to 16 before it).
hd_front_end us.v11 0x21040 0x29E90 0x20090 0x27440 0x29690 --split 10850 --split 8380 \
    --bss 80210E90:8021AC30 --data-split 214F0:9570 --bss-split 802159F0:9570

# layouts: <.data>:<end>:<island offset/length>,... (hex); the islands are
# the hd_code/hd_front_end --bin regions above, in the same order.
port us.v10 A4360:CADF0:68790/804,69040/884,7D920/1EE0,8002C/1A4,8E860/F50,A0B80/37E0 21010:29E60:20060/FB0
# jp's drawtext.c (14B30) prints "null jstring": its .rodata starts there.
# The text of D_802F5804[374] is the only one of 374-377 jp has.  pfsHandler's
# (E7B0) name buffers are twice the size in jp: it uses them the same way.
# Two of player.c's (1C40) .bss variables sit elsewhere in jp: the code that
# matches puts them there.
port jp A47E0:CAF60:68B80/804,69430/884,7DDA0/1EE0,804AC/1A4,8ECE0/F50,A1000/37E0 20F90:29B00:1FFE0/FB0 \
    "hd_code:--at 14B30:rodata:C47B0" "vermap:--pair 80303304:80303584:4" \
    "vermap:--pair 80218740:802183B0 --pair 80219F90:80219E80 --pair 80219FB0:80219EC0" \
    "vermap:--pair 80215458:802150C8:4 --pair 80215480:80215110:30"
# eu's stats.c (7800), 8380 and back_loop.c (17990) are rewritten: where
# their code and data start comes from their own references.  The --pairs
# are where code that matches puts a symbol (tools/vdiff.py lists them);
# most are hd.c's (00000) .bss, which eu defines in another order.  libm's
# NaN (C9650) follows 983F0's .rodata directly, and eu has no u16 text
# (BC8E0).  eu's text is in three languages: the objects whose .data and
# .rodata hold it are asm there (--asm-object) until their data is written
# for eu.
port eu A6210:CD2D0:6B180/804,6BA30/884,80340/1EE0,82A4C/1A4,91280/F50,A2A30/37E0 21990:2CAB0:209E0/FB0 \
    "hd_front_end:--at 17990:text:11D10 --at 7800:data:25930 --at 7800:rodata:29B60" \
    "hd_front_end:--at 8380:rodata:29DE0 --at 17990:rodata:2B810 --at 8380:bss:8021AEC0 --add 1A240:rodata:2B950" \
    "vermap:--pair 803109D0:80312DF8 --pair 80310820:80312C40 --pair 80310BD0:80313000" \
    "vermap:--pair 803153F0:803178B6 --pair 8020BD30:80208990 --pair 80217690:80213AB0" \
    "vermap:--pair 8020E3E0:8020E9F0 --pair 80210E90:80216380" \
    "vermap:--pair 8030F660:80317868 --pair 8030F664:803178B0:4 --pair 8030F668:803178B5" \
    "vermap:--pair 8030F669:803178B7 --pair 8030F66A:80317B50 --pair 80365078:80312DF0:4" \
    "vermap:--pair 8036507C:80312FF8:4 --pair 80365080:803131B0:4 --pair 80365084:803151B8:4" \
    "vermap:--pair 80365088:803171C0:4 --pair 8036508C:803171E0:4 --pair 803153F8:80317870:40" \
    "vermap:--pair 8030F670:80311A90:11B0 --pair 8030E390:803107C0" \
    "vermap:--pair 802E9F9C:802EBDA0:4 --pair 802E9FA0:802EBDA4:C" \
    "hd_code:--at 1D990:rodata:C4510" \
    "hd_code:--asm-object 26570" \
    "hd_code:--asm-object 45BB0" \
    "hd_code:--asm-object 53220" \
    "hd_front_end:--asm-object 00000" \
    "hd_front_end:--asm-object 1C40" \
    "hd_front_end:--asm-object 6790" \
    "hd_front_end:--asm-object 7800" \
    "hd_front_end:--asm-object E7B0" \
    "hd_front_end:--asm-object 1A240" \
    "hd_front_end:--asm-object 11530" \
    "hd_code:--at C9650:rodata:CBCA0 --at BC8E0:data:C0B80 --at 00000:bss:80311A90"
