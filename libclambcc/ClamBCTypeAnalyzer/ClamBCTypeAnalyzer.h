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
//#include "Common/ClamBCUtilities.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/TypeFinder.h>

#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Analysis/FunctionPropertiesAnalysis.h>

#include <vector>
#include <algorithm>

/*TODO: Take these out of hte header file.*/
using namespace llvm;
using namespace std;

#if 0
namespace {
#endif


    class ClamBCTypeAnalysis {
        protected:
            std::vector<llvm::Type*> types;

            virtual void insertType(llvm::Type * type){
                if (types.end() == std::find(types.begin(), types.end(), type)){
                    types.push_back(type);
                }
            }

            virtual void loadTypesFromDefinitions(llvm::Module * pMod){
                TypeFinder tf;
                tf.run(*pMod, false);
                for (auto *type : tf){
                    insertType(type);
                }
            }


            virtual void loadTypesFromAllocas(llvm::Function * pFunc) {

                if (pFunc->isDeclaration()){
                    return;
                }

                /*There should only ever be allocas in the first basic block, so only look there.*/
                BasicBlock * pBB = llvm::dyn_cast<BasicBlock>(pFunc->begin());
                if (nullptr == pBB){
                    ClamBCStop("How is this possible?\n", pFunc);
                }

                /*Allocas are grouped at the top of the block, 
                 * so we are done when we hit our first non-alloca*/
                for (auto i = pBB->begin(), e = pBB->end(); i != e; i++){
                    AllocaInst * pai = llvm::dyn_cast<AllocaInst>(i);
                    if (nullptr == pai){
                        break;
                    }
                    insertType(pai->getType());
                }
            }

            virtual void loadTypesFromVariables(llvm::Module * pMod){
                for (auto i = pMod->begin(), e = pMod->end(); i != e; i++){
                    GlobalVariable * pGlob = llvm::dyn_cast<GlobalVariable>(i);
                    if (pGlob){
                        insertType(pGlob->getType());
                        continue;
                    }

                    Function * pFunc = llvm::dyn_cast<Function> (i);
                    if (pFunc){
                        loadTypesFromAllocas(pFunc);
                    }
                }
            }

            virtual Type * testType(Type * test, const PointerType  * const pType){
                Type * t = test;
                Type * last = t;

                if (test == pType){
                    return test;
                }
                for (size_t j = 0; j < 3; j++){
                    t = PointerType::get(test, pType->getAddressSpace());
                    if (t == pType){
                        return last;
                    }
                    last = t;
                }
                return nullptr;
            }
        public:
            ClamBCTypeAnalysis(){
            }

            virtual void loadTypes(llvm::Module * pMod){
                loadTypesFromDefinitions(pMod);
                loadTypesFromVariables(pMod);

    for (size_t i = 1; i < 65; i++){
        Type * t = IntegerType::getIntNTy(pMod->getContext(), i);
        insertType(t);
    }
            }

            virtual llvm::Type * getPointerElementType(
                    const llvm::Module * const pMod, const PointerType * const pType) {
                for (auto type : types){
                    Type * ret = testType(type, pType);
                    if (nullptr != ret){
                        return ret;
                    }
                }

                DEBUGERR << "Dumping all types\n";
                for (auto type : types){
                    DEBUG_VALUE(type);
                }
                DEBUGERR  << "DONE\n";
                DEBUG_VALUE(pType);

                assert (0 && "NEED TO LOOK AT MORE TYPES");

                return nullptr;
            }
    };

    class ClamBCTypeAnalyzer : public AnalysisInfoMixin<ClamBCTypeAnalyzer >
    {

        protected:
            llvm::Module * pMod = nullptr;
            ClamBCTypeAnalysis result;

        public:

            friend AnalysisInfoMixin<ClamBCTypeAnalyzer > ;
            static AnalysisKey Key;


            ClamBCTypeAnalyzer(){
            }

            typedef ClamBCTypeAnalysis Result;

            ClamBCTypeAnalysis run(llvm::Module & m, llvm::ModuleAnalysisManager & mam){
                pMod = &m;

                result.loadTypes(pMod);

                return result;

            }

    };


#if 0
} // end of anonymous namespace
#endif

//AnalysisKey ClamBCTypeAnalyzer::Key;



