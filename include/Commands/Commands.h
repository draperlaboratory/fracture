//===--- dish/Commands.h - Command Line Parser-------------------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Simple (right now) command line parsing functions for an interactive shell
// system.
//
// Note: Desired features list:
//        1. Quick-Action Keys (e.g., press ? and see help for this subcmd.)
//        2. Optional Modifiers (e.g. ability to set options)
//        3. Command History
//        4. Command/SubCommand termination w/ pointers to a function that
//           calls the commands.
//        5. Pipe and I/O redirect and external command support.
//        6. Parameter parsing (instead of just sending the whole command line
//           and letting the function sort it out).
//
// TODO: Add functions to register "ConsistencyCheck", "error"
//       and "Quit" commands.
//
// Author: Richard Carback (rtc1032) <rcarback@draper.com>
// Date: Sep 17, 2012
//===----------------------------------------------------------------------===//

#ifndef COMMANDS_H
#define COMMANDS_H

#include "utils.h"

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <sys/wait.h>

#include "ExprAST.h"
#include "BinaryExprAST.h"
#include "CmdExprAST.h"


// Various values to indicate IO redirection
enum IORedirectMode {
  NoRedirection = 0, Overwrite = 1, Append = 2
};

enum Token {
  tok_eof = -1, tok_command = -2
};

class Commands {
public: // TODO: Document public commands!
  Commands();
  void registerCommand(std::string CommandString, CommandsFptr Function);
  void runShell(std::string Prompt);
  CommandsMap getCmdMap() {
    return CmdMap;
  }
  virtual
  ~Commands();
private:
  std::map<char, int> BinopPrecedence;
  std::string CommandLine;
  std::string CommandStr;
  size_t Index;
  int LastChar;
  int CurTok;

  CommandsMap CmdMap;

  int getCh();
  int getTok();
  int getNextToken();
  int getTokPrecedence();
  int isBinOp(const char Ch);
  int isEscapeSequence(char c);
  void handleCommand();

  ExprAST* Error(const char *Str);
  ExprAST* SyntaxError(std::string Str);

  ExprAST* parseCommand(std::vector<std::string> CommandWords);
  ExprAST* parseExpression();
  ExprAST* parseCommandLine();
  ExprAST* parsePrimary();
  ExprAST* parseBinOpRHS(int ExprPrec, ExprAST* LHS);

  std::vector<std::string> parse(std::string Str);
  void handleRedirection(std::vector<std::string> PipedTokens, const char* Filename,
      IORedirectMode Mode);
  void handleCommandLine();
  std::string getCommandLine();
};

#endif /* COMMANDS_H */
