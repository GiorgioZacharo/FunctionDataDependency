//===------------------------- FunctionDataDependency.cpp -------------------------===//
//
//                     The LLVM Compiler Infrastructure
// 
// This file is distributed under the Università della Svizzera italiana (USI) 
// Open Source License.
//
// Author         : Georgios Zacharopoulos 
// Date Started   : April, 2019
//
//===----------------------------------------------------------------------===//
//
// This file identifies Functions and Loops within the functions of an
// application.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/BlockFrequencyInfoImpl.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Transforms/Utils/Local.h"
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include "llvm/IR/CFG.h"
//#include "../Identify.h" // Common Header file for all RegionSeeker Passes.

#include "FunctionDataDependency.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugInfo.h"

#define DEBUG_TYPE "FunctionDataDependency"

using namespace llvm;

namespace {

  struct FunctionDataDependency : public FunctionPass {
    static char ID; // Pass Identification, replacement for typeid

    std::vector<Function *> Function_list; // Global Function List
    std::vector<StringRef> Function_Names_list; // Global Function List Names

    FunctionDataDependency() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {

      LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
      ScalarEvolution &SE = getAnalysis<ScalarEvolutionWrapperPass>().getSE();
      std::string Function_Name = F.getName();

      // Function_list.clear(); // Clear the Loops List
      // Initialize Function List.
      initFunctionList(&F);
      errs() << "\n\n\tFunction Name is : " << Function_Name << "\n";

      if (Function_Name == "_ZN12ILLIXR_AUDIO5SoundD2Ev")
        return false;
      printGVFile(&F);
      //getCallInstrOfFunction(&F);


      return false;
    }

    // Populate the list with all Functions of the app *except* for the System Calls.
    //
    bool initFunctionList(Function *F) {

      if (find_function(Function_list, F) == -1 && isSystemCall(F) == false){

        std::string Function_Name = F->getName();

        Function_list.push_back(F);


        Function_Names_list.push_back(F->getName());
      }

      return true;
    }



  
    void getCallInstrOfFunction (Function *F) {

      for(Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
        getCallInstrOfBB(&*BB);
      }
    
    }

        void getInvokeInstrOfFunction (Function *F) {

      for(Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
        getInvokeInstrOfBB(&*BB);
      }
    
    }


void isCall(Instruction *Inst, std::string NestedCallName) {

      if(CallInst *CI = dyn_cast<CallInst>(Inst)) {
        std::string CallName = CI->getCalledFunction()->getName();
        errs() << "\t\t\t Data Dependency Call " << CallName << "\n";

        std::string TopFunName = Function_Names_list[Function_Names_list.size()-1];
         errs() << "\t\t\t Data Dependency Call  - TopFunName" << TopFunName << "\n";

            myfile.open (TopFunName +".gv", std::ofstream::out | std::ofstream::app);
            myfile << CallName << "[weight = 1, style = filled]" << "\n"; 
            myfile << NestedCallName << "[weight = 1, style = filled]" << "\n"; 
            myfile << CallName << " -> " << NestedCallName << " ; "  << "\n";
            myfile.close();

      }

}

void isInvoke(Instruction *Inst, std::string NestedCallName) {

      if(InvokeInst *CI = dyn_cast<InvokeInst>(Inst)) {
        std::string CallName = CI->getCalledFunction()->getName();
        errs() << "\t\t\t Data Dependency Call " << CallName << "\n";

        std::string TopFunName = Function_Names_list[Function_Names_list.size()-1];
        errs() << "\t\t\t Data Dependency Call  - TopFunName" << TopFunName << "\n";

            myfile.open (TopFunName +".gv", std::ofstream::out | std::ofstream::app);
            myfile << CallName << "[weight = 1, style = filled]" << "\n"; 
            myfile << NestedCallName << "[weight = 1, style = filled]" << "\n"; 
            myfile << CallName << " -> " << NestedCallName << " ; "  << "\n";
            myfile.close();
      }

}


    void isStore (Instruction *Inst, Instruction *Alloca, std::string NestedCallName) {

            if(StoreInst *Store = dyn_cast<StoreInst>(&*Inst))
             if( Instruction *OP1 = dyn_cast<Instruction>(&*Store->getOperand(1)))
                if (OP1 == Alloca) {
                  if( Instruction *OP0 = dyn_cast<Instruction>(&*Store->getOperand(0))) {
              //&&Instruction *OP1 = dyn_cast<Instruction>(&*Store->getOperand(1)))
               errs() << "\t\t\t Data Dependency Stores " << *OP1 << " Source:  "  << *OP0
             << "\n";

             isCall(OP0, NestedCallName);
             isInvoke(OP0, NestedCallName);
             isAdd(OP0, NestedCallName);
              }

            }


    }


