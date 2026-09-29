/*
 * port-ilp32: run the game's C as 64-bit code with the N64's 32-bit data
 * layout (docs/PORT.md, "64-bit").
 *
 * The C is compiled by clang for i386 (-m32 -malign-double), so the
 * frontend lays everything out as the N64 does: 4-byte pointers and longs,
 * 8-byte aligned doubles and u64s, every struct at its N64 offsets.  This
 * pass then turns that module into one for a 64-bit target (x86-64 or
 * AArch64) without changing any of it:
 *
 *   - every offset is made explicit: GEPs become byte GEPs by the offsets
 *     the i386 layout gives, allocas and globals become byte arrays (or
 *     packed structs with explicit padding) of their i386 size;
 *   - a pointer in memory stays 4 bytes: a load of a pointer is a 32-bit
 *     load and a zero extension, a store truncates.  In registers,
 *     pointers are the target's 64 bits, so every address the C holds
 *     must be below 4 GB, which it is: RDRAM is at 0x80000000, the image
 *     right above it, the fibers' stacks at 0x90000000 (port.h);
 *   - pointer arithmetic wraps at 32 bits, as the N64's does: a GEP by a
 *     variable or large offset is computed on the 32-bit address (the
 *     game relocates offsets by adding a KSEG0 base as an s32); one by a
 *     small constant (a struct field) stays a plain GEP;
 *   - an access through a variable's name that reaches outside the
 *     variable (the game's `T x[1]` placeholders, `extern T x[]`) goes
 *     through an integer, so the optimiser can't assume it doesn't alias
 *     the neighbour it really reads (PORT_ILP32_STATS=2 lists them);
 *   - pointers in initializers become 32-bit relocations;
 *   - pointer arguments are re-zero-extended at function entry, since the
 *     C's K&R declarations often pass an int where the definition takes a
 *     pointer (on the N64, and on i386, the same thing);
 *   - narrow integers cross calls as the N64 passes them: a u8, s8, u16
 *     or s16 is returned extended to 32 bits (MIPS does; x86-64 leaves
 *     the upper bits undefined, and a K&R caller that declared the
 *     function as returning an int reads them), and a callee doesn't rely
 *     on its caller having extended its narrow arguments;
 *   - the module gets the 64-bit triple and data layout, and loses the
 *     i386 target attributes.
 *
 * What the pass can't take are varargs definitions (va_list is the
 * target's), aggregates of pointers loaded or stored whole, and pointer
 * atomics: it fails on them.  Varargs functions the game needs live on
 * the host side (port/host/libc64.c).
 *
 * -port-ilp32-check adds a check to every pointer store that the pointer
 * fits in 32 bits (calling __port_bad_ptr if not): a host pointer that
 * leaked into game memory would otherwise be truncated silently.
 *
 * Run by opt between the i386 frontend and the 64-bit backend
 * (port/tools/ilp32cc.py); BEPass runs after it, in the backend's clang.
 */
#include "llvm/Analysis/Utils/Local.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/PassManager.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Triple.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <cstdlib>
#include <map>
#include <memory>
#include <vector>

using namespace llvm;

static cl::opt<std::string> ILP32Triple("port-ilp32-triple", cl::desc("64-bit target of port-ilp32"),
                                        cl::init("x86_64-pc-linux-gnu"));
static cl::opt<bool> ILP32Check("port-ilp32-check", cl::desc("port-ilp32: check stored pointers fit in 32 bits"),
                                cl::init(false));

namespace {

struct ILP32 : PassInfoMixin<ILP32> {
    static bool isRequired() { return true; }

    Module *M = nullptr;
    LLVMContext *C = nullptr;
    const DataLayout *Old = nullptr;
    std::unique_ptr<DataLayout> New;
    IntegerType *I32 = nullptr, *I8 = nullptr;
    std::map<Type *, Type *> memTypes;
    /* for a flattened struct: old field -> new field index */
    std::map<Type *, std::vector<unsigned>> fieldMap;
    std::map<Constant *, Constant *> constMap;

    [[noreturn]] void fail(const Twine &what) {
        report_fatal_error("port-ilp32: " + what + " in " + M->getSourceFileName());
    }

