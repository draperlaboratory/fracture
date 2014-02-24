//===--- dish/BinFun.h - BinFun class declaration -----------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// The BinFun class is used to manage the list of BinFuns in an executable
//
//===----------------------------------------------------------------------===//

#ifndef BinFun_H_
#define BinFun_H_

#include <stdint.h>
#include <algorithm>
#include <string>
#include <vector>

//===----------------------------------------------------------------------===//
//                                BinFun Class
//===----------------------------------------------------------------------===//

/// BinFun - a BinFun in an executable
///
class BinFun;
typedef  std::vector<BinFun> FuncVect;
typedef  std::vector<BinFun *> FuncRefVect;

class BinFun
{
private:
	uint64_t    address;	/// The Function address
	uint64_t	size;		/// The Function size
	std::string name;		/// The Function name, if known (executable not stripped)
	FuncRefVect	calls;		/// Vector of Functions called

public:
  BinFun(uint64_t na, uint64_t ns, std::string nn = std::string()) : address(na), size(ns), name(nn) {}
  // virtual ~BinFun();
  // void findcalls(FuncList &funs);					/// Build the calls list
  std::string&  getname()	{ return name; }
  uint64_t getaddress()	{ return address; }
  uint64_t getsize()        { return size; }
  FuncRefVect& getcalls()      { return calls; }
  void addcall(BinFun &f)	{ if (std::find(calls.begin(), calls.end(), &f) == calls.end()) calls.push_back(&f);}
};


#endif /* BinFun_H_ */
