/*
 * BEPass: make a little-endian host keep memory in big-endian (N64) order.
 *
 * Every load and store of a multi-byte scalar (integers, floats, pointers)
 * in the module gets a byte swap, so memory holds exactly the bytes the N64
 * would: data loaded from the ROM, the handwritten engine's view of memory
 * (tools/recomp, which reads RDRAM as big-endian) and the decompiled C all
 * agree without knowing each other's types.  Byte accesses and memcpy are
 * unchanged.  See docs/PORT.md, "Memory model".
 *
 * Initialized globals are emitted by the compiler in host order; the pass
 * records every multi-byte scalar inside their initializers in a table, and
 * a constructor swaps them in place at startup (__bepass_fixup, in the port
 * runtime).  Pointer-valued initializers can only be swapped at run time,
 * which is why this is done for all of them.  Constant globals are made
 * writable for that.
 *
 * Functions that call llvm.va_start are left alone: the va_list and the
 * argument area are host memory written by the caller's native call.
 *
 * It also puts a call to __port_poll() on every loop back edge.  The game
 * busy-waits on counters other threads or interrupts advance (on the N64
 * something preempts it); the port runs its threads one at a time, and
 * the poll is where it lets time pass.  Being an opaque call, it also
 * makes such a loop reload what it waits on, as IDO's code did.  Not
 * with BEPASS_NOPOLL=1 (libaudio, which never waits).
 *
 * BEPASS_NATIVE=1 when compiling (the native-endian build, PORT_NATIVE_ENDIAN;
 * docs/PORT.md "Native-endian memory"): memory is in host order, so nothing
 * is swapped but the 64-bit scalars (u64, s64, double), whose two 32-bit
 * halves are exchanged: in memory they are two host-order words, the high
 * one first, as the translated asm's ld/sd/ldc1/sdc1 and every piece of code
 * that reads one half of a u64 as a word (the game mode D_80364A90) have
 * them.  Only their initializers need fixing then (__bepass_fixup_rot64).
 * The polls are the same in both modes.
 *
 * The pass runs at the start of the pipeline, before anything can combine
 * or reorder accesses; later passes see explicit bswaps and optimise them
 * (a swapped store to a local followed by a swapped load folds away).
 */
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

#include <cstdlib>
#include <vector>

using namespace llvm;

