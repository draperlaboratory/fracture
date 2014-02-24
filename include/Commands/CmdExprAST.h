//===--- ExprAST - Expression base class for parser--------------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// CmdExprAST -- Expression class for commands.
//
// Author: Richard Carback (rtc1032) <rcarback@draper.com>
// Date: August 28, 2013
//===----------------------------------------------------------------------===//


#ifndef CMDEXPRAST_H
#define CMDEXPRAST_H

#include "ExprAST.h"

#include <sys/wait.h>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <unistd.h>

typedef void (*CommandsFptr)(std::vector<std::string> &);
typedef std::map<std::string, CommandsFptr> CommandsMap;

class CmdExprAST: public ExprAST {
  std::vector<std::string> CommandWords;
  CommandsMap CmdMap;
  std::string ProgramName;
 public: //TODO: Document public commands!
  CmdExprAST(std::vector<std::string> &cw, CommandsMap* cm, std::string* pn)
    : CommandWords(cw), CmdMap(*cm), ProgramName(*pn) { }

  std::vector<std::string> getCommandWords();
  void executeForkedCommand();
  void Codegen();
};

#endif /* CMDEXPRAST_H */
