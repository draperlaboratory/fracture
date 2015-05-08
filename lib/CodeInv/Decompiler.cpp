//===--- Decompiler - Decompiles machine basic blocks -----------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class uses the disassembler and inverse instruction selector classes
// to decompile target specific code into LLVM IR and return function objects
// back to the user.
//
// Author: Richard Carback (rtc1032) <rcarback@draper.com>
// Date: August 28, 2013
//===----------------------------------------------------------------------===//

#include "CodeInv/Decompiler.h"

using namespace llvm;

#define DEBUG_TYPE "fracture-decompiler"

namespace fracture {

Decompiler::Decompiler(Disassembler *NewDis, Module *NewMod, raw_ostream &InfoOut, raw_ostream &ErrOut) :
    Dis(NewDis), Mod(NewMod), DAG(NULL), ViewMCDAGs(false), ViewIRDAGs(false), Infos(InfoOut), Errs(ErrOut){

  assert(NewDis && "Cannot initialize decompiler with null Disassembler!");
  if (Mod == NULL) {
    std::string ModID = Dis->getExecutable()->getFileName().data();
    ModID += "-IR";
    Mod = new Module(StringRef(ModID), *(Dis->getMCDirector()->getContext()));
  }
  Context = Dis->getMCDirector()->getContext();

  //Where is the getTargetInvISelDAG method?
  InvISel = getTargetInvISelDAG(Dis->getMCDirector()->getTargetMachine(), this);
  Emitter = InvISel->getEmitter(this, Infos, Errs);
}

Decompiler::~Decompiler() {
  delete Emitter;
  delete DAG;
  delete InvISel;
  delete Context;
  delete Mod;
  delete Dis;
}

void Decompiler::decompile(unsigned Address) {
  std::vector<unsigned> Children;
  Children.push_back(Address);

  do {
    //size_t ChildrenSize = Children.size();
    //errs() << "Size: " << ChildrenSize << "\n";
    Function* CurFunc = decompileFunction(Children.back());
    Children.pop_back();
    if (CurFunc == NULL) {
      continue;
    }
    // Scan Current Function for children (should probably record children
    // during decompile...)
    for (Function::iterator BI = CurFunc->begin(), BE = CurFunc->end();
         BI != BE; ++BI) {
      for (BasicBlock::iterator I = BI->begin(), E = BI->end();
           I != E; ++I) {
        CallInst *CI = dyn_cast<CallInst>(I);
        //outs() << "------CI------\n";
        if (CI == NULL || !CI->getCalledFunction()->hasFnAttribute("Address")) {
          //outs() << "Continue?\n";
          continue;
        }
        //CI->dump();
        StringRef AddrStr =
          CI->getCalledFunction()->getFnAttribute("Address").getValueAsString();
        uint64_t Addr;
        AddrStr.getAsInteger(10, Addr);
        DEBUG(outs() << "Read Address as: " << format("%1" PRIx64, Addr)
          << ", " << AddrStr << "\n");
        StringRef FName = Dis->getFunctionName(Addr);
        // Change sections to check if function address is paired with a 
        // relocated function and then set function name accordingly
        StringRef SectionName;
        object::SectionRef Section = Dis->getSectionByAddress(Addr);
        Dis->setSection(Section);
        Dis->getRelocFunctionName(Addr, FName);
        Section = Dis->getSectionByAddress(Address);
        Dis->setSection(Section);
        CI->getCalledFunction()->setName(FName);
        Function *NF = Mod->getFunction(FName);
        if (Addr != 0 && (NF == NULL || NF->empty())) {
          Children.push_back(Addr);
        }
      }
    }
  } while (Children.size() != 0); // While there are children, decompile
}

Function* Decompiler::decompileFunction(unsigned Address) {
  // Check that Address is inside the current section.
  // TODO: Find a better way to do this check. What we really care about is
  // avoiding reads to library calls and areas of memory we can't "see".
  const object::SectionRef Sect = Dis->getCurrentSection();
  uint64_t SectStart, SectEnd;
  SectStart = Sect.getAddress();
  SectEnd = Sect.getSize();
  SectEnd += SectStart;
  if (Address < SectStart || Address > SectEnd) {
    errs() << "Address out of bounds for section (is this a library call?): "
           << format("%1" PRIx64, Address) << "\n";
    return NULL;
  }

  MachineFunction *MF = Dis->disassemble(Address);

  // Get Function Name
  // TODO: Determine Function Type
  FunctionType *FType = FunctionType::get(Type::getPrimitiveType(*Context,
      Type::VoidTyID), false);
  Function *F =
    cast<Function>(Mod->getOrInsertFunction(MF->getName(), FType));

  if (!F->empty()) {
    return F;
  }

  // Create a basic block to hold entry point (alloca) information
  BasicBlock *entry = getOrCreateBasicBlock("entry", F);

  // For each basic block
  MachineFunction::iterator BI = MF->begin(), BE = MF->end();
  while (BI != BE) {
    //outs() << "-----BI------\n";
    //BI->dump();
    // Add branch from "entry"
    if (BI == MF->begin()) {
      entry->getInstList().push_back(
        BranchInst::Create(getOrCreateBasicBlock(BI->getName(), F)));
    } else {
      getOrCreateBasicBlock(BI->getName(), F);
    }
    ++BI;
  }

  BI = MF->begin();
  while (BI != BE) {
    BI->dump();
    if (decompileBasicBlock(BI, F) == NULL) {
      printError("Unable to decompile basic block!");
    }
    ++BI;
  }

  // During Decompilation, did any "in-between" basic blocks get created?
  // Nothing ever splits the entry block, so we skip it.
  for (Function::iterator I = ++F->begin(), E = F->end(); I != E; ++I) {
    if (!(I->empty())) {
      continue;
    }

    // Right now, the only way to get the right offset is to parse its name
    // it sucks, but it works.
    StringRef Name = I->getName();
    if (Name == "end" || Name == "entry") continue; // these can be empty

    size_t Off = F->getName().size() + 1;
    size_t Size = Name.size() - Off;

    //outs() << "-----I------\n";
    //outs() << "Name: " << Name << " Offset: " << Off << " Size: " << Size << "\n";
    //I->dump();

    StringRef BBAddrStr = Name.substr(Off, Size);
    unsigned long long BBAddr;
    getAsUnsignedInteger(BBAddrStr, 10, BBAddr);
    BBAddr += Address;
    DEBUG(errs() << "Split Target: " << Name << "\t Address: "
                 << BBAddr << "\n");
    // split Block at AddrStr
    Function::iterator SB;      // Split basic block
    BasicBlock::iterator SI, SE;    // Split instruction
    // Note the ++, nothing ever splits the entry block.
    for (SB = ++F->begin(); SB != E; ++SB) {
      DEBUG(SB->dump());
      if (SB->empty() || BBAddr < getBasicBlockAddress(SB)) {
        continue;
      }
      assert(SB->getTerminator() && "Decompiler::decompileFunction - getTerminator (missing llvm unreachable?)");
      DEBUG(outs() << "SB: " << SB->getName()
              << "\tRange: " << Dis->getDebugOffset(SB->begin()->getDebugLoc())
              << " " << Dis->getDebugOffset(SB->getTerminator()->getDebugLoc())
              << "\n");
      if (BBAddr > Dis->getDebugOffset(SB->getTerminator()->getDebugLoc())) {
        continue;
      }

      // Reorder instructions based on Debug Location
      sortBasicBlock(SB);
      DEBUG(errs() << "Found Split Block: " << SB->getName() << "\n");
      // Find iterator to split on.
      for (SI = SB->begin(), SE = SB->end(); SI != SE; ++SI) {
        // outs() << "SI: " << SI->getDebugLoc().getLine() << "\n";
        if (Dis->getDebugOffset(SI->getDebugLoc()) == BBAddr) break;
        if (Dis->getDebugOffset(SI->getDebugLoc()) > BBAddr) {
          errs() << "Could not find address inside basic block!\n"
                 << "SI: " << Dis->getDebugOffset(SI->getDebugLoc()) << "\n"
                 << "BBAddr: " << BBAddr << "\n";
          break;
        }
      }
      break;
    }
    if (!SB || SI == SE || SB == E) {
      errs() << "Decompiler: Failed to find instruction offset in function!\n";
      continue;
    }
    // outs() << SB->getName() << " " << SI->getName() << "\n";
    // outs() << "Creating Block...";
    splitBasicBlockIntoBlock(SB, SI, I);
  }

  // Clean up unnecessary stores and loads
  FunctionPassManager FPM(Mod);
  //Line below adds decompiler optimization.  Function pass manager for optimization.
  //    This is where to focus for type recovery.
  // FPM.add(createPromoteMemoryToRegisterPass()); // See Scalar.h for more.
  //FPM.add(createTypeRecoveryPass());
  FPM.run(*F);

  return F;
}

void Decompiler::sortBasicBlock(BasicBlock *BB) {
  BasicBlock::InstListType *Cur = &BB->getInstList();
  BasicBlock::InstListType::iterator P, I, E, S;
  I = Cur->begin();
  E = Cur->end();
  while (I != E) {
    P = I;
    if (++I == E) {
      break; // Note the terminator is always last instruction
    }
    if (Dis->getDebugOffset(P->getDebugLoc())
      <= Dis->getDebugOffset(I->getDebugLoc())) {
      continue;
    }
    while (--P != Cur->begin()
      && Dis->getDebugOffset(P->getDebugLoc())
      > Dis->getDebugOffset(I->getDebugLoc())) {
      // Do nothing.
    }
    // Insert at P, remove at I
    S = I;
    ++S;
    Instruction *Tmp = &(*I);
    Cur->remove(I);
    Cur->insertAfter(P, Tmp);
    I = S;
  }
  I = Cur->begin();
  E = Cur->end();
  while (I != E) {
    // outs() << "Line #: " << I->getDebugLoc().getLine() << "\n";
    ++I;
  }
}

// This is basically the split basic block function but it does not create
// a new basic block.
void Decompiler::splitBasicBlockIntoBlock(Function::iterator Src,
  BasicBlock::iterator FirstInst, BasicBlock *Tgt) {
  assert(Src->getTerminator() && "Can't use splitBasicBlock on degenerate BB!");
  assert(FirstInst != Src->end() &&
         "Trying to get me to create degenerate basic block!");

  Tgt->moveAfter(Src);

  // Move all of the specified instructions from the original basic block into
  // the new basic block.
  Tgt->getInstList().splice(Tgt->end(), Src->getInstList(),
    FirstInst, Src->end());

  // Add a branch instruction to the newly formed basic block.
  BranchInst *BI = BranchInst::Create(Tgt, Src);
  // Set debugLoc to the instruction before the terminator's DebugLoc.
  // Note the pre-inc which can confuse folks.
  BI->setDebugLoc((++Src->rbegin())->getDebugLoc());

  // Now we must loop through all of the successors of the New block (which
  // _were_ the successors of the 'this' block), and update any PHI nodes in
  // successors.  If there were PHI nodes in the successors, then they need to
  // know that incoming branches will be from New, not from Old.
  //
  for (succ_iterator I = succ_begin(Tgt), E = succ_end(Tgt); I != E; ++I) {
    // Loop over any phi nodes in the basic block, updating the BB field of
    // incoming values...
    BasicBlock *Successor = *I;
    PHINode *PN;
    for (BasicBlock::iterator II = Successor->begin();
         (PN = dyn_cast<PHINode>(II)); ++II) {
      int IDX = PN->getBasicBlockIndex(Src);
      while (IDX != -1) {
        PN->setIncomingBlock((unsigned)IDX, Tgt);
        IDX = PN->getBasicBlockIndex(Src);
      }
    }
  }
}


void Decompiler::printInstructions(formatted_raw_ostream &Out,
  unsigned Address) {
  Out << *(decompileFunction(Address));
}

BasicBlock* Decompiler::decompileBasicBlock(MachineBasicBlock *MBB,
  Function *F) {
  // Create a Selection DAG of MachineSDNodes
  DAG = createDAGFromMachineBasicBlock(MBB);

  if (ViewMCDAGs) {
    MBB->print(Infos);
    DAG->viewGraph(MBB->getName());
  }

  // Run the engine to decompile into SDNodes
  InvISel->SetDAG(DAG);
  DAG->AssignTopologicalOrder();
  // This sets the use on the first node and prevents root from being deleted.
  HandleSDNode Dummy(DAG->getRoot());
  // Start at root and go to entry token
  SelectionDAG::allnodes_iterator ISelPosition(DAG->getRoot().getNode());
  ++ISelPosition;

  // Make sure that ISelPosition gets properly updated when nodes are deleted
  // in calls made from this function.
  ISelUpdater ISU(*DAG, ISelPosition);

  while (ISelPosition != DAG->allnodes_begin()) {
    SDNode *Node = --ISelPosition;

    for (SelectionDAG::allnodes_iterator beg = DAG->allnodes_begin(),
           end = DAG->allnodes_end(); beg != end; ++beg) {
      DEBUG(errs() << "Current Node: ";);
      DEBUG(Node->dump());
      DEBUG(beg->dump());
    }

    // Skip dead nodes
    // if (Node->use_empty())
    //   continue;

    SDNode *ResNode = InvISel->Transmogrify(Node);

    if (ResNode == Node || Node->getOpcode() == ISD::DELETED_NODE)
      continue;

    if (ResNode) {
      DAG->ReplaceAllUsesWith(Node, ResNode);
    }
    if (Node->use_empty()) {
      DAG->RemoveDeadNode(Node);
    }
  }
  DAG->setRoot(Dummy.getValue());

  printDAG(DAG);
  if (ViewIRDAGs) {
    DAG->viewGraph(MBB->getName());
  }

  // Create a new basic block (if necessary)
  BasicBlock *BB = getOrCreateBasicBlock(MBB->getName(), F);

  // Convert the SDNodes into instructions inside the basic block
  // Infos << "OP_END: " << ISD::BUILTIN_OP_END << "\n";
  // Note: there are about 180 or so ISD's, and only a subset are
  // instructions.
  std::stack<SDNode *> NodeStack;
  std::map<SDValue, Value*> OpMap;
  NodeStack.push(DAG->getEntryNode().getNode());
  SDNode *CurNode;
  Emitter->setDAG(DAG);
  Emitter->getIRB()->SetInsertPoint(BB);
  // TODO: Fix this bitrot...
  while (!NodeStack.empty()) {
    CurNode = NodeStack.top();
    NodeStack.pop();
    Emitter->EmitIR(BB, CurNode, NodeStack, OpMap);
  }
  Emitter->endDAG();
  free(DAG);

  return BB;
}

SelectionDAG* Decompiler::createDAGFromMachineBasicBlock(
  MachineBasicBlock *MBB) {

  SelectionDAG *DAG =
    new SelectionDAG(*Dis->getMCDirector()->getTargetMachine(),
      CodeGenOpt::Default);
  DAG->init(*MBB->getParent());
  SDValue prevNode(DAG->getEntryNode());

  std::pair<SDValue, SDValue> NullVal;
  IndexedMap<std::pair<SDValue, SDValue> > Deps;
  Deps.grow(Dis->getMCDirector()->getMCRegisterInfo()->getNumRegs());
  for (MachineBasicBlock::iterator I = MBB->instr_begin(), E = MBB->instr_end();
       I != E; ++I) {
    // Need these (in this order) to create an SDNode for the inst
    unsigned OpCode = I->getOpcode();
    SDLoc Loc;
    std::vector<EVT> ResultTypes;
    SmallVector<SDValue, 8> Ops;

    // Detect Chain node and add prevNode to ops list
    bool isChain = false;
    if (I->mayLoad() || I->mayStore() || I->isBranch() || I->isReturn()
      || I->isCall()) {
      isChain = true;
    }

    // Parse Operands for the Instruction
    SmallVector<MachineOperand*, 8> Defs;
    for (unsigned i = 0, e = I->getNumOperands(); i != e; ++i) {
      MachineOperand *MOp = &I->getOperand(i);
      if (MOp->isReg()) {
        unsigned Reg = MOp->getReg();
        // NOTE: These are register definitions by the instruction, which must
        // be processed AFTER creating the SDNode.
        if (MOp->isDef()) {
          Defs.push_back(MOp);
          ResultTypes.push_back(Dis->getMCDirector()->getRegType(Reg));
        } else {
          // Always create a CopyFromReg Node to define a register.
          // These are also inserted into the Chain.
          Deps[Reg].first = DAG->getCopyFromReg(prevNode, Loc, Reg,
            Dis->getMCDirector()->getRegType(Reg));
          Deps[Reg].first->setDebugLoc(I->getDebugLoc());
          prevNode = SDValue(Deps[Reg].first.getNode(), 1);
          Ops.push_back(Deps[Reg].first);
        }
        continue;
      } else if (MOp->isImm()) {
        // FIXME: Using MVT::i32 here is kinda messed up, we should be able
        // to detect what type it is.
        // As it stands, this will cause Inverse dag selector to fail to match
        // on 64 bit instructions.
        Ops.push_back(DAG->getConstant(MOp->getImm(), EVT(MVT::i32), false));
      } else {
        Ops.push_back(DAG->getUNDEF(EVT(MVT::i32)));
      }
    }

    //assert(Ops.size() && "Number of ops cannot be 0!");
    if(Ops.size() == 0){
      //outs() << "Op Size == 0: " << I->getOpcode() << "\n";
      std::string msg = "Op Size == 0: ";
      msg += I->getOpcode();
      msg += "\n";
      printInfo(msg);
    }

    // If we were a chain, add MVT::Other to ResultTypes list
    if (isChain) {
      ResultTypes.push_back(EVT(MVT::Other));
      Ops.insert(Ops.begin(), prevNode);
    }

    //This if block handles NOPs (hopefully just) and the else block handles everything else
    //  NOPs appear to be the only OpCode that has a size of 0, which breaks getMachineNode
    //  We are going to replace a NOP with an abstract CFR and then C2R which is extremely
    //  similar to how NOPs are treated in the hardware (xchg).  It should also leave any
    //  flag registers alone.
    DEBUG(errs() << "Decompiler::createDAGFromMachineBasicBlock dumping I: \tOpCode: "
              << OpCode << "\tSize: " << ResultTypes.size() << "\n");
    DEBUG(I->dump());
    if(ResultTypes.size() == 0){
      uint64_t Address = Dis->getDebugOffset(I->getDebugLoc());
      const long int InstrSize = Dis->getMCInst(Address)->size();
      for(int j = 0; j<= InstrSize; j++){
        DebugLoc *Location = Dis->setDebugLoc(Address+j);
        //(unsigned) 1 should be a register on any platform.
        SDValue CFRNode = DAG->getCopyFromReg(prevNode, Loc, (unsigned) 1, MVT::i32);
        CFRNode.getNode()->setDebugLoc(*Location);
        SDValue C2RNode = DAG->getCopyToReg(SDValue(CFRNode.getNode(),1), Loc, (unsigned) 1, CFRNode);
        C2RNode.getNode()->setDebugLoc(*Location);
        prevNode = C2RNode;
      }
    } else {

      MachineSDNode *MSD = DAG->getMachineNode(OpCode, Loc, ResultTypes, Ops);
      MSD->setDebugLoc(I->getDebugLoc());
      MSD->setMemRefs(I->memoperands_begin(), I->memoperands_end());

      // If we were a chain, make us the prevNode.
      if (isChain) {
        prevNode = SDValue(MSD, ResultTypes.size() - 1);
      }
      // Update instruction Defs (should always get registers here)
      for (unsigned i = 0, e = Defs.size(); i != e; ++i) {
        Deps[Defs[i]->getReg()].first = SDValue(MSD, i);
        // Also track the chain at the time this reg is defined (for insert later)
        Deps[Defs[i]->getReg()].second = prevNode;
        // Add CopyToReg for every register definition
        SDValue CTR = DAG->getCopyToReg(prevNode, Loc, Defs[i]->getReg(),
            SDValue(MSD, i));
        CTR.getNode()->setDebugLoc(I->getDebugLoc());
        prevNode = CTR;
      }
    }
  }
  DAG->setRoot(prevNode);

  return DAG;
}

BasicBlock* Decompiler::getOrCreateBasicBlock(unsigned Address, Function *F) {
  // Grab offset from the first basic block in the function
  BasicBlock *FB =
    getOrCreateBasicBlock(StringRef(Twine(F->getName() + "+0").str()), F);
  if (FB->begin() == FB->end()) {
    printError("Cannot find by address when BasicBlock '+0' is empty!");
    return NULL;
  }
  uint64_t FuncAddr = getBasicBlockAddress(FB);

  // Get Target block offset (and check if the bb is inside this func!)
  if (FuncAddr > Address) {
    printError("Address is before the function starts!");
    // TODO: What do we do in this situation?
    return NULL;
  }

  uint64_t TBAddr = Address - FuncAddr;
  std::string TBName;
  // Find and/or create the basic block
  raw_string_ostream TBOut(TBName);
  TBOut << F->getName() << "+" << TBAddr;

  return getOrCreateBasicBlock(StringRef(TBOut.str()), F);
}

BasicBlock* Decompiler::getOrCreateBasicBlock(StringRef BBName, Function *F) {
  // Set this basic block as the target
  BasicBlock *BBTgt = NULL;
  Function::iterator BI = F->begin(), BE = F->end();
  while (BI->getName() != BBName && BI != BE) ++BI;
  if (BI == BE) {
    BBTgt =
      BasicBlock::Create(*(Dis->getMCDirector()->getContext()), BBName, F);
  } else {
    BBTgt = &(*BI);
  }

  return BBTgt;
}

void Decompiler::printSDNode(std::map<SDValue, std::string> &OpMap,
    std::stack<SDNode *> &NodeStack, SDNode *CurNode, SelectionDAG *DAG) {
  static unsigned curVR = 0;

  uint16_t Opc = CurNode->getOpcode();

  // What are the node's outputs?
  //   - MVT::Other's get added to a queue, and CurNode takes the next one.
  //   - Value Types get map entries (so you can look them up later) and
  //     nominal numerical names if CopyToReg is not a user.

  // Who uses this node (so we can find the next node)
  for (SDNode::use_iterator I = CurNode->use_begin(), E = CurNode->use_end();
      I != E; ++I) {
    // Save any chain uses to the Nodestack (to guarantee they get evaluated)
    //I->dump();
    //outs() << "EVT String: " << I.getUse().getValueType().getEVTString() << "\n";
    if (I.getUse().getValueType() == MVT::Other) {
      NodeStack.push(*I);
      continue;
    }
    // If there is a CopyToReg user, set the register name to the OpMap
    if (I->getOpcode() == ISD::CopyToReg && Opc != ISD::CopyFromReg
        && Opc != ISD::Constant && Opc != ISD::ConstantFP) {
      const RegisterSDNode *R = dyn_cast<RegisterSDNode>(I->getOperand(1));
      if (R) {
        std::string RegName;
        raw_string_ostream RP(RegName);
        RP
            << PrintReg(R->getReg(),
              DAG ? DAG->getTarget().getSubtargetImpl()->getRegisterInfo() : 0);
        OpMap[I.getUse().get()] = RP.str();
      } else {
        errs() << "CopyToReg with no register!?\n";
      }
    }
  }

  // Handle cases which do not print instructions
  switch (Opc) {
  case ISD::EntryToken:
    curVR = 0;
  case ISD::HANDLENODE:
  case ISD::CopyToReg: {
    if (CurNode->getNumOperands() != 3)
      return;
    // If the 2nd operand doesn't have a chain, add it to the list, too
    SDNode *Op2 = CurNode->getOperand(2).getNode();
    if (Op2->getValueType(Op2->getNumValues() - 1) != MVT::Other) {
      printSDNode(OpMap, NodeStack, Op2, DAG);
      //NodeStack.push(Op2);
    }
    // If the Op is non-printable, create an equivalence statement
    // and print it.
    switch (Op2->getOpcode()) {
    case ISD::CopyFromReg:
    case ISD::Constant:
    case ISD::ConstantFP: {
      const RegisterSDNode *R = dyn_cast<RegisterSDNode>(
          CurNode->getOperand(1));

      // call print to make sure OpMap is set up
      outs()
        << PrintReg(R->getReg(),
          DAG ? DAG->getTarget().getSubtargetImpl()->getRegisterInfo() : 0)
          << " = " << OpMap[SDValue(Op2, 0)] << "\n";
    }
    default:
      break;
    }
  }
    return;
  case ISD::CopyFromReg: {
    const RegisterSDNode *R = dyn_cast<RegisterSDNode>(CurNode->getOperand(1));
    if (R) {
      std::string RegName;
      raw_string_ostream RP(RegName);
      RP << PrintReg(R->getReg(),
        DAG ? DAG->getTarget().getSubtargetImpl()->getRegisterInfo() : 0);
      OpMap[SDValue(CurNode, 0)] = RP.str();
    } else {
      errs() << "CopyFromReg with no register!?\n";
    }
    return;
  }
  case ISD::Constant:
  case ISD::ConstantFP: {
    std::string CName;
    raw_string_ostream CP(CName);
    if (const ConstantSDNode *CSDN = dyn_cast<ConstantSDNode>(CurNode)) {
      CP << "Constant<" << CSDN->getAPIntValue() << '>';
    } else if (const ConstantFPSDNode *CSDN = dyn_cast<ConstantFPSDNode>(
        CurNode)) {
      if (&CSDN->getValueAPF().getSemantics() == &APFloat::IEEEsingle)
        CP << "FPConstant<" << CSDN->getValueAPF().convertToFloat() << '>';
      else if (&CSDN->getValueAPF().getSemantics() == &APFloat::IEEEdouble)
        CP << "FPConstant<" << CSDN->getValueAPF().convertToDouble() << '>';
    }
    OpMap[SDValue(CurNode, 0)] = CP.str();
    return;
  }
  default:
    break;
  }

  // What are the node's inputs? Output each operand first
  std::vector<SDValue> Ins;
  for (unsigned i = 0, e = CurNode->getNumOperands(); i != e; ++i) {
    SDValue OpVal = CurNode->getOperand(i);
    // Chain and undefined vals get dropped from output.
    if (OpVal.getValueType() == MVT::Other
        || OpVal.getNode()->getOpcode() == ISD::UNDEF) {
      continue;
    }

    std::map<SDValue, std::string>::iterator MapItr = OpMap.find(OpVal);
    if (MapItr == OpMap.end()) {
      printSDNode(OpMap, NodeStack, CurNode->getOperand(i).getNode(), DAG);
    }

    Ins.push_back(OpVal);
  }

  // Handle the values generated by this operand
  std::vector<SDValue> Outs;
  for (unsigned i = 0, e = CurNode->getNumValues(); i != e; ++i) {
    if (CurNode->getValueType(i) == MVT::Other) {
      continue;
    }
    SDValue CurVal(CurNode, i);
    if (OpMap.find(CurVal) == OpMap.end()) {
      std::stringstream SS;
      SS << "%" << curVR;
      OpMap[CurVal] = SS.str();
      curVR++;
    }
    Outs.push_back(CurVal);
  }

  // Build the string stream to output Outs, Op, Ins
  std::stringstream Out;
  for (unsigned i = 0, e = Outs.size(); i != e; ++i) {
    Out << OpMap[Outs[i]];
    if (i == (e - 1)) {
      Out << " = ";
    } else {
      Out << ", ";
    }
  }

  Out << CurNode->getOperationName(DAG) << " ";

  for (unsigned i = 0, e = Ins.size(); i != e; ++i) {
    Out << OpMap[Ins[i]];
    if (i != (e - 1)) {
      Out << ", ";
    }
  }

  Out << "\n";

  outs() << Out.str();
}

///===---------------------------------------------------------------------===//
/// printDAG - outputs DAG elements in 3-address format
///
/// This is a temporary function to just get useful output until we can generate
/// an emitter that emits LLVM Instructions and Values.
///
/// @param DAG - The current DAG element to print from
///
void Decompiler::printDAG(SelectionDAG *DAG) {
  std::stack<SDNode *> NodeStack;
  std::map<SDValue, std::string> OpMap;
  NodeStack.push(DAG->getEntryNode().getNode());
  SDNode *CurNode;
  while (!NodeStack.empty()) {
    CurNode = NodeStack.top();
    NodeStack.pop();
    printSDNode(OpMap, NodeStack, CurNode, DAG);
  }
}

// Note: Users should not use this function if the BB is empty.
uint64_t Decompiler::getBasicBlockAddress(BasicBlock *BB) {
  if (BB->empty()) {
    errs() << "Empty basic block encountered, these do not have addresses!\n";
    return 0;                   // In theory, a BB could have an address of 0
                                // in practice it is invalid.
  } else {
    return Dis->getDebugOffset(BB->begin()->getDebugLoc());
  }
}

} // End namespace fracture
