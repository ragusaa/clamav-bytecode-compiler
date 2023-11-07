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
            llvm::Function * umin = nullptr;
            const char * const UMIN_NAME = ".umin";

            virtual void gatherUMINCalls(Function *pFunc, std::vector<CallInst*> & umin) {
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

            FunctionType * uminType = nullptr;
            IntegerType * uminArgType = nullptr;

            virtual llvm::IntegerType* getUMINArgType(){

                if (nullptr == uminArgType){
                    uminArgType = Type::getInt32Ty(pMod->getContext());
                }

                return uminArgType;
            }

            virtual llvm::FunctionType * getUMINFunctionType(){
                if (nullptr == uminType){
                    IntegerType  * it = getUMINArgType();
                    uminType = FunctionType::get(it, {it, it}, false);
                }
                return uminType;
            }

            virtual llvm::Function * getUMIN(){
                if (nullptr == umin){
                    uint32_t addressSpace = pMod->getDataLayout().getProgramAddressSpace();

                    IntegerType * it = getUMINArgType();
                    FunctionType * ft = getUMINFunctionType();

                    umin = Function::Create(ft, GlobalValue::InternalLinkage, UMIN_NAME, *pMod);
                    Value * pLeft = umin->getArg(0);
                    Value * pRight = umin->getArg(1);
                    BasicBlock * pEntry = BasicBlock::Create(pMod->getContext(), "entry", umin);
                    BasicBlock * pLHS = BasicBlock::Create(pMod->getContext(), "left", umin);
                    BasicBlock * pRHS = BasicBlock::Create(pMod->getContext(), "right", umin);
                    BasicBlock * pRetBlock = BasicBlock::Create(pMod->getContext(), "ret", umin);

                    //entry  block
                    AllocaInst * retVar = new AllocaInst(it, addressSpace , "ret", pEntry);
                    ICmpInst * cmp = new ICmpInst(*pEntry, CmpInst::ICMP_ULT, pLeft, pRight, "icmp");
                    BranchInst::Create(pLHS, pRHS, cmp,  pEntry);

                    //left < right
                    new StoreInst (pLeft, retVar, pLHS);
                    BranchInst::Create(pRetBlock, pLHS);

                    //right >= left
                    new StoreInst (pRight, retVar, pRHS);
                    BranchInst::Create(pRetBlock, pRHS);

                    LoadInst * pli = new LoadInst(it, retVar, "load", pRetBlock);
                    ReturnInst::Create(pMod->getContext(), pli, pRetBlock);
                }

                return umin;
            }

            virtual void processFunction(Function *pFunc) {
                vector<CallInst*> calls;
                gatherUMINCalls(pFunc, calls);

                for (size_t i = 0; i < calls.size(); i++){
                    bChanged = true;
                    CallInst * pci = calls[i];

                    Function * umin = getUMIN();

                    BasicBlock * pBlock = pci->getParent();

                    std::vector<Value*> args;
                    for (size_t i = 0; i < pci->arg_size(); i++){
                        args.push_back(pci->getArgOperand(i));
                    }
                    CallInst * pNew = CallInst::Create(getUMINFunctionType(), umin, args, "ClamBCRemoveUMIN_",  pci);
                    pci->replaceAllUsesWith(pNew);
                    pci->eraseFromParent();

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



