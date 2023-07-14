#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/LegacyPassManager.h"

using namespace llvm;

namespace {
struct Migration : public FunctionPass {
  static char ID;
  Migration() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override {
    errs() << "Migration: ";
    errs().write_escaped(F.getName()) << '\n';
    return false;
  }
}; // end of struct Migration
}  // end of anonymous namespace

char Migration::ID = 0;
static RegisterPass<Migration> X("migration", "Migration World Pass",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);

