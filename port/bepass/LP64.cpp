/*
 * port-lp64: what the game's C needs from the compiler when it is built as
 * an ordinary LP64 program (PORT_LP64, docs/PORT.md "The LP64 build").
 *
 * The frontend lays the C out for the host: 8-byte pointers, except in the
 * structs the handwritten code, the ROM's data or the renderer share, which
 * the headers declare with 32-bit pointers (PTR32, clang's `__ptr32
 * __uptr`).  So nothing here is about layout.  What is left of port-ilp32
 * (ILP32.cpp) is what the C assumes of the N64 beyond it:
 *
 *   - pointer arithmetic wraps at 32 bits: a GEP by a variable or large
 *     offset is computed on the 32-bit address (libaudio relocates a
 *     bank's offsets with `(u8 *)p + (s32)base`), and a pointer made from
 *     an integer is its low 32 bits zero-extended (`(void *)(s32)addr` of a
 *     KSEG0 address would otherwise be sign-extended).  Every address the
 *     game's C holds is below 4 GB (RDRAM, the image, the fibers' stacks:
 *     port.h), as a PTR32 field needs anyway;
 *   - an access through a variable's name that reaches outside it (`T
 *     x[1]` placeholders, `extern T x[]`) goes through an integer, so the
 *     optimiser can't assume it doesn't alias the neighbour it really
 *     reaches;
 *   - K&R calls behave as on the N64: pointer arguments are re-zero-extended
 *     on entry and pointer results at the call (an int may have been passed
 *     or returned), narrow results are returned extended to 32 bits, and a
 *     callee extends its own narrow arguments;
 *   - a call through a 32-bit function pointer calls its zero-extended
 *     value (clang calls through the `__ptr32` operand itself, and the
 *     backend then loads 8 bytes where the field has 4).
 *
 * Run at the start of the pipeline (BEPASS_LP64=1), before BEPass.
 */
#include "llvm/Analysis/Utils/Local.h"
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

#include <cstdlib>
#include <vector>

using namespace llvm;

namespace {

struct LP64 : PassInfoMixin<LP64> {
    static bool isRequired() { return true; }

    Module *M = nullptr;
    LLVMContext *C = nullptr;
    const DataLayout *DL = nullptr;
    IntegerType *I32 = nullptr, *I8 = nullptr;
    int Stats = getenv("PORT_LP64_STATS") ? atoi(getenv("PORT_LP64_STATS")) : 0;
    unsigned oobWrapped = 0;

    [[noreturn]] void fail(const Twine &what) {
        report_fatal_error("port-lp64: " + what + " in " + M->getSourceFileName());
    }

    /* a pointer as the N64 has it: its low 32 bits, zero-extended */
    Value *wrap(IRBuilder<> &b, Value *p) {
        return b.CreateIntToPtr(b.CreatePtrToInt(p, I32), p->getType(), p->getName());
    }

    void function(Function &f) {
        std::vector<Instruction *> work;
        for (BasicBlock &bb : f)
            for (Instruction &i : bb)
                if (isa<GetElementPtrInst>(i) || isa<IntToPtrInst>(i) || isa<CallBase>(i))
                    work.push_back(&i);
        for (Instruction *i : work) {
            IRBuilder<> b(i);
            if (auto *gep = dyn_cast<GetElementPtrInst>(i)) {
                if (gep->getType()->isVectorTy())
                    continue;
                /* a small constant offset (a struct's field) stays a GEP, so
                   it still folds into the access */
                APInt off(DL->getIndexTypeSizeInBits(gep->getType()), 0);
                if (gep->accumulateConstantOffset(*DL, off) && off.getSExtValue() < 0x10000000 &&
                    off.getSExtValue() > -0x10000000)
                    continue;
                Value *o = emitGEPOffset(&b, *DL, gep, true);
                Value *n = b.CreateGEP(I8, gep->getPointerOperand(), o);
                n = wrap(b, n);
                n->takeName(gep);
                gep->replaceAllUsesWith(n);
                gep->eraseFromParent();
            } else if (auto *ip = dyn_cast<IntToPtrInst>(i)) {
                Value *v = ip->getOperand(0);
                if (v->getType() == I32)
                    continue;           /* ours: already zero-extended */
                Value *n = b.CreateIntToPtr(b.CreateZExt(b.CreateTrunc(v, I32), b.getInt64Ty()), ip->getType());
                n->takeName(ip);
                ip->replaceAllUsesWith(n);
                ip->eraseFromParent();
            } else if (auto *cb = dyn_cast<CallBase>(i)) {
                if (isa<IntrinsicInst>(cb) || cb->isInlineAsm())
                    continue;
                Value *callee = cb->getCalledOperand();
                if (callee->getType()->getPointerAddressSpace() != 0)
                    cb->setCalledOperand(
                        b.CreateIntToPtr(b.CreateZExt(b.CreatePtrToInt(callee, I32), b.getInt64Ty()),
                                         PointerType::get(*C, 0)));
                /* a pointer result may come from a callee that returns an
                   int (K&R declarations) */
                if (cb->getType()->isPointerTy() && isa<CallInst>(cb) && !cb->use_empty()) {
                    b.SetInsertPoint(cb->getNextNode());
                    Value *v = wrap(b, cb);
                    cb->replaceUsesWithIf(v, [&](Use &u) { return u.getUser() != cast<User>(v)->getOperand(0); });
                }
            }
        }
    }

