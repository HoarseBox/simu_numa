#ifndef TLAYOUT_H
#define TLAYOUT_H

namespace llvm {

  class FunctionPass;

  // LoopPass *createLAMPBuildLoopMapPass();
  // ModulePass *createLAMPLoadProfilePass();

  // class LAMPBuildLoopMap : public LoopPass {
  //   static unsigned int loop_id;
  //   static bool IdInitFlag;
    
  // public:
  //   std::map<unsigned int, BasicBlock*> IdToLoopMap;    // LoopID -> headerBB*
  //   std::map<BasicBlock*, unsigned int> LoopToIdMap;    // headerBB* -> LoopID
  //   static char ID;
  //   LAMPBuildLoopMap() : LoopPass(ID) {}

  //   virtual bool runOnLoop (Loop *L, LPPassManager &LPM);
  //   virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  // };  // LLVM Loop Pass can not be required => can not pass analysis info

  // class LAMPLoadProfile : public ModulePass {
  // public:
  //   std::map<unsigned int, Instruction*> IdToInstMap;   // Inst* -> InstId
  //   std::map<Instruction*, unsigned int> InstToIdMap;   // InstID -> Inst*
  //   std::map<unsigned int, BasicBlock*> IdToLoopMap;    // LoopID -> headerBB*
  //   std::map<BasicBlock*, unsigned int> LoopToIdMap;    // headerBB* -> LoopID
  //   std::map<std::pair<Instruction*, Instruction*>*, unsigned int> DepToTimesMap; 
  //   std::map<BasicBlock*, std::set<std::pair<Instruction*, Instruction*>* > > LoopToDepSetMap;
  //   std::map<BasicBlock*, unsigned int> LoopToMaxDepTimesMap;

  //   static unsigned int lamp_id;
  //   static char ID;
  //   LAMPLoadProfile() : ModulePass (ID) {}

  //   virtual bool runOnModule (Module &M);
  //   virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  // };


  struct Tlayout : public FunctionPass {
    static const bool DEBUG = true;
    static const StringRef FUNCNAME_PTHREAD_CREATE;
    static const StringRef FUNCNAME_PTHREAD_ATTR_SETAFFINITY_NP;

    static char ID;
    Tlayout() : FunctionPass(ID) {}

    bool runOnFunction(Function &F);

    DenseMap<CallInst*, Instruction*> Thread2AffinityMap; // thread_create Inst -> setaffinity Inst
    std::set<StringRef> ThreadFuncNameSet;
  }; // end of struct Hello

  // const StringRef Tlayout::FUNCNAME_PTHREAD_CREATE = StringRef("pthread_create");
  // const StringRef Tlayout::FUNCNAME_PTHREAD_ATTR_SETAFFINITY_NP = StringRef("pthread_attr_setaffinity_np");

}
#endif 
