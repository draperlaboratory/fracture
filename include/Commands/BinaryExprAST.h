//===--- ExprAST - Expression base class for parser--------------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// BinaryExprAST --- expression class for binary operators.
//
// Author: Richard Carback (rtc1032) <rcarback@draper.com>
// Date: August 28, 2013
//===----------------------------------------------------------------------===//


#ifndef BINARYEXPRAST_H
#define BINARYEXPRAST_H

#include "ExprAST.h"
#include "CmdExprAST.h"

#include <sstream>
#include <cstdlib>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

class BinaryExprAST: public ExprAST {
public:
  BinaryExprAST(char op, ExprAST* lhs, ExprAST* rhs, std::string* pn) 
    : Op(op), LHS(lhs), RHS(rhs), ProgramName(*pn) { }

  void BinOpError(const char Ch);
  void executeBinOp();
  void handleRead();
  void handleOverwrite();
  void handlePipes(); 
  void Codegen();

private:
  char Op;
  ExprAST *LHS, *RHS;
  std::string ProgramName;
};

#endif /* BINARYEXPRAST_H */