    /* An access through one variable's name that reaches outside it: LLVM
       takes two globals for two objects that can't alias, and an access
       outside an object for undefined behaviour, so such an address is
       laundered through an integer.  (Variable offsets already are.) */
    void launderOutOfBounds(Function &f) {
        std::vector<std::pair<Instruction *, unsigned>> uses;
        for (BasicBlock &bb : f)
            for (Instruction &i : bb) {
                if (isa<LoadInst>(i) || isa<StoreInst>(i)) {
                    unsigned op = isa<LoadInst>(i) ? 0 : 1;
                    Type *t = isa<LoadInst>(i) ? i.getType() : cast<StoreInst>(i).getValueOperand()->getType();
                    if (outside(i.getOperand(op), DL->getTypeStoreSize(t)))
                        uses.push_back({&i, op});
                } else if (auto *mi = dyn_cast<MemIntrinsic>(&i)) {
                    auto *len = dyn_cast<ConstantInt>(mi->getLength());
                    uint64_t n = len ? len->getZExtValue() : ~0ull >> 1;
                    if (outside(mi->getRawDest(), n))
                        uses.push_back({&i, 0});
                    if (auto *mt = dyn_cast<MemTransferInst>(mi))
                        if (outside(mt->getRawSource(), n))
                            uses.push_back({&i, 1});
                }
            }
        for (auto &u : uses) {
            IRBuilder<> b(u.first);
            u.first->setOperand(u.second, wrap(b, u.first->getOperand(u.second)));
            oobWrapped++;
        }
    }

    bool outside(Value *p, uint64_t n) {
        APInt off(64, 0);
        Value *base = p->stripAndAccumulateConstantOffsets(*DL, off, true);
        uint64_t size;
        if (auto *g = dyn_cast<GlobalVariable>(base)) {
            if (!g->getValueType()->isSized())
                return true;
            size = DL->getTypeAllocSize(g->getValueType());
        } else if (auto *a = dyn_cast<AllocaInst>(base)) {
            if (a->isArrayAllocation() || !a->getAllocatedType()->isSized())
                return false;
            size = DL->getTypeAllocSize(a->getAllocatedType());
        } else {
            return false;
        }
        int64_t o = off.getSExtValue();
        bool out = o < 0 || (uint64_t)o + n > size;
        if (out && Stats > 1)
            errs() << "port-lp64: outside " << base->getName() << " (" << size << " bytes): " << n << " bytes at "
                   << o << " in " << M->getSourceFileName() << "\n";
        return out;
    }

    /* zero-extend incoming pointer arguments: a K&R caller may have
       passed a 32-bit int */
    void normalizeArgs(Function &f) {
        if (f.arg_empty())
            return;
        IRBuilder<> b(&*f.getEntryBlock().getFirstInsertionPt());
        for (Argument &a : f.args()) {
            if (!a.getType()->isPointerTy() || a.hasByValAttr() || a.hasStructRetAttr() || a.use_empty())
                continue;
            Value *v = b.CreateIntToPtr(b.CreatePtrToInt(&a, I32), a.getType(), a.getName() + ".n64");
            a.replaceUsesWithIf(v, [&](Use &u) { return u.getUser() != cast<Instruction>(v)->getOperand(0); });
        }
    }

    static bool narrow(Type *t) {
        return t->isIntegerTy() && t->getIntegerBitWidth() < 32;
    }

