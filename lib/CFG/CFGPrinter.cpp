//===--- CFGPrinter.cpp - Pretty-printing of CFGs ----------------*- C++ -*-==//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2015 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://swift.org/LICENSE.txt for license information
// See http://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This file defines the logic to pretty-print CFGs, Instructions, etc.
//
//===----------------------------------------------------------------------===//

#include "swift/CFG/CFGVisitor.h"
#include "swift/AST/Decl.h"
#include "swift/AST/Expr.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/OwningPtr.h"
using namespace swift;

struct ID {
  enum {
    BasicBlock, SSAValue
  } Kind;
  unsigned Number;
};

raw_ostream &operator<<(raw_ostream &OS, ID i) {
  switch (i.Kind) {
  case ID::BasicBlock: OS << "bb"; break;
  case ID::SSAValue: OS << '%'; break;
  }
  return OS << i.Number;
}


namespace {
/// CFGPrinter class - This is the internal implementation details of printing
/// for CFG structures.
class CFGPrinter : public CFGVisitor<CFGPrinter> {
  raw_ostream &OS;

  llvm::DenseMap<const BasicBlock *, unsigned> BlocksToIDMap;
  ID getID(const BasicBlock *B);

  llvm::DenseMap<const Instruction*, unsigned> InstructionToIDMap;
  ID getID(const Instruction *I);
  ID getID(const BasicBlockArg *BBarg);
  ID getID(const CFGValue &V);

public:
  CFGPrinter(raw_ostream &OS) : OS(OS) {
  }

  void print(const BasicBlock *BB) {
    OS << getID(BB) << ":\t";

    OS << " ; Preds:";
    for (const BasicBlock *B : BB->getPreds())
      OS << ' ' << getID(B);
    OS << '\n';

    for (const Instruction &I : *BB)
      print(&I);

    OS << '\n';
  }

  //===--------------------------------------------------------------------===//
  // Instruction Printing Logic

  void print(const Instruction *I) {
    OS << "  " << getID(I) << " = ";
    visit(const_cast<Instruction*>(I));
    OS << '\n';
  }

  void visitInstruction(Instruction *I) {
    assert(0 && "CFGPrinter not implemented for this instruction!");
  }

  void visitCallInst(CallInst *CI) {
    OS << "Call(fn=" << getID(CI->function);
    auto args = CI->arguments();
    if (!args.empty()) {
      bool first = true;
      OS << ",args=(";
      for (auto arg : args) {
        if (first)
          first = false;
        else
          OS << ' ';
        OS << getID(arg);
      }
      OS << ')';
    }
    OS << ')';
  }

  void visitDeclRefInst(DeclRefInst *DRI) {
    OS << "DeclRef(decl=" << DRI->expr->getDecl()->getName() << ')';
  }
  void visitIntegerLiteralInst(IntegerLiteralInst *ILI) {
    const auto &lit = ILI->literal->getValue();
    OS << "Integer(val=" << lit << ",width=" << lit.getBitWidth() << ')';
  }
  void visitLoadInst(LoadInst *LI) {
    OS << "Load(lvalue=" << getID(LI->lvalue) << ')';
  }
  void visitThisApplyInst(ThisApplyInst *TAI) {
    OS << "ThisApply(fn=" << getID(TAI->function) << ",arg="
       << getID(TAI->argument) << ')';
  }
  void visitTupleInst(TupleInst *TI) {
    OS << "Tuple(";
    bool isFirst = true;
    for (const auto &Elem : TI->elements()) {
      if (isFirst)
        isFirst = false;
      else
        OS << ',';
      OS << getID(Elem);
    }
    OS << ')';
  }
  void visitTypeOfInst(TypeOfInst *TOI) {
    OS << "TypeOf(type=" << TOI->Expr->getType().getString() << ')';
  }

  void visitReturnInst(ReturnInst *RI) {
    OS << "Return";
    if (RI->returnValue) {
      OS << '(' << getID(RI->returnValue) << ')';
    }
  }

  void visitUncondBranchInst(UncondBranchInst *UBI) {
    OS << "br " << getID(UBI->targetBlock());
    const UncondBranchInst::ArgsTy Args = UBI->blockArgs();
    if (!Args.empty()) {
      OS << '(';
      for (auto Arg : Args) { OS << "%" << Arg; }
      OS << ')';
    }
  }

  void visitCondBranchInst(CondBranchInst *CBI) {
    OS << "cond_br(cond=";
    OS << "?";
    //      printID(BI.condition);
    OS << ",branches=(" << getID(CBI->branches()[0]);
    OS << ',' << getID(CBI->branches()[1]);
    OS << "))";
  }
};
} // end anonymous namespace

ID CFGPrinter::getID(const BasicBlock *Block) {
  // Lazily initialize the Blocks-to-IDs mapping.
  if (BlocksToIDMap.empty()) {
    unsigned idx = 0;
    for (const BasicBlock &B : *Block->getParent())
      BlocksToIDMap[&B] = idx++;
  }

  ID R = { ID::BasicBlock, BlocksToIDMap[Block] };
  return R;
}

ID CFGPrinter::getID(const Instruction *Inst) {
  // Lazily initialize the instruction -> ID mapping.
  if (InstructionToIDMap.empty()) {
    unsigned idx = 0;
    for (auto &BB : *Inst->getParent()->getParent())
      for (auto &I : BB)
        InstructionToIDMap[&I] = idx++;
  }

  ID R = { ID::SSAValue, InstructionToIDMap[Inst] };
  return R;
}

ID CFGPrinter::getID(const BasicBlockArg *BBArg) {
  // FIXME: Not implemented yet.
  ID R = { ID::SSAValue, ~0U };
  return R;
}

ID CFGPrinter::getID(const CFGValue &Val) {
  if (const Instruction *Inst = Val.dyn_cast<Instruction*>())
    return getID(Inst);
  return getID(Val.get<BasicBlockArg*>());
}

//===----------------------------------------------------------------------===//
// Printing for Instruction, BasicBlock, and CFG
//===----------------------------------------------------------------------===//

void Instruction::dump() const {
  print(llvm::errs());
}

void Instruction::print(raw_ostream &OS) const {
  CFGPrinter(OS).print(this);
}

void BasicBlock::dump() const {
  print(llvm::errs());
}

/// Pretty-print the BasicBlock with the designated stream.
void BasicBlock::print(raw_ostream &OS) const {
  CFGPrinter(OS).print(this);
}

/// Pretty-print the basic block.
void CFG::dump() const {
  print(llvm::errs());
}

/// Pretty-print the basi block with the designated stream.
void CFG::print(llvm::raw_ostream &OS) const {
  CFGPrinter Printer(OS);
  for (const BasicBlock &B : *this)
    Printer.print(&B);
}