    static bool hasPtr(Type *t) {
        if (t->isPointerTy())
            return true;
        if (auto *st = dyn_cast<StructType>(t)) {
            for (Type *e : st->elements())
                if (hasPtr(e))
                    return true;
            return false;
        }
        if (auto *at = dyn_cast<ArrayType>(t))
            return hasPtr(at->getElementType());
        if (auto *vt = dyn_cast<VectorType>(t))
            return hasPtr(vt->getElementType());
        return false;
    }

    /* does the new layout give `t` the same size and offsets? */
    bool sameLayout(Type *t) {
        if (!t->isSized())
            return true;
        if (Old->getTypeAllocSize(t) != New->getTypeAllocSize(t))
            return false;
        if (auto *st = dyn_cast<StructType>(t)) {
            const StructLayout *a = Old->getStructLayout(st), *b = New->getStructLayout(st);
            for (unsigned i = 0; i < st->getNumElements(); i++)
                if (a->getElementOffset(i) != b->getElementOffset(i) || !sameLayout(st->getElementType(i)))
                    return false;
        } else if (auto *at = dyn_cast<ArrayType>(t)) {
            return sameLayout(at->getElementType());
        }
        return true;
    }

    /* the in-memory type with the i386 layout under the 64-bit data layout */
    Type *memType(Type *t) {
        auto it = memTypes.find(t);
        if (it != memTypes.end())
            return it->second;
        Type *r = t;
        if (t->isPointerTy()) {
            r = I32;
        } else if (t->isX86_FP80Ty() || t->isFP128Ty()) {
            fail("long double");
        } else if (auto *vt = dyn_cast<VectorType>(t)) {
            if (hasPtr(vt))
                fail("vector of pointers");
        } else if (auto *at = dyn_cast<ArrayType>(t)) {
            Type *e = memType(at->getElementType());
            if (e != at->getElementType())
                r = ArrayType::get(e, at->getNumElements());
        } else if (auto *st = dyn_cast<StructType>(t)) {
            if (st->isOpaque()) {
                r = t;
            } else if (hasPtr(st) || !sameLayout(st)) {
                /* a packed struct: the fields at their i386 offsets */
                const StructLayout *sl = Old->getStructLayout(st);
                std::vector<Type *> els;
                std::vector<unsigned> idx;
                uint64_t off = 0;
                for (unsigned i = 0; i < st->getNumElements(); i++) {
                    uint64_t fo = sl->getElementOffset(i);
                    if (fo > off)
                        els.push_back(ArrayType::get(I8, fo - off));
                    idx.push_back(els.size());
                    Type *ft = memType(st->getElementType(i));
                    els.push_back(ft);
                    off = fo + Old->getTypeAllocSize(st->getElementType(i));
                }
                uint64_t size = Old->getTypeAllocSize(st);
                if (size > off)
                    els.push_back(ArrayType::get(I8, size - off));
                StructType *ns = StructType::get(*C, els, true);
                fieldMap[ns] = idx;
                r = ns;
            }
        }
        if (r->isSized() && t->isSized() && New->getTypeAllocSize(r) != Old->getTypeAllocSize(t))
            fail("layout of a type changed");
        memTypes[t] = r;
        return r;
    }

    Type *bytes(Type *t) { return ArrayType::get(I8, Old->getTypeAllocSize(t)); }

