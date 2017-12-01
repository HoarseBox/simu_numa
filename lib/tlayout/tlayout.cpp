#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/DenseMap.h"

using namespace llvm;

namespace {

  struct Tlayout : public FunctionPass {
    static const bool DEBUG = true;
    static const StringRef FUNCNAME_PTHREAD_CREATE;
    static const StringRef FUNCNAME_PTHREAD_ATTR_SETAFFINITY_NP;

    static char ID;
    Tlayout() : FunctionPass(ID) {}

    bool runOnFunction(Function &F);

  private:
    DenseMap<CallInst*, Argument*> ThreadInfo; //TODO: thread_id -> func_called_by_thread
  }; // end of struct Hello

  const StringRef Tlayout::FUNCNAME_PTHREAD_CREATE = StringRef("pthread_create");
  const StringRef Tlayout::FUNCNAME_PTHREAD_ATTR_SETAFFINITY_NP = StringRef("pthread_attr_setaffinity_np");

}  // end of anonymous namespace

char Tlayout::ID = 0;
static RegisterPass<Tlayout> X("tlayout", "Hello World Pass",
                              false /* Only looks at CFG */,
                              false /* Analysis Pass */);


bool Tlayout::runOnFunction(Function &F) {
  if (DEBUG) errs() << "Func Name: ";
  if (DEBUG) errs().write_escaped(F.getName()) << '\n';

  // Find the CallInst
  for (Function::iterator BB = F.begin(); BB != F.end(); ++BB) {
    for (BasicBlock::iterator II = BB->begin(); II != BB->end(); ++II) {
      Instruction &I = *II;
      if (!isa<CallInst>(I)) {
        continue;
      }
      CallInst *callInst = dyn_cast<CallInst>(&I);
      Function *calledFunc = callInst->getCalledFunction();
      if (!calledFunc) {
        errs() << "\tWARN: Indirect function call.\n";
        continue;
      }

      // Find function call of pthread_create and pthread_attr_setaffinity_np
      StringRef funcName = calledFunc->getName();
      if (funcName.equals(FUNCNAME_PTHREAD_CREATE)) {
        if (DEBUG) errs() << "\t";
        if (DEBUG) errs().write_escaped(funcName) << '\n';

        // Find the function called by a thread
        unsigned arg_index = 0;
        for (Function::arg_iterator AI = calledFunc->arg_begin(); AI != calledFunc->arg_end(); ++AI) {
          if (DEBUG) errs() << "\t\t" << *AI << "\n";
          if (arg_index == 2) { //3rd argument is the thread function name

            // TODO: the 3rd argument(function called by the thread) is a pointer. 
            // The pointer cannot be dereferenced in the compile time. 
            // How to know which function this pointer is pointing to?
            // What we want is mapping thread_id -> function
            ThreadInfo[callInst] = AI; // TODO
          }
          arg_index++;
        }

      } else if (funcName.equals(FUNCNAME_PTHREAD_ATTR_SETAFFINITY_NP)) {
        if (DEBUG) errs() << "\t";
        if (DEBUG) errs().write_escaped(funcName) << '\n';

      }
      
    }
  }

  if (DEBUG) {
    errs() << "ThreadInfo:\n";
    for (DenseMap<CallInst*, Argument*>::iterator DMI = ThreadInfo.begin(); DMI != ThreadInfo.end(); ++DMI) {
      errs() << *(DMI->first) << "\t" << *(DMI->second) << "\n";
    }
  }

  return false;
}