    void isLoad (Instruction *Inst, Instruction *Alloca, std::string NestedCallName) {

      if(LoadInst *Load = dyn_cast<LoadInst>(&*Inst))
       if( Instruction *OP0 = dyn_cast<Instruction>(&*Load->getOperand(0))){
          //if (OP0 == Alloca){

        //&&Instruction *OP1 = dyn_cast<Instruction>(&*Store->getOperand(1)))
         errs() << "\t\t\t Data Dependency Loads " << *Inst << " Source:  "  << *OP0
       << "\n";

       isCall(OP0, NestedCallName);
       isInvoke(OP0, NestedCallName);
        }

    }


    void isAdd (Instruction *Inst, std::string NestedCallName) {

      if (Inst->getOpcode() == Instruction::Add) {

        for (int i=0; i<2; i++) {
          
          if (Instruction *OP = dyn_cast<Instruction>(&*Inst->getOperand(i))) {
            errs() << "\t\t Data Dependency Add OP: " << i << " " << *OP << "\n";


        // Instruction *OP0 = dyn_cast<Instruction>(&*Inst->getOperand(0));
        // Instruction *OP1= dyn_cast<Instruction>(&*Inst->getOperand(1));
        // errs() << "\t\t Data Dependency Add " << *OP0 << "  "  << *OP1 << "\n";
    
         if (LoadInst *LD = dyn_cast<LoadInst>(&*OP)){
          Instruction *LD_OP0 = dyn_cast<Instruction>(&*LD->getOperand(0));
          isStoredFromAdd(LD_OP0, NestedCallName);
        } 
      }
      }

    }
  }

  void isMul (Instruction *Inst, std::string NestedCallName) {

      if (Inst->getOpcode() == Instruction::Mul) {

        for (int i=0; i<2; i++) {

          if (Instruction *OP = dyn_cast<Instruction>(&*Inst->getOperand(i))) {
            errs() << "\t\t Data Dependency Mul OP: " << i << " " << *OP << "\n";


          // Instruction *OP0 = dyn_cast<Instruction>(&*Inst->getOperand(0));
          // Instruction *OP1= dyn_cast<Instruction>(&*Inst->getOperand(1));
          // errs() << "\t\t Data Dependency Add " << *OP0 << "  "  << *OP1 << "\n";
      
           if (LoadInst *LD = dyn_cast<LoadInst>(&*OP)){
            Instruction *LD_OP0 = dyn_cast<Instruction>(&*LD->getOperand(0));
            isStoredFromMul(LD_OP0, NestedCallName);
          } 
        }
      }

    }
  }

    void isStoredFromMul (Instruction *Inst, std::string NestedCallName) { 

      Function *TopFun = Function_list[Function_list.size()-1];

      for(Function::iterator BB = TopFun->begin(), E = TopFun->end(); BB != E; ++BB)
        for(BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE; ++BI)
          if (StoreInst *Store = dyn_cast<StoreInst>(&*BI)) {
            if ( dyn_cast<Instruction>(&*Store->getOperand(0)) &&
             dyn_cast<Instruction>(&*Store->getOperand(1))) {

            Instruction *OP0 = dyn_cast<Instruction>(&*Store->getOperand(0));
            Instruction *OP1 = dyn_cast<Instruction>(&*Store->getOperand(1));

              if (OP1 == Inst){
                errs() << "\t\t Data Dependency Inst Stored From Mul " << *OP1 << " Source:  " 
               << " "  << *Store << "\n";
                isCall(OP0, NestedCallName);
                isInvoke(OP0, NestedCallName);
                //isMul(OP0, NestedCallName);
              }
            }
          }

    } 

