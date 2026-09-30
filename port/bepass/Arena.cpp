/*
 * port-arena: the game's C reaches game memory through a base the host
 * chooses (PORT_MOVABLE, docs/PORT.md "Movable memory").
 *
 * Pointer values stay what they are, N64 addresses.  What changes is
 * where memory is: every access the C makes (loads, stores, the memory
 * intrinsics, and the pointer arguments of calls into the host, which
 * dereferences them natively) goes to
 *
 *     moved(p ^ 0x80000000) ? port_arena + (p ^ 0x80000000) : p
 *
 * so an address in the moved ranges (RDRAM, the fibers' stacks: port_arena.h)
 * is found in the host's arena, and anything else (the image, the host's
 * own memory) is left where it is.  port_arena starts out as 0x80000000 itself (so the
 * static constructors that run before main see the image's memory), and
 * main moves the range and repoints it.
 *
 * A local whose address escapes (to a call, into memory, into an integer)
 * is given its N64 address, which a fiber's stack in the arena has; one
 * that doesn't escape is accessed as it is.  A local on another stack (the
 * main loop's) keeps its host address, which the map leaves alone.
 *
 * Run at the end of the optimisation pipeline, after ICount, so the
 * instruction counts (and with them the timing) are those of the build
 * without it.  Switched on with BEPASS_ARENA=1 when compiling.
 */
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"

#include <cstdlib>
#include <vector>

using namespace llvm;

namespace {

/* the span of N64 addresses from 0x80000000 that live in the arena (port.h,
   PORT_ARENA_SPAN) */
static const uint64_t K0 = 0x80000000u;
static const uint64_t SPAN = 0x00400000u;
static const uint64_t STACKS = 0x10000000u, STACKS_SPAN = 0x01000000u;

struct Arena : PassInfoMixin<Arena> {
    static bool isRequired() { return true; }

    Module *M = nullptr;
    GlobalVariable *base = nullptr;
    unsigned rewritten = 0, hostArgs = 0;

    /* functions of the host (port/host, libc) the game's C calls with
       pointers they dereference (host_thread_create's argument is only
       handed back to the thread's entry, in the game's C) */
    static bool hostCallee(StringRef n) {
        if (n == "host_thread_create")
            return false;
        return n.starts_with("host_") || n == "memset" || n == "memmove" || n == "memcpy" ||
               n.starts_with("n64_") || n == "port_counter" || n.starts_with("__bepass_");
    }

    Value *map(IRBuilder<> &b, Value *p, Value *arena) {
        Type *pt = p->getType();
        Value *q = p;
        if (pt->getPointerAddressSpace() != 0)
            q = b.CreateAddrSpaceCast(p, PointerType::get(M->getContext(), 0));
        const DataLayout &dl = M->getDataLayout();
        IntegerType *ip = b.getIntNTy(dl.getPointerSizeInBits(0));
        /* (p ^ 0x80000000, not p - 0x80000000: for the address of a
           variable the backend would fold the subtraction into a
           PC-relative relocation that can't reach) */
        Value *off = b.CreateXor(b.CreatePtrToInt(q, ip), ConstantInt::get(ip, K0));
        Value *in = b.CreateOr(b.CreateICmpULT(off, ConstantInt::get(ip, SPAN)),
                               b.CreateICmpULT(b.CreateSub(off, ConstantInt::get(ip, STACKS)),
                                               ConstantInt::get(ip, STACKS_SPAN)));
        Value *h = b.CreateGEP(b.getInt8Ty(), arena, off);
        rewritten++;
        /* (a 32-bit pointer, PTR32, becomes an ordinary one: the host
           address doesn't fit) */
        return b.CreateSelect(in, h, q);
    }

