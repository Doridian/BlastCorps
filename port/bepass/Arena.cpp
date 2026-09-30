/*
 * port-arena: the movable build's arena link (PORT_MOVABLE, docs/PORT.md
 * "Movable memory").  `opt -passes=port-arena` runs it over the whole N64
 * side (the game's C, port/src and the asm data, asm2ll.py) as one module.
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
 *   - maps every access (loads, stores, atomics, memory intrinsics, and the
 *     pointer arguments of calls into the host, which dereferences them) to
 *     port_arena + (p & 0x1FFFFFFF);
 *   - gives a local whose address escapes (to a call, into memory, into an
 *     integer) its N64 address, which it has on a fiber's stack; one that
 *     doesn't escape is accessed where it is.
 *
 * Also here: port-wrap (below), per file.
 */
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
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
#include <cstdlib>
#include <map>
#include <set>
#include <vector>

using namespace llvm;

static cl::opt<std::string> SymsFile("port-arena-syms", cl::desc("gen_syms.py table: name address kind"));
static cl::opt<std::string> HostNames("port-arena-host", cl::desc("N64-side variables the host names (comma list)"));
static cl::opt<bool> Native("port-arena-native", cl::desc("the native-endian build's byte order"));

namespace {

/* the arena, by N64 physical address (port_arena.h) */
static const uint64_t K0 = 0x80000000u, MASK = 0x1FFFFFFFu;
static const uint64_t RDRAM = 0x00400000u;
static const uint64_t STACKS = 0x00C00000u, STACKS_SPAN = 0x01000000u;

struct Arena : PassInfoMixin<Arena> {
    static bool isRequired() { return true; }

    Module *M = nullptr;
    LLVMContext *C = nullptr;
    const DataLayout *DL = nullptr;
    IntegerType *IP = nullptr;          /* the pointer-sized integer */
    GlobalVariable *base = nullptr;     /* port_arena */
    unsigned rewritten = 0, hostArgs = 0, escaped = 0;

    [[noreturn]] void fail(const Twine &what) { report_fatal_error("port-arena: " + what); }

    /* ---- names ------------------------------------------------------- */

    std::map<std::string, std::pair<uint64_t, char>> syms;

    void readSyms() {
        if (SymsFile.empty())
            fail("no -port-arena-syms");
        auto buf = MemoryBuffer::getFile(SymsFile);
        if (!buf)
            fail("can't read " + SymsFile);
        SmallVector<StringRef, 0> lines;
        (*buf)->getBuffer().split(lines, '\n', -1, false);
        for (StringRef l : lines) {
            SmallVector<StringRef, 3> p;
            l.split(p, ' ', -1, false);
            if (p.size() != 3)
                continue;
            uint64_t a;
            if (p[1].getAsInteger(16, a))
                continue;
            syms[p[0].str()] = {a, p[2][0]};
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
            if (!portSrc && it != syms.end() && it->second.first - K0 < RDRAM)
                placed.push_back({g, it->second.first & MASK});
            else
                extra.push_back(g);
        }
        for (GlobalVariable *g : extra) {
            uint64_t align = std::max<uint64_t>(g->getAlign() ? g->getAlign()->value() : 1,
                                                DL->getABITypeAlign(g->getValueType()).value());
            extraEnd = (extraEnd + align - 1) / align * align;
            placed.push_back({g, extraEnd});
            extraEnd += DL->getTypeAllocSize(g->getValueType());
        }
        extraEnd = (extraEnd + 15) & ~15ull;
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
            g->replaceAllUsesWith(addrConst(syms[g->getName().str()].first, g->getType()));
            g->eraseFromParent();
        }
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
       their 64-bit ones).  The asm data's symbolic words are in memory
       order already (bigEndianWords). */
    bool bigEndianWords = false;
    void putInt(uint64_t off, uint64_t v, unsigned size) {
        if (off + size > image.size())
            fail("an initializer outside the arena image");
        for (unsigned k = 0; k < size; k++)
            image[off + k] = (uint8_t)(v >> (8 * (bigEndianWords ? size - 1 - k : k)));
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
            bigEndianWords = !Native && p.g->hasSection() && p.g->getSection() == "port.asmdata";
            if (p.g->hasInitializer())
                put(p.addr, p.g->getInitializer());
        }
        bigEndianWords = false;
        applySwaps(at);
        for (auto &p : placed) {
            if (!p.g->use_empty())
                fail(p.g->getName() + " is still used");
            p.g->eraseFromParent();
        }
        /* runs of non-zero bytes (zero runs of 64 or more between them) */
        Type *i32 = Type::getInt32Ty(*C), *i64 = Type::getInt64Ty(*C);
        PointerType *ptr = PointerType::get(*C, 0);
        StructType *runT = StructType::get(*C, {i32, i32, ptr});
        std::vector<Constant *> runs;
        size_t k = 0, n = image.size();
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