  void isStoredFromAdd (Instruction *Inst, std::string NestedCallName) { 

      Function *TopFun = Function_list[Function_list.size()-1];

      for(Function::iterator BB = TopFun->begin(), E = TopFun->end(); BB != E; ++BB)
        for(BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE; ++BI)
          if (StoreInst *Store = dyn_cast<StoreInst>(&*BI)) {
            if ( dyn_cast<Instruction>(&*Store->getOperand(0)) &&
             dyn_cast<Instruction>(&*Store->getOperand(1))) {

            Instruction *OP0 = dyn_cast<Instruction>(&*Store->getOperand(0));
            Instruction *OP1 = dyn_cast<Instruction>(&*Store->getOperand(1));

              if (OP1 == Inst){
                errs() << "\t\t Data Dependency Inst Stored From Add " << *OP1 << " Source:  " 
               << " "  << *Store << "\n";
                isCall(OP0, NestedCallName);
                isInvoke(OP0, NestedCallName);
                isMul(OP0, NestedCallName);
              }
            }
          }

    }

    void isStored (Instruction *Inst, std::string NestedCallName) { 

      Function *TopFun = Function_list[Function_list.size()-1];

      for(Function::iterator BB = TopFun->begin(), E = TopFun->end(); BB != E; ++BB)
        for(BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE; ++BI)
          if (StoreInst *Store = dyn_cast<StoreInst>(&*BI)) {
            if ( dyn_cast<Instruction>(&*Store->getOperand(0)) &&
             dyn_cast<Instruction>(&*Store->getOperand(1))) {
            Instruction *OP0 = dyn_cast<Instruction>(&*Store->getOperand(0));
            Instruction *OP1 = dyn_cast<Instruction>(&*Store->getOperand(1));
   
          //   Instruction *OP0 = dyn_cast<Instruction>(&*Store->getOperand(0));
          //   Instruction *OP1 = dyn_cast<Instruction>(&*Store->getOperand(1));

              if (OP1 == Inst){
                errs() << "\t\t Data Dependency GEP Stored " << *OP1 << " Source:  "  << *Store << "\n";
                isAdd(OP0, NestedCallName);
              }
          }
        }

    }

    void isGep(Instruction *Inst, Instruction *Alloca, std::string NestedCallName) {

      if(GetElementPtrInst *GetElementPtr = dyn_cast<GetElementPtrInst>(&*Inst))
       if( Instruction *OP0 = dyn_cast<Instruction>(&*GetElementPtr->getOperand(0))){
          if (OP0 == Alloca){

        //&&Instruction *OP1 = dyn_cast<Instruction>(&*Store->getOperand(1)))
          errs() << "\t\t\t Data Dependency GetElementPtrs " << *Inst << " Source:  "  << *OP0
          << "\n";


          isStored(Inst, NestedCallName);
        }
      }
    }


    void isAlloca (Instruction *Ins, Function *F, std:: string NestedCallName) {

      if(AllocaInst *Alloca = dyn_cast<AllocaInst>(&*Ins)) 

        for(Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) 
          for(BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE; ++BI) {
              isStore(&*BI, Alloca, NestedCallName);
              isGep(&*BI, Alloca, NestedCallName);
              //isLoad(&*BI, Alloca);
            }
            // if(StoreInst *Store = dyn_cast<StoreInst>(&*BI))
            //  if( Instruction *OP1 = dyn_cast<Instruction>(&*Store->getOperand(1)))
            //     if (OP1 == Ins) {
            //       if( Instruction *OP0 = dyn_cast<Instruction>(&*Store->getOperand(0)))
            //   //&&Instruction *OP1 = dyn_cast<Instruction>(&*Store->getOperand(1)))
            //    errs() << "\t\t\t Data Dependency Stores " << *OP1 << " Source:  "  << *OP0
            //  << "\n";
            // }
    }


