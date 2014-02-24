//===--- ExprAST - Expression base class for parser--------------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// ExprAST --- Expression Abstract Syntax Tree, provides a base class for
// different types of command line expressions. 
//
// Author: Richard Carback (rtc1032) <rcarback@draper.com>
// Date: August 28, 2013
//===----------------------------------------------------------------------===//


#ifndef EXPRAST_H
#define EXPRAST_H

#include <iostream>

class ExprAST {
public:
  virtual ~ExprAST() {
  }
  virtual void
  Codegen() = 0;

  virtual void Error(const char *Str) {
    std::cerr << "Error: " << Str << std::endl;
  }
};

#endif /* EXPRAST_H */
