#include "llvm/Pass.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/DenseMap.h"

#include <set>


using namespace llvm;

namespace {

  struct Tlayout : public ModulePass {

    static const StringRef FUNCNAME_PTHREAD_CREATE;
    static const StringRef FUNCNAME_PTHREAD_ATTR_SETAFFINITY_NP;

    static char ID;
    Tlayout() : ModulePass(ID) {}

    bool runOnModule(Module &M);

  private:
    DenseMap<CallInst*, Instruction*> Thread2AffinityMap; // thread_create Inst -> setaffinity Inst
    // std::set<StringRef> ThreadFuncNameSet;

    DenseMap<Instruction*, std::set<Instruction*> > ThreadGlobalDataMap;


    void createThreadInfoRecord(Module &M, const bool DEBUG = false);
    void createGlobalDataRecord(Module &M, const bool DEBUG = false);
    void findDataOverlapping(const bool DEBUG = false);
    void optimizeThreadLocation(const bool DEBUG = false);

    void dfsRootAncestor(Instruction* threadInst, Instruction* child, const bool DEBUG = false);
  }; // end of struct Hello

const StringRef Tlayout::FUNCNAME_PTHREAD_CREATE = StringRef("pthread_create");
const StringRef Tlayout::FUNCNAME_PTHREAD_ATTR_SETAFFINITY_NP = StringRef("pthread_attr_setaffinity_np");

}  // end of anonymous namespace

char Tlayout::ID = 0;
static RegisterPass<Tlayout> X("tlayout", "Hello World Pass");


bool Tlayout::runOnModule(Module &M) {

  /* TODO: add comments to explain the function
   */
  createThreadInfoRecord(M);

  /* TODO: explain the function
   */
  createGlobalDataRecord(M, true);

  /* TODO: explain the function
  */
  findDataOverlapping();

  /* TODO: explain the function
  */
  optimizeThreadLocation();

  return false;
}

void Tlayout::createThreadInfoRecord(Module &M, const bool DEBUG) {

  for (Module::iterator F = M.begin(); F != M.end(); ++F) {
    if (DEBUG) errs() << "Func Name: " << F->getName() << '\n';
    
    for (Function::iterator BB = F->begin(); BB != F->end(); ++BB) {
      for (BasicBlock::iterator II = BB->begin(); II != BB->end(); ++II) {
        
        // Find the CallInst
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

          // StringRef threadFuncName = callInst->getArgOperand(2)->getName();
          // ThreadFuncNameSet.insert(threadFuncName);


        } else if (funcName.equals(FUNCNAME_PTHREAD_ATTR_SETAFFINITY_NP)) {
          if (DEBUG) errs() << "\t";
          if (DEBUG) errs().write_escaped(funcName) << '\n';

        }
        
      } //end for Instr
    } // end for BB
  } // end for Function

}

void Tlayout::dfsRootAncestor(Instruction* threadInst, Instruction* child, const bool DEBUG){
  if (!child){
    return;
  }

  if (isa<AllocaInst>(child) && !child->getNumOperands()){
    ThreadGlobalDataMap[threadInst].insert(child);
  }

  for (unsigned i = 0; i != child->getNumOperands(); ++i){
    dfsRootAncestor(threadInst, dyn_cast<Instruction>(child->getOperand(i)), DEBUG);
  }
}

void Tlayout::createGlobalDataRecord(Module &M, const bool DEBUG) {

  for (DenseMap<CallInst*, Instruction*>::iterator MI = Thread2AffinityMap.begin();
    MI != Thread2AffinityMap.end(); ++MI) {

    CallInst *threadCreateInst = MI->first;
    errs() << *threadCreateInst << '\n';
    StringRef funcName = threadCreateInst->getArgOperand(2)->getName();
    Function *threadFunc = M.getFunction(funcName);
    errs() << threadFunc->getName() << '\n';

    Value *arg = threadCreateInst->getArgOperand(3);
    errs() << *arg << '\n';

    //find ancestor 
    /*
    Instruction *argInst = dyn_cast<Instruction>(arg);
    for (unsigned i = 0; i < argInst->getNumOperands(); ++i) {
      Value *operand = argInst->getOperand(i);
      errs() << '\t' << *operand << '\n';
    }
    */
    dfsRootAncestor(threadCreateInst, dyn_cast<Instruction>(arg), DEBUG);
    /*
    for (size_t i = 0; i != ThreadGlobalDataMap[threadCreateInst].size(); ++i){
      errs() <<'\t'<< *(ThreadGlobalDataMap[threadCreateInst] + i) << '\n';
    }
    */
    errs() << "~~~~~~~ size  " << ThreadGlobalDataMap[threadCreateInst].size() << '\n';
    
    for (std::set<Instruction*>::iterator SI = ThreadGlobalDataMap[threadCreateInst].begin(); SI != ThreadGlobalDataMap[threadCreateInst].end(); ++SI){
      errs() <<"\t\t"<< *SI << '\n';
    }
  }



}


void Tlayout::findDataOverlapping(const bool DEBUG) {
  return;
}

void Tlayout::optimizeThreadLocation(const bool DEBUG) {
  return;
}

