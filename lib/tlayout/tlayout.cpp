#include "llvm/Pass.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/DenseMap.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ProfileInfo.h"

#include <algorithm>
#include <set>
#include <vector>


using namespace llvm;

namespace {

  struct Tlayout : public ModulePass {

    static const StringRef FUNCNAME_PTHREAD_CREATE;
    static const StringRef FUNCNAME_PTHREAD_ATTR_SETAFFINITY_NP;
    static const StringRef FUNCNAME_PTHREAD_MUTEX_LOCK;
    static const StringRef FUNCNAME_PTHREAD_MUTEX_UNLOCK;
    static const unsigned CORE_NUM_PER_NODE;
    static const unsigned NUM_NODES;
    static const unsigned TOTAL_CORE_NUM;

    static char ID;
    ProfileInfo* PI;
    // LoopInfo *LI;

    Tlayout() : ModulePass(ID) {}

    void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<LoopInfo>();
      AU.addRequired<ProfileInfo>();
    }
    bool runOnModule(Module &M);

  private:
    DenseMap<unsigned, std::vector<unsigned> > NODE2CORE;

    DenseMap<CallInst*, CallInst*> Thread2AffinityMap; // thread_create Inst -> setaffinity Inst

    DenseMap<CallInst*, Instruction*> Thread2CPUSetInstMap;
    // DenseMap<CallInst*, Instruction*> Thread2CPUIdInstMap;

    DenseMap<Instruction*, std::set<Instruction*> > ThreadGlobalDataMap;

    DenseMap<CallInst*, double> Thread2ExecutionCount;

    DenseMap<Instruction*, std::set<CallInst*> > Data2Threads;

    DenseMap<Value*, std::set<CallInst*> > Mutex2Threads;

    DenseMap<CallInst*, int> Thread2NewCoreNum;

    //DenseMap<int, std::set<CallInst*> > coreNum2Threads;

    bool hasIntersect(std::set<CallInst*>* set1, std::set<CallInst*>* set2);
    double getThreadCreateExecutionCount(std::set<CallInst*>* threadSet);


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
  
  // According to your machine, please hard coded hardware info here
  const unsigned Tlayout::CORE_NUM_PER_NODE = 16;
  const unsigned Tlayout::NUM_NODES = 2;
  const unsigned Tlayout::TOTAL_CORE_NUM = 32;

}  // end of anonymous namespace

char Tlayout::ID = 0;
static RegisterPass<Tlayout> X("tlayout", "Optimize Thread Layout on Multicore");


bool Tlayout::runOnModule(Module &M) {

  bool changed = false;
  PI = &getAnalysis<ProfileInfo>();
  
  // According to your machine, please hard coded hardware info here
  for (size_t i = 0; i < TOTAL_CORE_NUM; ++i) {
    if (i < 8 || (i >= 16 && i <=23) ) {
      NODE2CORE[0].push_back(i);
    } else {
      NODE2CORE[1].push_back(i);
    }
  }

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
                  //   Thread2CPUIdInstMap[callInst] = temp;
                  // } else if (storeCount == 1) {
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

  for (DenseMap<CallInst*, CallInst*>::iterator MI = Thread2AffinityMap.begin();
    MI != Thread2AffinityMap.end(); ++MI) {
    BasicBlock *bb = MI->first->getParent();
    double executionCount = PI->getExecutionCount(bb);
    Thread2ExecutionCount[MI->first] = executionCount;
    if (DEBUG) errs() << "map size ===  " << Thread2ExecutionCount.size() << "\n"; 
    if (DEBUG) errs() << "ExecutionCount: " << executionCount << "\n";
  }

  return;
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
          Value *mutexValue = lockInst->getOperand(0);
          if (DEBUG) errs() << "mutex value : " << *mutexValue << "\n";
          Mutex2Threads[mutexValue].insert(callInst);
          hasLock = true;
        }
      }
    }
  }

  if (DEBUG) {
    errs() << "\tMutex2Threads.size = " << Mutex2Threads.size() << '\n';
    for (DenseMap<Value*, std::set<CallInst*> >::iterator MI = Mutex2Threads.begin(); 
      MI != Mutex2Threads.end(); ++MI) {
      errs() <<"For mutex ["<< *(MI->first) << "] used by thread(s):\n";
      std::set<CallInst*> &tempSet = MI->second;
      for (std::set<CallInst*>::iterator SI = tempSet.begin(); SI != tempSet.end(); ++SI) {
        errs() << '\t' << **SI << '\n';
      }
    }
  }
}

bool Tlayout::hasIntersect(std::set<CallInst*>* set1, std::set<CallInst*>* set2){
  for (std::set<CallInst*>::iterator SI = set1->begin(); SI != set1->end(); ++SI){
    CallInst* cur = *SI;
    if (set2->find(cur) != set2->end()){
      return true;
    }
  }
  return false;
}