namespace {

struct AccessHooks {
    /* BEPASS_ACCESS=1 when compiling: every load and store of memory the
       game's C does (as the source has it, before optimisation can merge
       or split them; locals left out) calls __port_access(address,
       size | store << 8 | big-endian << 9, function, the bytes as an
       integer),
       and memcpy/memset calls __port_access_copy/_set: the access-width
       profiler (port/host/access.c, docs/PORT.md "Native-endian memory"). */
    static void addAccessHooks(Module &m) {
        LLVMContext &c = m.getContext();
        Type *i32 = Type::getInt32Ty(c), *vd = Type::getVoidTy(c);
        PointerType *ptr = PointerType::getUnqual(c);
        Type *i64 = Type::getInt64Ty(c);
        /* the bytes as two i32s: a 64-bit build would otherwise materialize
           a 32-bit value like OS_K0_TO_PHYSICAL(&var) as a RIP-relative
           lea of var - 0x80000000, which can't reach */
        FunctionCallee acc = m.getOrInsertFunction("__port_access", FunctionType::get(vd, {ptr, i32, i32, i32, i32}, false));
        FunctionCallee cpy = m.getOrInsertFunction("__port_access_copy",
                                                   FunctionType::get(vd, {ptr, ptr, i32, i32}, false));
        FunctionCallee set = m.getOrInsertFunction("__port_access_set", FunctionType::get(vd, {ptr, i32, i32}, false));
        const DataLayout &dl = m.getDataLayout();
        for (Function &f : m) {
            if (f.isDeclaration())
                continue;
            uint32_t h = 2166136261u;
            for (char ch : f.getName())
                h = (h ^ (uint8_t)ch) * 16777619u;
            std::vector<Instruction *> work;
            for (BasicBlock &bb : f)
                for (Instruction &i : bb)
                    if (isa<LoadInst>(i) || isa<StoreInst>(i) || isa<MemIntrinsic>(i))
                        work.push_back(&i);
            for (Instruction *i : work) {
                IRBuilder<> b(i);
                Value *site = b.getInt32(h);
                if (auto *mi = dyn_cast<MemIntrinsic>(i)) {
                    Value *n = b.CreateZExtOrTrunc(mi->getLength(), i32);
                    if (auto *mt = dyn_cast<MemTransferInst>(mi))
                        b.CreateCall(cpy, {mt->getRawDest(), mt->getRawSource(), n, site});
                    else
                        b.CreateCall(set, {mi->getRawDest(), n, site});
                    continue;
                }
                Value *p = getLoadStorePointerOperand(i);
                const Value *base = getUnderlyingObject(p);
                if (isa<AllocaInst>(base))
                    continue;
                if (auto *g = dyn_cast<GlobalVariable>(base))
                    if (g->getName().starts_with("__port_"))
                        continue;
                Value *v = isa<LoadInst>(i) ? i : cast<StoreInst>(i)->getValueOperand();
                Type *t = v->getType();
                unsigned size = dl.getTypeStoreSize(t);
                if (size > 8 || t->isVectorTy() || t->isAggregateType())
                    continue;
                /* a load only swapped, or a store of a swapped value: data
                   the source keeps in the N64's byte order (0x200: bytes) */
                auto isBswap = [](const Value *x) {
                    auto *ii = dyn_cast<IntrinsicInst>(x);
                    return ii && ii->getIntrinsicID() == Intrinsic::bswap;
                };
                bool be = isa<StoreInst>(i) ? isBswap(v) : (v->hasOneUse() && isBswap(*v->user_begin()));
                if (isa<LoadInst>(i))
                    b.SetInsertPoint(i->getNextNode());
                Value *raw = v;
                if (t->isPointerTy())
                    raw = b.CreatePtrToInt(raw, b.getIntNTy(size * 8));
                else if (!t->isIntegerTy())
                    raw = b.CreateBitCast(raw, b.getIntNTy(size * 8));
                Value *lo, *hi;
                if (size > 4) {
                    raw = b.CreateZExtOrTrunc(raw, i64);
                    lo = b.CreateTrunc(raw, i32);
                    hi = b.CreateTrunc(b.CreateLShr(raw, 32), i32);
                } else {
                    lo = b.CreateZExtOrTrunc(raw, i32);
                    hi = b.getInt32(0);
                }
                b.CreateCall(acc, {p, b.getInt32(size | (isa<StoreInst>(i) ? 0x100 : 0) | (be ? 0x200 : 0)), site,
                                   lo, hi});
            }
        }
    }
};

struct BEPass : PassInfoMixin<BEPass> {
    static bool isRequired() { return true; }

    /* BEPASS_NATIVE=1: host-order memory, 64-bit scalars as two words */
    static bool native;

    /* integer type of the same size, or null if nothing to swap */
    static IntegerType *swapType(Type *t, const DataLayout &dl, LLVMContext &c) {
        IntegerType *it = nullptr;
        if (t->isIntegerTy()) {
            unsigned bits = t->getIntegerBitWidth();
            if (bits <= 8)
                return nullptr;
            it = cast<IntegerType>(t);
        } else if (t->isFloatTy() || t->isDoubleTy() || t->isPointerTy() || t->isHalfTy())
            it = IntegerType::get(c, dl.getTypeSizeInBits(t));
        /* native: only the 64-bit scalars, not an LP64 build's pointers,
           whose low word first is what a 32-bit reader of them sees */
        if (it && native && (it->getBitWidth() != 64 || t->isPointerTy()))
            return nullptr;
        return it;
    }

    static Value *bswap(IRBuilder<> &b, Value *v) {
        IntegerType *t = cast<IntegerType>(v->getType());
        unsigned bits = t->getBitWidth();
        if (native) {       /* exchange the words: its own inverse */
            if (bits != 64)
                report_fatal_error("BEPass: native mode swaps only 64-bit scalars");
            return b.CreateIntrinsic(Intrinsic::fshl, {t}, {v, v, b.getInt64(32)});
        }
        if (bits % 8)
            report_fatal_error("BEPass: swap of a non-byte-sized integer");
        unsigned wide = (bits + 15) / 16 * 16;
        if (wide == bits)
            return b.CreateUnaryIntrinsic(Intrinsic::bswap, v);
        IntegerType *wt = IntegerType::get(t->getContext(), wide);
        Value *z = b.CreateZExt(v, wt);
        Value *s = b.CreateUnaryIntrinsic(Intrinsic::bswap, z);
        s = b.CreateLShr(s, wide - bits);
        return b.CreateTrunc(s, t);
    }

