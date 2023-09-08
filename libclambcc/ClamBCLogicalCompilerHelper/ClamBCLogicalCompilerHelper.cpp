/*
 *  Compile LLVM bytecode to logical signatures.
 *
 *  Copyright (C) 2009-2010 Sourcefire, Inc.
 *
 *  Authors: Török Edvin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301, USA.
 */

#include "Common/ClamBCModule.h"
#include "Common/clambc.h"
#include "Common/bytecode_api.h"
#include "Common/ClamBCDiagnostics.h"
#include "Common/ClamBCCommon.h"
#include "Common/ClamBCUtilities.h"

#include <llvm/Support/DataTypes.h>
#include <llvm/ADT/FoldingSet.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/IR/ConstantRange.h>
#include <llvm/Support/Debug.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Process.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils/Local.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/IR/Type.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/LinkAllPasses.h>

/*
 * Since the logical compiler requires 'setvirusname' to only be called with a string constant,
 * we are going to undo the PHI nodes added by O3 that would have to
 *
 *
 * Consider the code 

 return.sink.split:                                ; preds = %if.end39, %for.end
 %.str.1.sink = phi ptr [ @.str, %for.end ], [ @.str.1, %if.end39 ]
 %call.i70 = call i32 @setvirusname(ptr noundef nonnull %.str.1.sink, i32 noundef 0) #6
 br label %return

 We will just add the calls to setvirusname to the predecessor basic blocks.
 *
 *
 */


#define DEBUG_TYPE "lsigcompilerhelper"

using namespace llvm;

namespace ClamBCLogicalCompilerHelper
{

    class ClamBCLogicalCompilerHelper : public PassInfoMixin<ClamBCLogicalCompilerHelper> 
    {
        public:
            ClamBCLogicalCompilerHelper() {}

            virtual PreservedAnalyses run(Module & m, ModuleAnalysisManager & MAM);
            virtual void getAnalysisUsage(AnalysisUsage &AU) const
            {
            }

        protected:
            llvm::Module *pMod;
            bool bChanged = false;

            virtual void populateArgs(const CallInst * pci, std::vector<Value*> & args){
                for (auto i = pci->arg_begin(), e = pci->arg_end(); i != e; i++){
                    args.push_back(llvm::dyn_cast<Value>(i));
                }
            }
            virtual void processPHI(PHINode * phi, Function * pCalledFunction, std::vector<Value*> & args);

            virtual void fixupSetVirusNameCalls();
    };


    /*
     * Add calls to setvirusname for each constant string, rather allowing a phinode to
     * choose the string.  This is a requirement for ClamBCLogicalCompiler.
     */
    void ClamBCLogicalCompilerHelper::processPHI(PHINode * phi, Function * pCalledFunction, std::vector<Value*> & args){

        for (size_t i = 0; i < phi->getNumIncomingValues(); i++){
            BasicBlock * pBB = phi->getIncomingBlock(i);
            Value * pVal = phi->getIncomingValue(i);

            Instruction * pTerm = pBB->getTerminator();
            args[0] = pVal;

            CallInst::Create(pCalledFunction->getFunctionType(), pCalledFunction, args, "ClamBCLogicalCompilerHelper_callInst", pTerm);
        }

    }


    /*
     * Find all calls to setvirusname, and make sure they aren't loading the
     * first argument from a variable.
     */
    void ClamBCLogicalCompilerHelper::fixupSetVirusNameCalls(){

        std::vector<Instruction *> erase;
        Function *svn = pMod->getFunction("setvirusname");
        if (nullptr == svn){
            return;
        }
        for (auto iter : svn->users()) {
            if (CallInst * pci = llvm::dyn_cast<CallInst>(iter)){
                bChanged = true;
                Value * operand = pci->getOperand(0);

                if (PHINode * phi = llvm::dyn_cast<PHINode>(operand)){
                    std::vector<Value*> args;
                    populateArgs(pci, args);
                    processPHI(phi, svn, args);
                    erase.push_back(pci);
                    /*This leaves a block with only a branch instruction (essentially empty).
                     * I don't think that is an issue, but consider removing it.*/
                    erase.push_back(phi);
                }
            }
        }

        for (size_t i = 0; i < erase.size(); i++){
            erase[i]->eraseFromParent();
        }

    }


    PreservedAnalyses ClamBCLogicalCompilerHelper::run(Module & mod, ModuleAnalysisManager & mam)
    {
        pMod = &mod;

        fixupSetVirusNameCalls();

        if (bChanged){
            return PreservedAnalyses::none();
        }

        return PreservedAnalyses::all();
    }

    // This part is the new way of registering your pass
    extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
        llvmGetPassPluginInfo() {
            return {
                LLVM_PLUGIN_API_VERSION, "ClamBCLogicalCompilerHelper", "v0.1",
                    [](PassBuilder &PB) {
                        PB.registerPipelineParsingCallback(
                                [](StringRef Name, ModulePassManager &FPM,
                                    ArrayRef<PassBuilder::PipelineElement>) {
                                if(Name == "clambc-lcompiler-helper"){
                                FPM.addPass(ClamBCLogicalCompilerHelper());
                                return true;
                                }
                                return false;
                                }
                                );
                    }
            };
        }

} // namespace


