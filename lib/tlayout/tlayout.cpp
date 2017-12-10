#include "llvm/Pass.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/DenseMap.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ProfileInfo.h"

#include <set>


using namespace llvm;

namespace {

  struct Tlayout : public ModulePass {

    static const StringRef FUNCNAME_PTHREAD_CREATE;
    static const StringRef FUNCNAME_PTHREAD_ATTR_SETAFFINITY_NP;
    static const StringRef FUNCNAME_PTHREAD_MUTEX_LOCK;
    static const StringRef FUNCNAME_PTHREAD_MUTEX_UNLOCK;
    static const unsigned int NUM_CORES;

    static char ID;
    Tlayout() : ModulePass(ID) {}
    virtual void getAnalysisUsage(AnalysisUsage &AU) const{
      AU.addRequired<LoopInfo>();
      AU.addRequired<ProfileInfo>();
    }
    bool runOnModule(Module &M);

  private:
    DenseMap<CallInst*, CallInst*> Thread2AffinityMap; // thread_create Inst -> setaffinity Inst

    DenseMap<CallInst*, Instruction*> Thread2CPUSetInstMap;
    DenseMap<CallInst*, Instruction*> Thread2CPUIdInstMap;

    DenseMap<Instruction*, std::set<Instruction*> > ThreadGlobalDataMap;

    DenseMap<Instruction*, std::set<CallInst*> > Data2Threads;

    LoopInfo *LI;
    ProfileInfo* PI;
    DenseMap<CallInst*, int> Thread2NewCoreNum;

    //DenseMap<int, std::set<CallInst*> > coreNum2Threads;


    void createThreadInfoRecord(Module &M, const bool DEBUG = false);
    void createGlobalDataRecord(Module &M, const bool DEBUG = false);
    void findDataOverlapping(Module &M, const bool DEBUG = false);
    bool optimizeThreadLocation(const bool DEBUG = false);

    // void dfsRootAncestor(Instruction* threadInst, Instruction* child, const bool DEBUG = false);

  }; // end of struct Hello

  const StringRef Tlayout::FUNCNAME_PTHREAD_CREATE = StringRef("pthread_create");
  const StringRef Tlayout::FUNCNAME_PTHREAD_ATTR_SETAFFINITY_NP = StringRef("pthread_attr_setaffinity_np");
  const StringRef Tlayout::FUNCNAME_PTHREAD_MUTEX_LOCK = StringRef("pthread_mutex_lock");
  const StringRef Tlayout::FUNCNAME_PTHREAD_MUTEX_UNLOCK = StringRef("pthread_mutex_unlock");
  const unsigned int Tlayout::NUM_CORES = 8;

}  // end of anonymous namespace

char Tlayout::ID = 0;
static RegisterPass<Tlayout> X("tlayout", "Optimize Thread Layout on Multicore");


bool Tlayout::runOnModule(Module &M) {

  bool changed = false;

  /* TODO: add comments to explain the function
   */
  createThreadInfoRecord(M, true);

  /* TODO: explain the function
   */
  createGlobalDataRecord(M, true);

  /* TODO: explain the function
  */
  findDataOverlapping(M, true);

  /* TODO: explain the function
  */
  changed = optimizeThreadLocation(true);

  return true;
}

