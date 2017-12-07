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

    static char ID;
    Tlayout() : ModulePass(ID) {}
    virtual void getAnalysisUsage(AnalysisUsage &AU) const{
	AU.addRequired<LoopInfo>();
	AU.addRequired<ProfileInfo>();
    }
    bool runOnModule(Module &M);

  private:
    DenseMap<CallInst*, Instruction*> Thread2AffinityMap; // thread_create Inst -> setaffinity Inst
    // std::set<StringRef> ThreadFuncNameSet;

    DenseMap<Instruction*, std::set<Instruction*> > ThreadGlobalDataMap;

    DenseMap<Instruction*, std::set<CallInst*> > Data2Threads;

    LoopInfo *LI;
    ProfileInfo* PI;

    void createThreadInfoRecord(Module &M, const bool DEBUG = false);
    void createGlobalDataRecord(Module &M, const bool DEBUG = false);
    void findDataOverlapping(const bool DEBUG = false);
    void optimizeThreadLocation(const bool DEBUG = false);

    void dfsRootAncestor(Instruction* threadInst, Instruction* child, const bool DEBUG = false);
  }; // end of struct Hello

const StringRef Tlayout::FUNCNAME_PTHREAD_CREATE = StringRef("pthread_create");
const StringRef Tlayout::FUNCNAME_PTHREAD_ATTR_SETAFFINITY_NP = StringRef("pthread_attr_setaffinity_np");
const StringRef Tlayout::FUNCNAME_PTHREAD_MUTEX_LOCK = StringRef("pthread_mutex_lock");
const StringRef Tlayout::FUNCNAME_PTHREAD_MUTEX_UNLOCK = StringRef("pthread_mutex_unlock");
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

  PI = &getAnalysis<ProfileInfo>();

  for (Module::iterator F = M.begin(); F != M.end(); ++F) {
	if (F->isDeclaration()) {
		continue;
	}
	Function &func = *F;
	LoopInfo &testLI = getAnalysis<LoopInfo>(func);
	for (LoopInfo::iterator LIT = testLI.begin(); LIT!=testLI.end(); LIT++){
	//	errs()<<"Function Name: "<<F->getName()<<**LIT<<"\n";
		
		// need to make sure the loop contains "pthread_create"
		bool containPthreadCreate = false;

		for (Loop::block_iterator bb = (*LIT)->block_begin(); bb!=(*LIT)->block_end(); bb++){
			//errs()<<**bb<<"\n";
			for (BasicBlock::iterator II = (*bb)->begin(); II!=(*bb)->end(); II++){
				errs()<<*II<<"\n";
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

				// find the pthread create function and 
				// get the function name so we can know which 
				// is the "DoWork" function called by pthread
				StringRef funcName = calledFunc->getName();
				if (funcName.equals(FUNCNAME_PTHREAD_CREATE)) {
				  errs().write_escaped(funcName)<<'\n';
				  containPthreadCreate = true;
				  break;
       			        }
			}
			if (containPthreadCreate) break;
		}

		if (containPthreadCreate){
			// get the preheader of the current loop
			BasicBlock* Preheader = (*LIT)->getLoopPreheader();
			errs()<<"Preheader: "<<*Preheader<<"\n";

			// get the entry basic block
			// which is the next basic block of the preheader
			




			//if ((*LIT)->getCanonicalInductionVariable()!=NULL){
			//	errs()<<"&&&&&&&&&&&&&&&&&&&&&&&"<<(*LIT)->getCanonicalInductionVariable()->getName()<<"\n";
			//}
		}
	}
  }

   for (Module::iterator F = M.begin(); F != M.end(); ++F) {
	 
    errs() << "Func Name: " << F->getName() << '\n';
    
    for (Function::iterator BB = F->begin(); BB != F->end(); ++BB) {
//	errs()<<*BB<<"\n";

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

	// find the pthread create function and 
	// get the function name so we can know which 
	// is the "DoWork" function called by pthread
	StringRef funcName = calledFunc->getName();
	if (funcName.equals(FUNCNAME_PTHREAD_CREATE)) {
	  errs().write_escaped(funcName)<<'\n';
	
	  //find the function created by pthread
	  Value *createFunc = callInst->getArgOperand(2);
       	 // errs()<<*createFunc<<"\n";
	
	  Function* calledFunc = dyn_cast<Function> (createFunc);
	 
	  // iterate through the function to find the lock and unlock pair
	 for (Function::iterator FI = calledFunc->begin(); FI!=calledFunc->end(); FI++){
           for (BasicBlock::iterator BI = FI->begin(); BI!=FI->end(); BI++){
	     // find the lock and unlock instruction
	     Instruction& Inst = *BI;
	     errs()<<Inst<<"\n";
	     if (!isa<CallInst>(&Inst)) continue;
	     CallInst *lockInst = dyn_cast<CallInst>(&Inst);
	     Function *lockFunc = lockInst->getCalledFunction();
	     if (!lockFunc){
		errs()<<"\tWARN: Indirect function call.\n";
		continue;
	     }
	     StringRef lockFuncName = lockFunc->getName();
	     if (lockFuncName.equals(FUNCNAME_PTHREAD_MUTEX_LOCK)){
		errs()<<"###########pthread mutex############\n";
	     }
	
	    }	   
	  } 
        }

 /*       // Find the CallInst
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
*/        
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

  for (DenseMap<CallInst*, Instruction*>::iterator MI = Thread2AffinityMap.begin();
    MI != Thread2AffinityMap.end(); ++MI) {

    CallInst *threadCreateInst = MI->first;
    errs() << *threadCreateInst << '\n';
    StringRef funcName = threadCreateInst->getArgOperand(2)->getName();
    Function *threadFunc = M.getFunction(funcName);
    errs() << threadFunc->getName() << '\n';

    Value *arg = threadCreateInst->getArgOperand(3);
    errs() << arg << '\n';
    Instruction* argInst = dyn_cast<Instruction>(arg);
    errs() << "argInst\t\t" << *argInst << '\n';

    Instruction* originalData = dyn_cast<Instruction>(argInst->getOperand(0));
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