    // Check if Instruction I used as Input Argument by another function is used in Function F
    //
    bool isDataDependent(Function *F, Instruction *I, std::string NestedCallName) {

      //errs() << "Data Dependent called\n";

      unsigned int NumberOfLLVMInstructions=0;

      for(Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
       // errs() << "BB Name " << BB->getName() << "\n";
          // Iterate inside the basic block.
        for(BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE; ++BI) {
          // errs() << "BI Name " << BI->getName() << "\n";
          if (Instruction *Inst = &*BI){
            // if (Inst->getName() == I->getName()){
          if (Inst == I) {
              errs() << "\t\t\t Data Dependency " << F->getName() << " \n\t\t\t" 
                  << Inst->getName() << " Ins: " << *Inst << "\n";


                // while (I->getOperand(0)) {
                  while(I = dyn_cast<Instruction>(&*I->getOperand(0))) {
                 

                    errs() << "\t\t\t Data Dependency " << Inst->getName() << " Ins: " << *I << "\n";
                    isAlloca(I,F, NestedCallName);

                    // This tests only for Add - Make a list of possible opcodes (e.g. mul, etc)
                    if  (Inst->getOpcode() == Instruction::Add) {
                      errs() << " Inst  " << *Inst<< "\n";
                      Instruction *OP1 = dyn_cast<Instruction>(&*Inst->getOperand(1));
                      errs() << " OP1  " << *OP1 << "\n";
                      
                      if (LoadInst *LD = dyn_cast<LoadInst>(&*OP1)) {
                        errs() << " LD of OP1  " << *LD->getOperand(0) << "\n";
                        Instruction *LD_OP0 = dyn_cast<Instruction>(&*LD->getOperand(0));
                        isAlloca(LD_OP0,F, NestedCallName);
                      }
                    }

                  // I = dyn_cast<Instruction>(&*I->getOperand(0));
                }

                // if (I = dyn_cast<Instruction>(&*I->getOperand(1))) {
                //   while(I = dyn_cast<Instruction>(&*I->getOperand(1))) {
                //     errs() << "\t\t\t Data Dependency " << Inst->getName() << " Ins: " << *I << "\n";
                //     isAlloca(I,F);
                //   // I = dyn_cast<Instru}ction>(&*I->getOperand(0));
                //   }
                // }


                         


           // std::string TopFunName =  F->getName();    

            // myfile.open (TopFunName +".gv", std::ofstream::out | std::ofstream::app);
            //  //       myfile.open ("audio_pipeline.gv", std::ofstream::out | std::ofstream::app);
            // myfile << NestedCallName << "[weight = 1, style = filled]" << "\n"; 
            // myfile << TopFunName << " -> " << NestedCallName << " ; "  << "\n";
            // myfile.close();
  


              return true;
            }
          }
        }
      }
    
       return false;   
    }


    // //
    // //
    // void getCallInstrOfFunction (Function *F) {

    //   // unsigned int NumberOfInstructions = 0, NumberOfAllInstructions = 0;

    //   for(Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {

    //     BasicBlock *CurrentBlock = &*BB;

    //     // Iterate inside the basic block.
    //     for(BasicBlock::iterator BI = CurrentBlock->begin(), BE = CurrentBlock->end(); BI != BE; ++BI) {

    //            // Load Info
    //       if(CallInst *CI = dyn_cast<CallInst>(&*BI)) {

    //         StringRef CallName = CI->getCalledFunction()->getName();
            
    //         if (CallName == "llvm.dbg.value" || CallName == "llvm.lifetime.start" || CallName == "llvm.lifetime.start" ||
    //             CallName == "llvm.lifetime.end")
    //           continue;

    //         errs() 
    //         // <<  *CI  <<"\n"
    //         // << "OP1: " << *CI->getOperand(0) <<"\n"
    //         // << "OP2 " << *CI->getOperand(1) <<"\n"
    //         << "C[name: " << CI->getCalledFunction()->getName() 
    //         <<"; n_of_instructions:" <<  Functions_instr_list[ find_function(Functions_list, CI->getCalledFunction())] 
    //         << "]\n";
    //       }
    //     }
    //   }
    // }

    //
    void getCallInstrOfBB (BasicBlock *BB) {

      // Iterate inside the basic block.
      for(BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE; ++BI) {

             // Load Info
         if(CallInst *CI = dyn_cast<CallInst>(&*BI)) {
        //     if(InvokeInst *CI = dyn_cast<InvokeInst>(&*BI)) {

          StringRef CallName = CI->getCalledFunction()->getName();
          
          if (CallName == "llvm.dbg.value" || CallName == "llvm.lifetime.start" || CallName == "llvm.lifetime.start" ||
              CallName == "llvm.lifetime.end")
            continue;

          unsigned int NumberOfArgs = CI->getNumArgOperands();
          for (int index=0; index<NumberOfArgs; index++ ){
            if (CI->getArgOperand(index)){
              Instruction *InputArg = dyn_cast<Instruction>(&*CI->getArgOperand(index));

            errs() 
            // <<  *CI  <<"\n"
              << "\tC[name: " << CI->getCalledFunction()->getName() 
             << "\n\t\tOP: " << index << " " << *CI->getArgOperand(index) <<"\n"
               << "\t\tOP - Name: " << index << " " << CI->getArgOperand(index)->getName() <<"\n"
               ;
            
            std::string NestedCallName = CI->getCalledFunction()->getName();
           //errs() << "For called\n";
             for (unsigned i = 0; i < Function_list.size(); i++){

               //errs() << "Data Dependency " << Function_list[i]->getName() << "\n" ;

              isDataDependent(Function_list[i],InputArg, NestedCallName);
             
             }
           // errs() << "For End\n";

           }
        }

        }
      }
    }

