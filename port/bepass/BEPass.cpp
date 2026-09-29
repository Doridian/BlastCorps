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
 * makes such a loop reload what it waits on, as IDO's code did.
 *
 * The pass runs at the start of the pipeline, before anything can combine
 * or reorder accesses; later passes see explicit bswaps and optimise them
 * (a swapped store to a local followed by a swapped load folds away).
 *
 * A second pass (ICount), at the end of the optimisation pipeline, adds each
 * basic block's size to the global counter __port_icount_c, as a stand-in for
 * the MIPS instructions the N64 would execute there: the port charges the
 * CPU's time from it (docs/PORT.md, "Timing").  The size is the optimised
 * IR's, without what is free or doesn't exist on the N64 (phis, casts,
 * constant address arithmetic, the byte swaps, debug intrinsics).
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
       size | store << 8, function, the bytes as an integer),
       and memcpy/memset calls __port_access_copy/_set: the access-width
       profiler (port/host/access.c, docs/PORT.md "Native-endian memory"). */
    static void addAccessHooks(Module &m) {
        LLVMContext &c = m.getContext();
        Type *i32 = Type::getInt32Ty(c), *vd = Type::getVoidTy(c);
        PointerType *ptr = PointerType::getUnqual(c);
        Type *i64 = Type::getInt64Ty(c);
        FunctionCallee acc = m.getOrInsertFunction("__port_access", FunctionType::get(vd, {ptr, i32, i32, i64}, false));
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
                if (isa<LoadInst>(i))
                    b.SetInsertPoint(i->getNextNode());
                Value *raw = v;
                if (t->isPointerTy())
                    raw = b.CreatePtrToInt(raw, b.getIntNTy(size * 8));
                else if (!t->isIntegerTy())
                    raw = b.CreateBitCast(raw, b.getIntNTy(size * 8));
                raw = b.CreateZExtOrTrunc(raw, i64);
                b.CreateCall(acc, {p, b.getInt32(size | (isa<StoreInst>(i) ? 0x100 : 0)), site, raw});
            }
        }
    }
};

struct BEPass : PassInfoMixin<BEPass> {
    static bool isRequired() { return true; }

    /* integer type of the same size, or null if nothing to swap */
    static IntegerType *swapType(Type *t, const DataLayout &dl, LLVMContext &c) {
        if (t->isIntegerTy()) {
            unsigned bits = t->getIntegerBitWidth();
            if (bits <= 8)
                return nullptr;
            return cast<IntegerType>(t);
        }
        if (t->isFloatTy() || t->isDoubleTy() || t->isPointerTy() || t->isHalfTy())
            return IntegerType::get(c, dl.getTypeSizeInBits(t));
        return nullptr;
    }

