#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/DenseMap.h"

#include <set>

#include "tlayout.h"

using namespace llvm;

namespace {

  struct GlobalData : public ModulePass {
    static const bool DEBUG = true;

    static char ID;
    GlobalData() : ModulePass(ID) {}

    bool runOnModule(Module &M);

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<Tlayout>();
    }

  private:
    Tlayout *TL;

  }; // end of struct Hello


}  // end of anonymous namespace

char GlobalData::ID = 0;
static RegisterPass<GlobalData> X("global-data", "Find Global Data Pass",
                                  false /* Only looks at CFG */,
                                  false /* Analysis Pass */);


bool GlobalData::runOnModule(Module &M) {

  TL = &getAnalysis<Tlayout>();

  std::set<StringRef> *ThreadFuncNameSet = &TL->ThreadFuncNameSet;

  for (std::set<StringRef>::iterator SI = ThreadFuncNameSet->begin(); SI != ThreadFuncNameSet->end(); ++SI) {
    errs() << *SI << '\n';
  }

  // if (DEBUG) errs() << "Func Name: ";
  // if (DEBUG) errs().write_escaped(F.getName());
  // if (DEBUG) errs() << '\t' << &F << '\n';

  // Find the CallInst
  // for (Module::GlobalListType::iterator FF = M.begin(); FF != M.end(); ++FF) {
  //   for (Function::iterator BB = FF->begin(); BB != FF->end(); ++BB) {
  //     for (BasicBlock::iterator II = BB->begin(); II != BB->end(); ++II) {
  //       Instruction &I = *II;
  //     }
  //   }    
  // }


  return false;
}