    static Value *toInt(IRBuilder<> &b, Value *v, IntegerType *it) {
        Type *t = v->getType();
        if (t == it)
            return v;
        if (t->isPointerTy())
            return b.CreatePtrToInt(v, it);
        return b.CreateBitCast(v, it);
    }

    static Value *fromInt(IRBuilder<> &b, Value *v, Type *t) {
        if (v->getType() == t)
            return v;
        if (t->isPointerTy())
            return b.CreateIntToPtr(v, t);
        return b.CreateBitCast(v, t);
    }

    static void fail(Instruction *i, const char *what) {
        std::string s;
        raw_string_ostream os(s);
        os << "BEPass: " << what << " in " << i->getFunction()->getName() << ": " << *i;
        report_fatal_error(StringRef(os.str()));
    }

    bool runOnFunction(Function &f, const DataLayout &dl) {
        for (BasicBlock &bb : f)
            for (Instruction &i : bb)
                if (auto *ii = dyn_cast<IntrinsicInst>(&i))
                    if (ii->getIntrinsicID() == Intrinsic::vastart)
                        return false;
        std::vector<Instruction *> work;
        for (BasicBlock &bb : f)
            for (Instruction &i : bb)
                if (isa<LoadInst>(i) || isa<StoreInst>(i) || isa<AtomicRMWInst>(i) ||
                    isa<AtomicCmpXchgInst>(i) || isa<VAArgInst>(i))
                    work.push_back(&i);
        LLVMContext &c = f.getContext();
        bool changed = false;
        for (Instruction *i : work) {
            /* the port's own variables (__port_*) are host data */
            if (Value *p = getLoadStorePointerOperand(i))
                if (auto *g = dyn_cast<GlobalVariable>(getUnderlyingObject(p)))
                    if (g->getName().starts_with("__port_"))
                        continue;
            if (auto *ld = dyn_cast<LoadInst>(i)) {
                Type *t = ld->getType();
                if (t->isVectorTy() || t->isStructTy() || t->isArrayTy())
                    fail(i, "aggregate or vector load");
                IntegerType *it = swapType(t, dl, c);
                if (!it)
                    continue;
                IRBuilder<> b(ld);
                LoadInst *nl = b.CreateAlignedLoad(it, ld->getPointerOperand(), ld->getAlign(),
                                                   ld->isVolatile());
                nl->setOrdering(ld->getOrdering());
                nl->setSyncScopeID(ld->getSyncScopeID());
                Value *v = fromInt(b, bswap(b, nl), t);
                ld->replaceAllUsesWith(v);
                ld->eraseFromParent();
                changed = true;
            } else if (auto *st = dyn_cast<StoreInst>(i)) {
                Type *t = st->getValueOperand()->getType();
                if (t->isVectorTy() || t->isStructTy() || t->isArrayTy())
                    fail(i, "aggregate or vector store");
                IntegerType *it = swapType(t, dl, c);
                if (!it)
                    continue;
                IRBuilder<> b(st);
                Value *v = bswap(b, toInt(b, st->getValueOperand(), it));
                StoreInst *ns = b.CreateAlignedStore(v, st->getPointerOperand(), st->getAlign(),
                                                     st->isVolatile());
                ns->setOrdering(st->getOrdering());
                ns->setSyncScopeID(st->getSyncScopeID());
                st->eraseFromParent();
                changed = true;
            } else if (isa<VAArgInst>(i)) {
                fail(i, "va_arg outside a va_start function");
            } else {
                Type *t = isa<AtomicRMWInst>(i) ? cast<AtomicRMWInst>(i)->getValOperand()->getType()
                                                : cast<AtomicCmpXchgInst>(i)->getCompareOperand()->getType();
                if (swapType(t, dl, c))
                    fail(i, "atomic access");
            }
        }
        return changed;
    }

    /* (offset, size) of each scalar inside an initializer that `mode` wants:
       0 every multi-byte one (big-endian memory's swaps), 1 the 64-bit ones
       (native memory's word exchange), 2 all, bytes too (the profiler's
       map of the C's initialized data) */
    static bool wanted(unsigned sz, int mode) {
        return mode == 2 ? sz >= 1 : mode == 1 ? sz == 8 : sz > 1;
    }