    /* constants: GEPs by the i386 offsets */
    Constant *mapC(Constant *k) {
        auto it = constMap.find(k);
        if (it != constMap.end())
            return it->second;
        Constant *r = k;
        if (auto *ce = dyn_cast<ConstantExpr>(k)) {
            if (auto *gep = dyn_cast<GEPOperator>(ce)) {
                Constant *base = mapC(cast<Constant>(gep->getPointerOperand()));
                APInt off(Old->getIndexTypeSizeInBits(gep->getType()), 0);
                if (gep->accumulateConstantOffset(*Old, off)) {
                    r = off.isZero() ? base
                                     : ConstantExpr::getGetElementPtr(I8, base, ConstantInt::get(I32, off.getSExtValue()));
                } else {
                    /* an index that is itself a constant expression (the
                       difference of two linker symbols): allowed where its
                       stride is one byte */
                    Constant *sum = ConstantInt::get(I32, 0);
                    for (gep_type_iterator gi = gep_type_begin(gep), ge = gep_type_end(gep); gi != ge; ++gi) {
                        auto *idx = cast<Constant>(gi.getOperand());
                        if (StructType *st = gi.getStructTypeOrNull()) {
                            uint64_t fo = Old->getStructLayout(st)->getElementOffset(
                                cast<ConstantInt>(idx)->getZExtValue());
                            sum = ConstantExpr::getAdd(sum, ConstantInt::get(I32, fo));
                            continue;
                        }
                        uint64_t stride = gi.getSequentialElementStride(*Old);
                        if (auto *ci = dyn_cast<ConstantInt>(idx)) {
                            sum = ConstantExpr::getAdd(sum, ConstantInt::get(I32, ci->getSExtValue() * stride));
                        } else if (stride == 1 && idx->getType() == I32) {
                            sum = ConstantExpr::getAdd(sum, mapC(idx));
                        } else {
                            fail("GEP constant with a symbolic index of stride " + Twine(stride));
                        }
                    }
                    r = ConstantExpr::getGetElementPtr(I8, base, sum);
                }
            } else {
                std::vector<Constant *> ops;
                bool changed = false;
                for (Use &u : ce->operands()) {
                    Constant *o = mapC(cast<Constant>(u.get()));
                    changed |= o != u.get();
                    ops.push_back(o);
                }
                if (changed)
                    r = ce->getWithOperands(ops);
            }
        } else if (isa<ConstantAggregate>(k)) {
            std::vector<Constant *> ops;
            bool changed = false;
            for (Use &u : k->operands()) {
                Constant *o = mapC(cast<Constant>(u.get()));
                changed |= o != u.get();
                ops.push_back(o);
            }
            if (changed) {
                if (auto *st = dyn_cast<StructType>(k->getType()))
                    r = ConstantStruct::get(st, ops);
                else if (auto *at = dyn_cast<ArrayType>(k->getType()))
                    r = ConstantArray::get(at, ops);
                else
                    r = ConstantVector::get(ops);
            }
        }
        constMap[k] = r;
        return r;
    }

    /* an initializer of type t, in its memory type */
    Constant *mapInit(Constant *k, Type *t) {
        Type *nt = memType(t);
        if (nt == t)
            return mapC(k);
        if (isa<ConstantAggregateZero>(k) || k->isNullValue())
            return Constant::getNullValue(nt);
        if (isa<PoisonValue>(k))
            return PoisonValue::get(nt);
        if (isa<UndefValue>(k))
            return UndefValue::get(nt);
        if (t->isPointerTy())
            return ConstantExpr::getPtrToInt(mapC(k), I32);
        if (auto *at = dyn_cast<ArrayType>(t)) {
            std::vector<Constant *> els;
            for (unsigned i = 0; i < at->getNumElements(); i++)
                els.push_back(mapInit(k->getAggregateElement(i), at->getElementType()));
            return ConstantArray::get(cast<ArrayType>(nt), els);
        }
        if (auto *st = dyn_cast<StructType>(t)) {
            auto *ns = cast<StructType>(nt);
            const std::vector<unsigned> &idx = fieldMap.at(ns);
            std::vector<Constant *> els(ns->getNumElements(), nullptr);
            for (unsigned i = 0; i < st->getNumElements(); i++)
                els[idx[i]] = mapInit(k->getAggregateElement(i), st->getElementType(i));
            for (unsigned i = 0; i < els.size(); i++)
                if (!els[i])
                    els[i] = Constant::getNullValue(ns->getElementType(i));
            return ConstantStruct::get(ns, els);
        }
        fail("unhandled initializer");
    }

    void fixAttrs(AttributeList &al, unsigned nargs) {
        static const Attribute::AttrKind kinds[] = {Attribute::ByVal, Attribute::StructRet, Attribute::ByRef,
                                                    Attribute::InAlloca, Attribute::Preallocated,
                                                    Attribute::ElementType};
        for (unsigned a = 0; a < nargs; a++)
            for (Attribute::AttrKind kind : kinds) {
                Type *t = al.getParamAttrs(a).getAttribute(kind).isValid()
                              ? al.getParamAttrs(a).getAttribute(kind).getValueAsType()
                              : nullptr;
                if (!t || !t->isSized())
                    continue;
                Type *b = bytes(t);
                al = al.removeParamAttribute(*C, a, kind);
                al = al.addParamAttribute(*C, a, Attribute::get(*C, kind, b));
            }
    }

