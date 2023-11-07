/*
 *  Compile LLVM bytecode to ClamAV bytecode.
 *
 *  Copyright (C) 2020-2023 Sourcefire, Inc.
 *
 *  Authors: Andy Ragusa
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

#include "Common/clambc.h"
#include "Common/ClamBCUtilities.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>

#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>

#include <vector>

using namespace llvm;
using namespace std;

namespace
{
    /*
     * Remove umin intrinsic because it's not supported by our runtime.
     */
    struct ClamBCRemoveUMIN : public PassInfoMixin<ClamBCRemoveUMIN > 
    {
        protected:
            Module *pMod = nullptr;
            bool bChanged = false;

            virtual void gatherUMIN(Function *pFunc, std::vector<CallInst*> & umin) {
                for (auto fi = pFunc->begin(), fe = pFunc->end(); fi != fe; fi++){
                    BasicBlock * pBB = llvm::cast<BasicBlock>(fi);
                    for (auto bi = pBB->begin(), be = pBB->end(); bi != be; bi++){
                        if (CallInst * pci = llvm::dyn_cast<CallInst>(bi)){
                            Function * pCalled = pci->getCalledFunction();
                            if (pCalled->isIntrinsic()){




                                DEBUG_VALUE(pci);
                                DEBUG_VALUE(pCalled);
                                if ("llvm.umin.i32" == pCalled->getName()) {

                                    DEBUG_VALUE(pci);
                                    umin.push_back(pci);
                                }
                            }
                        }
                    }
                }

            }

            virtual void processFunction(Function *pFunc) {
                vector<CallInst*> umin;
                gatherUMIN(pFunc, umin);

                for (size_t i = 0; i < umin.size(); i++){
                    bChanged = true;
                    

#if 0
                    FreezeInst * pfi = umin[i];
                    pfi->replaceAllUsesWith(pfi->getOperand(0));
                    pfi->eraseFromParent();
#endif
                }
            }

        public:

            virtual ~ClamBCRemoveUMIN() {}

            PreservedAnalyses run(Module & m, ModuleAnalysisManager & MAM)
            {
                pMod = &m;

                DEBUGERR  << "TODO: ADD umin detection to the validator" << "<END>\n";

                for (auto i = pMod->begin(), e = pMod->end(); i != e; i++) {
                    Function *pFunc = llvm::cast<Function>(i);
                    if (pFunc->isDeclaration()) {
                        continue;
                    }

                    processFunction(pFunc);
                }

                if (bChanged){
                    return PreservedAnalyses::none();
                } else{
                    return PreservedAnalyses::all();
                }
            }
    }; // end of struct ClamBCRemoveUMIN

} // end of anonymous namespace

// This part is the new way of registering your pass
extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "ClamBCRemoveUMIN", "v0.1",
            [](PassBuilder &PB) {
                PB.registerPipelineParsingCallback(
                        [](StringRef Name, ModulePassManager &FPM,
                            ArrayRef<PassBuilder::PipelineElement>) {
                        if(Name == "clambc-remove-umin"){
                        FPM.addPass(ClamBCRemoveUMIN());
                        return true;
                        }
                        return false;
                        }
                        );
            }
    };
}