    static void collect(Constant *k, uint64_t off, const DataLayout &dl,
                        std::vector<std::pair<uint64_t, unsigned>> &out, int mode) {
        Type *t = k->getType();
        if (isa<ConstantAggregateZero>(k) || isa<UndefValue>(k))
            return;
        if (auto *cds = dyn_cast<ConstantDataSequential>(k)) {
            Type *et = cds->getElementType();
            unsigned es = dl.getTypeAllocSize(et);
            unsigned sz = dl.getTypeStoreSize(et);
            if (!wanted(sz, mode))
                return;
            for (unsigned n = 0; n < cds->getNumElements(); n++)
                out.push_back({off + (uint64_t)n * es, sz});
            return;
        }
        if (auto *ca = dyn_cast<ConstantArray>(k)) {
            uint64_t es = dl.getTypeAllocSize(ca->getType()->getElementType());
            for (unsigned n = 0; n < ca->getNumOperands(); n++)
                collect(ca->getOperand(n), off + n * es, dl, out, mode);
            return;
        }
        if (auto *cs = dyn_cast<ConstantStruct>(k)) {
            const StructLayout *sl = dl.getStructLayout(cs->getType());
            for (unsigned n = 0; n < cs->getNumOperands(); n++)
                collect(cs->getOperand(n), off + sl->getElementOffset(n), dl, out, mode);
            return;
        }
        if (auto *cv = dyn_cast<ConstantVector>(k)) {
            (void)cv;
            report_fatal_error("BEPass: vector initializer");
        }
        if (t->isIntegerTy() || t->isFloatingPointTy() || t->isPointerTy()) {
            unsigned sz = dl.getTypeStoreSize(t);
            if (mode == 1 && t->isPointerTy())
                return;         /* an LP64 pointer isn't two words (swapType) */
            if (wanted(sz, mode))
                out.push_back({off, sz});
            return;
        }
        report_fatal_error("BEPass: unhandled initializer");
    }

    /* runs of equally sized, contiguous scalars: {address, count, size} */
    static void addRuns(const std::vector<std::pair<uint64_t, unsigned>> &items, GlobalVariable *g,
                        LLVMContext &c, StructType *ent, std::vector<Constant *> &table) {
        Type *i32 = Type::getInt32Ty(c);
        Type *i8 = Type::getInt8Ty(c);
        size_t n = 0;
        while (n < items.size()) {
            size_t e = n + 1;
            while (e < items.size() && items[e].second == items[n].second &&
                   items[e].first == items[e - 1].first + items[n].second)
                e++;
            Constant *addr = ConstantExpr::getGetElementPtr(
                i8, g, ConstantInt::get(Type::getInt64Ty(c), items[n].first));
            table.push_back(ConstantStruct::get(
                ent, {addr, ConstantInt::get(i32, e - n), ConstantInt::get(i32, items[n].second)}));
            n = e;
        }
    }