    void globals() {
        std::vector<GlobalVariable *> gvs;
        for (GlobalVariable &g : M->globals())
            if (!g.getName().starts_with("llvm."))
                gvs.push_back(&g);
        std::vector<std::pair<GlobalVariable *, GlobalVariable *>> done;
        for (GlobalVariable *g : gvs) {
            Type *t = g->getValueType();
            /* what BEPass would give it: the type's own alignment, not
               x86's 16 for arrays (the port places game data at N64
               addresses) */
            Align natural = t->isSized() ? Old->getABITypeAlign(t) : Align(1);
            Type *nt = memType(t);
            GlobalVariable *n = g;
            if (nt != t) {
                n = new GlobalVariable(*M, nt, g->isConstant(), g->getLinkage(), nullptr, "", g,
                                       g->getThreadLocalMode(), g->getAddressSpace(),
                                       g->isExternallyInitialized());
                n->copyAttributesFrom(g);
                n->takeName(g);
                SmallVector<std::pair<unsigned, MDNode *>, 4> mds;
                g->getAllMetadata(mds);
                for (auto &md : mds)
                    n->setMetadata(md.first, md.second);
                g->replaceAllUsesWith(n);
                done.push_back({g, n});
            }
            n->setAlignment(natural);
            /* BEPass: keep it (the new type may be a byte array) */
            n->setMetadata("port.align", MDNode::get(*C, {}));
        }
        constMap.clear();
        for (auto &p : done) {
            if (p.first->hasInitializer())
                p.second->setInitializer(mapInit(p.first->getInitializer(), p.first->getValueType()));
            p.first->eraseFromParent();
        }
        constMap.clear();
        for (GlobalVariable &g : M->globals())
            if (g.hasInitializer() && !g.getName().starts_with("llvm."))
                g.setInitializer(mapC(g.getInitializer()));
        for (GlobalAlias &a : M->aliases())
            a.setAliasee(mapC(a.getAliasee()));
    }

