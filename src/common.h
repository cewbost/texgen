#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#include <string>
#include <memory>
#include <cstdint>

#include <squirrel/squirrel.h>

class tgException: public std::exception
{
  char msg[512];
  
public:

  template<class... T>
  tgException(T... args)
  {
    snprintf(msg, 512, args...);
  }
  virtual const char* what()const noexcept override
  {
    return msg;
  }
};

class h3dException: public std::exception
{
  char msg[512];
  
public:
  
  template<class... T>
  h3dException(T... args)
  {
    snprintf(msg, 512, args...);
  }
  virtual const char* what()const noexcept override
  {
    return msg;
  }
};

namespace Common
{
  using FileBuffer = std::pair<std::unique_ptr<char[]>, size_t>;

  extern std::string app_path;
  extern SQInteger done;
  FileBuffer loadFile(const char*);
  uint32_t hashFile(const FileBuffer&);
  
  inline std::string path(const char* filename)
  {
    std::string str = app_path + filename;
    return str;
  }
}

#endif