    void fixGlobals(Module &m, const DataLayout &dl, bool widths) {
        LLVMContext &c = m.getContext();
        Type *i32 = Type::getInt32Ty(c);
        PointerType *ptr = PointerType::getUnqual(c);
        StructType *ent = StructType::get(c, {ptr, i32, i32});
        std::vector<Constant *> table, wtable;
        std::vector<GlobalVariable *> gvs;
        for (GlobalVariable &g : m.globals())
            gvs.push_back(&g);
        for (GlobalVariable *g : gvs) {
            if (g->getName().starts_with("llvm."))
                continue;
            /* nor on a declaration: x86-64 clang gives an extern array of
               16 bytes or more 16-byte alignment too, and the optimiser
               then drops the low bits of addresses computed from it (the
               LP64 build's `&D_802F49F4[i]`, an array at ...944) */
            if (!g->hasInitializer()) {
                Align natural = g->getValueType()->isSized() ? dl.getABITypeAlign(g->getValueType()) : Align(1);
                if (g->getAlign().value_or(Align(1)) > natural)
                    g->setAlignment(natural);
                continue;
            }
            /* no over-alignment (x86 wants arrays 16-aligned): the port
               places the game's variables at their N64 addresses */
            if (!g->getMetadata("port.align"))      /* port-ilp32 did it */
                g->setAlignment(dl.getABITypeAlign(g->getValueType()));
            /* what it is now, which KeepAlign holds it to after the
               optimiser */
            g->setMetadata("port.align.set",
                           MDNode::get(c, {ConstantAsMetadata::get(ConstantInt::get(
                                              i32, g->getAlign().value_or(Align(1)).value()))}));
            if (g->hasSection() && g->getSection().starts_with("llvm."))
                continue;
            std::vector<std::pair<uint64_t, unsigned>> items;
            if (widths) {
                std::vector<std::pair<uint64_t, unsigned>> all;
                collect(g->getInitializer(), 0, dl, all, 2);
                addRuns(all, g, c, ent, wtable);
            }
            collect(g->getInitializer(), 0, dl, items, 0);
            if (items.empty())
                continue;
            g->setConstant(false);          /* its multi-byte scalars are read from memory */
            if (native) {
                items.clear();
                collect(g->getInitializer(), 0, dl, items, 1);
            }
            addRuns(items, g, c, ent, table);
        }
        if (widths && !wtable.empty()) {
            /* PORT_ACCESS_PROFILE (BEPASS_ACCESS=1), native memory: every
               scalar of the C's initialized data by width, which the
               profiler seeds its map of RDRAM with */
            ArrayType *at = ArrayType::get(ent, wtable.size());
            auto *wg = new GlobalVariable(m, at, true, GlobalValue::PrivateLinkage,
                                          ConstantArray::get(at, wtable), "__bepass_widths");
            wg->setSection("port_cwidths");
            appendToCompilerUsed(m, {wg});
        }
        if (table.empty())
            return;
        ArrayType *at = ArrayType::get(ent, table.size());
        auto *tg = new GlobalVariable(m, at, true, GlobalValue::PrivateLinkage,
                                      ConstantArray::get(at, table), "__bepass_table");
        FunctionType *fixty = FunctionType::get(Type::getVoidTy(c), {ptr, i32}, false);
        FunctionCallee fix = m.getOrInsertFunction(native ? "__bepass_fixup_rot64" : "__bepass_fixup", fixty);
        Function *ctor = Function::Create(FunctionType::get(Type::getVoidTy(c), false),
                                          GlobalValue::InternalLinkage, "__bepass_ctor", &m);
        IRBuilder<> b(BasicBlock::Create(c, "entry", ctor));
        b.CreateCall(fix, {tg, ConstantInt::get(i32, table.size())});
        b.CreateRetVoid();
        appendToGlobalCtors(m, ctor, 101);
    }

    static void addPolls(Function &f, FunctionCallee poll) {
        /* BEPASS_NOPOLL=1 (CMakeLists.txt's NOPOLL_C; BEPASS_ENGINE=1 implies
           it): code that never waits on another thread needs no polls, and
           without them its loops' shape doesn't move --deterministic's
           clock (every 64th poll advances it), so a replacement needn't
           have the original's (docs/PORT.md, "Timing") */
        const char *np = getenv("BEPASS_NOPOLL");
        if (np && *np == '1')
            return;
        DominatorTree dt(f);
        std::vector<Instruction *> at;
        for (BasicBlock &bb : f) {
            Instruction *t = bb.getTerminator();
            if (!t)
                continue;
            for (BasicBlock *succ : successors(&bb))
                if (dt.dominates(succ, &bb)) {
                    at.push_back(t);
                    break;
                }
        }
        for (Instruction *t : at) {
            IRBuilder<> b(t);
            b.CreateCall(poll);
        }
    }

    /* BEPASS_TRACE=1 when compiling: every function calls
       __port_trace(hash of its name) on entry, for comparing two builds
       call by call (port/host/runtime.c, PORT_TRACE) */
    static void addTrace(Function &f, FunctionCallee trace) {
        uint32_t h = 2166136261u;
        for (char ch : f.getName())
            h = (h ^ (uint8_t)ch) * 16777619u;
        IRBuilder<> b(&*f.getEntryBlock().getFirstInsertionPt());
        b.CreateCall(trace, {b.getInt32(h)});
    }