double Tlayout::getThreadCreateExecutionCount(std::set<CallInst*>* threadSet) {
  double executionCount = 0;
  for (std::set<CallInst*>::iterator SI = threadSet->begin(); SI != threadSet->end(); ++SI) {
    executionCount += Thread2ExecutionCount[*SI];
  }
  return executionCount;
}

void Tlayout::findDataOverlapping(Module &M, const bool DEBUG) {
  if (DEBUG) errs() << "-----------------findDataOverlapping----------------\n";

  // union find thread sets
  std::vector<std::set<CallInst*>* > unionFindVec;
  std::vector<bool> hasCommon;
  
  for (DenseMap<Instruction*, std::set<CallInst*> >::iterator DTI = Data2Threads.begin();
    DTI != Data2Threads.end(); ++DTI){
    unionFindVec.push_back(&(DTI->second));
    hasCommon.push_back(false);
  }

  for (DenseMap<Value*, std::set<CallInst*> >::iterator MTI = Mutex2Threads.begin();
      MTI != Mutex2Threads.end(); ++MTI){
    unionFindVec.push_back(&(MTI->second));
    hasCommon.push_back(false);
  }

  for (size_t i = 0; i != unionFindVec.size() - 1; ++i){
    std::set<CallInst*>* iSetPtr = unionFindVec[i];
    for (size_t j = i + 1; j != unionFindVec.size(); ++j){
      std::set<CallInst*>* jSetPtr = unionFindVec[j];
      if (hasIntersect(iSetPtr, jSetPtr)){
        for (std::set<CallInst*>::iterator SI = iSetPtr->begin(); SI != iSetPtr->end(); ++SI){
          jSetPtr->insert(*SI);
        }
        hasCommon[i] = true;
      } 
    }
  }

  //greed assign threads to nodes (related to K partition problem)
  std::vector<std::pair<double, std::set<CallInst*>* > > groupedThreads;
  for (size_t i = 0; i < hasCommon.size(); ++i) {
    if (!hasCommon[i]) {
      double executionCount = getThreadCreateExecutionCount(unionFindVec[i]);
      if (DEBUG) errs() << "ExecutionCount for " << i << " = " << executionCount << '\n';
      groupedThreads.push_back(std::make_pair(executionCount, unionFindVec[i]));
    }
  }
  std::sort(groupedThreads.begin(), groupedThreads.end());
  std::reverse(groupedThreads.begin(), groupedThreads.end());

  std::vector<double> curCount;
  for(size_t i = 0; i < NUM_NODES; ++i) {
    curCount.push_back(0);
  }
  for (size_t i = 0; i < groupedThreads.size(); ++i) {
    
    if (DEBUG) errs() << groupedThreads[i].first << '\t' << groupedThreads[i].second->size() << '\n';
    
    std::vector<double>::iterator minI = std::min_element(curCount.begin(), curCount.end());
    unsigned currentNode = minI - curCount.begin();
    
    if (DEBUG) errs() << "test: " << *minI << "\t" << currentNode << '\n'; 
    
    curCount[currentNode] += groupedThreads[i].first;
    int counter = 0;
    std::set<CallInst*>* threadSet = groupedThreads[i].second;
    for (std::set<CallInst*>::iterator SI = threadSet->begin(); SI != threadSet->end(); ++SI) {
      Thread2NewCoreNum[*SI] = NODE2CORE[currentNode][counter]; 
      if (DEBUG) errs() << "core id: " << NODE2CORE[currentNode][counter] << '\n'; 
      counter = (counter + 1) % CORE_NUM_PER_NODE;
    }
  }

  return;
}

bool Tlayout::optimizeThreadLocation(const bool DEBUG) {
  if (DEBUG) errs() << "---------------optimizeThreadLocation---------------\n";

  for (DenseMap<CallInst*, CallInst*>::iterator MI = Thread2AffinityMap.begin();
    MI != Thread2AffinityMap.end(); ++MI){

    CallInst* threadCreateInst = MI->first;
    // Instruction* setCoreId = Thread2CPUIdInstMap[threadCreateInst];
    Instruction* setCoreInst = Thread2CPUSetInstMap[threadCreateInst];
    if (DEBUG) errs() << "thread:\t" << *threadCreateInst << '\n';
    // if (DEBUG) errs() << "set id:\t" << *setCoreId << '\n';
    if (DEBUG) errs() << "set core:\t" << *setCoreInst << '\n';

    int coreNum = Thread2NewCoreNum[threadCreateInst];
    
    // Type *valueType = &(*(setCoreId->getOperand(0))->getType());
    // Constant *coreNumValue = ConstantInt::get(valueType, coreNum);
    // setCoreId->setOperand(0, coreNumValue);
    // if (DEBUG) errs() << "id after:\t" << *setCoreId << '\n';

    Type *valueType = &(*(setCoreInst->getOperand(0))->getType());
    Constant *coreNumValue = ConstantInt::get(valueType, coreNum);
    setCoreInst->setOperand(0, coreNumValue);
    if (DEBUG) errs() << "core after:\t" << *setCoreInst << '\n';
  } 
  return true;
}

