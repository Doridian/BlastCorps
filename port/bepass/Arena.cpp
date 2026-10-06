/*
 * port-arena: the movable build's arena link (PORT_MOVABLE, docs/PORT.md
 * "Movable memory").  `opt -passes=port-arena` runs it over the whole N64
 * side (the game's C, port/src and the asm data, asm2c.py) as one module.
 *
 * The N64 side's pointer values are N64 addresses; memory is the arena, a
 * block the host allocates anywhere (port_arena, port_arena.h), holding
 * N64 physical memory from 0: RDRAM, then the N64 side's data that has no
 * N64 address, then the fibers' host stacks.  The pass
 *
 *   - resolves every global: a definition with an N64 name
 *     (-port-arena-syms, gen_syms.py table) in RDRAM is at its N64 address,
 *     every other definition is laid out after RDRAM, and a declaration
 *     with an N64 name (another module's variable, a ROM offset, a fixed
 *     address) is that name's value.  Every reference becomes the constant.
 *     What is left is the host's (-port-arena-host names more);
 *   - writes the arena's initial contents (__port_arena_runs), from the
 *     initializers, in the build's byte order: big-endian (what BEPass's
 *     __bepass_fixup did at run time), or native with 64-bit scalars' words
 *     exchanged (-port-arena-native); a pointer to a function or to the
 *     host's data is left to startup (__port_arena_relocs);
 *     -port-arena-image writes the contents to a file too, and
 *     -port-arena-no-runs leaves them out of the module: PORT_ROM_DATA's
 *     host makes them from the ROM (tools/rom_data.py, host/romdata.c);
 *   - maps every access (loads, stores, atomics, memory intrinsics, and the
 *     pointer arguments of calls into the host, which dereferences them) to
 *     port_arena + (p & 0x1FFFFFFF);
 *   - gives a local whose address escapes (to a call, into memory, into an
 *     integer) its N64 address, which it has on a fiber's stack; one that
 *     doesn't escape is accessed where it is;
 *   - makes a function used as a value its N64 address (the game's by its
 *     name, the port's own from PORT_FN_BASE), and a call through a value
 *     a call through port_fn(address) (__port_fns, port/host/runtime.c), so
 *     that no host address is left in game memory;
 *   - names the caller of the replay's hooks (port_replay_set_caller), for
 *     the replay (port/host/replay.c), which on the other builds finds it
 *     by its return address;
 *   - makes every direct call with its callee's type (callFix, first): the
 *     decompiled C's K&R declarations disagree between files, which
 *     WebAssembly can't call at all and i386 gets wrong for narrow
 *     results.
 *
 * Also here: port-wrap (below), per file.
 */
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <optional>
#include <set>
#include <vector>

using namespace llvm;

static cl::opt<std::string> SymsFile("port-arena-syms", cl::desc("gen_syms.py table: name address kind"));
static cl::opt<std::string> HostNames("port-arena-host", cl::desc("N64-side variables the host names (comma list)"));
static cl::opt<bool> Native("port-arena-native", cl::desc("the native-endian build's byte order"));
static cl::opt<bool> TypedCalls("port-arena-typed-calls",
                                cl::desc("a call through a value goes to a thunk of the call's type (WebAssembly)"));
static cl::opt<bool> X86FpToInt("port-arena-x86-fptoint",
                                cl::desc("float to integer conversions out of range as x86 gives them (WebAssembly)"));
static cl::opt<std::string> Retarget("port-arena-triple",
                                     cl::desc("the module's target from here on (WebAssembly: the N64 side is i386's)"));
static cl::opt<std::string> RetargetLayout("port-arena-datalayout", cl::desc("... and its data layout"));
static cl::opt<std::string> FnValues("port-arena-fn-values",
                                     cl::desc("functions the translated code takes as values (gen_glue.py's "
                                              "fn_values.txt: one name a line)"));
static cl::opt<std::string> ImageFile("port-arena-image",
                                      cl::desc("also write the arena's initial contents to FILE, as bytes from 0"));
static cl::opt<bool> NoRuns("port-arena-no-runs",
                            cl::desc("leave the initial contents out of the module (__port_arena_runs empty): "
                                     "the host makes them from the ROM (PORT_ROM_DATA, tools/rom_data.py)"));
static cl::opt<std::string> SymsHeader("port-arena-header",
                                       cl::desc("where the moved variables went, as a header (SYM_, PORT_N64_)"));
/* the scattered layout (PORT_SCATTER, docs/PORT.md "The scattered layout") */
static cl::opt<std::string> Scatter("port-arena-scatter",
                                    cl::desc("SEED: every N64-named variable at a shuffled address after RDRAM, "
                                             "with padding between, instead of its N64 address"));
static cl::opt<bool> ScatterFE("port-arena-scatter-fe", cl::desc("... the front end's variables too"));
static cl::opt<bool> ScatterCheck("port-arena-scatter-check",
                                  cl::desc("every mapped access calls port_scatter_check(address, size, site)"));
static cl::opt<std::string> ScatterReport("port-arena-scatter-report",
                                          cl::desc("where the names inside other variables are, and who uses them"));

namespace {

/* the arena, by N64 physical address (port_arena.h) */
static const uint64_t K0 = 0x80000000u, MASK = 0x1FFFFFFFu;
static const uint64_t RDRAM = 0x00400000u;
static const uint64_t STACKS = 0x00C00000u, STACKS_SPAN = 0x01000000u;
/* a thread's slot of the escaping locals' stacks (locals(); port_arena.h's
   PORT_LOCALS_SIZE) */
static const uint64_t LOCALS_SLOT = 0x00010000u;
/* the port's own functions' "N64 addresses": nothing is there */
static const uint64_t FN_BASE = 0x7F000000u;

struct Arena : PassInfoMixin<Arena> {
    static bool isRequired() { return true; }

    Module *M = nullptr;
    LLVMContext *C = nullptr;
    const DataLayout *DL = nullptr;
    IntegerType *IP = nullptr;          /* the pointer-sized integer */
    GlobalVariable *base = nullptr;     /* port_arena */
    unsigned rewritten = 0, hostArgs = 0, escaped = 0, kept = 0, frames = 0, undefs = 0;
    uint64_t frameMax = 0;
    GlobalVariable *lsp = nullptr, *lsplim = nullptr;  /* port_locals_sp, port_locals_end */
    std::optional<DataLayout> frameDL;  /* the module's own, before -port-arena-triple */

    [[noreturn]] void fail(const Twine &what) { report_fatal_error("port-arena: " + what); }

    /* ---- names ------------------------------------------------------- */

    std::map<std::string, std::pair<uint64_t, char>> syms;
    std::map<std::string, uint64_t> symSize;

    void readSyms() {
        if (SymsFile.empty())
            fail("no -port-arena-syms");
        auto buf = MemoryBuffer::getFile(SymsFile);
        if (!buf)
            fail("can't read " + SymsFile);
        SmallVector<StringRef, 0> lines;
        (*buf)->getBuffer().split(lines, '\n', -1, false);
        for (StringRef l : lines) {
            SmallVector<StringRef, 4> p;
            l.split(p, ' ', -1, false);
            if (p.size() < 3)
                continue;
            uint64_t a, n = 0;
            if (p[1].getAsInteger(16, a))
                continue;
            syms[p[0].str()] = {a, p[2][0]};
            if (p.size() > 3 && !p[3].getAsInteger(16, n))
                symSize[p[0].str()] = n;
        }
    }

    /* functions of the host (port/host, libc) the game's C calls with
       pointers they dereference (host_thread_create's argument is only
       handed back to the thread's entry, in the game's C) */
    static bool hostCallee(StringRef n) {
        if (n == "host_thread_create")
            return false;
        return n.starts_with("host_") || n == "memset" || n == "memmove" || n == "memcpy" ||
               n.starts_with("n64_") || n == "port_counter" || n == "vsprintf" || n == "sqrtf";
    }

    /* ---- the layout -------------------------------------------------- */

    struct Placed {
        GlobalVariable *g;
        uint64_t addr;
    };
    std::vector<Placed> placed;
    std::vector<GlobalVariable *> moved;
    uint64_t extraEnd = RDRAM;

    Constant *addrConst(uint64_t a, Type *t) {
        return ConstantExpr::getIntToPtr(ConstantInt::get(IP, a), t);
    }