    PreservedAnalyses run(Module &m, ModuleAnalysisManager &) {
        const DataLayout &dl = m.getDataLayout();
        const char *nat = getenv("BEPASS_NATIVE");
        native = nat && *nat == '1';
        const char *tr = getenv("BEPASS_TRACE");
        FunctionCallee trace;
        if (tr && *tr == '1')
            trace = m.getOrInsertFunction("__port_trace", FunctionType::get(Type::getVoidTy(m.getContext()),
                                                                            {Type::getInt32Ty(m.getContext())}, false));
        FunctionCallee poll = m.getOrInsertFunction(
            "__port_poll", FunctionType::get(Type::getVoidTy(m.getContext()), false));
        /* (not in the engine's replacement, BEPASS_ENGINE=1: the translated
           code it stands in for never polls) */
        const char *eng = getenv("BEPASS_ENGINE");
        bool polls = !(eng && *eng == '1');
        for (Function &f : m)
            if (!f.isDeclaration()) {
                runOnFunction(f, dl);
                if (polls)
                    addPolls(f, poll);
                if (trace)
                    addTrace(f, trace);
            }
        const char *acc = getenv("BEPASS_ACCESS");
        bool access = acc && *acc == '1';
        fixGlobals(m, dl, access && native);
        if (access)
            AccessHooks::addAccessHooks(m);
        return PreservedAnalyses::none();
    }
};

bool BEPass::native = false;

/* The optimiser may raise a global's alignment to suit the code it made
   (clang 23's InferAlignment, after the SLP vectoriser read a float[4][4]
   as <8 x i32>: 23C20.c's D_8036B8C8 went to 32), but the port places the
   game's variables at their N64 addresses (gen_ld.py), which the raised
   alignment doesn't fit (the link fails) or the code's aligned accesses
   would fault on.  So each global goes back to what BEPass gave it
   (port.align.set), and in a function that uses one that was raised, no
   access claims more than 8 bytes' alignment (none of x86's instructions
   that need more is chosen then). */
struct KeepAlign : PassInfoMixin<KeepAlign> {
    static bool isRequired() { return true; }

    PreservedAnalyses run(Module &m, ModuleAnalysisManager &) {
        std::vector<GlobalVariable *> raised;
        for (GlobalVariable &g : m.globals()) {
            MDNode *md = g.getMetadata("port.align.set");
            if (!md)
                continue;
            uint64_t set = mdconst::extract<ConstantInt>(md->getOperand(0))->getZExtValue();
            if (g.getAlign() && g.getAlign()->value() > set) {
                g.setAlignment(Align(set));
                raised.push_back(&g);
            }
        }
        if (raised.empty())
            return PreservedAnalyses::all();
        const Align most(8);
        for (Function &f : m) {
            bool uses = false;
            for (GlobalVariable *g : raised)
                for (User *u : g->users()) {
                    Instruction *i = dyn_cast<Instruction>(u);
                    if (auto *ce = dyn_cast<ConstantExpr>(u))
                        for (User *cu : ce->users())
                            if (auto *ci = dyn_cast<Instruction>(cu))
                                uses |= ci->getFunction() == &f;
                    uses |= i && i->getFunction() == &f;
                }
            if (!uses)
                continue;
            for (BasicBlock &bb : f)
                for (Instruction &i : bb) {
                    if (auto *ld = dyn_cast<LoadInst>(&i)) {
                        if (ld->getAlign() > most)
                            ld->setAlignment(most);
                    } else if (auto *st = dyn_cast<StoreInst>(&i)) {
                        if (st->getAlign() > most)
                            st->setAlignment(most);
                    } else if (auto *mi = dyn_cast<MemIntrinsic>(&i)) {
                        if (mi->getDestAlign() && *mi->getDestAlign() > most)
                            mi->setDestAlignment(most);
                        if (auto *mt = dyn_cast<MemTransferInst>(mi))
                            if (mt->getSourceAlign() && *mt->getSourceAlign() > most)
                                mt->setSourceAlignment(most);
                    }
                }
        }
        return PreservedAnalyses::none();
    }
};

} // namespace

/* ILP32.cpp: port-ilp32, for opt (the 64-bit build); LP64.cpp: port-lp64,
   ahead of BEPass (the LP64 build, BEPASS_LP64=1) */
void portRegisterILP32(PassBuilder &pb);
void portRegisterLP64(PassBuilder &pb);
void portRegisterArena(PassBuilder &pb);

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "BEPass", "1", [](PassBuilder &pb) {
                portRegisterILP32(pb);
                portRegisterLP64(pb);
                pb.registerPipelineStartEPCallback(
                    [](ModulePassManager &mpm, OptimizationLevel) { mpm.addPass(BEPass()); });
                pb.registerOptimizerLastEPCallback(
                    [](ModulePassManager &mpm, OptimizationLevel, ThinOrFullLTOPhase) {
                        mpm.addPass(KeepAlign());
                    });
                portRegisterArena(pb);      /* after KeepAlign: BEPASS_ARENA=1 */
            }};
}
