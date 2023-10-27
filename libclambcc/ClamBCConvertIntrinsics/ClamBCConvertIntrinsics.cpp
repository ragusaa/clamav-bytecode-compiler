
#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>

#include <llvm/IR/Dominators.h>
#include <llvm/IR/Constants.h>

//#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "Common/clambc.h"

#include <vector>

using namespace llvm;

namespace ClamBCConvertIntrinsics {

#if 0
class ClamBCConvertIntrinsics : public ModulePass
#else
class ClamBCConvertIntrinsics : public PassInfoMixin<ClamBCConvertIntrinsics > 
#endif
{

  public:
    static char ID;

    ClamBCConvertIntrinsics()
        /* : ModulePass(ID) */ {}

    virtual ~ClamBCConvertIntrinsics() {}

#if 0
    virtual bool runOnModule(Module& mod)
#else
    PreservedAnalyses run(Module & mod, ModuleAnalysisManager & MAM)
#endif
    {
        bChanged = false;
        pMod     = &mod;

        for (auto i = pMod->begin(), e = pMod->end(); i != e; i++) {
            Function* pFunc = llvm::cast<Function>(i);
            processFunction(pFunc);
        }

        for (size_t i = 0; i < delLst.size(); i++) {
            delLst[i]->eraseFromParent();
        }

#if 0
        return bChanged;
#else
        if (bChanged){
            return PreservedAnalyses::none();
        }

        return PreservedAnalyses::all();
#endif
    }

  protected:
    Module* pMod  = nullptr;
    bool bChanged = false;
    std::vector<CallInst*> delLst;

    void processFunction(Function* pFunc)
    {

        for (auto i = pFunc->begin(), e = pFunc->end(); i != e; i++) {
            BasicBlock* pBB = llvm::cast<BasicBlock>(i);
            processBasicBlock(pBB);
        }
    }

    void processBasicBlock(BasicBlock* pBB)
    {
        for (auto i = pBB->begin(), e = pBB->end(); i != e; i++) {
            if (CallInst* pci = llvm::dyn_cast<CallInst>(i)) {
                Function * f = pci->getCalledFunction();
                if (nullptr != f) {
                    if ("llvm.memset.p0i8.i64" == f->getName()) {
                        convertMemset(pci);
                    }
                }
            }
        }
    }

    void convertMemset(CallInst* pci)
    {
        std::vector<Value*> args;
        Type* i32Ty = Type::getInt32Ty(pMod->getContext());

#if 0
        for (size_t i = 0; i < pci->getNumArgOperands(); i++)
#else
        for (size_t i = 0; i < pci->arg_size(); i++)
#endif
        {
            Value* pv = pci->getArgOperand(i);
            if (2 == i) {
                if (ConstantInt* ci = llvm::dyn_cast<ConstantInt>(pv)) {
                    pv = ConstantInt::get(i32Ty, ci->getValue().getLimitedValue());
                } else {
                    pv = CastInst::CreateTruncOrBitCast(pv, i32Ty, "ClamBCConvertIntrinsics_trunc_", pci);
                }
            }

            args.push_back(pv);
        }

#if 0
        Constant* f = getNewMemset();
        CallInst::Create(getMemsetType(), f, args, "", pci);
#else
        FunctionCallee * f = getNewMemset();
        CallInst::Create(*f, args, "", pci);
#endif
        delLst.push_back(pci);
    }

    llvm::FunctionCallee* getNewMemset()
    {
        static llvm::FunctionCallee* ret = nullptr;

        if (nullptr == ret) {

            FunctionType* retType = getMemsetType();
            static llvm::FunctionCallee fc = pMod->getOrInsertFunction("llvm.memset.p0i8.i32", retType);
            ret                   = &fc;

            assert(ret && "Could not get memset");
        }

        return ret;
    }

    llvm::FunctionType* getMemsetType()
    {
        static FunctionType* retType = nullptr;
        if (nullptr == retType) {
            LLVMContext& c = pMod->getContext();
            retType        = FunctionType::get(Type::getVoidTy(c),
                                        {Type::getInt8PtrTy(c), Type::getInt8Ty(c), Type::getInt32Ty(c), Type::getInt1Ty(c)},
                                        false);
        }
        return retType;
    }
};

} // end of anonymous namespace

#if 0
char ClamBCConvertIntrinsics::ID = 0;
static RegisterPass<ClamBCConvertIntrinsics> XX("clambc-convert-intrinsics", "Convert Intrinsics to 32-bit",
                                          false /* Only looks at CFG */,
                                          false /* Analysis Pass */);
#else

// This part is the new way of registering your pass
extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "ClamBCConvertIntrinsics", "v0.1",
    [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
        [](StringRef Name, ModulePassManager &FPM,
        ArrayRef<PassBuilder::PipelineElement>) {
          if(Name == "clambc-convert-intrinsics"){
            FPM.addPass(ClamBCConvertIntrinsics::ClamBCConvertIntrinsics());
            return true;
          }
          return false;
        }
      );
    }
  };
}


#endif
