//===--- Commands.cpp - Command Parser --------------------------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Simple Command parser
//
// Author: Louis Bloom <lbloom@draper.com>
// Date: Sep 17, 2012
//
//===----------------------------------------------------------------------===//

#include "Commands/Commands.h"

#include "config.h"
#ifdef HAVE_EDITLINE_READLINE_H
#include <editline/readline.h>
#else
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include <algorithm>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

std::string ProgramName;

// Given a string, creates a vector of its individual whitespace-separated
// elements.
// All items in quotes are treated as one contiguous block of characters (i.e.
// whitespace is ignored).
std::vector<std::string> Commands::parse(std::string Str) {

  std::vector<std::string> CurCmd;
  std::string CurArg;

  size_t Index = 0;
  size_t End = Str.length();

  while (Index < End) {
    // Everything inside double quotes is one token
    if (Str.at(Index) == '"') {
      // Consume first double quotes
      ++Index;
      // Collect all contents inside double quotes and add to current argument
      while (Index < End && Str.at(Index) != '"') {
        // If a backslash is encountered and there is at least one more
        // character after it, it might be an escape sequence
        if (Str.at(Index) == '\\' && (Index + 1) != End) {
          char NextChar = Str.at(Index + 1);
          if (isEscapeSequence(NextChar))
            Index++;
        }

        CurArg.push_back(Str.at(Index));
        ++Index;
      }
      ++Index;
    }
    // Everything inside single quotes is one token
    else if (Str.at(Index) == '\'') {
      // Consume first single quote
      ++Index;
      // Collect all contents inside single quotes and add to current argument
      while (Index < End && Str.at(Index) != '\'') {
        // If a backslash is encountered and there is at least one more
        // character after it, it might be an escape sequence
        if (Str.at(Index) == '\\' && (Index + 1) != End) {
          char NextChar = Str.at(Index + 1);
          if (isEscapeSequence(NextChar))
            Index++;
        }

        CurArg.push_back(Str.at(Index));
        ++Index;
      }
      ++Index;
    }
    // If this character is whitespace...
    else if (isspace(Str.at(Index))) {
      // add the current argument to the command if it is nonempty
      if (CurArg.length() > 0) {
        CurCmd.push_back(CurArg);
        CurArg.clear();
      }
      // Consume all whitespace
      while (Index < End && isspace(Str.at(Index))) {
        ++Index;
      }
    }
    // Else add this character to the current argument
    else {
      CurArg.push_back(Str.at(Index));
      ++Index;
    }
  }

  if (CurArg.length() > 0) {
    CurCmd.push_back(CurArg);
    CurArg.clear();
  }

  return CurCmd;
}

int Commands::isEscapeSequence(char c) {
  return (c == '\\' || c == '\"' || c == '\'' || c == '\?');
}

Commands::Commands() {
  // Install standard binary operators.
  // 1 is lowest precedence.
  BinopPrecedence['<'] = 20; // highest
  BinopPrecedence['>'] = 20;
  BinopPrecedence['|'] = 10;
}

Commands::~Commands() {
  // TODO Auto-generated destructor stub
}


// Commands::registerCommand : std::string CommandsFptr -> Void
// EFFECT: Registers a given string as a command for an internal function within
// the shell
// @param CommandString - The user-level representation of an internal command
// @param Function - The function to be executed when CommandString is
// encountered
void Commands::registerCommand(std::string CommandString,
    CommandsFptr Function) {
  if (CommandString.size() <= 0 || Function == 0)
    return;

  CommandsMap::iterator CmdIt, CmdEnd;
  CmdEnd = CmdMap.end();

  CmdIt = CmdMap.find(CommandString);
  if (CmdIt != CmdEnd) {
    // TODO: Implement a callback or iostream ctor for printable errors.
    std::cerr << "Commands::Error: Command already exists!" << std::endl;
  }

  CmdMap[CommandString] = Function;
}

//
// Prompts the user for a command line
// @return - The command line entered by the user
///
std::string Commands::getCommandLine() {
  std::string TempPrompt = ProgramName + "> ";
  const char* Prompt = TempPrompt.c_str();
  char* TempLine;

  // Grab an input line from the user. readline automatically
  // allocates space for the line, and returns a pointer to it. We
  // store the pointer in our variable "TempLine".
  TempLine = readline(Prompt);

  // We can convert TempLine to a std::string for ease of use
  std::string CommandLine(TempLine);

  // We have now finished with the input line, so we free the space
  // that readline() allocated for it ("man readline" explains why you
  // should do this).
  free(TempLine);

  return CommandLine;
}

