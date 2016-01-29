#ifndef SQ_H_INCLUDED
#define SQ_H_INCLUDED

#include <exception>
#include <vector>
#include <string>

#include <cstdio>

#include <squirrel/squirrel.h>

namespace Sq
{
  extern HSQUIRRELVM vm;
  extern bool reload;
  
  //these are all throwing functions
  bool executeNut(const char*, bool = false);
  bool executeNutRawPath(const char*, bool = false);
  void executeCode(const char*, size_t = 0);
  
  std::vector<std::string> getRootTableSymbols();
  
  void reloadVM();
  
  void init();
  void deinit() noexcept;
}

#endif
