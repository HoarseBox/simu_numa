#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/DenseMap.h"

#include <set>

#include "tlayout.h"


using namespace llvm;

// namespace {

//   struct Tlayout : public FunctionPass {
//     static const bool DEBUG = true;
//     static const StringRef FUNCNAME_PTHREAD_CREATE;
//     static const StringRef FUNCNAME_PTHREAD_ATTR_SETAFFINITY_NP;

//     static char ID;
//     Tlayout() : FunctionPass(ID) {}

//     bool runOnFunction(Function &F);

//     DenseMap<CallInst*, Instruction*> Thread2AffinityMap; // thread_create Inst -> setaffinity Inst
//     std::set<StringRef> ThreadFuncNameSet;
//   }; // end of struct Hello

const StringRef Tlayout::FUNCNAME_PTHREAD_CREATE = StringRef("pthread_create");
const StringRef Tlayout::FUNCNAME_PTHREAD_ATTR_SETAFFINITY_NP = StringRef("pthread_attr_setaffinity_np");

// }  // end of anonymous namespace

char Tlayout::ID = 0;
static RegisterPass<Tlayout> X("tlayout", "Hello World Pass");


bool Tlayout::runOnFunction(Function &F) {
  if (DEBUG) errs() << "Func Name: ";
  if (DEBUG) errs().write_escaped(F.getName());
  if (DEBUG) errs() << '\t' << &F << '\n';

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
        Value *threadAttr = callInst->getArgOperand(1);
        for (Value::use_iterator UI = threadAttr->use_begin(), E = threadAttr->use_end(); UI != E; ++UI){
          Instruction* user = dyn_cast<Instruction>(*UI);
          if (DEBUG) errs() << "\t\t" << *user << '\n';
          if (user == callInst) {
            UI++;
            if (UI == E) {
              errs() << "ERROR: didn't set affinity before create thread!\n";
              break; 
            }
            Instruction* setAffinityInst = dyn_cast<Instruction>(*UI);
            Thread2AffinityMap[callInst] = setAffinityInst;
            if (DEBUG) errs() << "\t\t\t" << *setAffinityInst << '\n';
            break;
          }
        }

        StringRef threadFuncName = callInst->getArgOperand(2)->getName();
        ThreadFuncNameSet.insert(threadFuncName);


      } else if (funcName.equals(FUNCNAME_PTHREAD_ATTR_SETAFFINITY_NP)) {
        if (DEBUG) errs() << "\t";
        if (DEBUG) errs().write_escaped(funcName) << '\n';

      }
      
    }
  }

  // if (DEBUG) {
  //   errs() << "Thread2AffinityMap:\n";
  //   for (DenseMap<CallInst*, Instruction*>::iterator DMI = Thread2AffinityMap.begin(); DMI != Thread2AffinityMap.end(); ++DMI) {
  //     errs() << *(DMI->first) << "\t" << *(DMI->second) << "\n";
  //   }
  // }

  return false;
}