    void layout() {
        std::set<std::string> host;
        SmallVector<StringRef, 8> hn;
        StringRef(HostNames).split(hn, ',', -1, false);
        for (StringRef n : hn)
            host.insert(n.str());
        std::vector<GlobalVariable *> gs;
        for (GlobalVariable &g : M->globals())
            if (!g.getName().starts_with("llvm.") && !host.count(g.getName().str()))
                gs.push_back(&g);
        std::vector<GlobalVariable *> extra;
        std::vector<GlobalVariable *> decls;
        /* an N64 variable's room: its size or up to the next sized one, as
           gen_ld.py has it; one of the C's the host lays out bigger (LP64:
           its native pointers), or aligns where the N64 address isn't, is
           laid out after RDRAM instead ("moved") */
        std::vector<uint64_t> sized;
        for (auto &s : symSize)
            if (s.second) {
                auto it = syms.find(s.first);
                if (it != syms.end() && it->second.first - K0 < RDRAM)
                    sized.push_back(it->second.first & MASK);
            }
        sized.push_back(RDRAM);
        std::sort(sized.begin(), sized.end());
        auto room = [&](const std::string &name, uint64_t addr) {
            uint64_t next = *std::upper_bound(sized.begin(), sized.end(), addr);
            uint64_t n = symSize.count(name) ? symSize[name] : 0;
            return std::max(n, next - addr);
        };
        for (GlobalVariable *g : gs) {
            auto it = syms.find(g->getName().str());
            /* (a variable is data, whatever nm says: the islands in .text) */
            if (g->isDeclaration()) {
                if (it != syms.end())
                    decls.push_back(g);
                continue;
            }
            bool portSrc = g->hasSection() && (g->getSection().starts_with(".data.port.") ||
                                               g->getSection().starts_with(".bss.port."));
            if (!portSrc && it != syms.end() && it->second.first - K0 < RDRAM) {
                uint64_t a = it->second.first & MASK;
                uint64_t align = std::max<uint64_t>(g->getAlign() ? g->getAlign()->value() : 1, 1);
                if (DL->getTypeAllocSize(g->getValueType()) > room(g->getName().str(), a) || a % align) {
                    moved.push_back(g);
                    extra.push_back(g);
                } else {
                    placed.push_back({g, a});
                }
            } else {
                extra.push_back(g);
            }
        }
        if (!Scatter.empty())
            scatterPick(extra, room);
        for (GlobalVariable *g : extra) {
            uint64_t align = std::max<uint64_t>(g->getAlign() ? g->getAlign()->value() : 1,
                                                DL->getABITypeAlign(g->getValueType()).value());
            extraEnd = (extraEnd + align - 1) / align * align;
            placed.push_back({g, extraEnd});
            extraEnd += DL->getTypeAllocSize(g->getValueType());
        }
        extraEnd = (extraEnd + 15) & ~15ull;
        if (!Scatter.empty())
            scatterPlace(decls);
        /* the moved ones, for the translated code (SYM_) and the host
           (PORT_N64_), whose own tables have the N64's addresses */
        if (!SymsHeader.empty()) {
            std::error_code ec;
            raw_fd_ostream os(SymsHeader, ec);
            if (ec)
                fail("can't write " + SymsHeader);
            os << "/* Generated by port-arena (bepass/Arena.cpp): where the variables that\n"
                  "   outgrew their N64 room are in the arena. */\n";
            for (auto &p : placed)
                if (std::find(moved.begin(), moved.end(), p.g) != moved.end())
                    os << "#define SYM_" << p.g->getName() << " 0x" << format_hex_no_prefix(K0 | p.addr, 8)
                       << "u\n#define PORT_N64_" << p.g->getName() << " 0x"
                       << format_hex_no_prefix(K0 | p.addr, 8) << "u\n";
            for (auto &h : scatterHeader)
                os << "#define SYM_" << h.first << " 0x" << format_hex_no_prefix(K0 | h.second, 8)
                   << "u\n#define PORT_N64_" << h.first << " 0x" << format_hex_no_prefix(K0 | h.second, 8) << "u\n";
        }
        /* PORT_ARENA_MAP=FILE: where everything went, "address size name" */
        if (const char *mp = getenv("PORT_ARENA_MAP")) {
            std::error_code ec;
            raw_fd_ostream os(mp, ec);
            for (auto &p : placed)
                os << format_hex_no_prefix(K0 | p.addr, 8) << " " << DL->getTypeAllocSize(p.g->getValueType()) << " "
                   << p.g->getName() << "\n";
        }
        if (extraEnd > STACKS)
            fail("the N64 side's data without N64 addresses doesn't fit below the stacks");
        /* no two in one place */
        std::vector<Placed> s = placed;
        std::sort(s.begin(), s.end(), [](const Placed &a, const Placed &b) { return a.addr < b.addr; });
        for (size_t k = 1; k < s.size(); k++) {
            uint64_t end = s[k - 1].addr + DL->getTypeAllocSize(s[k - 1].g->getValueType());
            if (end > s[k].addr)
                fail(s[k - 1].g->getName() + " overlaps " + s[k].g->getName());
        }
        /* the references, all but the initializers' own writing below */
        removeFromUsed("llvm.used");
        removeFromUsed("llvm.compiler.used");
        for (auto &p : placed)
            p.g->replaceAllUsesWith(addrConst(K0 | p.addr, p.g->getType()));
        for (GlobalVariable *g : decls) {
            auto sd = scatterDecl.find(g);
            g->replaceAllUsesWith(addrConst(sd != scatterDecl.end() ? K0 | sd->second : syms[g->getName().str()].first,
                                            g->getType()));
            g->eraseFromParent();
        }
    }

    /* ---- the scattered layout (-port-arena-scatter) -------------------
       The oracle for the code that depends on where the N64 put things:
       every N64-named variable of RDRAM (the C's, and the asm data's: a
       label each, asm2c.py) is laid out after the rest of the data instead,
       in an order the seed shuffles, at an address with the N64's one's
       offset in 16 bytes, after padding of 16 to 2,048 bytes and as many
       as the one before has (up to 16 KB: so that an access past the end
       of one, by an index the N64 had room for, lands in padding).  A
       name the N64 link has inside one of them (a declaration: a field
       read through a name of its own, a table's second half) gets room of
       its own there ("alias"); a struct's members but the first in the
       asm data (asm2c.py's FILE_STRUCTS) are such names too.  The front
       end's variables stay (port/src/overlay.c restores them by address)
       unless -port-arena-scatter-fe.  What
       nothing should touch any more (the N64 places left behind, the
       padding, the aliases' own room) is listed in __port_scatter_bad:
       the host fills it with a pattern (PORT_SCATTER_POISON) and, with
       -port-arena-scatter-check, reports the accesses that reach it and
       takes them where the N64's layout has what they meant
       (port_scatter_check, port_scatter_mem: host/runtime.c). */
    struct ScItem {
        GlobalVariable *g;
        uint64_t n64, n64size, size;
        bool alias;
        std::string what;
        uint64_t addr = 0;
        size_t cont = 0;    /* an alias's: what it is inside of, and where */
        uint64_t off = 0;
    };
    std::vector<ScItem> scItems;
    std::map<GlobalVariable *, uint64_t> scatterDecl;
    std::vector<std::pair<std::string, uint64_t>> scatterHeader;
    struct Bad {
        uint64_t start, len;
        unsigned kind;      /* 1 an N64 place left, 2 padding ("before|after"), 3 an alias's room */
        std::string desc;
        /* what its first byte stands for in the N64's layout (padding: the
           N64 end of the variable before it, n64b the start of the one
           after), and an N64 place's new address */
        uint64_t n64a = 0, n64b = 0, now = 0;
    };
    std::vector<Bad> bad;
    uint64_t rng = 0;