    /* whether a local's address goes anywhere but the pointer operand of
       loads, stores and memory intrinsics */
    static bool escapes(Value *v) {
        for (User *u : v->users()) {
            if (auto *ld = dyn_cast<LoadInst>(u)) {
                (void)ld;
                continue;
            }
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

    /* an escaping local gets its N64 address (if it is on a fiber's stack,
       in the arena) */
    unsigned escaped = 0;
    void locals(Function &f, Value *arena) {
        std::vector<AllocaInst *> as;
        /* (not in a variadic function: its va_list is the host's) */
        if (f.isVarArg())
            return;
        for (BasicBlock &bb : f)
            for (Instruction &i : bb)
                if (auto *a = dyn_cast<AllocaInst>(&i))
                    if (escapes(a))
                        as.push_back(a);
        const DataLayout &dl = M->getDataLayout();
        IntegerType *ip = IntegerType::get(M->getContext(), dl.getPointerSizeInBits(0));
        for (AllocaInst *a : as) {
            Instruction *at = a->getNextNode();
            while (isa<AllocaInst>(at))
                at = at->getNextNode();
            IRBuilder<> b(at);
            Value *h = b.CreatePtrToInt(a, ip);
            Value *off = b.CreateSub(h, b.CreatePtrToInt(arena, ip));
            Value *in = b.CreateICmpULT(b.CreateSub(off, ConstantInt::get(ip, STACKS)), ConstantInt::get(ip, STACKS_SPAN));
            Value *n = b.CreateSelect(in, b.CreateOr(off, ConstantInt::get(ip, K0)), h);
            Value *np = b.CreateIntToPtr(n, a->getType(), a->getName() + ".n64");
            a->replaceUsesWithIf(np, [&](Use &u) {
                auto *ii = dyn_cast<IntrinsicInst>(u.getUser());
                if (ii && (ii->isLifetimeStartOrEnd() || isa<DbgInfoIntrinsic>(ii)))
                    return false;
                return u.getUser() != h;
            });
            escaped++;
        }
    }

    /* a local, or the port's own counters (ICount's __port_icount_c) */
    static bool local(Value *p) {
        const Value *u = getUnderlyingObject(p);
        if (auto *g = dyn_cast<GlobalVariable>(u))
            return g->getName().starts_with("__port_") || g->getName() == "port_arena";
        return isa<AllocaInst>(u);
    }

    void function(Function &f) {
        std::vector<std::pair<Instruction *, unsigned>> ops;
        for (BasicBlock &bb : f)
            for (Instruction &i : bb) {
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
                    Function *callee = cb->getCalledFunction();
                    if (!callee || !callee->isDeclaration() || !hostCallee(callee->getName()))
                        continue;
                    for (unsigned a = 0; a < cb->arg_size(); a++)
                        if (cb->getArgOperand(a)->getType()->isPointerTy() && !cb->paramHasAttr(a, Attribute::ByVal)) {
                            ops.push_back({&i, a});
                            hostArgs++;
                        }
                }
            }
        IRBuilder<> eb(&*f.getEntryBlock().getFirstInsertionPt());
        Value *arena = eb.CreateLoad(PointerType::get(M->getContext(), 0), base, "port.arena");
        locals(f, arena);
        /* a memory intrinsic on 32-bit pointers (PTR32) is made again on
           ordinary ones, which is what the mapped addresses are */
        std::vector<MemIntrinsic *> redo;
        for (auto &o : ops)
            if (auto *mi = dyn_cast<MemIntrinsic>(o.first))
                if (o.second == 0 || isa<MemTransferInst>(mi))
                    if (mi->getOperand(o.second)->getType()->getPointerAddressSpace() != 0 &&
                        (redo.empty() || redo.back() != mi))
                        redo.push_back(mi);
        for (MemIntrinsic *mi : redo) {
            IRBuilder<> b(mi);
            PointerType *p0 = PointerType::get(M->getContext(), 0);
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
            if (local(p) || isa<ConstantPointerNull>(p))
                continue;
            IRBuilder<> b(o.first);
            /* (a variadic argument can change its type: n64_sprintf's PTR32
               strings) */
            auto *cb = dyn_cast<CallBase>(o.first);
            bool vararg = cb && !isa<IntrinsicInst>(cb) && o.second >= cb->getFunctionType()->getNumParams();
            if (p->getType()->getPointerAddressSpace() != 0 && !isa<LoadInst>(o.first) &&
                !isa<StoreInst>(o.first) && !vararg)
                report_fatal_error(Twine("port-arena: a 32-bit pointer operand of ") + o.first->getOpcodeName() + " in " +
                                   f.getName());
            o.first->setOperand(o.second, map(b, p, arena));
        }
    }

    PreservedAnalyses run(Module &m, ModuleAnalysisManager &) {
        const char *on = getenv("BEPASS_ARENA");
        if (!on || *on != '1')
            return PreservedAnalyses::all();
        M = &m;
        PointerType *ptr = PointerType::get(m.getContext(), 0);
        base = m.getGlobalVariable("port_arena");
        if (!base)
            base = new GlobalVariable(m, ptr, false, GlobalValue::ExternalLinkage, nullptr, "port_arena");
        for (Function &f : m)
            if (!f.isDeclaration())
                function(f);
        if (getenv("PORT_ARENA_STATS"))
            errs() << "port-arena: " << rewritten << " accesses (" << hostArgs << " host arguments), " << escaped
                   << " escaping locals in "
                   << m.getSourceFileName() << "\n";
        return PreservedAnalyses::none();
    }
};

} // namespace

void portRegisterArena(PassBuilder &pb) {
    pb.registerOptimizerLastEPCallback(
        [](ModulePassManager &mpm, OptimizationLevel, ThinOrFullLTOPhase) { mpm.addPass(Arena()); });
}
