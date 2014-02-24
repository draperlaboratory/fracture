//===--- BinaryExprAST - Expression class for binops ------------*- C++ -*-===//
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

#include "Commands/BinaryExprAST.h"

void BinaryExprAST::BinOpError(const char Ch) {
  std::stringstream err;
  err << "behavior for the binary operator " << Ch << " is not yet implemented";
  Error(err.str().c_str());
}

void BinaryExprAST::executeBinOp() {
  if (LHS == 0 || RHS == 0)
    return;

  switch (Op) {
    case '|':
      handlePipes();
      exit(0);
    case '>':
      handleOverwrite();
      exit(0);
    case '<':
      handleRead();
      exit(0);
    default:
      BinOpError(Op);
      exit(EXIT_FAILURE);
  }
}

void BinaryExprAST::handleRead() {
  // Make a copy of stdin
  int old_stdin = dup(0);
  // Close stdin
  if (close(0) < 0) {
    Error("Failed to close file descriptor");
    exit(EXIT_FAILURE);
  }
  int fd;

  CmdExprAST* CmdNode = (CmdExprAST*) RHS;

  const char* Filename = CmdNode->getCommandWords().at(0).c_str();

  // Open a file descriptor with the intent of overwriting a file
  // which may already exist
  if ((fd = open(Filename, O_RDONLY, 0777)) < 0) {
    Error("File open failed");
    exit(EXIT_FAILURE);
  }

  LHS->Codegen();

  // Close the file descriptor
  if (close(fd) < 0) {
    Error("Failed to close file descriptor");
    exit(EXIT_FAILURE);
  }
  // Restore stdin to its original value
  if (dup(old_stdin) < 0) {
    Error("dup2 failed");
    exit(EXIT_FAILURE);
  }
}

void BinaryExprAST::handleOverwrite() {
  // Make a copy of stdout
  int old_stdout = dup(1);
  // Close stdout
  if (close(1) < 0) {
    Error("Failed to close file descriptor");
    exit(EXIT_FAILURE);
  }
  int fd;

  CmdExprAST* CmdNode = (CmdExprAST*) RHS;

  const char* Filename = CmdNode->getCommandWords().at(0).c_str();

  // Open a file descriptor with the intent of overwriting a file
  // which may already exist
  if ((fd = open(Filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
    Error("File open failed");
    exit(EXIT_FAILURE);
  }

  LHS->Codegen();

  // Close the file descriptor
  if (close(fd) < 0) {
    Error("Failed to close file descriptor");
    exit(EXIT_FAILURE);
  }
  // Restore stdout to its original value
  if (dup(old_stdout) < 0) {
    Error("dup2 failed");
    exit(EXIT_FAILURE);
  }
}

void BinaryExprAST::handlePipes() {
  int status;
  pid_t pid;

  const int NumFds = 2;
  int MyPipe[NumFds];

  // Initialize pipe
  pipe(MyPipe);

  std::vector<ExprAST*> Nodes;
  Nodes.push_back(LHS);
  Nodes.push_back(RHS);

  for (std::vector<ExprAST*>::iterator It = Nodes.begin(); It != Nodes.end();
       It++) {

    ExprAST* Cmd = (*It);

    pid = fork();
    if (pid == 0) {

      // If not first command,
      // redirect stdin to output of previous process.
      if (It != Nodes.begin()) {
        if (dup2(MyPipe[0], 0) < 0) {
          Error("dup2 failed");
          exit(EXIT_FAILURE);
        }
      }

      // If not last command,
      // redirect stdout to input of next process.
      if (It != Nodes.end() - 1) {
        if (dup2(MyPipe[1], 1) < 0) {
          Error("dup2 failed");
          exit(EXIT_FAILURE);
        }
      }

      // Close all pipes in child
      for (int i = 0; i < NumFds; i++) {
        if (close(MyPipe[i]) < 0) {
          Error("Pipe closure failed in child");
          exit(EXIT_FAILURE);
        }
      }

      // Execute the node's code
      Cmd->Codegen();
      exit(0);

      // This line is never reached
    } else if (pid < 0) {
      Error("Fork failed");
      exit(EXIT_FAILURE);
    }
  }

  // Parent closes all pipes and waits for all of its children
  for (int i = 0; i < NumFds; i++) {
    if (close(MyPipe[i]) < 0) {
      Error("Pipe closure failed in parent");
      exit(EXIT_FAILURE);
    }
  }

  // Parent waits for child to finish
  wait(&status);
}

void BinaryExprAST::Codegen() {
  int status;
  pid_t pid;

  pid = fork();

  if (pid == 0) {
    executeBinOp();
  } else if (pid > 0) {
    wait(&status);
  } else {
    Error("Fork failed");
    exit(EXIT_FAILURE);
  }
}
