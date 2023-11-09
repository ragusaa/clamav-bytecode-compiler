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
     * Remove smax intrinsic because it's not supported by our runtime.
     */
    struct ClamBCRemoveSMAX : public PassInfoMixin<ClamBCRemoveSMAX > 
    {
        protected:
            Module *pMod = nullptr;
            const char * const SMAX_NAME = ".smax";

            FunctionType * smaxType = nullptr;

            virtual llvm::IntegerType* addSMAXArgType(){
                return Type::getInt32Ty(pMod->getContext());
            }

            virtual llvm::FunctionType * getSMAXFunctionType(){
                IntegerType  * it = addSMAXArgType();
                return FunctionType::get(it, {it, it}, false);
            }

            virtual llvm::Function * addSMAX(){
                uint32_t addressSpace = pMod->getDataLayout().getProgramAddressSpace();

                IntegerType * it = addSMAXArgType();
                FunctionType * ft = getSMAXFunctionType();

                llvm::Function * smax = Function::Create(ft, GlobalValue::InternalLinkage, SMAX_NAME, *pMod);
                Value * pLeft = smax->getArg(0);
                Value * pRight = smax->getArg(1);
                BasicBlock * pEntry = BasicBlock::Create(pMod->getContext(), "entry", smax);
                BasicBlock * pLHS = BasicBlock::Create(pMod->getContext(), "left", smax);
                BasicBlock * pRHS = BasicBlock::Create(pMod->getContext(), "right", smax);
                BasicBlock * pRetBlock = BasicBlock::Create(pMod->getContext(), "ret", smax);

                //entry  block
                AllocaInst * retVar = new AllocaInst(it, addressSpace , "ret", pEntry);
                ICmpInst * cmp = new ICmpInst(*pEntry, CmpInst::ICMP_SGT, pLeft, pRight, "icmp");
                BranchInst::Create(pLHS, pRHS, cmp,  pEntry);

                //left < right
                new StoreInst (pLeft, retVar, pLHS);
                BranchInst::Create(pRetBlock, pLHS);

                //right >= left
                new StoreInst (pRight, retVar, pRHS);
                BranchInst::Create(pRetBlock, pRHS);

                LoadInst * pli = new LoadInst(it, retVar, "load", pRetBlock);
                ReturnInst::Create(pMod->getContext(), pli, pRetBlock);
                return smax;
            }

        public:

            virtual ~ClamBCRemoveSMAX() {}

            PreservedAnalyses run(Module & m, ModuleAnalysisManager & MAM)
            {
                pMod = &m;

                DEBUGERR  << "TODO: ADD smax detection to the validator" << "<END>\n";
                const char * const  INTRINSIC_NAME = "llvm.smax.i32";

                std::vector<CallInst*> calls;
                gatherCallsToIntrinsic(pMod, INTRINSIC_NAME, calls);
                if (calls.size()){
                    Function * smax = addSMAX();
                    replaceAllCalls(getSMAXFunctionType(), smax, calls, "ClamBCRemoveSMAX_");

                    return PreservedAnalyses::none();
                }

                return PreservedAnalyses::all();
            }

    }; // end of struct ClamBCRemoveSMAX

} // end of anonymous namespace

// This part is the new way of registering your pass
extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "ClamBCRemoveSMAX", "v0.1",
            [](PassBuilder &PB) {
                PB.registerPipelineParsingCallback(
                        [](StringRef Name, ModulePassManager &FPM,
                            ArrayRef<PassBuilder::PipelineElement>) {
                        if(Name == "clambc-remove-smax"){
                        FPM.addPass(ClamBCRemoveSMAX());
                        return true;
                        }
                        return false;
                        }
                        );
            }
    };
}