    static Value *bswap(IRBuilder<> &b, Value *v) {
        IntegerType *t = cast<IntegerType>(v->getType());
        unsigned bits = t->getBitWidth();
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

    /* (offset, size) of each multi-byte scalar inside an initializer */
    static void collect(Constant *k, uint64_t off, const DataLayout &dl,
                        std::vector<std::pair<uint64_t, unsigned>> &out) {
        Type *t = k->getType();
        if (isa<ConstantAggregateZero>(k) || isa<UndefValue>(k))
            return;
        if (auto *cds = dyn_cast<ConstantDataSequential>(k)) {
            Type *et = cds->getElementType();
            unsigned es = dl.getTypeAllocSize(et);
            unsigned sz = dl.getTypeStoreSize(et);
            if (sz <= 1)
                return;
            for (unsigned n = 0; n < cds->getNumElements(); n++)
                out.push_back({off + (uint64_t)n * es, sz});
            return;
        }
        if (auto *ca = dyn_cast<ConstantArray>(k)) {
            uint64_t es = dl.getTypeAllocSize(ca->getType()->getElementType());
            for (unsigned n = 0; n < ca->getNumOperands(); n++)
                collect(ca->getOperand(n), off + n * es, dl, out);
            return;
        }
        if (auto *cs = dyn_cast<ConstantStruct>(k)) {
            const StructLayout *sl = dl.getStructLayout(cs->getType());
            for (unsigned n = 0; n < cs->getNumOperands(); n++)
                collect(cs->getOperand(n), off + sl->getElementOffset(n), dl, out);
            return;
        }
        if (auto *cv = dyn_cast<ConstantVector>(k)) {
            (void)cv;
            report_fatal_error("BEPass: vector initializer");
        }
        if (t->isIntegerTy() || t->isFloatingPointTy() || t->isPointerTy()) {
            unsigned sz = dl.getTypeStoreSize(t);
            if (sz > 1)
                out.push_back({off, sz});
            return;
        }
        report_fatal_error("BEPass: unhandled initializer");
    }

    void fixGlobals(Module &m, const DataLayout &dl) {
        LLVMContext &c = m.getContext();
        Type *i32 = Type::getInt32Ty(c);
        Type *i8 = Type::getInt8Ty(c);
        PointerType *ptr = PointerType::getUnqual(c);
        StructType *ent = StructType::get(c, {ptr, i32, i32});
        std::vector<Constant *> table;
        std::vector<GlobalVariable *> gvs;
        for (GlobalVariable &g : m.globals())
            gvs.push_back(&g);
        for (GlobalVariable *g : gvs) {
            if (!g->hasInitializer() || g->getName().starts_with("llvm."))
                continue;
            /* no over-alignment (x86 wants arrays 16-aligned): the port
               places the game's variables at their N64 addresses */
            if (!g->getMetadata("port.align"))      /* port-ilp32 did it */
                g->setAlignment(dl.getABITypeAlign(g->getValueType()));
            if (g->hasSection() && g->getSection().starts_with("llvm."))
                continue;
            std::vector<std::pair<uint64_t, unsigned>> items;
            collect(g->getInitializer(), 0, dl, items);
            if (items.empty())
                continue;
            g->setConstant(false);
            /* runs of equally sized, contiguous scalars */
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
        if (table.empty())
            return;
        ArrayType *at = ArrayType::get(ent, table.size());
        auto *tg = new GlobalVariable(m, at, true, GlobalValue::PrivateLinkage,
                                      ConstantArray::get(at, table), "__bepass_table");
        FunctionType *fixty = FunctionType::get(Type::getVoidTy(c), {ptr, i32}, false);
        FunctionCallee fix = m.getOrInsertFunction("__bepass_fixup", fixty);
        Function *ctor = Function::Create(FunctionType::get(Type::getVoidTy(c), false),
                                          GlobalValue::InternalLinkage, "__bepass_ctor", &m);
        IRBuilder<> b(BasicBlock::Create(c, "entry", ctor));
        b.CreateCall(fix, {tg, ConstantInt::get(i32, table.size())});
        b.CreateRetVoid();
        appendToGlobalCtors(m, ctor, 101);
    }

    static void addPolls(Function &f, FunctionCallee poll) {
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
        const char *tr = getenv("BEPASS_TRACE");
        FunctionCallee trace;
        if (tr && *tr == '1')
            trace = m.getOrInsertFunction("__port_trace", FunctionType::get(Type::getVoidTy(m.getContext()),
                                                                            {Type::getInt32Ty(m.getContext())}, false));
        FunctionCallee poll = m.getOrInsertFunction(
            "__port_poll", FunctionType::get(Type::getVoidTy(m.getContext()), false));
        for (Function &f : m)
            if (!f.isDeclaration()) {
                runOnFunction(f, dl);
                addPolls(f, poll);
                if (trace)
                    addTrace(f, trace);
            }
        fixGlobals(m, dl);
        const char *acc = getenv("BEPASS_ACCESS");
        if (acc && *acc == '1')
            AccessHooks::addAccessHooks(m);
        return PreservedAnalyses::none();
    }
};

struct ICount : PassInfoMixin<ICount> {
    static bool isRequired() { return true; }

    static bool costs(const Instruction &i) {
        if (isa<PHINode>(i) || isa<CastInst>(i) || isa<DbgInfoIntrinsic>(i))
            return false;
        if (auto *ci = dyn_cast<CallInst>(&i))      /* the profiler's hooks are free */
            if (Function *f = ci->getCalledFunction())
                if (f->getName().starts_with("__port_access"))
                    return false;
        if (auto *ii = dyn_cast<IntrinsicInst>(&i)) {
            switch (ii->getIntrinsicID()) {
            case Intrinsic::bswap:
            case Intrinsic::lifetime_start:
            case Intrinsic::lifetime_end:
            case Intrinsic::assume:
                return false;
            default:
                return true;
            }
        }
        if (auto *gep = dyn_cast<GetElementPtrInst>(&i))
            return !gep->hasAllConstantIndices();
        return true;
    }

    PreservedAnalyses run(Module &m, ModuleAnalysisManager &) {
        LLVMContext &c = m.getContext();
        Type *i32 = Type::getInt32Ty(c);
        GlobalVariable *cnt = m.getGlobalVariable("__port_icount_c");
        if (!cnt)
            cnt = new GlobalVariable(m, i32, false, GlobalValue::ExternalLinkage, nullptr, "__port_icount_c");
        for (Function &f : m) {
            if (f.isDeclaration())
                continue;
            for (BasicBlock &bb : f) {
                unsigned n = 0;
                for (Instruction &i : bb)
                    n += costs(i);
                if (n == 0)
                    continue;
                IRBuilder<> b(&*bb.getFirstInsertionPt());
                Value *v = b.CreateLoad(i32, cnt);
                b.CreateStore(b.CreateAdd(v, ConstantInt::get(i32, n)), cnt);
            }
        }
        return PreservedAnalyses::none();
    }

};

} // namespace

/* ILP32.cpp: port-ilp32, for opt (the 64-bit build) */
void portRegisterILP32(PassBuilder &pb);

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "BEPass", "1", [](PassBuilder &pb) {
                portRegisterILP32(pb);
                pb.registerPipelineStartEPCallback(
                    [](ModulePassManager &mpm, OptimizationLevel) { mpm.addPass(BEPass()); });
                pb.registerOptimizerLastEPCallback(
                    [](ModulePassManager &mpm, OptimizationLevel, ThinOrFullLTOPhase) {
                        mpm.addPass(ICount());
                    });
            }};
}
