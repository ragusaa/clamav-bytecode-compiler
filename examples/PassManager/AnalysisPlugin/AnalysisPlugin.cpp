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

//#include "Common/clambc.h"
//#include "Common/ClamBCUtilities.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>

#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Analysis/FunctionPropertiesAnalysis.h>

#include <vector>

using namespace llvm;
using namespace std;

/* Modeled after CallGraphAnalysis */

namespace
{

    class AnalysisResult {
        public:
            AnalysisResult(){
                llvm::errs() << "<" << __FUNCTION__ << "::" << __LINE__ << ">" << "<END>\n";
            }

    };

    class ExampleAnalysis : public AnalysisInfoMixin<ExampleAnalysis >
    {

        public:

        friend AnalysisInfoMixin<ExampleAnalysis > ;
            static AnalysisKey Key;


        ExampleAnalysis(){
        }

        typedef AnalysisResult Result;

        AnalysisResult run(llvm::Module & F, llvm::ModuleAnalysisManager & fam){

            llvm::errs() << "<" << "Analysis::" << __LINE__ << ">" << "<END>\n";
            return AnalysisResult();

        }

    };

    class FunctionAnalysisResult {
        public:
            FunctionAnalysisResult(){
                llvm::errs() << "<" << __FUNCTION__ << "::" << __LINE__ << ">" << "<END>\n";
            }

    };

    class ExampleFunctionAnalysis : public AnalysisInfoMixin<ExampleFunctionAnalysis >
    {

        public:

        friend AnalysisInfoMixin<ExampleFunctionAnalysis > ;
            static AnalysisKey Key;


        ExampleFunctionAnalysis(){
        }

        typedef FunctionAnalysisResult Result;

        FunctionAnalysisResult run(llvm::Function & F, llvm::FunctionAnalysisManager & fam){

            llvm::errs() << "<" << "Function Analysis::" << __LINE__ << ">" << "<END>\n";
            return FunctionAnalysisResult();

        }

    };

    AnalysisKey ExampleAnalysis::Key;
    AnalysisKey ExampleFunctionAnalysis::Key;

    struct ExamplePass : public PassInfoMixin<ExamplePass >
    {
        protected:
            Module *pMod = nullptr;
            bool bChanged = false;

        public:

            virtual ~ExamplePass() {}

            PreservedAnalyses run(Module & m, ModuleAnalysisManager & MAM)
            {
                pMod = &m;
                llvm::errs() << "<" << __FUNCTION__ << "::" << __LINE__ << ">" << "Transform Pass" << "<END>\n";

                FunctionAnalysisManager &fam = MAM.getResult<FunctionAnalysisManagerModuleProxy>(*pMod).getManager();

                MAM.getResult<ExampleAnalysis>(m);
                for (auto i = pMod->begin(), e = pMod->end(); i != e; i++){
                    if (Function * pFunc = llvm::dyn_cast<Function>(i)){
                        fam.getResult<ExampleFunctionAnalysis>(*pFunc);
                    }
                }

                llvm::errs() << "<" << __FUNCTION__ << "::" << __LINE__ << ">" << "Transform Pass (leaving)" << "<END>\n";

                return PreservedAnalyses::all();
            }
    }; // end of struct ExamplePass





} // end of anonymous namespace

// This part is the new way of registering your pass
extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "ExamplePass", "v0.1",
            [](PassBuilder &PB) {
                PB.registerPipelineParsingCallback(
                        [](StringRef Name, ModulePassManager &FPM,
                            ArrayRef<PassBuilder::PipelineElement>) {
                        if(Name == "example-pass-with-analysis"){
                        FPM.addPass(ExamplePass());
                        return true;
                        }
                        return false;
                        }
                        );

                PB.registerAnalysisRegistrationCallback(
                        [](ModuleAnalysisManager &mam) {
                            mam.registerPass([] () { return ExampleAnalysis(); } );
                        }
                        );

                PB.registerAnalysisRegistrationCallback(
                        [](FunctionAnalysisManager &mam) {
                            mam.registerPass([] () { return ExampleFunctionAnalysis(); } );
                        }
                        );
            }
    };
}



