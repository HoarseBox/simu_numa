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
    static const unsigned int NUM_CORES;

    static char ID;
    Tlayout() : ModulePass(ID) {}

    bool runOnModule(Module &M);

  private:
    DenseMap<CallInst*, CallInst*> Thread2AffinityMap; // thread_create Inst -> setaffinity Inst

    DenseMap<CallInst*, Instruction*> Thread2CPUSetInstMap;

    DenseMap<Instruction*, std::set<Instruction*> > ThreadGlobalDataMap;

    DenseMap<Instruction*, std::set<CallInst*> > Data2Threads;

    DenseMap<CallInst*, int> Thread2NewCoreNum;

    //DenseMap<int, std::set<CallInst*> > coreNum2Threads;


    void createThreadInfoRecord(Module &M, const bool DEBUG = false);
    void createGlobalDataRecord(Module &M, const bool DEBUG = false);
    void findDataOverlapping(const bool DEBUG = false);
    void optimizeThreadLocation(const bool DEBUG = false);

    void dfsRootAncestor(Instruction* threadInst, Instruction* child, const bool DEBUG = false);
  }; // end of struct Hello

const StringRef Tlayout::FUNCNAME_PTHREAD_CREATE = StringRef("pthread_create");
const StringRef Tlayout::FUNCNAME_PTHREAD_ATTR_SETAFFINITY_NP = StringRef("pthread_attr_setaffinity_np");
const unsigned int Tlayout::NUM_CORES = 8;

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
              //errs() << "\t\t"<< dyn_cast<Value>(*UI)->getArgOperand(2) << '\n';

              // Instruction* setAffinityInst = dyn_cast<Instruction>(*UI);
              CallInst* setAffinityInst = dyn_cast<CallInst>(*UI);
              errs() << "\t\t"<< *(setAffinityInst->getArgOperand(2)) << '\n';
              //BasicBlock* pBB = setAffinityInst->getParent();
              LLVMBasicBlockRef currentBBRef = wrap(setAffinityInst->getParent());
              LLVMBasicBlockRef setCoreBBRef;
              for (int i = 0; i < 3; ++i){
                setCoreBBRef = LLVMGetPreviousBasicBlock(currentBBRef);
                currentBBRef = setCoreBBRef;
              }
              BasicBlock* setCoreBB = unwrap(setCoreBBRef);
              for (BasicBlock::iterator I = setCoreBB->begin(); I != setCoreBB->end(); ++I){
                Instruction* temp = dyn_cast<Instruction>(I);
                if (I && isa<StoreInst>(temp)){
                  //errs() << *I << '\n';
                  //errs() << I->getNumOperands() << '\n';
                  Thread2CPUSetInstMap[callInst] = temp;
                }
              }
              //errs() << *(unwrap(setCoreBBRef)) << '\n';
              /*
              errs() << *pBB << '\n';
              LLVMBasicBlockRef previousBBRef = LLVMGetPreviousBasicBlock(wrap(pBB));
              errs() << *(unwrap(previousBBRef)) << '\n';
              LLVMBasicBlockRef ppBBRef = LLVMGetPreviousBasicBlock(previousBBRef);
              errs() << *(unwrap(ppBBRef)) << '\n';
              LLVMBasicBlockRef pppBBRef = LLVMGetPreviousBasicBlock(ppBBRef);
              errs() << *(unwrap(pppBBRef)) << '\n';
              */
              //Instruction* setCPUInst = dyn_cast<Instruction>(setAffinityInst->getArgOperand(2));
              //errs() << "\t\tset CPU operand "<< *(setCPUInst->getOperand(0)) << '\n';

              Thread2AffinityMap[callInst] = setAffinityInst;
              
              //Thread2CPUMap[callInst] = 
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
/*
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
*/

void Tlayout::createGlobalDataRecord(Module &M, const bool DEBUG) {
  // TODO: serialized threads


  // TODO: parallelized threads
  // find mutex, if two thread functions acquire the same mutex, they use the shared data


  for (DenseMap<CallInst*, CallInst*>::iterator MI = Thread2AffinityMap.begin();
    MI != Thread2AffinityMap.end(); ++MI) {

    CallInst *threadCreateInst = MI->first;
    errs() << *threadCreateInst << '\n';
    StringRef funcName = threadCreateInst->getArgOperand(2)->getName();
    Function *threadFunc = M.getFunction(funcName);
    errs() << threadFunc->getName() << '\n';

    Value *arg = threadCreateInst->getArgOperand(3);
    errs() << arg << '\n';
    Instruction* argInst = dyn_cast<Instruction>(arg);
    /*
    errs() << "argInst\t\t" << *argInst << '\n';

    Instruction* originalData = dyn_cast<Instruction>(argInst->getOperand(0));
    Data2Threads[originalData].insert(threadCreateInst);
    */





    //find ancestor 
    /*
    Instruction *argInst = dyn_cast<Instruction>(arg);
    for (unsigned i = 0; i < argInst->getNumOperands(); ++i) {
      Value *operand = argInst->getOperand(i);
      errs() << '\t' << *operand << '\n';
    }
    */
    //dfsRootAncestor(threadCreateInst, dyn_cast<Instruction>(arg), DEBUG);
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
  unsigned int currentCore = 0;
  for (DenseMap<Instruction*, std::set<CallInst*> >::iterator MI = Data2Threads.begin();
    MI != Data2Threads.end(); ++MI){
    std::set<CallInst*> accessCommonDataThreads = MI->second;
    currentCore %= NUM_CORES;
    //coreNum2Threads[currentCore] = accessCommonDataThreads;
    for (std::set<CallInst*>::iterator SI = accessCommonDataThreads.begin(); SI != accessCommonDataThreads.end(); ++SI){
      CallInst* threadCreateInst = dyn_cast<CallInst>(*SI);
      Thread2NewCoreNum[threadCreateInst] = currentCore;
    }
    currentCore++;
  }


  return;
}

void Tlayout::optimizeThreadLocation(const bool DEBUG) {
  for (DenseMap<CallInst*, CallInst*>::iterator MI = Thread2AffinityMap.begin();
    MI != Thread2AffinityMap.end(); ++MI){
    CallInst* threadCreateInst = dyn_cast<CallInst>(MI->first);
    Instruction* setCoreInst = Thread2CPUSetInstMap[threadCreateInst];
    int coreNum = Thread2NewCoreNum[threadCreateInst];
    errs() << coreNum << '\n';
    errs() << *threadCreateInst << '\n';
  } 
  return;
}


