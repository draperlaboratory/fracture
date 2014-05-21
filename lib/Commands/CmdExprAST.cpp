//===--- CmdExprAST - Expression base class for cmds-------------*- C++ -*-===//
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

#include "Commands/CmdExprAST.h"



std::vector<std::string> CmdExprAST::getCommandWords() {
  return CommandWords;
}

void CmdExprAST::executeForkedCommand() {
  const int MAX_ARGS = 20;

  // Initialize array of char* to store arguments
  char* Args[MAX_ARGS];
  for (int i = 0; i < MAX_ARGS; i++) {
    Args[i] = NULL;
  }

  // Convert array of std::string to array of char*
  for (size_t i = 0; i < CommandWords.size(); i++) {
    Args[i] = const_cast<char*>(CommandWords.at(i).c_str());
  }

  // If running the system call fails, report an error and kill
  // the process
  if (execvp(Args[0], Args) < 0) {
    std::stringstream err;
    err << ProgramName.c_str() << ": " << Args[0] << ": command not found";
    Error(err.str().c_str());
    exit(EXIT_FAILURE);
  }
}

void CmdExprAST::Codegen() {
  CommandsMap::iterator CmdIt, CmdEnd;
  CmdEnd = CmdMap.end();

  // If the command is an internal command, executes it and returns; otherwise,
  // we fork the command to a new process as it is a system call which would
  // terminate our process upon exit.

  // Assuming the search algorithm for map is faster than the fuzzy match i'm
  // doing after this, let us check if the whole string exists already.
  std::string CommandWord = CommandWords.front();
  CmdIt = CmdMap.find(CommandWord);
  if (CmdIt != CmdEnd) {
    (*CmdIt->second)(CommandWords);
    return;
  }
  // If the command is found within our defined command map, execute
  // the function associated with that command
  for (CmdIt = CmdMap.begin(); CmdIt != CmdEnd; ++CmdIt) {
    if (CmdIt->first.substr(0, CommandWord.size()) == CommandWord) {
      (*CmdIt->second)(CommandWords);
      return;
    }
  }

  int status;
  pid_t pid;

  pid = fork();

  if (pid == 0) {
    executeForkedCommand();
  } else if (pid > 0) {
    wait(&status);
    // Signal to quit (chosen arbitrarily)
    if (status == 33280) {
      exit(EXIT_SUCCESS);
    }
  } else {
    Error("Fork failed");
    exit(EXIT_FAILURE);
  }
}
