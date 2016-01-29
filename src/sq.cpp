#include <set>

#include <memory>
#include <cstdarg>
#include <cstring>

#include <squirrel/squirrel.h>
#include <squirrel/sqstdio.h>
#include <squirrel/sqstdblob.h>
#include <squirrel/sqstdmath.h>
#include <squirrel/sqstdsystem.h>
#include <squirrel/sqstdstring.h>
#include <squirrel/sqstdaux.h>

#include "common.h"
#include "sqapi.h"
#include "terminal.h"

#include "sq.h"

namespace
{
  bool _inited = false;

  void _squPrint(HSQUIRRELVM v, const char* text, ...)
  {
    va_list list;
    va_start(list, text);
    
    std::unique_ptr<char[]> buffer(new char[256]);
    int read = vsnprintf(buffer.get(), 256, text, list);
    if(read >= 256)
    {
      va_end(list);
      va_start(list, text);
      buffer.reset(new char[read + 1]);
      vsnprintf(buffer.get(), read + 1, text, list);
    }
    va_end(list);
    
    Terminal::printfm("%s\n", buffer.get());
  }
  
  void _squErrPrint(HSQUIRRELVM v, const char* text, ...)
  {
    va_list list;
    va_start(list, text);
    
    std::unique_ptr<char[]> buffer(new char[256]);
    int read = vsnprintf(buffer.get(), 256, text, list);
    if(read >= 256)
    {
      va_end(list);
      va_start(list, text);
      buffer.reset(new char[read + 1]);
      vsnprintf(buffer.get(), read + 1, text, list);
    }
    va_end(list);
    
    Terminal::printfm("%s", buffer.get());
  }
  
  std::set<uint32_t> _executed_nuts;
}

namespace Sq
{
  HSQUIRRELVM vm;
  bool reload = false;
  
  bool executeNut(const char* filename, bool reload)
  {
    auto buffer = Common::loadFile(Common::path(filename).c_str());
    auto hash = Common::hashFile(buffer);
    if(!reload && _executed_nuts.find(hash) != _executed_nuts.end())
      return false;
    _executed_nuts.insert(hash);
    executeCode(buffer.first.get(), buffer.second);
    
    return true;
  }
  
  bool executeNutRawPath(const char* filename, bool reload)
  {
    auto buffer = Common::loadFile(filename);
    auto hash = Common::hashFile(buffer);
    if(!reload && _executed_nuts.find(hash) != _executed_nuts.end())
      return false;
    _executed_nuts.insert(hash);
    executeCode(buffer.first.get(), buffer.second);
    
    return true;
  }
  
  void executeCode(const char* code, size_t size)
  {
    if(size == 0) size = strlen(code);
    
    if(SQ_FAILED(sq_compilebuffer(vm, code, size, "interpreter", true)))
      throw tgException("compilation failed.\n");
    sq_pushroottable(vm);
    if(SQ_FAILED(sq_call(vm, 1, false, true)))
      throw tgException("execution failed.\n");
    sq_pop(vm, 1);
  }
  
  std::vector<std::string> getRootTableSymbols()
  {
    sq_pushroottable(vm);
    sq_pushnull(vm);
    
    std::vector<std::string> result;
    
    while(SQ_SUCCEEDED(sq_next(vm, -2)))
    {
      const SQChar* res;
      if(SQ_SUCCEEDED(sq_getstring(vm, -2, &res)))
        result.push_back(std::string(res));

      sq_pop(vm, 2);
    }
    sq_pop(vm, 2);
    
    return result;
  }
  
  void reloadVM()
  {
    _executed_nuts.clear();
    deinit();
    init();
    Terminal::updateSymbols();
    Terminal::start();
  }
  
  void init()
  {
    vm = sq_open(1024);
    
    sqstd_seterrorhandlers(vm);
    sq_setprintfunc(vm, _squPrint, _squErrPrint);

    sq_pushroottable(vm);
    if(SQ_FAILED(sqstd_register_iolib(vm)))
      Terminal::print("warning: failed to register sqiolib.\n");
    if(SQ_FAILED(sqstd_register_bloblib(vm)))
      Terminal::print("warning: failed to register sqbloblib.\n");
    if(SQ_FAILED(sqstd_register_mathlib(vm)))
      Terminal::print("warning: failed to register sqmathlib.\n");
    if(SQ_FAILED(sqstd_register_systemlib(vm)))
      Terminal::print("warning: failed to register sqsystemlib.\n");
    if(SQ_FAILED(sqstd_register_stringlib(vm)))
      Terminal::print("warning: failed to register sqstringlib.\n");
    
    SqAPI::init();
    
    try
    {
      executeNut("init.nut");
    }
    catch(std::exception& e)
    {
      Terminal::printfm("Error loading init.nut: %s\n", e.what());
    }
    
    _inited = true;
  }
  
  void deinit() noexcept
  {
    if(!_inited)
      return;
    sq_settop(vm, 0);
    sq_close(vm);
  }
}