    void function(Function &f) {
        for (BasicBlock &bb : f)
            for (Instruction &i : bb)
                if (auto *ii = dyn_cast<IntrinsicInst>(&i))
                    if (ii->getIntrinsicID() == Intrinsic::vastart)
                        fail("va_start in " + f.getName() + " (varargs definitions belong on the host side)");
        std::vector<Instruction *> work;
        for (BasicBlock &bb : f)
            for (Instruction &i : bb)
                work.push_back(&i);
        FunctionCallee bad;
        if (ILP32Check)
            bad = M->getOrInsertFunction("__port_bad_ptr",
                                         FunctionType::get(Type::getVoidTy(*C), {PointerType::getUnqual(*C)}, false));
        for (Instruction *i : work) {
            for (Use &u : i->operands())
                if (auto *k = dyn_cast<Constant>(u.get()))
                    if (!isa<GlobalValue>(k) && !isa<BasicBlock>(u.get())) {
                        Constant *n = mapC(k);
                        if (n != k)
                            u.set(n);
                    }
            IRBuilder<> b(i);
            if (auto *gep = dyn_cast<GetElementPtrInst>(i)) {
                if (gep->getType()->isVectorTy())
                    fail("vector GEP");
                Value *off = emitGEPOffset(&b, *Old, gep, true);
                Value *n = b.CreateGEP(I8, gep->getPointerOperand(), off, gep->getName());
                /* The N64's pointer arithmetic wraps at 32 bits, and the
                   game relies on it: libaudio relocates a bank's offsets
                   with `(u8 *)p + (s32)base`.  A 64-bit GEP sign-extends
                   the offset instead, which lands below 0 or above 4 GB.
                   So a GEP by a variable or large offset is wrapped to 32
                   bits; one by a small constant (the fields of a struct)
                   stays a plain GEP, so its offset still folds into the
                   access. */
                auto *ci = dyn_cast<ConstantInt>(off);
                if (!ci || ci->getSExtValue() >= 0x10000000 || ci->getSExtValue() <= -0x10000000)
                    n = wrap(b, n);
                gep->replaceAllUsesWith(n);
                gep->eraseFromParent();
            } else if (auto *al = dyn_cast<AllocaInst>(i)) {
                Type *t = al->getAllocatedType();
                Type *nt = memType(t);
                if (nt == t)
                    continue;
                AllocaInst *n;
                if (al->isArrayAllocation())
                    n = b.CreateAlloca(I8, b.CreateMul(b.CreateZExtOrTrunc(al->getArraySize(), I32),
                                                       ConstantInt::get(I32, Old->getTypeAllocSize(t))));
                else
                    n = b.CreateAlloca(nt);
                n->setAlignment(al->getAlign());
                n->takeName(al);
                al->replaceAllUsesWith(n);
                al->eraseFromParent();
            } else if (auto *ld = dyn_cast<LoadInst>(i)) {
                Type *t = ld->getType();
                if (!hasPtr(t))
                    continue;
                if (!t->isPointerTy())
                    fail("load of an aggregate with pointers");
                LoadInst *nl = b.CreateAlignedLoad(I32, ld->getPointerOperand(), ld->getAlign(), ld->isVolatile());
                nl->setOrdering(ld->getOrdering());
                nl->setSyncScopeID(ld->getSyncScopeID());
                Value *v = b.CreateIntToPtr(nl, t);
                ld->replaceAllUsesWith(v);
                ld->eraseFromParent();
            } else if (auto *st = dyn_cast<StoreInst>(i)) {
                Type *t = st->getValueOperand()->getType();
                if (!hasPtr(t))
                    continue;
                if (!t->isPointerTy())
                    fail("store of an aggregate with pointers");
                Value *p = st->getValueOperand();
                if (ILP32Check) {
                    /* upper half 0, or 1 or -1 from arithmetic that
                       wraps (the store truncates it right) */
                    Value *hi = b.CreateAShr(b.CreatePtrToInt(p, b.getInt64Ty()), 32);
                    Value *isBad = b.CreateICmpUGT(b.CreateAdd(hi, b.getInt64(1)), b.getInt64(2));
                    Instruction *term = SplitBlockAndInsertIfThen(isBad, st, false);
                    IRBuilder<> tb(term);
                    tb.CreateCall(bad, {p});
                    b.SetInsertPoint(st);
                }
                StoreInst *ns = b.CreateAlignedStore(b.CreatePtrToInt(p, I32), st->getPointerOperand(),
                                                     st->getAlign(), st->isVolatile());
                ns->setOrdering(st->getOrdering());
                ns->setSyncScopeID(st->getSyncScopeID());
                st->eraseFromParent();
            } else if (isa<AtomicRMWInst>(i) || isa<AtomicCmpXchgInst>(i)) {
                for (Use &u : i->operands())
                    if (u.get()->getType()->isPointerTy() && u.getOperandNo() > 0)
                        fail("pointer atomic");
            } else if (isa<VAArgInst>(i)) {
                fail("va_arg");
            } else if (auto *cb = dyn_cast<CallBase>(i)) {
                AttributeList al = cb->getAttributes();
                fixAttrs(al, cb->arg_size());
                cb->setAttributes(al);
                /* a pointer result may come from a callee that returns an
                   int (K&R declarations again) */
                if (cb->getType()->isPointerTy() && !isa<IntrinsicInst>(cb) && isa<CallInst>(cb) &&
                    !cb->use_empty()) {
                    b.SetInsertPoint(cb->getNextNode());
                    Value *v = wrap(b, cb);
                    cb->replaceUsesWithIf(v, [&](Use &u) { return u.getUser() != cast<User>(v)->getOperand(0); });
                }
            }
        }
    }