    uint64_t next() {       /* splitmix64 */
        uint64_t z = (rng += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }

    static bool frontEnd(uint64_t a) { return a >= 0x1E7000 && a < 0x21ED00; }

    template <typename Room> void scatterPick(std::vector<GlobalVariable *> &extra, Room &room) {
        std::vector<Placed> keep;
        for (auto &p : placed) {
            if (!ScatterFE && frontEnd(p.addr)) {
                keep.push_back(p);
                continue;
            }
            uint64_t n = DL->getTypeAllocSize(p.g->getValueType());
            scItems.push_back({p.g, p.addr, n, n, false, ""});
        }
        placed = keep;
        std::vector<GlobalVariable *> stay;
        for (GlobalVariable *g : moved) {
            uint64_t a = syms[g->getName().str()].first & MASK;
            if (!ScatterFE && frontEnd(a)) {
                stay.push_back(g);
                continue;
            }
            uint64_t n = DL->getTypeAllocSize(g->getValueType());
            scItems.push_back({g, a, std::min(n, room(g->getName().str(), a)), n, false, ""});
            extra.erase(std::find(extra.begin(), extra.end(), g));
        }
        moved = stay;
    }

    /* the asm data (asm2c.py's gen/data_c), by its debug info's file */
    static bool asmData(GlobalVariable *g) {
        SmallVector<DIGlobalVariableExpression *, 1> gves;
        g->getDebugInfo(gves);
        for (auto *gve : gves)
            if (gve->getVariable()->getFilename().contains("data_c"))
                return true;
        return false;
    }

    /* where a value is used: "function (file:line)", or a variable's initializer */
    void users(Value *v, std::set<std::string> &out, int depth = 0) {
        for (User *u : v->users()) {
            if (auto *i = dyn_cast<Instruction>(u)) {
                out.insert(where(i));
            } else if (auto *g = dyn_cast<GlobalVariable>(u)) {
                out.insert("initializer of " + g->getName().str());
            } else if (depth < 8) {
                users(u, out, depth + 1);
            }
        }
    }

    static std::string where(Instruction *i) {
        std::string s = i->getFunction()->getName().str();
        if (const DebugLoc &dl = i->getDebugLoc()) {
            DILocation *l = dl.get();
            while (l->getInlinedAt())   /* the outermost: the function's own line */
                l = l->getInlinedAt();
            StringRef f = l->getFilename();
            for (const char *root : {"/blastcorps/src/", "/port/"}) {
                size_t k = f.find(root);
                if (k != StringRef::npos) {
                    f = f.substr(k + 1);
                    break;
                }
            }
            s += " (" + f.str() + ":" + std::to_string(l->getLine()) + ")";
        }
        return s;
    }

    void scatterPlace(std::vector<GlobalVariable *> &decls) {
        for (char c : Scatter.getValue())
            rng = rng * 131 + (uint8_t)c;
        /* the containers, by N64 address */
        std::vector<size_t> byAddr(scItems.size());
        for (size_t k = 0; k < byAddr.size(); k++)
            byAddr[k] = k;
        std::sort(byAddr.begin(), byAddr.end(), [&](size_t a, size_t b) { return scItems[a].n64 < scItems[b].n64; });
        auto container = [&](uint64_t a) -> long {
            long best = -1;
            for (size_t k : byAddr) {
                const ScItem &it = scItems[k];
                if (it.n64 > a)
                    break;
                if (a < it.n64 + std::max<uint64_t>(it.n64size, 1))
                    best = (long)k;
            }
            return best;
        };
        std::string report;
        raw_string_ostream rs(report);
        std::vector<std::pair<GlobalVariable *, std::pair<size_t, uint64_t>>> follows;
        size_t nItems = scItems.size();
        for (GlobalVariable *g : decls) {
            uint64_t a = syms[g->getName().str()].first;
            if (a - K0 >= RDRAM)
                continue;
            a &= MASK;
            long c = container(a);
            if (c < 0)
                continue;
            ScItem &ct = scItems[c];
            std::set<std::string> us;
            users(g, us);
            rs << "alias " << g->getName() << " " << format_hex_no_prefix(K0 | a, 8) << " "
               << ct.g->getName() << "+0x" << format_hex_no_prefix(a - ct.n64, 1) << " "
               << (asmData(ct.g) ? "asm" : "c") << " " << DL->getTypeAllocSize(g->getValueType());
            for (auto &u : us)
                rs << " | " << u;
            rs << "\n";
            /* (with the check, a name inside the asm data goes with its
               variable: the generated data points into the variable, not
               at the name, and a pointer compared, as func_802A06B4 does
               its keys, can't be taken where it meant; the report lists
               them all the same) */
            if (ScatterCheck && asmData(ct.g)) {
                follows.push_back({g, {(size_t)c, a - ct.n64}});
                continue;
            }
            uint64_t n = DL->getTypeAllocSize(g->getValueType());
            auto ss = symSize.find(g->getName().str());
            if (ss != symSize.end())
                n = std::max(n, ss->second);
            /* (room to the container's end: what is reached from it as the
               N64 has it, an array's later elements, stays in the room,
               where the check knows what it meant) */
            n = std::max(n, ct.n64size - (a - ct.n64));
            n = alignTo(std::max<uint64_t>(n, 4), 4);
            std::string what = g->getName().str() + " (" + ct.g->getName().str() + "+0x" +
                               utohexstr(a - ct.n64) + ")";
            scItems.push_back({g, a, 0, n, true, what, 0, (size_t)c, a - ct.n64});
        }
        /* the shuffle, and the layout from after the data without N64 addresses */
        std::vector<size_t> order(scItems.size());
        for (size_t k = 0; k < order.size(); k++)
            order[k] = k;
        for (size_t k = order.size(); k > 1; k--)
            std::swap(order[k - 1], order[next() % k]);
        uint64_t cur = alignTo(extraEnd, 16);
        std::string prev = "(the data without N64 addresses)";
        uint64_t prevEnd = 0, prevSize = 0;
        for (size_t k : order) {
            ScItem &it = scItems[k];
            uint64_t align = it.alias ? 4 : std::max<uint64_t>(it.g->getAlign() ? it.g->getAlign()->value() : 1, 1);
            uint64_t at = alignTo(cur + 16 * (1 + next() % 128) + std::min<uint64_t>(prevSize, 0x4000), 16) + it.n64 % 16;
            at = alignTo(at, align);
            bad.push_back({cur, at - cur, 2, prev + "|" + it.g->getName().str(), prevEnd, it.n64});
            it.addr = at;
            cur = at + it.size;
            prev = it.g->getName().str();
            prevEnd = it.n64 + (it.alias ? it.size : it.n64size);
            prevSize = it.size;
            if (it.alias) {
                bad.push_back({at, it.size, 3, it.what, it.n64});
                scatterDecl[it.g] = at;
            } else {
                placed.push_back({it.g, at});
                if (it.n64size)
                    bad.push_back({it.n64, it.n64size, 1, it.g->getName().str() + "'s N64 place", it.n64, 0, at});
            }
            if (!it.alias)
                scatterHeader.push_back({it.g->getName().str(), at});
        }
        /* (the host's PORT_N64_ of an alias is its room, as the C has it;
           with the check, which takes the C's accesses where the N64 has
           them, where the N64 has it) */
        for (ScItem &it : scItems)
            if (it.alias)
                scatterHeader.push_back({it.g->getName().str(), ScatterCheck ? scItems[it.cont].addr + it.off : it.addr});
        uint64_t end = alignTo(cur + 256 + std::min<uint64_t>(prevSize, 0x4000), 16);
        bad.push_back({cur, end - cur, 2, prev + "|(the end)", prevEnd});
        extraEnd = end;
        for (auto &f : follows) {
            uint64_t at = scItems[f.second.first].addr + f.second.second;
            scatterDecl[f.first] = at;
            scatterHeader.push_back({f.first->getName().str(), at});
        }
        /* the names the module doesn't use, for the host (PORT_N64_):
           where they are inside what moved */
        std::set<std::string> seen;
        for (auto &h : scatterHeader)
            seen.insert(h.first);
        for (auto &s : syms) {
            if (seen.count(s.first) || s.second.second != 'D' || s.second.first - K0 >= RDRAM ||
                s.first.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_") !=
                    std::string::npos)
                continue;
            uint64_t a = s.second.first & MASK;
            long c = container(a);
            if (c < 0 || (size_t)c >= nItems)
                continue;
            scatterHeader.push_back({s.first, scItems[c].addr + (a - scItems[c].n64)});
            rs << "host " << s.first << " " << format_hex_no_prefix(K0 | a, 8) << " " << scItems[c].g->getName()
               << "+0x" << format_hex_no_prefix(a - scItems[c].n64, 1) << "\n";
        }
        for (ScItem &it : scItems)
            if (!it.alias)
                rs << "item " << it.g->getName() << " " << format_hex_no_prefix(K0 | it.n64, 8) << " "
                   << format_hex_no_prefix(it.n64size, 1) << " " << format_hex_no_prefix(K0 | it.addr, 8) << " "
                   << format_hex_no_prefix(it.size, 1) << " " << (asmData(it.g) ? "asm" : "c") << "\n";
        if (!ScatterReport.empty()) {
            std::error_code ec;
            raw_fd_ostream os(ScatterReport, ec);
            if (ec)
                fail("can't write " + ScatterReport);
            os << "# port-arena -port-arena-scatter=" << Scatter
               << (ScatterFE ? " (the front end too)" : "") << "\n"
               << "# alias NAME N64 CONTAINER+OFF c|asm SIZE | users...: a name inside another variable, given its own room\n"
               << "# host NAME N64 CONTAINER+OFF: a name only the host may use (PORT_N64_), where it went\n"
               << "# item NAME N64 N64SIZE ADDRESS SIZE c|asm: a variable placed\n"
               << rs.str();
        }
    }

    /* __port_scatter_bad, for the host: {start, length, kind, n64a, n64b, now, description} */
    void writeBad() {
        Type *i32 = Type::getInt32Ty(*C);
        PointerType *ptr = PointerType::get(*C, 0);
        StructType *st = StructType::get(*C, {i32, i32, i32, i32, i32, i32, ptr});
        std::vector<Constant *> rows;
        for (auto &b : bad) {
            if (!b.len)
                continue;
            Constant *s = ConstantDataArray::getString(*C, b.desc);
            auto *g = new GlobalVariable(*M, s->getType(), true, GlobalValue::PrivateLinkage, s, "__port_scatter_desc");
            rows.push_back(ConstantStruct::get(
                st, {ConstantInt::get(i32, b.start), ConstantInt::get(i32, b.len), ConstantInt::get(i32, b.kind),
                     ConstantInt::get(i32, b.n64a), ConstantInt::get(i32, b.n64b), ConstantInt::get(i32, b.now), g}));
        }
        ArrayType *at = ArrayType::get(st, rows.size());
        new GlobalVariable(*M, at, true, GlobalValue::ExternalLinkage, ConstantArray::get(at, rows), "__port_scatter_bad");
        new GlobalVariable(*M, i32, true, GlobalValue::ExternalLinkage, ConstantInt::get(i32, rows.size()),
                           "__port_scatter_bad_n");
    }

    /* -port-arena-scatter-check: the sites, "function (file:line) what" */
    std::vector<std::string> sites;
    std::map<std::string, unsigned> siteIndex;
    unsigned siteOf(Instruction *i, const std::string &what) {
        std::string s = where(i) + " " + what;
        auto it = siteIndex.find(s);
        if (it != siteIndex.end())
            return it->second;
        siteIndex[s] = sites.size();
        sites.push_back(s);
        return sites.size() - 1;
    }

    /* the access's address as port_scatter_check gives it back: where the
       N64's layout would have had it (PORT_SCATTER_REDIRECT), or as it is */
    Value *checkAccess(Instruction *i, unsigned op, Value *p) {
        IRBuilder<> b(i);
        Type *i32 = Type::getInt32Ty(*C);
        Value *size;
        std::string what;
        if (auto *ld = dyn_cast<LoadInst>(i)) {
            size = ConstantInt::get(i32, DL->getTypeStoreSize(ld->getType()));
            what = "load";
        } else if (auto *st = dyn_cast<StoreInst>(i)) {
            size = ConstantInt::get(i32, DL->getTypeStoreSize(st->getValueOperand()->getType()));
            what = "store";
        } else if (auto *rmw = dyn_cast<AtomicRMWInst>(i)) {
            size = ConstantInt::get(i32, DL->getTypeStoreSize(rmw->getValOperand()->getType()));
            what = "atomic";
        } else if (auto *cx = dyn_cast<AtomicCmpXchgInst>(i)) {
            size = ConstantInt::get(i32, DL->getTypeStoreSize(cx->getNewValOperand()->getType()));
            what = "atomic";
        } else if (auto *mi = dyn_cast<MemIntrinsic>(i)) {
            size = b.CreateZExtOrTrunc(mi->getLength(), i32);
            what = isa<MemSetInst>(mi) ? "memset" : op == 0 ? "copy to" : "copy from";
        } else {
            auto *cb = cast<CallBase>(i);
            if (cb->paramHasAttr(op, Attribute::ByVal)) {
                size = ConstantInt::get(i32, DL->getTypeAllocSize(cb->getParamByValType(op)));
                what = "by value";
            } else {
                size = ConstantInt::get(i32, 1);
                what = "to " + (cb->getCalledFunction() ? cb->getCalledFunction()->getName().str() : std::string("?"));
            }
        }
        Value *q = p;
        if (q->getType()->getPointerAddressSpace() != 0)
            q = b.CreateAddrSpaceCast(q, PointerType::get(*C, 0));
        FunctionCallee chk = M->getOrInsertFunction("port_scatter_check", FunctionType::get(i32, {i32, i32, i32}, false));
        Value *r = b.CreateCall(chk, {b.CreateTrunc(b.CreatePtrToInt(q, IP), i32), size,
                                      ConstantInt::get(i32, siteOf(i, what))});
        return b.CreateIntToPtr(b.CreateZExt(r, IP), PointerType::get(*C, 0));
    }

    /* a memset, memcpy or memmove of the game's memory is done by the host,
       byte by byte where it reaches what nothing should
       (port_scatter_mem(dst, src or value, length, kind, site): kind 0 a
       memset, 1 a copy) */
    void checkMemOps(std::vector<std::pair<Instruction *, unsigned>> &ops) {
        std::set<Instruction *> done;
        Type *i32 = Type::getInt32Ty(*C);
        FunctionCallee mem = M->getOrInsertFunction(
            "port_scatter_mem", FunctionType::get(Type::getVoidTy(*C), {i32, i32, i32, i32, i32}, false));
        for (auto &o : ops) {
            auto *mi = dyn_cast<MemIntrinsic>(o.first);
            if (!mi || done.count(mi))
                continue;
            auto *mt = dyn_cast<MemTransferInst>(mi);
            Value *d = mi->getRawDest(), *src = mt ? mt->getRawSource() : nullptr;
            if (host(d) || isa<ConstantPointerNull>(d) || (src && (host(src) || isa<ConstantPointerNull>(src))))
                continue;
            done.insert(mi);
        }
        for (Instruction *i : done) {
            auto *mi = cast<MemIntrinsic>(i);
            IRBuilder<> b(mi);
            auto addr = [&](Value *v) {
                if (v->getType()->getPointerAddressSpace())
                    v = b.CreateAddrSpaceCast(v, PointerType::get(*C, 0));
                return b.CreateTrunc(b.CreatePtrToInt(v, IP), i32);
            };
            Value *second;
            if (auto *ms = dyn_cast<MemSetInst>(mi))
                second = b.CreateZExt(ms->getValue(), i32);
            else
                second = addr(cast<MemTransferInst>(mi)->getRawSource());
            bool set = isa<MemSetInst>(mi);
            b.CreateCall(mem, {addr(mi->getRawDest()), second, b.CreateZExtOrTrunc(mi->getLength(), i32),
                               ConstantInt::get(i32, set ? 0 : 1),
                               ConstantInt::get(i32, siteOf(mi, set ? "memset" : "copy"))});
        }
        ops.erase(std::remove_if(ops.begin(), ops.end(), [&](auto &o) { return done.count(o.first); }), ops.end());
        for (Instruction *i : done)
            i->eraseFromParent();
    }

    void writeSites() {
        PointerType *ptr = PointerType::get(*C, 0);
        std::vector<Constant *> rows;
        for (auto &s : sites) {
            Constant *c = ConstantDataArray::getString(*C, s);
            rows.push_back(new GlobalVariable(*M, c->getType(), true, GlobalValue::PrivateLinkage, c, "__port_scatter_site"));
        }
        ArrayType *at = ArrayType::get(ptr, rows.size());
        new GlobalVariable(*M, at, true, GlobalValue::ExternalLinkage, ConstantArray::get(at, rows),
                           "__port_scatter_sites");
    }

    void removeFromUsed(StringRef name) {
        GlobalVariable *u = M->getGlobalVariable(name);
        if (!u)
            return;
        std::set<Value *> gone;
        for (auto &p : placed)
            gone.insert(p.g);
        auto *ca = dyn_cast<ConstantArray>(u->getInitializer());
        std::vector<GlobalValue *> keep;
        if (ca)
            for (Value *op : ca->operands())
                if (auto *gv = dyn_cast<GlobalValue>(op->stripPointerCasts()))
                    if (!gone.count(gv))
                        keep.push_back(gv);
        u->eraseFromParent();
        if (keep.empty())
            return;
        if (name == "llvm.used")
            appendToUsed(*M, keep);
        else
            appendToCompilerUsed(*M, keep);
    }

    /* ---- the image --------------------------------------------------- */

    std::vector<uint8_t> image;
    struct Reloc {
        uint64_t off;
        unsigned width;
        GlobalValue *target;
        int64_t addend;
    };
    std::vector<Reloc> relocs;

    /* the initializers in host order, as the compiler would have emitted
       them; then what BEPass's tables say to swap is swapped (big-endian
       memory: every scalar the C's initializers had; native: the words of
       their 64-bit ones). */
    void putInt(uint64_t off, uint64_t v, unsigned size) {
        if (off + size > image.size())
            fail("an initializer outside the arena image");
        for (unsigned k = 0; k < size; k++)
            image[off + k] = (uint8_t)(v >> (8 * k));
    }

    struct Swap {
        GlobalVariable *g;
        uint64_t off;
        unsigned count, size;
    };
    std::vector<Swap> swaps;

    void applySwaps(const std::map<GlobalVariable *, uint64_t> &at) {
        for (auto &s : swaps) {
            auto it = at.find(s.g);
            if (it == at.end())
                continue;           /* the host's */
            for (unsigned n = 0; n < s.count; n++) {
                uint8_t *p = image.data() + it->second + s.off + (uint64_t)n * s.size;
                if (Native) {       /* __bepass_fixup_rot64: the two words exchanged */
                    if (s.size != 8)
                        fail("a native fixup that isn't 8 bytes");
                    for (unsigned k = 0; k < 4; k++)
                        std::swap(p[k], p[4 + k]);
                } else {
                    std::reverse(p, p + s.size);
                }
            }
        }
    }

    /* an address-valued constant as a base (a function, the host's data)
       and an offset, or a plain integer (base null) */
    bool eval(Constant *c, GlobalValue *&gv, int64_t &off) {
        if (auto *ci = dyn_cast<ConstantInt>(c)) {
            gv = nullptr;
            off = (int64_t)ci->getZExtValue();
            return true;
        }
        if (isa<ConstantPointerNull>(c)) {
            gv = nullptr;
            off = 0;
            return true;
        }
        if (auto *g = dyn_cast<GlobalValue>(c)) {
            gv = g;
            off = 0;
            return true;
        }
        auto *ce = dyn_cast<ConstantExpr>(c);
        if (!ce)
            return false;
        switch (ce->getOpcode()) {
        case Instruction::PtrToInt:
        case Instruction::IntToPtr:
        case Instruction::BitCast:
        case Instruction::AddrSpaceCast:
        case Instruction::ZExt:
            return eval(ce->getOperand(0), gv, off);
        case Instruction::Trunc: {
            if (!eval(ce->getOperand(0), gv, off))
                return false;
            if (!gv)
                off &= (int64_t)(~0ull >> (64 - ce->getType()->getIntegerBitWidth()));
            return true;
        }
        case Instruction::Add:
        case Instruction::Sub: {
            GlobalValue *g2;
            int64_t o2;
            if (!eval(ce->getOperand(0), gv, off) || !eval(ce->getOperand(1), g2, o2) || g2)
                return false;
            off = ce->getOpcode() == Instruction::Add ? off + o2 : off - o2;
            return true;
        }
        case Instruction::GetElementPtr: {
            auto *gep = cast<GEPOperator>(ce);
            APInt a(DL->getIndexTypeSizeInBits(gep->getType()), 0);
            if (!gep->accumulateConstantOffset(*DL, a) || !eval(cast<Constant>(gep->getPointerOperand()), gv, off))
                return false;
            off += a.getSExtValue();
            return true;
        }
        default:
            return false;
        }
    }

    void put(uint64_t off, Constant *c) {
        if (auto *ce = dyn_cast<ConstantExpr>(c))
            c = ConstantFoldConstant(ce, *DL);
        Type *t = c->getType();
        if (isa<ConstantAggregateZero>(c) || isa<UndefValue>(c) || isa<ConstantPointerNull>(c))
            return;
        if (auto *ci = dyn_cast<ConstantInt>(c)) {
            unsigned size = DL->getTypeStoreSize(t);
            if (size > 8)
                fail("an integer wider than 64 bits");
            putInt(off, ci->getZExtValue(), size);
            return;
        }
        if (auto *cf = dyn_cast<ConstantFP>(c)) {
            putInt(off, cf->getValueAPF().bitcastToAPInt().getZExtValue(), DL->getTypeStoreSize(t));
            return;
        }
        if (auto *cds = dyn_cast<ConstantDataSequential>(c)) {
            uint64_t es = DL->getTypeAllocSize(cds->getElementType());
            for (unsigned n = 0; n < cds->getNumElements(); n++)
                put(off + n * es, cds->getElementAsConstant(n));
            return;
        }
        if (auto *cs = dyn_cast<ConstantStruct>(c)) {
            const StructLayout *sl = DL->getStructLayout(cs->getType());
            for (unsigned n = 0; n < cs->getNumOperands(); n++)
                put(off + sl->getElementOffset(n), cs->getOperand(n));
            return;
        }
        if (isa<ConstantArray>(c) || isa<ConstantVector>(c)) {
            Type *et = isa<ConstantArray>(c) ? cast<ArrayType>(t)->getElementType() : cast<VectorType>(t)->getElementType();
            uint64_t es = DL->getTypeAllocSize(et);
            for (unsigned n = 0; n < c->getNumOperands(); n++)
                put(off + n * es, cast<Constant>(c->getOperand(n)));
            return;
        }
        GlobalValue *gv;
        int64_t o;
        if (!eval(c, gv, o)) {
            std::string s;
            raw_string_ostream os(s);
            os << *c;
            fail("can't write the initializer " + os.str());
        }
        unsigned size = DL->getTypeStoreSize(t);
        if (!gv) {
            putInt(off, (uint64_t)o, size);
            return;
        }
        relocs.push_back({off, size, gv, o});
    }

    void writeImage() {
        image.assign(extraEnd, 0);
        std::map<GlobalVariable *, uint64_t> at;
        for (auto &p : placed) {
            at[p.g] = p.addr;
            if (p.g->hasInitializer())
                put(p.addr, p.g->getInitializer());
        }
        applySwaps(at);
        for (auto &p : placed) {
            if (!p.g->use_empty())
                fail(p.g->getName() + " is still used");
            p.g->eraseFromParent();
        }
        if (!ImageFile.empty()) {
            std::error_code ec;
            raw_fd_ostream os(ImageFile, ec, sys::fs::OF_None);
            if (ec)
                fail("can't write " + ImageFile);
            os.write((const char *)image.data(), image.size());
        }
        /* runs of non-zero bytes (zero runs of 64 or more between them) */
        Type *i32 = Type::getInt32Ty(*C), *i64 = Type::getInt64Ty(*C);
        PointerType *ptr = PointerType::get(*C, 0);
        StructType *runT = StructType::get(*C, {i32, i32, ptr});
        std::vector<Constant *> runs;
        size_t k = 0, n = image.size();
        if (NoRuns)
            k = n;
        while (k < n) {
            while (k < n && !image[k])
                k++;
            if (k == n)
                break;
            size_t e = k, zeros = 0;
            while (e < n && zeros < 64) {
                zeros = image[e] ? 0 : zeros + 1;
                e++;
            }
            e -= zeros;
            auto *data = new GlobalVariable(*M, ArrayType::get(Type::getInt8Ty(*C), e - k), true,
                                            GlobalValue::PrivateLinkage,
                                            ConstantDataArray::get(*C, ArrayRef<uint8_t>(image.data() + k, e - k)),
                                            "__port_arena_data");
            runs.push_back(ConstantStruct::get(runT, {ConstantInt::get(i32, k), ConstantInt::get(i32, e - k), data}));
            k = e;
        }
        auto table = [&](const char *name, StructType *st, std::vector<Constant *> &rows) {
            ArrayType *at = ArrayType::get(st, rows.size());
            new GlobalVariable(*M, at, true, GlobalValue::ExternalLinkage, ConstantArray::get(at, rows), name);
            new GlobalVariable(*M, i32, true, GlobalValue::ExternalLinkage, ConstantInt::get(i32, rows.size()),
                               std::string(name) + "_n");
        };
        table("__port_arena_runs", runT, runs);
        StructType *relT = StructType::get(*C, {i32, i32, ptr, i64});
        std::vector<Constant *> rels;
        for (auto &r : relocs)
            rels.push_back(ConstantStruct::get(relT, {ConstantInt::get(i32, r.off), ConstantInt::get(i32, r.width),
                                                      r.target, ConstantInt::get(i64, r.addend)}));
        table("__port_arena_relocs", relT, rels);
        new GlobalVariable(*M, i32, true, GlobalValue::ExternalLinkage, ConstantInt::get(i32, extraEnd),
                           "__port_arena_data_end");
    }

    /* ---- the accesses ------------------------------------------------ */

    /* whether a local's address goes anywhere but the pointer operand of
       loads, stores and memory intrinsics */
    static bool escapes(Value *v) {
        for (User *u : v->users()) {
            if (isa<LoadInst>(u))
                continue;
            if (auto *st = dyn_cast<StoreInst>(u)) {
                if (st->getValueOperand() == v)
                    return true;
                continue;
            }
            if (auto *ii = dyn_cast<IntrinsicInst>(u)) {
                if (ii->isLifetimeStartOrEnd() || isa<DbgInfoIntrinsic>(ii))
                    continue;
                if (auto *mi = dyn_cast<MemIntrinsic>(ii)) {
                    if (mi->getLength() == v)
                        return true;
                    if (auto *ms = dyn_cast<MemSetInst>(mi))
                        if (ms->getValue() == v)
                            return true;
                    continue;
                }
                return true;
            }
            if (isa<GetElementPtrInst>(u) || isa<BitCastInst>(u) || isa<AddrSpaceCastInst>(u)) {
                if (escapes(u))
                    return true;
                continue;
            }
            return true;
        }
        return false;
    }

    /* The locals left in memory (-O2 promoted the rest: the escaping ones,
       and the arrays and structs it couldn't) get N64 addresses on a stack
       of the pass's own (the locals' stack: PORT_ARENA_LOCALS, a slot per
       thread, port_locals_sp the running thread's pointer, threads.c), laid
       out here: each function's are one frame, at offsets decided from the
       IR, which is the same wherever it is code-generated (WebAssembly's is
       i386's, -port-arena-triple).  So the addresses the game stores in
       RDRAM, and what is left in the frames between calls (which the
       decompiled C reads where IDO's code read an uninitialized local),
       don't depend on the backend's frames: the Linux 32-bit build and
       wasm32 have the same.  (The ones that don't escape go there too, for
       what they may read before they are written.)  The frame is taken at
       the entry and given back at every return; a thread's frames are
       dropped with it, and a thread starts at the top of its slot.  A frame
       outside the running thread's slot (past its end, port_locals_end,
       which is 0 outside a thread) stops the port (port_arena_bad_local). */
    void locals(Function &f, LoadInst *arena) {
        if (f.isVarArg())       /* its va_list is the host's */
            return;
        std::vector<Value *> as;
        for (BasicBlock &bb : f)
            for (Instruction &i : bb)
                if (auto *a = dyn_cast<AllocaInst>(&i)) {
                    as.push_back(a);
                    if (!escapes(a))
                        kept++;
                }
        /* (a struct passed by value is a local too, the call's copy) */
        for (Argument &a : f.args())
            if (a.hasByValAttr() && escapes(&a))
                as.push_back(&a);
        if (as.empty())
            return;
        /* the entry block's allocas first, so that the frame's setup (and
           its check's branch) comes after all of them */
        BasicBlock &entry = f.getEntryBlock();
        std::vector<AllocaInst *> top;
        for (Instruction &i : entry)
            if (auto *ai = dyn_cast<AllocaInst>(&i))
                if (isa<ConstantInt>(ai->getArraySize()))
                    top.push_back(ai);
        for (AllocaInst *ai : top)
            ai->moveBefore(arena->getIterator());
        /* the frame */
        std::vector<uint64_t> offs, sizes;
        uint64_t size = 0, align = 16;
        for (Value *a : as) {
            Type *t;
            uint64_t n = 1, al;
            if (auto *ai = dyn_cast<AllocaInst>(a)) {
                if (!ai->isStaticAlloca())
                    fail("a variable-sized local in " + f.getName());
                t = ai->getAllocatedType();
                n = cast<ConstantInt>(ai->getArraySize())->getZExtValue();
                al = ai->getAlign().value();
            } else {
                Argument *arg = cast<Argument>(a);
                t = arg->getParamByValType();
                al = arg->getParamAlign().valueOrOne().value();
            }
            uint64_t sz = frameDL->getTypeAllocSize(t) * n;
            if (DL->getTypeAllocSize(t) * n != sz)
                fail("a local's size differs from the target's in " + f.getName());
            al = std::max<uint64_t>(al, 1);
            size = alignTo(size, al);
            offs.push_back(size);
            sizes.push_back(sz);
            size += sz;
            align = std::max(align, al);
        }
        size = alignTo(size, align);
        IntegerType *i32 = Type::getInt32Ty(*C);
        if (!lsp) {
            lsp = new GlobalVariable(*M, i32, false, GlobalValue::ExternalLinkage, nullptr, "port_locals_sp");
            lsplim = new GlobalVariable(*M, i32, false, GlobalValue::ExternalLinkage, nullptr, "port_locals_end");
        }
        IRBuilder<> b(arena->getNextNode());
        Value *old = b.CreateLoad(i32, lsp, "port.lsp");
        Value *sp = b.CreateAnd(b.CreateSub(old, ConstantInt::get(i32, size)), ConstantInt::get(i32, ~(align - 1)));
        b.CreateStore(sp, lsp);
        /* (the slot's bottom is port_locals_end, 0 outside a thread) */
        Value *out = b.CreateICmpUGE(b.CreateSub(sp, b.CreateLoad(i32, lsplim)), ConstantInt::get(i32, LOCALS_SLOT));
        Instruction *at = &*b.GetInsertPoint();
        FunctionCallee bad = M->getOrInsertFunction("port_arena_bad_local",
                                                    FunctionType::get(Type::getVoidTy(*C), {IP}, false));
        Instruction *then = SplitBlockAndInsertIfThen(out, at, false);
        IRBuilder<> tb(then);
        tb.CreateCall(bad, {tb.CreateZExt(sp, IP)});
        IRBuilder<> nb(at);
        /* (the markers go after the loop: `at` may be one of them, a
           lifetime.start just after the allocas) */
        std::vector<Instruction *> dead;
        for (size_t k = 0; k < as.size(); k++) {
            Value *a = as[k];
            Value *np = nb.CreateIntToPtr(nb.CreateZExt(nb.CreateAdd(sp, ConstantInt::get(i32, offs[k])), IP),
                                          a->getType(), a->getName() + ".n64");
            Instruction *copy = nullptr;
            if (auto *arg = dyn_cast<Argument>(a))
                copy = nb.CreateMemCpy(np, MaybeAlign(1), arg, arg->getParamAlign(), sizes[k]);
            a->replaceUsesWithIf(np, [&](Use &u) { return u.getUser() != copy; });
            if (auto *ai = dyn_cast<AllocaInst>(a)) {
                /* what is left of it: lifetime markers */
                for (User *u : np->users())
                    if (auto *ii = dyn_cast<IntrinsicInst>(u))
                        if (ii->isLifetimeStartOrEnd())
                            dead.push_back(ii);
                ai->eraseFromParent();
            }
            escaped++;
        }
        for (Instruction *d : dead)
            d->eraseFromParent();
        for (BasicBlock &bb : f)
            if (auto *r = dyn_cast<ReturnInst>(bb.getTerminator()))
                IRBuilder<>(r).CreateStore(old, lsp);
        frames++;
        frameMax = std::max(frameMax, size);
    }

    Value *map(IRBuilder<> &b, Value *p, Value *arena) {
        Type *pt = p->getType();
        Value *q = p;
        if (pt->getPointerAddressSpace() != 0)
            q = b.CreateAddrSpaceCast(p, PointerType::get(*C, 0));
        Value *off = b.CreateAnd(b.CreatePtrToInt(q, IP), ConstantInt::get(IP, MASK));
        rewritten++;
        return b.CreateGEP(b.getInt8Ty(), arena, off);
    }

    /* memory where it is: a local that doesn't escape, the host's data */
    static bool host(Value *p) {
        const Value *u = getUnderlyingObject(p, 0);
        return isa<AllocaInst>(u) || isa<GlobalVariable>(u) || isa<Argument>(u) && cast<Argument>(u)->hasByValAttr();
    }

    void function(Function &f) {
        IRBuilder<> eb(&*f.getEntryBlock().getFirstInsertionPt());
        LoadInst *arena = eb.CreateLoad(PointerType::get(*C, 0), base, "port.arena");
        locals(f, arena);
        std::vector<std::pair<Instruction *, unsigned>> ops;
        for (BasicBlock &bb : f)
            for (Instruction &i : bb) {
                if (&i == arena)
                    continue;
                if (auto *ld = dyn_cast<LoadInst>(&i))
                    ops.push_back({&i, ld->getPointerOperandIndex()});
                else if (auto *st = dyn_cast<StoreInst>(&i))
                    ops.push_back({&i, st->getPointerOperandIndex()});
                else if (auto *rmw = dyn_cast<AtomicRMWInst>(&i))
                    ops.push_back({&i, rmw->getPointerOperandIndex()});
                else if (auto *cx = dyn_cast<AtomicCmpXchgInst>(&i))
                    ops.push_back({&i, cx->getPointerOperandIndex()});
                else if (auto *mi = dyn_cast<MemIntrinsic>(&i)) {
                    ops.push_back({&i, 0});
                    if (isa<MemTransferInst>(mi))
                        ops.push_back({&i, 1});
                } else if (auto *cb = dyn_cast<CallBase>(&i)) {
                    if (isa<IntrinsicInst>(cb) || cb->isInlineAsm())
                        continue;
                    /* a struct passed by value is copied by the call from
                       memory: the host's address of it */
                    for (unsigned a = 0; a < cb->arg_size(); a++)
                        if (cb->paramHasAttr(a, Attribute::ByVal))
                            ops.push_back({&i, a});
                    Function *callee = cb->getCalledFunction();
                    if (!callee || !callee->isDeclaration() || callee->isIntrinsic() || !hostCallee(callee->getName()))
                        continue;
                    for (unsigned a = 0; a < cb->arg_size(); a++)
                        if (cb->getArgOperand(a)->getType()->isPointerTy() && !cb->paramHasAttr(a, Attribute::ByVal)) {
                            ops.push_back({&i, a});
                            hostArgs++;
                        }
                }
            }
        /* a memory intrinsic on 32-bit pointers (PTR32) is made again on
           ordinary ones, which is what the mapped addresses are */
        std::vector<MemIntrinsic *> redo;
        for (auto &o : ops)
            if (auto *mi = dyn_cast<MemIntrinsic>(o.first))
                if (mi->getOperand(o.second)->getType()->getPointerAddressSpace() != 0 &&
                    (redo.empty() || redo.back() != mi))
                    redo.push_back(mi);
        for (MemIntrinsic *mi : redo) {
            IRBuilder<> b(mi);
            PointerType *p0 = PointerType::get(*C, 0);
            auto as0 = [&](Value *v) {
                return v->getType()->getPointerAddressSpace() ? b.CreateAddrSpaceCast(v, p0) : v;
            };
            CallInst *n;
            if (auto *ms = dyn_cast<MemSetInst>(mi))
                n = b.CreateMemSet(as0(ms->getDest()), ms->getValue(), ms->getLength(), ms->getDestAlign(),
                                   ms->isVolatile());
            else if (isa<MemMoveInst>(mi))
                n = b.CreateMemMove(as0(mi->getRawDest()), mi->getDestAlign(),
                                    as0(cast<MemTransferInst>(mi)->getRawSource()),
                                    cast<MemTransferInst>(mi)->getSourceAlign(), mi->getLength(), mi->isVolatile());
            else
                n = b.CreateMemCpy(as0(mi->getRawDest()), mi->getDestAlign(),
                                   as0(cast<MemTransferInst>(mi)->getRawSource()),
                                   cast<MemTransferInst>(mi)->getSourceAlign(), mi->getLength(), mi->isVolatile());
            for (auto &o : ops)
                if (o.first == mi)
                    o.first = n;
            mi->eraseFromParent();
        }
        if (ScatterCheck)
            checkMemOps(ops);
        for (auto &o : ops) {
            Value *p = o.first->getOperand(o.second);
            if (host(p) || isa<ConstantPointerNull>(p))
                continue;
            IRBuilder<> b(o.first);
            /* (a variadic argument can change its type: n64_sprintf's PTR32
               strings) */
            auto *cb = dyn_cast<CallBase>(o.first);
            bool vararg = cb && !isa<IntrinsicInst>(cb) && o.second >= cb->getFunctionType()->getNumParams();
            if (p->getType()->getPointerAddressSpace() != 0 && !isa<LoadInst>(o.first) &&
                !isa<StoreInst>(o.first) && !vararg)
                fail(Twine("a 32-bit pointer operand of ") + o.first->getOpcodeName() + " in " + f.getName());
            if (ScatterCheck)
                p = checkAccess(o.first, o.second, p);
            o.first->setOperand(o.second, map(b, p, arena));
        }
    }

    /* ---- functions --------------------------------------------------- */

    std::vector<std::pair<uint64_t, Function *>> fns;

    static bool calleeUse(const Use &u) {
        auto *cb = dyn_cast<CallBase>(u.getUser());
        return cb && cb->isCallee(&u);
    }

    /* the functions the translated code takes the address of (a lui/addiu
       pair: jp's func_801F57B0 starts the pak thread at func_801F58E8), which
       the C then calls through: they are values too, though no use in the
       module says so */
    std::set<std::string> fnValues;

    void readFnValues() {
        if (FnValues.empty())
            return;
        auto buf = MemoryBuffer::getFile(FnValues);
        if (!buf)
            fail("can't read " + FnValues);
        SmallVector<StringRef, 0> lines;
        (*buf)->getBuffer().split(lines, '\n', -1, false);
        for (StringRef l : lines)
            if (!l.trim().empty())
                fnValues.insert(l.trim().str());
    }

    void functions() {
        uint64_t next = FN_BASE;
        readFnValues();
        std::vector<Function *> taken;
        for (Function &f : *M) {
            if (f.isIntrinsic())
                continue;
            bool value = false;
            for (const Use &u : f.uses()) {
                if (calleeUse(u))
                    continue;
                if (auto *g = dyn_cast<GlobalVariable>(u.getUser()->stripPointerCasts()))
                    if (g->getName().starts_with("llvm."))
                        fail(f.getName() + " is both a value and in " + g->getName());
                value = true;
            }
            if (value || fnValues.count(f.getName().str()))
                taken.push_back(&f);
        }
        for (Function *f : taken) {
            auto it = syms.find(f->getName().str());
            uint64_t a;
            if (it != syms.end() && it->second.second == 'F')
                a = it->second.first;
            else {
                a = next;
                next += 16;
            }
            fns.push_back({a, f});
            /* the calls stay direct: RAUW, then the callees back */
            std::vector<CallBase *> calls;
            for (Use &u : f->uses())
                if (calleeUse(u))
                    calls.push_back(cast<CallBase>(u.getUser()));
            f->replaceAllUsesWith(addrConst(a, f->getType()));
            for (CallBase *cb : calls)
                cb->setCalledOperand(f);
        }
        std::sort(fns.begin(), fns.end());
        for (size_t k = 1; k < fns.size(); k++)
            if (fns[k].first == fns[k - 1].first)
                fail(fns[k].second->getName() + " and " + fns[k - 1].second->getName() + " at one address");
    }

    /* a call through a value: through port_fn(value) */
    unsigned indirect = 0;
    void indirectCalls(Function &f) {
        std::vector<CallBase *> cs;
        for (BasicBlock &bb : f)
            for (Instruction &i : bb)
                if (auto *cb = dyn_cast<CallBase>(&i))
                    if (!cb->isInlineAsm() && !isa<Function>(cb->getCalledOperand()->stripPointerCasts()))
                        cs.push_back(cb);
        if (cs.empty())
            return;
        PointerType *ptr = PointerType::get(*C, 0);
        Type *i32 = Type::getInt32Ty(*C);
        FunctionCallee lookup = M->getOrInsertFunction("port_fn", FunctionType::get(ptr, {i32}, false));
        FunctionCallee typed = M->getOrInsertFunction("port_fn_typed", FunctionType::get(ptr, {i32, i32}, false));
        for (CallBase *cb : cs) {
            IRBuilder<> b(cb);
            Value *v = cb->getCalledOperand();
            if (v->getType()->getPointerAddressSpace())
                v = b.CreateAddrSpaceCast(v, ptr);
            Value *a = b.CreateTrunc(b.CreatePtrToInt(v, IP), i32);
            if (TypedCalls)
                cb->setCalledOperand(
                    b.CreateCall(typed, {a, ConstantInt::get(i32, sigIndex.at(cb->getFunctionType()))}));
            else
                cb->setCalledOperand(b.CreateCall(lookup, {a}));
            indirect++;
        }
    }

    /* -port-arena-typed-calls: WebAssembly can't call a function through
       another type than its own, and the decompiled C calls through values
       of other types than the callees' (a thread entry defined without its
       argument, a handler's K&R declaration).  So each function used as a
       value gets a thunk for every type it is called through, which calls
       it as callFix makes a direct call; port_fn_typed(address, type) picks
       it.  Type 0 is the host's, void(ptr) (a thread's entry, threads.c):
       port_fn's table has those. */
    std::vector<FunctionType *> sigs;
    std::map<FunctionType *, unsigned> sigIndex;
    unsigned thunks = 0;

    unsigned sigOf(FunctionType *t) {
        auto it = sigIndex.find(t);
        if (it != sigIndex.end())
            return it->second;
        sigIndex[t] = sigs.size();
        sigs.push_back(t);
        return sigs.size() - 1;
    }

    void collectSigs() {
        sigOf(FunctionType::get(Type::getVoidTy(*C), {PointerType::get(*C, 0)}, false));
        for (Function &f : *M)
            for (BasicBlock &bb : f)
                for (Instruction &i : bb)
                    if (auto *cb = dyn_cast<CallBase>(&i))
                        if (!cb->isInlineAsm() && !isa<Function>(cb->getCalledOperand()->stripPointerCasts()))
                            sigOf(cb->getFunctionType());
    }

    Function *thunk(Function *f, unsigned s) {
        FunctionType *t = sigs[s], *ft = f->getFunctionType();
        if (t == ft)
            return f;
        Function *th = Function::Create(t, GlobalValue::InternalLinkage, f->getName() + ".as" + Twine(s), M);
        IRBuilder<> b(BasicBlock::Create(*C, "", th));
        std::vector<Value *> args;
        for (unsigned a = 0; a < ft->getNumParams(); a++)
            args.push_back(a < t->getNumParams() ? conv(b, th->getArg(a), ft->getParamType(a), false)
                                                 : Constant::getNullValue(ft->getParamType(a)));
        if (ft->isVarArg())
            for (unsigned a = ft->getNumParams(); a < t->getNumParams(); a++)
                args.push_back(th->getArg(a));
        CallInst *n = b.CreateCall(ft, f, args);
        n->setCallingConv(f->getCallingConv());
        n->setAttributes(f->getAttributes());
        if (t->getReturnType()->isVoidTy())
            b.CreateRetVoid();
        else if (ft->getReturnType()->isVoidTy())
            b.CreateRet(Constant::getNullValue(t->getReturnType()));
        else
            b.CreateRet(conv(b, n, t->getReturnType(), f->getAttributes().hasRetAttr(Attribute::SExt)));
        thunks++;
        return th;
    }

    void writeFns() {
        Type *i32 = Type::getInt32Ty(*C);
        PointerType *ptr = PointerType::get(*C, 0);
        StructType *st = StructType::get(*C, {i32, ptr});
        StructType *tst = StructType::get(*C, {i32, i32, ptr});
        std::vector<Constant *> rows, trows;
        if (TypedCalls)
            collectSigs();
        for (auto &f : fns) {
            Constant *a = ConstantInt::get(i32, f.first);
            rows.push_back(ConstantStruct::get(st, {a, TypedCalls ? thunk(f.second, 0) : f.second}));
            if (TypedCalls)
                for (unsigned s = 0; s < sigs.size(); s++)
                    trows.push_back(ConstantStruct::get(tst, {a, ConstantInt::get(i32, s), thunk(f.second, s)}));
        }
        ArrayType *at = ArrayType::get(st, rows.size());
        new GlobalVariable(*M, at, true, GlobalValue::ExternalLinkage, ConstantArray::get(at, rows), "__port_fns");
        new GlobalVariable(*M, i32, true, GlobalValue::ExternalLinkage, ConstantInt::get(i32, rows.size()),
                           "__port_fns_n");
        /* (sorted by address, then type, as fns is) */
        ArrayType *tat = ArrayType::get(tst, trows.size());
        new GlobalVariable(*M, tat, true, GlobalValue::ExternalLinkage, ConstantArray::get(tat, trows),
                           "__port_fns_typed");
        new GlobalVariable(*M, i32, true, GlobalValue::ExternalLinkage, ConstantInt::get(i32, trows.size()),
                           "__port_fns_typed_n");
    }

    /* What is left undefined (undef, poison: an argument the caller never
       set, a function that falls off its end, a phi from a path that set
       nothing) each backend makes something of its own, which in i386's
       case is what was in a register or on the stack: the N64 had
       whatever was in its register too.  Made 0, it is the same on every
       target. */
    void defineUndef() {
        for (Function &f : *M)
            for (BasicBlock &bb : f)
                for (Instruction &i : bb) {
                    if (isa<ShuffleVectorInst>(&i) || isa<DbgInfoIntrinsic>(&i))
                        continue;
                    for (Use &u : i.operands())
                        if (isa<UndefValue>(u.get()) && u->getType()->isFirstClassType() &&
                            !u->getType()->isLabelTy() && !u->getType()->isTokenTy() &&
                            !u->getType()->isMetadataTy()) {
                            u.set(Constant::getNullValue(u->getType()));
                            undefs++;
                        }
                }
    }

    /* the replay's hooks (port/src/replay_hooks.c) are told who called */
    void replayCallers() {
        PointerType *ptr = PointerType::get(*C, 0);
        FunctionCallee set = M->getOrInsertFunction("port_replay_set_caller",
                                                    FunctionType::get(Type::getVoidTy(*C), {ptr}, false));
        std::map<Function *, Constant *> names;
        for (const char *hook : {"__wrap_func_802D4E10", "__wrap_alCSeqGetLoc"}) {
            Function *h = M->getFunction(hook);
            if (!h)
                continue;
            std::vector<CallBase *> calls;
            for (Use &u : h->uses())
                if (calleeUse(u))
                    calls.push_back(cast<CallBase>(u.getUser()));
            for (CallBase *cb : calls) {
                Function *caller = cb->getFunction();
                Constant *&n = names[caller];
                if (!n) {
                    Constant *s = ConstantDataArray::getString(*C, caller->getName());
                    n = new GlobalVariable(*M, s->getType(), true, GlobalValue::PrivateLinkage, s,
                                           "__port_caller_name");
                }
                IRBuilder<> b(cb);
                b.CreateCall(set, {n});
            }
        }
    }

    /* ---- calls ------------------------------------------------------- */

    /* a value as the N64's registers would carry it into a slot of type
       `to`: integers truncated or extended (sext by the caller's say),
       pointers and integers through their bits, a float and an integer of
       one size through their bits; anything else is lost (poison) */
    Value *conv(IRBuilder<> &b, Value *v, Type *to, bool sext) {
        Type *from = v->getType();
        if (from == to)
            return v;
        auto bitsOf = [&](Type *t) -> unsigned {
            if (t->isPointerTy())
                return DL->getPointerSizeInBits(t->getPointerAddressSpace());
            return t->getPrimitiveSizeInBits();
        };
        unsigned fb = bitsOf(from), tb = bitsOf(to);
        if (!fb || !tb || from->isStructTy() || to->isStructTy() || from->isVectorTy() || to->isVectorTy()) {
            unfixable++;
            return PoisonValue::get(to);
        }
        Value *i = v;
        if (from->isPointerTy())
            i = b.CreatePtrToInt(v, b.getIntNTy(fb));
        else if (from->isFloatingPointTy())
            i = b.CreateBitCast(v, b.getIntNTy(fb));
        i = sext ? b.CreateSExtOrTrunc(i, b.getIntNTy(tb)) : b.CreateZExtOrTrunc(i, b.getIntNTy(tb));
        if (to->isPointerTy())
            return b.CreateIntToPtr(i, to);
        if (to->isFloatingPointTy())
            return b.CreateBitCast(i, to);
        return i;
    }

    unsigned fixedCalls = 0, unfixable = 0, voidResults = 0, extFixes = 0, argExts = 0;

    void callFix() {
        std::vector<CallInst *> work;
        for (Function &f : *M)
            for (BasicBlock &bb : f)
                for (Instruction &i : bb)
                    if (auto *ci = dyn_cast<CallInst>(&i))
                        if (auto *callee = dyn_cast<Function>(ci->getCalledOperand()))
                            if (!callee->isIntrinsic() && ci->getFunctionType() != callee->getFunctionType())
                                work.push_back(ci);
        for (CallInst *ci : work) {
            auto *callee = cast<Function>(ci->getCalledOperand());
            FunctionType *ft = callee->getFunctionType();
            if (const char *st = getenv("PORT_ARENA_STATS"); st && atoi(st) > 1)
                errs() << "port-arena: " << ci->getFunction()->getName() << " calls " << callee->getName()
                       << " as " << *ci->getFunctionType() << ", not " << *ft << "\n";
            IRBuilder<> b(ci);
            std::vector<Value *> args;
            for (unsigned a = 0; a < ft->getNumParams(); a++) {
                Type *pt = ft->getParamType(a);
                if (a < ci->arg_size())
                    args.push_back(conv(b, ci->getArgOperand(a), pt, ci->paramHasAttr(a, Attribute::SExt)));
                else
                    args.push_back(Constant::getNullValue(pt));      /* a register the caller didn't set */
            }
            /* a variadic callee takes the rest as they are (34430.c
               declares func_8029A7E4 with four parameters) */
            if (ft->isVarArg())
                for (unsigned a = ft->getNumParams(); a < ci->arg_size(); a++)
                    args.push_back(ci->getArgOperand(a));
            CallInst *n = b.CreateCall(ft, callee, args);
            n->setCallingConv(callee->getCallingConv());
            n->setAttributes(callee->getAttributes());
            n->setTailCallKind(ci->getTailCallKind() == CallInst::TCK_MustTail ? CallInst::TCK_None
                                                                               : ci->getTailCallKind());
            n->setDebugLoc(ci->getDebugLoc());
            if (!ci->getType()->isVoidTy() && !ci->use_empty()) {
                Value *r;
                if (ft->getReturnType()->isVoidTy()) {
                    r = Constant::getNullValue(ci->getType());      /* v0 as the callee left it */
                    voidResults++;
                } else {
                    r = conv(b, n, ci->getType(), callee->getAttributes().hasRetAttr(Attribute::SExt));
                }
                ci->replaceAllUsesWith(r);
            }
            ci->eraseFromParent();
            fixedCalls++;
        }
        /* A call of the callee's type that says the narrow result is
           extended one way when the callee extends it the other (1D990.c's
           s16 func_8028604C(s32) for 409D0.c's u16 (u32)): the N64 used v0
           as the callee left it, and so does x86, which takes the call's
           word for it and drops the caller's extension; WebAssembly extends
           again.  The caller's extension of the result is made the
           callee's, which every target does alike. */
        for (Function &f : *M)
            for (BasicBlock &bb : f)
                for (Instruction &i : bb) {
                    auto *ci = dyn_cast<CallInst>(&i);
                    auto *callee = ci ? dyn_cast<Function>(ci->getCalledOperand()) : nullptr;
                    if (callee && !callee->isIntrinsic() && !callee->isDeclaration())
                        for (unsigned a = 0; a < ci->arg_size() && a < callee->arg_size(); a++)
                            if (ci->getArgOperand(a)->getType()->isIntegerTy() &&
                                ci->getArgOperand(a)->getType()->getIntegerBitWidth() < 32 &&
                                ((ci->paramHasAttr(a, Attribute::SExt) && callee->hasParamAttribute(a, Attribute::ZExt)) ||
                                 (ci->paramHasAttr(a, Attribute::ZExt) && callee->hasParamAttribute(a, Attribute::SExt)))) {
                                argExts++;
                                if (getenv("PORT_ARENA_STATS"))
                                    errs() << "port-arena: " << f.getName() << " passes " << callee->getName()
                                           << "'s argument " << a << " extended otherwise\n";
                            }
                    if (!callee || callee->isIntrinsic() || !ci->getType()->isIntegerTy() ||
                        ci->getType()->getIntegerBitWidth() >= 32)
                        continue;
                    bool cs = ci->hasRetAttr(Attribute::SExt), cz = ci->hasRetAttr(Attribute::ZExt);
                    bool fs = callee->hasRetAttribute(Attribute::SExt), fz = callee->hasRetAttribute(Attribute::ZExt);
                    if (!(cs || cz) || !(fs || fz) || cs == fs)
                        continue;
                    std::vector<CastInst *> exts;
                    for (User *u : ci->users())
                        if ((cs && isa<SExtInst>(u)) || (cz && isa<ZExtInst>(u)))
                            exts.push_back(cast<CastInst>(u));
                    for (CastInst *e : exts) {
                        IRBuilder<> b(e);
                        IntegerType *i32 = b.getInt32Ty();
                        Value *w = fs ? b.CreateSExt(ci, i32) : b.CreateZExt(ci, i32);
                        unsigned bits = e->getType()->getIntegerBitWidth();
                        Value *r = bits == 32 ? w : bits < 32 ? b.CreateTrunc(w, e->getType())
                                                   : cs ? b.CreateSExt(w, e->getType()) : b.CreateZExt(w, e->getType());
                        e->replaceAllUsesWith(r);
                        e->eraseFromParent();
                    }
                    AttributeList al = ci->getAttributes()
                                           .removeRetAttribute(*C, cs ? Attribute::SExt : Attribute::ZExt)
                                           .addRetAttribute(*C, fs ? Attribute::SExt : Attribute::ZExt);
                    ci->setAttributes(al);
                    extFixes++;
                }
    }

    /* -port-arena-x86-fptoint: a float to integer conversion of a value out
       of the type's range (or a NaN) is poison to LLVM, and each target
       gives what its instruction does.  x86's cvtt* give 0x80000000 (and
       the i386 build's narrow and unsigned conversions go through them),
       which is what mupen64plus on x86 gave the game when the TAS was made;
       WebAssembly's saturate (a negative float to an unsigned type is 0
       there, -1 on x86).  So the conversions are made to give x86's
       results, as i386 code-generates them, on any target. */
    unsigned fpConversions = 0;

    Value *cvtt(IRBuilder<> &b, Value *f, unsigned bits) {
        Type *ft = f->getType();
        IntegerType *it = b.getIntNTy(bits);
        Value *ok = b.CreateAnd(b.CreateFCmpOGE(f, ConstantFP::get(ft, -std::ldexp(1.0, bits - 1))),
                                b.CreateFCmpOLT(f, ConstantFP::get(ft, std::ldexp(1.0, bits - 1))));
        return b.CreateSelect(ok, b.CreateFPToSI(f, it), ConstantInt::get(it, APInt::getSignedMinValue(bits)));
    }

    void x86FpToInt() {
        std::vector<CastInst *> work;
        for (Function &f : *M)
            for (BasicBlock &bb : f)
                for (Instruction &i : bb)
                    if ((isa<FPToSIInst>(&i) || isa<FPToUIInst>(&i)) && i.getType()->isIntegerTy() &&
                        i.getOperand(0)->getType()->isFloatingPointTy())
                        work.push_back(cast<CastInst>(&i));
        for (CastInst *ci : work) {
            IRBuilder<> b(ci);
            Value *f = ci->getOperand(0);
            unsigned bits = ci->getType()->getIntegerBitWidth();
            Value *r;
            if (bits > 64)
                continue;
            if (bits == 64 && isa<FPToUIInst>(ci)) {
                /* below 2^63 as it is, above it less 2^63 with the top bit set */
                Value *big = b.CreateFCmpOGE(f, ConstantFP::get(f->getType(), std::ldexp(1.0, 63)));
                Value *hi = b.CreateXor(cvtt(b, b.CreateFSub(f, ConstantFP::get(f->getType(), std::ldexp(1.0, 63))), 64),
                                        ConstantInt::get(b.getInt64Ty(), APInt::getSignedMinValue(64)));
                r = b.CreateSelect(big, hi, cvtt(b, f, 64));
            } else if (bits == 64) {
                r = cvtt(b, f, 64);
            } else if (bits == 32 && isa<FPToUIInst>(ci)) {
                /* c | (d & (c >> 31)), d the conversion of f - 2^31 */
                Value *c = cvtt(b, f, 32);
                Value *d = cvtt(b, b.CreateFSub(f, ConstantFP::get(f->getType(), std::ldexp(1.0, 31))), 32);
                r = b.CreateOr(c, b.CreateAnd(d, b.CreateAShr(c, 31)));
            } else {
                r = b.CreateTrunc(cvtt(b, f, 32), ci->getType());
            }
            ci->replaceAllUsesWith(r);
            ci->eraseFromParent();
            fpConversions++;
        }
    }

    /* no more __bepass_fixup: the image has the build's byte order */
    void dropFixups() {
        GlobalVariable *ctors = M->getGlobalVariable("llvm.global_ctors");
        if (!ctors)
            return;
        std::vector<std::pair<Function *, int>> keep;
        std::vector<Function *> drop;
        if (auto *ca = dyn_cast<ConstantArray>(ctors->getInitializer()))
            for (Value *op : ca->operands()) {
                auto *cs = cast<ConstantStruct>(op);
                auto *fn = dyn_cast<Function>(cs->getOperand(1)->stripPointerCasts());
                int pri = (int)cast<ConstantInt>(cs->getOperand(0))->getZExtValue();
                if (fn && fn->getName().starts_with("__bepass_ctor"))
                    drop.push_back(fn);
                else if (fn)
                    keep.push_back({fn, pri});
                else
                    fail("a constructor that isn't a function");
            }
        ctors->eraseFromParent();
        for (auto &k : keep)
            appendToGlobalCtors(*M, k.first, k.second);
        for (Function *fn : drop)
            fn->eraseFromParent();
        std::vector<GlobalVariable *> tables;
        for (GlobalVariable &g : M->globals())
            if (g.getName().starts_with("__bepass_table") && g.use_empty())
                tables.push_back(&g);
        /* what they said to swap: {address, count, size} */
        for (GlobalVariable *t : tables) {
            auto *ca = dyn_cast<ConstantArray>(t->getInitializer());
            if (!ca)
                fail("a __bepass_table that isn't an array");
            for (Value *op : ca->operands()) {
                auto *cs = cast<ConstantStruct>(op);
                GlobalValue *gv;
                int64_t off;
                if (!eval(cs->getOperand(0), gv, off) || !gv || !isa<GlobalVariable>(gv))
                    fail("a __bepass_table entry that isn't in a variable");
                swaps.push_back({cast<GlobalVariable>(gv), (uint64_t)off,
                                 (unsigned)cast<ConstantInt>(cs->getOperand(1))->getZExtValue(),
                                 (unsigned)cast<ConstantInt>(cs->getOperand(2))->getZExtValue()});
            }
        }
        for (GlobalVariable *g : tables)
            g->eraseFromParent();
    }

    /* -port-arena-triple/-datalayout: the N64 side is compiled and
       optimised for i386 as in the 32-bit build (the same IR, so the same
       instruction counts and the same layout as there), then code-generated
       for another target of the same layout (wasm32: 4-byte pointers,
       8-aligned doubles and long longs as -malign-double has them).  The
       i386 function attributes go (the target's CPU and features, the
       stack protector's canary); -mattr gives the new
       target's. */
    void retarget() {
        M->setTargetTriple(Triple(Retarget));
        M->setDataLayout(RetargetLayout);
        for (Function &f : *M) {
            f.removeFnAttr("target-cpu");
            f.removeFnAttr("target-features");
            f.removeFnAttr("tune-cpu");
            f.removeFnAttr(Attribute::StackProtect);
            f.removeFnAttr(Attribute::StackProtectStrong);
            f.removeFnAttr(Attribute::StackProtectReq);
        }
    }

    PreservedAnalyses run(Module &m, ModuleAnalysisManager &) {
        M = &m;
        C = &m.getContext();
        frameDL.emplace(m.getDataLayout());
        if (!Retarget.empty())
            retarget();
        DL = &m.getDataLayout();
        IP = IntegerType::get(*C, DL->getPointerSizeInBits(0));
        readSyms();
        callFix();
        if (X86FpToInt)
            x86FpToInt();
        dropFixups();
        functions();
        layout();
        writeImage();
        if (!Scatter.empty())
            writeBad();
        writeFns();
        PointerType *ptr = PointerType::get(*C, 0);
        base = m.getGlobalVariable("port_arena");
        if (!base)
            base = new GlobalVariable(m, ptr, false, GlobalValue::ExternalLinkage, nullptr, "port_arena");
        for (Function &f : m)
            if (!f.isDeclaration()) {
                function(f);
                indirectCalls(f);
            }
        if (ScatterCheck)
            writeSites();
        replayCallers();
        defineUndef();
        if (getenv("PORT_ARENA_STATS"))
            errs() << "port-arena: " << placed.size() << " variables (" << moved.size() << " moved), data to "
                   << format_hex(extraEnd, 8) << ", "
                   << relocs.size() << " relocations; " << rewritten << " accesses (" << hostArgs
                   << " host arguments), " << escaped << " locals on the locals' stack (" << kept
                   << " that don't escape; " << frames << " frames, the biggest " << frameMax << " bytes), "
                   << undefs << " undefined values made 0; " << fns.size()
                   << " functions as values, " << indirect << " calls through one (" << sigs.size()
                   << " types, " << thunks << " thunks); " << fixedCalls
                   << " calls made with their callee's type (" << voidResults << " void results used, "
                   << unfixable << " arguments lost), " << extFixes << " narrow results extended as the callee does, "
                   << argExts << " narrow arguments extended otherwise than the callee says; " << fpConversions << " float conversions as x86's\n";
        return PreservedAnalyses::none();
    }
};

/* port-wrap, per file (the plugin's pipeline start, BEPASS_WRAP=a,b,...):
   --wrap as GNU ld does it, in a file that calls X without defining it the
   calls go to __wrap_X, and __real_X is X.  The arena link puts the N64
   side into one module, where the linker's --wrap would no longer see the
   calls.  (BEPASS_PORT_SRC=1, for port/src: the port's own data gets
   sections of its own, which the arena link places after RDRAM whatever
   its name, as the other builds leave it to the host linker.) */
struct Wrap : PassInfoMixin<Wrap> {
    static bool isRequired() { return true; }