        void getInvokeInstrOfBB (BasicBlock *BB) {

      // Iterate inside the basic block.
      for(BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE; ++BI) {

             // Load Info
        // if(CallInst *CI = dyn_cast<CallInst>(&*BI)) {
             if(InvokeInst *CI = dyn_cast<InvokeInst>(&*BI)) {

          StringRef CallName = CI->getCalledFunction()->getName();
          
          if (CallName == "llvm.dbg.value" || CallName == "llvm.lifetime.start" || CallName == "llvm.lifetime.start" ||
              CallName == "llvm.lifetime.end")
            continue;

          unsigned int NumberOfArgs = CI->getNumArgOperands();
          for (int index=0; index<NumberOfArgs; index++ ){
            if (CI->getArgOperand(index)){
              Instruction *InputArg = dyn_cast<Instruction>(&*CI->getArgOperand(index));

            errs() 
            // <<  *CI  <<"\n"
              << "\tC[name: " << CI->getCalledFunction()->getName() 
             << "\n\t\tOP: " << index << " " << *CI->getArgOperand(index) <<"\n"
               << "\t\tOP - Name: " << index << " " << CI->getArgOperand(index)->getName() <<"\n"
               ;
            
            std::string NestedCallName = CI->getCalledFunction()->getName();
           //errs() << "For called\n";
             for (unsigned i = 0; i < Function_list.size(); i++){

               //errs() << "Data Dependency " << Function_list[i]->getName() << "\n" ;

              isDataDependent(Function_list[i],InputArg, NestedCallName);
             
             }
           // errs() << "For End\n";

           }
        }

        }
      }
    }


    void printGVFile(Function *F) {
      std::string Function_Name = F->getName();

      // if ( Function_Name == "_ZN12ILLIXR_AUDIO7ABAudioC2ESsNS0_11ProcessTypeE" || 
      //       Function_Name == "_ZSt3maxIfERKT_S2_S2_" || Function_Name == "_ZN12ILLIXR_AUDIO7ABAudio14updateRotationEv" ||
      //       Function_Name == "_ZN12ILLIXR_AUDIO7ABAudio10updateZoomEv" || Function_Name == "main" ||
      //       Function_Name == "_ZN12ILLIXR_AUDIO5SoundC2ESsjb" || Function_Name == "_ZN12ILLIXR_AUDIO5Sound9setSrcPosER10PolarPoint"
      //          )
      //           return;

        myfile.open (Function_Name +".gv", std::ofstream::out | std::ofstream::app);
         // myfile.open ("audio_pipeline.gv", std::ofstream::out | std::ofstream::app);
          myfile << "digraph \"" << Function_Name << "\" {" << "\n";
         // myfile << "digraph \"" << "audio_pipeline" << "\" {" << "\n";
         myfile << Function_Name << "[weight = 1, style = filled]" << "\n"; 
         myfile.close();

         getCallInstrOfFunction(F);
         getInvokeInstrOfFunction(F);

         myfile.open (Function_Name +".gv", std::ofstream::out | std::ofstream::app);
        // myfile.open ("audio_pipeline.gv", std::ofstream::out | std::ofstream::app);
          myfile << "}" << "\n";
          myfile.close();

       }





    virtual void getAnalysisUsage(AnalysisUsage& AU) const override {
              
        AU.addRequired<LoopInfoWrapperPass>();
        AU.addRequiredTransitive<ScalarEvolutionWrapperPass>();
        // AU.addRequired<RegionInfoPass>();
        //AU.addRequired<DependenceAnalysis>();
        // AU.addRequiredTransitive<RegionInfoPass>();      
        // AU.addRequired<BlockFrequencyInfoWrapperPass>();
        AU.setPreservesAll();
    } 
  };
}

char FunctionDataDependency::ID = 0;
static RegisterPass<FunctionDataDependency> X("FunctionDataDependency", "Identify Loops within Functions");