// Primes CurTok with the value of the next token and returns it
int Commands::getNextToken() {
  return CurTok = getTok();
}

// Get the precedence of the pending binary operator token.
int Commands::getTokPrecedence() {
  if (!isascii(CurTok)) {
    return -1;
  }

// Make sure it's a declared binop.
  int TokPrec = BinopPrecedence[CurTok];
  if (TokPrec <= 0)
    return -1;
  return TokPrec;
}

// If Index != End, returns the character at the Index'th position inside
// CommandLine, then increments Index; else returns -1
int Commands::getCh() {
  size_t End = CommandLine.length();
  if (Index == End)
    return EOF;
  return CommandLine.at(Index++);
}

int Commands::getTok() {

  // Skip all whitespace
  while (isspace(LastChar))
    LastChar = getCh();

  // Check for end of command line.
  if (LastChar == EOF)
    return tok_eof;

  // If a binary operator is encountered,
  // return the operator
  else if (isBinOp(LastChar)) {
    int ThisChar = LastChar;
    LastChar = getCh();
    return ThisChar;
  }

  // Else add all of the non-binary operator characters
  // to the current command
  else {
    while (!isBinOp(LastChar) && LastChar != EOF) {
      CommandStr.push_back(LastChar);
      LastChar = getCh();
    }
    return tok_command;
  }
}

// Returns 1 if the character is a binary operator; else returns 0
int Commands::isBinOp(const char Ch) {
  return (Ch == '|' || Ch == '>' || Ch == '<');
}

ExprAST* Commands::Error(const char* Str) {
  fprintf(stderr, "%s: ", ProgramName.c_str());
  fprintf(stderr, "%s\n", Str);
  return 0;
}

ExprAST* Commands::SyntaxError(std::string Str) {
  std::string Source = "syntax error near unexpected token `";
  Source += Str;
  Source += "'";
  return Error(Source.c_str());
}

ExprAST* Commands::parseCommandLine() {
  ExprAST *LHS = parsePrimary();
  if (!LHS)
    return 0;

  return parseBinOpRHS(0, LHS);
}

ExprAST* Commands::parsePrimary() {
  switch (CurTok) {
    default:
      return Error("unknown token when expecting an expression");
    case '>':
      return SyntaxError(">");
    case '<':
      return SyntaxError("<");
    case '|':
      return SyntaxError("|");
    case tok_eof:
      return SyntaxError("newline");
    case tok_command:
      std::vector<std::string> CommandWords = parse(CommandStr);
      CommandStr.clear();
      return parseCommand(CommandWords);
  }
}

ExprAST* Commands::parseBinOpRHS(int ExprPrec, ExprAST* LHS) {
// If this is a binop, find its precedence.
  while (1) {
    int TokPrec = getTokPrecedence();

    // If this is a binop that binds at least as tightly as the current binop,
    // consume it, otherwise we are done.
    if (TokPrec < ExprPrec)
      return LHS;

    // Okay, we know this is a binop.
    int BinOp = CurTok;
    getNextToken(); // eat binop

    // Parse the primary expression after the binary operator.
    ExprAST* RHS = parsePrimary();
    if (!RHS)
      return 0;

    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    int NextPrec = getTokPrecedence();
    if (TokPrec < NextPrec) {
      RHS = parseBinOpRHS(TokPrec + 1, RHS);
      if (RHS == 0)
        return 0;
    }

    // Merge LHS/RHS.
    LHS = new BinaryExprAST(BinOp, LHS, RHS, &ProgramName);
  }
}

ExprAST* Commands::parseCommand(std::vector<std::string> CommandWords) {
  ExprAST* Result = new CmdExprAST(CommandWords, &CmdMap, &ProgramName);
  getNextToken();
  return Result;
}

void Commands::handleCommandLine() {
  LastChar = ' ';
  Index = 0;
  CommandStr.clear();

  do {
    // Prime the next token
    CurTok = getNextToken();

    switch (CurTok) {
      case tok_eof:
        return;
      default:
        if (ExprAST* RunMe = parseCommandLine())
          RunMe->Codegen();
        return;
    }
  } while (CurTok != EOF);
}

//
// Runs the disassembler shell
// @param Prompt - The prompt for the shell
///
void Commands::runShell(std::string Prompt) {

  ProgramName = Prompt;

  while (1) {
    // Prompt the user for a command line
    CommandLine = getCommandLine();

    // If nothing was entered, prompt the user again
    if (CommandLine.length() == 0) {
      continue;
    }

    // Else we add the new line to the history buffer, so that the user
    // can use the up/down arrow keys to navigate through previous responses
    add_history(CommandLine.c_str());

    // Process the command
    handleCommandLine();
  }
}