    PreservedAnalyses run(Module &m, ModuleAnalysisManager &) {
        const char *ps = getenv("BEPASS_PORT_SRC");
        if (ps && *ps == '1')
            for (GlobalVariable &g : m.globals())
                if (g.hasInitializer() && !g.hasSection() && !g.getName().starts_with("llvm."))
                    g.setSection((g.getInitializer()->isNullValue() ? ".bss.port." : ".data.port.") +
                                 g.getName().str());
        const char *list = getenv("BEPASS_WRAP");
        if (!list || !*list)
            return PreservedAnalyses::all();
        SmallVector<StringRef, 8> names;
        StringRef(list).split(names, ',', -1, false);
        for (StringRef n : names) {
            Function *f = m.getFunction(n);
            if (f && f->isDeclaration()) {
                std::string w = ("__wrap_" + n).str();
                if (Function *wf = m.getFunction(w)) {
                    f->replaceAllUsesWith(wf);
                    f->eraseFromParent();
                } else {
                    f->setName(w);
                }
            }
            if (Function *r = m.getFunction(("__real_" + n).str())) {
                if (Function *real = m.getFunction(n)) {
                    r->replaceAllUsesWith(real);
                    r->eraseFromParent();
                } else {
                    r->setName(n);
                }
            }
        }
        return PreservedAnalyses::none();
    }
};

} // namespace

void portRegisterArena(PassBuilder &pb) {
    pb.registerPipelineStartEPCallback([](ModulePassManager &mpm, OptimizationLevel) { mpm.addPass(Wrap()); });
    pb.registerPipelineParsingCallback([](StringRef name, ModulePassManager &mpm,
                                          ArrayRef<PassBuilder::PipelineElement>) {
        if (name != "port-arena")
            return false;
        mpm.addPass(Arena());
        return true;
    });
}
