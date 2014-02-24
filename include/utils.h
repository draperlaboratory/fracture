//===--- utils.h - [Name] ----------------------------------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// [Description]
//
// Author: rtc1032
// Date: Sep 18, 2012
//
//===----------------------------------------------------------------------===//


#ifndef UTILS_H_
#define UTILS_H_

#include <vector>

namespace utils  {

  template <class S>
  std::vector<S> split(const S &Splitee,
                       const S &Deliminator);

}


#endif /* UTILS_H_ */