    /* An access through one variable's name that reaches outside it: the
       game reads and writes past a variable into its neighbours (a
       `u16 D_8036BB48[1]` that is really a longer buffer, an `extern
       u8 x[]`).  LLVM takes two globals for two objects that can't alias,
       and an access outside an object for undefined behaviour, so such an
       address is laundered through an integer: to the optimiser it is
       then an unknown pointer.  (Variable offsets already are, by wrap().) */
    unsigned oobWrapped = 0;
    int ILP32Stats = getenv("PORT_ILP32_STATS") ? atoi(getenv("PORT_ILP32_STATS")) : 0;
    void launderOutOfBounds(Function &f) {
        std::vector<std::pair<Instruction *, unsigned>> uses;
        for (BasicBlock &bb : f)
            for (Instruction &i : bb) {
                if (isa<LoadInst>(i) || isa<StoreInst>(i)) {
                    unsigned op = isa<LoadInst>(i) ? 0 : 1;
                    Type *t = isa<LoadInst>(i) ? i.getType() : cast<StoreInst>(i).getValueOperand()->getType();
                    if (outside(i.getOperand(op), New->getTypeStoreSize(t)))
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
        Value *base = p->stripAndAccumulateConstantOffsets(*New, off, true);
        uint64_t size;
        if (auto *g = dyn_cast<GlobalVariable>(base)) {
            if (!g->getValueType()->isSized())
                return true;
            size = New->getTypeAllocSize(g->getValueType());
        } else if (auto *a = dyn_cast<AllocaInst>(base)) {
            if (a->isArrayAllocation() || !a->getAllocatedType()->isSized())
                return false;
            size = New->getTypeAllocSize(a->getAllocatedType());
        } else {
            return false;
        }
        int64_t o = off.getSExtValue();
        bool out = o < 0 || (uint64_t)o + n > size;
        if (out && ILP32Stats > 1)
            errs() << "port-ilp32: outside " << base->getName() << " (" << size << " bytes): " << n
                   << " bytes at " << o << " in " << M->getSourceFileName() << "\n";
        return out;
    }

    /* a pointer as the N64 has it: its low 32 bits, zero-extended */
    Value *wrap(IRBuilder<> &b, Value *p) {
        return b.CreateIntToPtr(b.CreatePtrToInt(p, I32), p->getType(), p->getName());
    }

    /* zero-extend incoming pointer arguments: a K&R caller may have
       passed a 32-bit int */
    void normalizeArgs(Function &f) {
        if (f.isDeclaration() || f.arg_empty())
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

    /* definitions that return a narrow integer return it extended to i32 */
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
        /* every call that expects a narrow result takes it as i32 and
           truncates: from a converted callee that is its extended value,
           from the host's the low bits */
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
        /* a callee extends its own narrow arguments, as IDO's code does:
           a K&R caller may have passed a whole int */
        for (Function &f : *M) {
            if (f.isDeclaration())
                continue;
            for (unsigned a = 0; a < f.arg_size(); a++) {
                f.removeParamAttr(a, Attribute::SExt);
                f.removeParamAttr(a, Attribute::ZExt);
            }
        }
    }

    PreservedAnalyses run(Module &m, ModuleAnalysisManager &) {
        M = &m;
        C = &m.getContext();
        I32 = Type::getInt32Ty(*C);
        I8 = Type::getInt8Ty(*C);
        DataLayout oldDL = m.getDataLayout();
        Old = &oldDL;
        if (Old->getPointerSizeInBits(0) != 32)
            fail("the module isn't ILP32 (compile it for i386)");
        std::string err;
        Triple tt(ILP32Triple);
        const Target *tgt = TargetRegistry::lookupTarget(tt, err);
        if (!tgt)
            fail("target " + ILP32Triple + ": " + err);
        std::unique_ptr<TargetMachine> tm(tgt->createTargetMachine(tt, "", "", TargetOptions(), std::nullopt));
        New = std::make_unique<DataLayout>(tm->createDataLayout());

        widenReturns();
        globals();
        for (Function &f : m) {
            AttributeList al = f.getAttributes();
            fixAttrs(al, f.arg_size());
            f.setAttributes(al);
            f.removeFnAttr("target-cpu");
            f.removeFnAttr("target-features");
            f.removeFnAttr("tune-cpu");
            if (!f.isDeclaration()) {
                function(f);
                normalizeArgs(f);
            }
        }
        m.setTargetTriple(tt);
        m.setDataLayout(*New);
        for (Function &f : m)
            if (!f.isDeclaration())
                launderOutOfBounds(f);
        if (ILP32Stats)
            errs() << "port-ilp32: " << oobWrapped << " out-of-bounds accesses in " << m.getSourceFileName() << "\n";
        return PreservedAnalyses::none();
    }
};

} // namespace

void portRegisterILP32(PassBuilder &pb) {
    pb.registerPipelineParsingCallback([](StringRef name, ModulePassManager &mpm,
                                          ArrayRef<PassBuilder::PipelineElement>) {
        if (name != "port-ilp32")
            return false;
        mpm.addPass(ILP32());
        return true;
    });
}