void Tlayout::createThreadInfoRecord(Module &M, const bool DEBUG) {
  if (DEBUG) errs() << "----------------createThreadInfoRecord--------------\n";

  // for (Module::iterator F = M.begin(); F != M.end(); ++F) {
  //   if (F->isDeclaration()) {
  //     continue;
  //   }
  //   Function &func = *F;
  //   LoopInfo &testLI = getAnalysis<LoopInfo>(func);
  //   PI = &getAnalysis<ProfileInfo>();
  

  //   for (LoopInfo::iterator LIT = testLI.begin(); LIT!=testLI.end(); LIT++){
      
  //     // need to make sure the loop contains "pthread_create"
  //     bool containPthreadCreate = false;
  //     int forIterationCounts = 0;

  //     for (Loop::block_iterator bb = (*LIT)->block_begin(); bb!=(*LIT)->block_end(); bb++){
  //       for (BasicBlock::iterator II = (*bb)->begin(); II!=(*bb)->end(); II++){
  //         if (DEBUG) errs() << *II << "\n";
  //         // Find the CallInst
  //         Instruction &I = *II;
  //         if (!isa<CallInst>(I)) {
  //           continue;
  //         }
  //         CallInst *callInst = dyn_cast<CallInst>(&I);
  //         Function *calledFunc = callInst->getCalledFunction();
  //         if (!calledFunc) {
  //           errs() << "\tWARN: Indirect function call.\n";
  //           continue;
  //         }

  //         // find the pthread create function and get the function name 
  //         StringRef funcName = calledFunc->getName();
  //         if (funcName.equals(FUNCNAME_PTHREAD_CREATE)) {
  //           errs().write_escaped(funcName)<<'\n';
  //           containPthreadCreate = true;
  //           break;
  //         }
  //       }
  //       if (containPthreadCreate) {
  //         // get the preheader of the current loop
  //         BasicBlock* Preheader = (*LIT)->getLoopPreheader();
  //         int forIterationCounts = PI->getExecutionCount(Preheader);  
  //         if (DEBUG) errs() << "forIterationCounts: " << forIterationCounts <<"\n";

  //         // get the entry basic block
  //         // which is the next basic block of the preheader
  //         int NumSuccessor = Preheader->getTerminator()->getNumSuccessors();
  //         if (NumSuccessor == 1){
  //           BasicBlock* nextBlock = Preheader->getTerminator()->getSuccessor(0);
    
  //           forIterationCounts = PI->getExecutionCount(nextBlock);  
  //           errs()<<"for iteration counts: "<< forIterationCounts << "\n";
  //         }
        
  //         //if ((*LIT)->getCanonicalInductionVariable()!=NULL){
  //         //  errs()<<"&&&&&&&&&&&&&&&&&&&&&&&"<<(*LIT)->getCanonicalInductionVariable()->getName()<<"\n";
  //         //}
  //       } //end if containPthreadCreate
  //     } //end for bb
  //   } //end for LIT
  // } //end for module

  for (Module::iterator F = M.begin(); F != M.end(); ++F) {
    if (DEBUG) errs() << "Func Name: " << F->getName() << '\n';
    
    for (Function::iterator BB = F->begin(); BB != F->end(); ++BB) {
      for (BasicBlock::iterator II = BB->begin(); II != BB->end(); ++II) {
        
        // Find the function called by CallInst
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
          if (DEBUG) errs() << "\t" << funcName << '\n';

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

              // Instruction* setAffinityInst = dyn_cast<Instruction>(*UI);
              CallInst* setAffinityInst = dyn_cast<CallInst>(*UI);
              if (DEBUG) errs() << "\t\t\t" << *setAffinityInst << '\n';
              Thread2AffinityMap[callInst] = setAffinityInst;

              // Find the BB where the core affinity is set
              LLVMBasicBlockRef currentBBRef = wrap(setAffinityInst->getParent());
              LLVMBasicBlockRef setCoreBBRef;
              for (int i = 0; i < 3; ++i){
                setCoreBBRef = LLVMGetPreviousBasicBlock(currentBBRef);
                currentBBRef = setCoreBBRef;
              }
              
              unsigned storeCount = 0;
              BasicBlock* setCoreBB = unwrap(setCoreBBRef);
              for (BasicBlock::iterator I = setCoreBB->begin(); I != setCoreBB->end(); ++I){
                Instruction* temp = dyn_cast<Instruction>(I);
                if (I && isa<StoreInst>(temp)){
                  if (DEBUG) errs() << *temp << '\n';
                  if (storeCount == 0) {
                    Thread2CPUIdInstMap[callInst] = temp;
                  } else if (storeCount == 1) {
                    Thread2CPUSetInstMap[callInst] = temp;
                  } else {
                    errs() << "ERROR: has more than 2 stores, check IR please!\n";
                  }
                  storeCount++;
                }
              }

              break;
            }
          }

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
  if (DEBUG) errs() << "----------------createGlobalDataRecord--------------\n";

  // TODO: serialized threads


  // TODO: parallelized threads
  // find mutex, if two thread functions acquire the same mutex, they use the shared data


  for (DenseMap<CallInst*, CallInst*>::iterator MI = Thread2AffinityMap.begin();
    MI != Thread2AffinityMap.end(); ++MI) {

    CallInst *threadCreateInst = MI->first;
    if (DEBUG) errs() << *threadCreateInst << '\n';
    
    StringRef funcName = threadCreateInst->getArgOperand(2)->getName();
    Function *threadFunc = M.getFunction(funcName);
    if (DEBUG) errs() << "Func Name:\t" << threadFunc->getName() << '\n';

    Value *arg = threadCreateInst->getArgOperand(3);
    Instruction* argInst = dyn_cast<Instruction>(arg);
    if (DEBUG) errs() << "Func arg:\t" << *argInst << '\n';
    
    Instruction* originalData = dyn_cast<Instruction>(argInst->getOperand(0));
    if (DEBUG) errs() << "before cast:\t" << *originalData << '\n';
    
    Data2Threads[originalData].insert(threadCreateInst);

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
  }

  if (DEBUG) {
    errs() << "\nData2Threads.size = " << Data2Threads.size() << '\n';
    for (DenseMap<Instruction*, std::set<CallInst*> >::iterator MI = Data2Threads.begin(); 
      MI != Data2Threads.end(); ++MI) {
      errs() <<"For global data ["<< *(MI->first) << "] used by thread(s):\n";
      std::set<CallInst*> &tempSet = MI->second;
      for (std::set<CallInst*>::iterator SI = tempSet.begin(); SI != tempSet.end(); ++SI) {
        errs() << '\t' << **SI << '\n';
      }
    }
  }

}


void Tlayout::findDataOverlapping(Module &M, const bool DEBUG) {
  if (DEBUG) errs() << "-----------------findDataOverlapping----------------\n";

  unsigned int currentCore = 0;
  for (DenseMap<Instruction*, std::set<CallInst*> >::iterator MI = Data2Threads.begin();
    MI != Data2Threads.end(); ++MI){

    //TODO: find the overlap btw set for each key, 
    //need a heuristic to separate threads into at most 8 cores(according to the machine)

    std::set<CallInst*> accessCommonDataThreads = MI->second;
    currentCore %= NUM_CORES;
    for (std::set<CallInst*>::iterator SI = accessCommonDataThreads.begin(); 
      SI != accessCommonDataThreads.end(); ++SI){
      CallInst* threadCreateInst = dyn_cast<CallInst>(*SI);
      Thread2NewCoreNum[threadCreateInst] = currentCore;
    }
    currentCore++;
  }

  bool hasLock = false;
      
  for (DenseMap<CallInst*, CallInst*>::iterator MI = Thread2AffinityMap.begin(); 
    MI != Thread2AffinityMap.end(); ++MI) {

    CallInst *callInst = MI->first;
    Value *createFunc = callInst->getArgOperand(2);
    Function* calledFunc = dyn_cast<Function> (createFunc);

    // iterate through the function to find the lock and unlock pair
    for (Function::iterator FI = calledFunc->begin(); FI!=calledFunc->end(); FI++){
      for (BasicBlock::iterator BI = FI->begin(); BI!=FI->end(); BI++){
        // find the lock and unlock instruction
        Instruction& Inst = *BI;
        
        if (!isa<CallInst>(&Inst)) {
          continue;
        }
        CallInst *lockInst = dyn_cast<CallInst>(&Inst);
        Function *lockFunc = lockInst->getCalledFunction();
        if (!lockFunc){
          continue;
        }
        StringRef lockFuncName = lockFunc->getName();
        if (lockFuncName.equals(FUNCNAME_PTHREAD_MUTEX_LOCK)){
          if (DEBUG) errs() << "find lockInst: " << *lockInst << "\n";
          hasLock = true;
        }
      }
    }
  }
  return;
}

bool Tlayout::optimizeThreadLocation(const bool DEBUG) {
  if (DEBUG) errs() << "---------------optimizeThreadLocation---------------\n";

  for (DenseMap<CallInst*, CallInst*>::iterator MI = Thread2AffinityMap.begin();
    MI != Thread2AffinityMap.end(); ++MI){

    CallInst* threadCreateInst = MI->first;
    Instruction* setCoreId = Thread2CPUIdInstMap[threadCreateInst];
    Instruction* setCoreInst = Thread2CPUSetInstMap[threadCreateInst];
    if (DEBUG) errs() << "thread:\t" << *threadCreateInst << '\n';
    if (DEBUG) errs() << "set id:\t" << *setCoreId << '\n';
    if (DEBUG) errs() << "set core:\t" << *setCoreInst << '\n';

    int coreNum = Thread2NewCoreNum[threadCreateInst];
    
    Type *valueType = &(*(setCoreId->getOperand(0))->getType());
    Constant *coreNumValue = ConstantInt::get(valueType, coreNum);
    setCoreId->setOperand(0, coreNumValue);
    if (DEBUG) errs() << "id after:\t" << *setCoreId << '\n';

    valueType = &(*(setCoreInst->getOperand(0))->getType());
    coreNumValue = ConstantInt::get(valueType, coreNum);
    setCoreInst->setOperand(0, coreNumValue);
    if (DEBUG) errs() << "core after:\t" << *setCoreInst << '\n';
  } 
  return true;
}