    /* definitions that return a narrow integer return it extended to i32,
       and calls that expect one truncate what they get; a callee extends
       its own narrow arguments, as IDO's code does */
    void widenReturns() {
        std::vector<Function *> fs;
        for (Function &f : *M)
            if (!f.isDeclaration() && !f.isIntrinsic() && narrow(f.getReturnType()))
                fs.push_back(&f);
        for (Function *f : fs) {
            bool sext = f->getAttributes().hasRetAttr(Attribute::SExt);
            FunctionType *ot = f->getFunctionType();
            FunctionType *nt = FunctionType::get(I32, ot->params(), ot->isVarArg());
            Function *n = Function::Create(nt, f->getLinkage(), f->getAddressSpace(), "", M);
            n->copyAttributesFrom(f);
            n->setAttributes(f->getAttributes()
                                 .removeRetAttribute(*C, Attribute::SExt)
                                 .removeRetAttribute(*C, Attribute::ZExt)
                                 .removeRetAttribute(*C, Attribute::NoUndef));
            n->setSubprogram(f->getSubprogram());
            n->takeName(f);
            n->splice(n->begin(), f);
            for (auto a = f->arg_begin(), b = n->arg_begin(); a != f->arg_end(); ++a, ++b) {
                b->takeName(&*a);
                a->replaceAllUsesWith(&*b);
            }
            for (BasicBlock &bb : *n)
                if (auto *ret = dyn_cast<ReturnInst>(bb.getTerminator())) {
                    IRBuilder<> b(ret);
                    Value *v = ret->getReturnValue();
                    b.CreateRet(sext ? b.CreateSExt(v, I32) : b.CreateZExt(v, I32));
                    ret->eraseFromParent();
                }
            f->replaceAllUsesWith(n);
            f->eraseFromParent();
        }
        std::vector<CallInst *> calls;
        for (Function &f : *M)
            for (BasicBlock &bb : f)
                for (Instruction &i : bb)
                    if (auto *ci = dyn_cast<CallInst>(&i))
                        if (!isa<IntrinsicInst>(ci) && !ci->isInlineAsm() && narrow(ci->getType()))
                            calls.push_back(ci);
        for (CallInst *ci : calls) {
            FunctionType *ot = ci->getFunctionType();
            FunctionType *nt = FunctionType::get(I32, ot->params(), ot->isVarArg());
            std::vector<Value *> args(ci->arg_begin(), ci->arg_end());
            SmallVector<OperandBundleDef, 1> bundles;
            ci->getOperandBundlesAsDefs(bundles);
            IRBuilder<> b(ci);
            CallInst *n = b.CreateCall(nt, ci->getCalledOperand(), args, bundles);
            n->setCallingConv(ci->getCallingConv());
            n->setTailCallKind(ci->getTailCallKind());
            n->setAttributes(ci->getAttributes()
                                 .removeRetAttribute(*C, Attribute::SExt)
                                 .removeRetAttribute(*C, Attribute::ZExt)
                                 .removeRetAttribute(*C, Attribute::NoUndef));
            n->setDebugLoc(ci->getDebugLoc());
            n->copyMetadata(*ci);
            Value *t = b.CreateTrunc(n, ci->getType());
            t->takeName(ci);
            ci->replaceAllUsesWith(t);
            ci->eraseFromParent();
        }
        for (Function &f : *M) {
            if (f.isDeclaration())
                continue;
            for (unsigned a = 0; a < f.arg_size(); a++) {
                f.removeParamAttr(a, Attribute::SExt);
                f.removeParamAttr(a, Attribute::ZExt);
            }
        }
    }

    /* A PTR32 field in a static initializer is an addrspacecast of the
       address, which the backend can't emit; its low 32 bits are the same
       relocation, and one it can (R_X86_64_32). */
    Constant *fixConst(Constant *c) {
        if (auto *ce = dyn_cast<ConstantExpr>(c)) {
            if (ce->getOpcode() == Instruction::AddrSpaceCast && ce->getType()->getPointerAddressSpace() != 0)
                return ConstantExpr::getIntToPtr(ConstantExpr::getPtrToInt(ce->getOperand(0), I32), ce->getType());
            return c;
        }
        if (!isa<ConstantStruct>(c) && !isa<ConstantArray>(c))
            return c;
        bool changed = false;
        SmallVector<Constant *, 16> ops;
        for (unsigned i = 0; i < c->getNumOperands(); i++) {
            Constant *o = cast<Constant>(c->getOperand(i));
            Constant *n = fixConst(o);
            changed |= n != o;
            ops.push_back(n);
        }
        if (!changed)
            return c;
        if (auto *st = dyn_cast<StructType>(c->getType()))
            return ConstantStruct::get(st, ops);
        return ConstantArray::get(cast<ArrayType>(c->getType()), ops);
    }

    PreservedAnalyses run(Module &m, ModuleAnalysisManager &) {
        const char *on = getenv("BEPASS_LP64");
        if (!on || *on != '1')
            return PreservedAnalyses::all();
        M = &m;
        C = &m.getContext();
        DL = &m.getDataLayout();
        I32 = Type::getInt32Ty(*C);
        I8 = Type::getInt8Ty(*C);
        for (GlobalVariable &g : m.globals())
            if (g.hasInitializer())
                g.setInitializer(fixConst(g.getInitializer()));
        widenReturns();
        for (Function &f : m)
            if (!f.isDeclaration()) {
                function(f);
                normalizeArgs(f);
                launderOutOfBounds(f);
            }
        if (Stats)
            errs() << "port-lp64: " << oobWrapped << " out-of-bounds accesses in " << m.getSourceFileName() << "\n";
        return PreservedAnalyses::none();
    }
};

} // namespace

void portRegisterLP64(PassBuilder &pb) {
    pb.registerPipelineStartEPCallback([](ModulePassManager &mpm, OptimizationLevel) { mpm.addPass(LP64()); });
}
