//===--- utils.cpp - simple utilities ---------------------------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// strsplit function. put other utils here as needed.
//
// Author: rtc1032
// Date: Sep 18, 2012
//
//===----------------------------------------------------------------------===//

#include "utils.h"

template <class S>
std::vector<S> utils::split(const S &Splitee,
                            const S &Deliminator) {
  std::vector<S> Result;
  int Start, End;

  if (Splitee.size() == 0 || Deliminator.size() == 0
      || Deliminator.size() >= Splitee.size())
    return Result;

  End = -1;
  do {
    Start = End + 1;
    End = Splitee.find_first_of(Deliminator, Start);
    // Skip empty objects
    if (Start != End)
      Result.push_back(Splitee.substr(Start, End-Start));
  } while (End != S::value_type::npos);

  return Result;
}
