//===------------------------- IdentifyFunctionLoops.h -------------------------===//
//
//                     The LLVM Compiler Infrastructure
// 
// This file is distributed under the Università della Svizzera italiana (USI) 
// Open Source License.
//
// Author         : Georgios Zacharopoulos 
// Date Started   : June, 2018
//
//===----------------------------------------------------------------------===//
//
// This file identifies Functions and Loops within the functions of an
// application.
//
//===----------------------------------------------------------------------===//

using namespace llvm;

std::ofstream myfile; // File


  int find_function(std::vector<Function *> list, Function *F) {

    for (unsigned i = 0; i < list.size(); i++)
      if (list[i] == F)
        return i;
    
    return -1;
  }


    int getEntryCount(Function *F) {

      int entry_freq = 0;

      if (F->hasMetadata()) {

        MDNode *node = F->getMetadata("prof");

        if (MDString::classof(node->getOperand(0))) {
          auto mds = cast<MDString>(node->getOperand(0));
          std::string metadata_str = mds->getString();

          if (metadata_str == "function_entry_count"){
            if (ConstantInt *CI = mdconst::dyn_extract<ConstantInt>(node->getOperand(1))) {
              entry_freq = CI->getSExtValue();
              //errs() <<" Func_Freq " << entry_freq << " "; //  Turn it back on mayne.
            }              

          }
        }
      }

      return entry_freq;
    }


bool isInductionVariable(Value *DependencyVar) {

  if (Operator *DependencyInstr = dyn_cast<Operator>(DependencyVar)) {

    if (DependencyInstr->getOpcode() == Instruction::Add) {

      if (ConstantInt *constant = dyn_cast<ConstantInt>(DependencyInstr->getOperand(1)) )
        if (constant->getSExtValue() == 1 || constant->getSExtValue() == -1 )
          return true;
    }
  }

  return false; 
}

  unsigned int getLoopCarriedDependencies(BasicBlock *BB) {

  unsigned int LoopCarriedDep = 0;
  bool IndVariableFound = false;

  //DependenceAnalysis *DA = &getAnalysis<DependenceAnalysis>();

  for(BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE; ++BI) {

    if (PHINode *PN  = dyn_cast<PHINode>(&*BI)) {

      for (unsigned int i =0; i < PN->getNumIncomingValues(); i++) {
        if (PN->getIncomingBlock(i) == BB) {
          LoopCarriedDep++;
          //errs() << "     Phi : " << *PN << "\n";
          Value *DependencyVar = PN->getIncomingValueForBlock(BB);

          if (!IndVariableFound)
            if (isInductionVariable(DependencyVar)){
              LoopCarriedDep--;
              IndVariableFound = true;
              //errs() << "     Induction Variable Found! " << "\n";
            }

          // Instruction * DepInstr = dyn_cast<Instruction>(DependencyVar);
          // std::unique_ptr<Dependence> Dep = DA->depends( BI, BI, false);

          // if  (Dep->isInput())
          //   errs() << " True Dependence! \n " ;

        }
      }
    }
  }

  return LoopCarriedDep;
}

// Check for System Calls or other than the application's functions.
  //
  bool isSystemCall(Function *F)
  {

    if (F->getName() == "llvm.lifetime.start")
      return true;

    else if (F->getName() == "llvm.lifetime.end")
      return true;

    else if (F->getName() == "llvm.memset.p0i8.i64")
      return true;

    else if (F->getName() == "llvm.memcpy.p0i8.p0i8.i64")
      return true;

    else if (F->getName() == "printf")
      return true;

    else if (F->getName() == "exit")
      return true;

     // H.264
    else if (F->getName() == "__assert_fail")
      return true;

    else if (F->getName() == "fwrite")
      return true;

    else if (F->getName() == "fflush")
      return true;
    else if (F->getName() == "fopen64")
      return true;

     else if (F->getName() == "fclose")
      return true;
    else if (F->getName() == "puts")
      return true;

    else if (F->getName() == "calloc")
      return true;
    else if (F->getName() == "no_mem_exit")
      return true;
    else if (F->getName() == "free_pointer")
      return true;
    else if (F->getName() == "free")
      return true;


    // H.264 - Synthesized
     else if (F->getName() == "llvm.bswap.i32")
      return true;
     else if (F->getName() == "fputc")
      return true;
     else if (F->getName() == "strlen")
      return true;
    else if (F->getName() == "fopen")
      return true;
    else if (F->getName() == "feof")
      return true;
    else if (F->getName() == "fgetc")
      return true;
    else if (F->getName() == "fseek")
      return true;
    else if (F->getName() == "fprintf")
      return true;
    else if (F->getName() == "sprintf")
      return true;
    else if (F->getName() == "system")
      return true;
     else if (F->getName() == "strcpy")
      return true;
    else if (F->getName() == "processinterMbType")  // Non synthesizable
      return true;
    else if (F->getName() == "inter_luma_double_skip")  // Non synthesizable
       return true;

    // H.264 JM-8.6 
    else if (F->getName() == "biari_init_context")  // Non synthesizable
       return true;
    else if (F->getName() == "intrapred")  // Non synthesizable
       return true;
    else if (F->getName() == "intrapred_chroma")  // Non synthesizable
       return true;
     else if (F->getName() == "itrans")  // Non synthesizable
       return true;
      else if (F->getName() == "itrans_2")  // Non synthesizable
       return true;

     
    // else if (F->getName() == "decode_main")  // Non synthesizable
    //   return true;
    //     else if (F->getName() == "ProcessSlice")  // Non synthesizable
    //   return true;
    //     else if (F->getName() == "main")  // Non synthesizable
     // return true;
     else if (F->getName() == "intrapred_luma_16x16")  // Non synthesizable
      return true;
    
    //else if (F->getName() == "total_zeros")  // Non synthesizable
//  return true;

   else if (F->getName() == "@0")  // Non synthesizable
      return true;
   else if (F->getName() == "@1")  // Non synthesizable
      return true;
   else if (F->getName() == "@2")  // Non synthesizable
      return true;

    else
      return false;

  }
