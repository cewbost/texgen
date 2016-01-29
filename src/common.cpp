#include <cstring>

#include "common.h"

namespace Common
{
  std::string app_path;
  SQInteger done = 0;
  
  FileBuffer loadFile(const char* filename)
  {
    FILE* file = fopen(filename, "rb");
    if(file == nullptr)
      throw tgException("unable to open file: %s", filename);
    
    fseek(file, 0, SEEK_END);
    auto size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if(size == 0)
    {
      fclose(file);
      throw tgException("file %s is empty", filename);
    }
    
    std::unique_ptr<char[]> buffer(new char[size]);
    if(fread(buffer.get(), 1, size, file) <= 0)
    {
      fclose(file);
      throw tgException("unable to read from file: %s", filename);
    }
    fclose(file);
    
    return std::make_pair(std::move(buffer), size);
  }
  
  uint32_t hashFile(const FileBuffer& file_buffer)
  {
    constexpr int seed = 37;
  
    union
    {
      uint32_t ui;
      char bytes[4];
    };
    uint32_t hash = 0;
    
    const char* reader = file_buffer.first.get();
    for(unsigned i = 0; i < file_buffer.second - 3; i += 4)
    {
      memcpy(bytes, reader, 4);
      reader += 4;
      hash *= seed;
      hash += ui;
    }
    if(file_buffer.second % 4 != 0)
    {
      ui = 0;
      memcpy(bytes, reader, file_buffer.second % 4);
      hash *= seed;
      hash += ui;
    }
    
    return hash;
  }
}