    /* an escaping local gets its N64 address; not being on a fiber's stack
       in the arena is fatal (port_arena_bad_local) */
    void locals(Function &f, Value *arena) {
        if (f.isVarArg())       /* its va_list is the host's */
            return;
        std::vector<Value *> as;
        for (BasicBlock &bb : f)
            for (Instruction &i : bb)
                if (auto *a = dyn_cast<AllocaInst>(&i))
                    if (escapes(a))
                        as.push_back(a);
        /* (a struct passed by value is a local too, the call's copy) */
        for (Argument &a : f.args())
            if (a.hasByValAttr() && escapes(&a))
                as.push_back(&a);
        if (as.empty())
            return;
        FunctionCallee bad = M->getOrInsertFunction("port_arena_bad_local",
                                                    FunctionType::get(Type::getVoidTy(*C), {IP}, false));
        for (Value *a : as) {
            Instruction *at = isa<Argument>(a) ? cast<Instruction>(arena) : cast<Instruction>(a)->getNextNode();
            while (isa<AllocaInst>(at) || at == arena)
                at = at->getNextNode();
            IRBuilder<> b(at);
            Value *h = b.CreatePtrToInt(a, IP);
            Value *off = b.CreateSub(h, b.CreatePtrToInt(arena, IP));
            Value *out = b.CreateICmpUGE(b.CreateSub(off, ConstantInt::get(IP, STACKS)), ConstantInt::get(IP, STACKS_SPAN));
            Instruction *then = SplitBlockAndInsertIfThen(out, at, false);
            IRBuilder<> tb(then);
            tb.CreateCall(bad, {h});
            IRBuilder<> nb(at);
            Value *np = nb.CreateIntToPtr(nb.CreateOr(off, ConstantInt::get(IP, K0)), a->getType(), a->getName() + ".n64");
            a->replaceUsesWithIf(np, [&](Use &u) {
                auto *ii = dyn_cast<IntrinsicInst>(u.getUser());
                if (ii && (ii->isLifetimeStartOrEnd() || isa<DbgInfoIntrinsic>(ii)))
                    return false;
                return u.getUser() != h;
            });
            escaped++;
        }
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
            o.first->setOperand(o.second, map(b, p, arena));
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

    PreservedAnalyses run(Module &m, ModuleAnalysisManager &) {
        M = &m;
        C = &m.getContext();
        DL = &m.getDataLayout();
        IP = IntegerType::get(*C, DL->getPointerSizeInBits(0));
        readSyms();
        dropFixups();
        layout();
        writeImage();
        PointerType *ptr = PointerType::get(*C, 0);
        base = m.getGlobalVariable("port_arena");
        if (!base)
            base = new GlobalVariable(m, ptr, false, GlobalValue::ExternalLinkage, nullptr, "port_arena");
        for (Function &f : m)
            if (!f.isDeclaration())
                function(f);
        if (getenv("PORT_ARENA_STATS"))
            errs() << "port-arena: " << placed.size() << " variables, data to " << format_hex(extraEnd, 8) << ", "
                   << relocs.size() << " relocations; " << rewritten << " accesses (" << hostArgs
                   << " host arguments), " << escaped << " escaping locals\n";
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
