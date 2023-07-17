#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

// only needed for printing
#include  <iostream>

using namespace llvm;

namespace {

struct MyFunctionPass : public PassInfoMixin<MyFunctionPass> {

  // The first argument of the run() function defines on what level
  // of granularity your pass will run (e.g. Module, Function).
  // The second argument is the corresponding AnalysisManager
  // (e.g ModuleAnalysisManager, FunctionAnalysisManager)
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {

    std::cout << "MyFunctionPass in function: " << F.getName().str() << std::endl;

    // Here goes what you want to do with a pass

    // Assuming you did not change anything of the IR code
    return PreservedAnalyses::all();
  }
};
}

// This part is the new way of registering your pass
extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "MyFunctionPass", "v0.1",
    [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
        [](StringRef Name, FunctionPassManager &FPM,
        ArrayRef<PassBuilder::PipelineElement>) {
          if(Name == "my-pass"){
            FPM.addPass(MyFunctionPass());
            return true;
          }
          return false;
        }
      );
    }
  };
}

