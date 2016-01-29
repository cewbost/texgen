#include "sq.h"
#include "common.h"
#include "h3d.h"
#include "texture_manager.h"
#include "rand.h"
#include "point_set.h"
#include "terminal.h"

#include "sqapi.h"

#include <squirrel/sqstdblob.h>

namespace
{
  //helper functions
  template<int S>
  inline bool _getArrayf(HSQUIRRELVM vm, std::array<float, S> &arr, int idx)
  {
    SQFloat val;
    if(sq_getsize(vm, idx) != S)
    {
      return false;
    }
    sq_pushnull(vm);
    for(int i = 0; i < S; ++i)
    {
      sq_next(vm, idx);
      if(sq_gettype(vm, -1) != OT_FLOAT)
      {
        sq_pop(vm, 3);
        return false;
      }
      sq_getfloat(vm, -1, &val);
      arr[i] = val;
      sq_pop(vm, 2);
    }
    sq_pop(vm, 1);
    return true;
  }
  template<int S>
  inline bool _getArrayf(HSQUIRRELVM vm, std::array<float, S> &arr)
  {
    SQFloat val;
    if(sq_getsize(vm, -1) != S)
      return false;
    sq_pushnull(vm);
    for(int i = 0; i < S; ++i)
    {
      sq_next(vm, -2);
      if(sq_gettype(vm, -1) != OT_FLOAT)
      {
        sq_pop(vm, 3);
        return false;
      }
      sq_getfloat(vm, -1, &val);
      arr[i] = val;
      sq_pop(vm, 2);
    }
    sq_pop(vm, 1);
    return true;
  }
  
  template<int S>
  inline bool _getArrayi(HSQUIRRELVM vm, std::array<int, S> &arr, int idx)
  {
    SQInteger val;
    if(sq_getsize(vm, idx) != S)
    {
      return false;
    }
    sq_pushnull(vm);
    for(int i = 0; i < S; ++i)
    {
      sq_next(vm, idx);
      if(sq_gettype(vm, -1) != OT_INTEGER)
      {
        sq_pop(vm, 3);
        return false;
      }
      sq_getinteger(vm, -1, &val);
      arr[i] = val;
      sq_pop(vm, 2);
    }
    sq_pop(vm, 1);
    return true;
  }
  template<int S>
  inline bool _getArrayi(HSQUIRRELVM vm, std::array<int, S> &arr)
  {
    SQInteger val;
    if(sq_getsize(vm, -1) != S)
      return false;
    sq_pushnull(vm);
    for(int i = 0; i < S; ++i)
    {
      sq_next(vm, -2);
      if(sq_gettype(vm, -1) != OT_INTEGER)
      {
        sq_pop(vm, 3);
        return false;
      }
      sq_getinteger(vm, -1, &val);
      arr[i] = val;
      sq_pop(vm, 2);
    }
    sq_pop(vm, 1);
    return true;
  }
  
  inline bool _isValidChannel(int i)
  {
    switch(i)
    {
    case 1:
    case 2:
    case 4:
    case 8:
      return true;
    }
    return false;
  }
  
  inline std::vector<std::pair<double, double>>
    _extractPointSet(HSQUIRRELVM vm, int idx)
  {
    std::vector<std::pair<double, double>> ret;
    
    int size = sq_getsize(vm, idx);
    ret.reserve(size / 2);
    
    for(int i = 1; i < size; i += 2)
    {
      SQFloat x, y;
      sq_pushinteger(vm, i - 1);
      sq_rawget(vm, idx);
      if(sq_gettype(vm, -1) == OT_FLOAT)
        sq_getfloat(vm, -1, &x);
      else
      {
        sq_pop(vm, 1);
        continue;
      }
      sq_pop(vm, 1);
      sq_pushinteger(vm, i);
      sq_rawget(vm, idx);
      if(sq_gettype(vm, -1) == OT_FLOAT)
        sq_getfloat(vm, -1, &y);
      else
      {
        sq_pop(vm, 1);
        continue;
      }
      sq_pop(vm, 1);
      
      ret.push_back(std::make_pair((double)x, (double)y));
    }
    
    return ret;
  }
  
  template<class T, int mul>
  int _readBlob(HSQUIRRELVM vm, int idx, const SQChar* slot_name, std::vector<T>* target)
  {
    SQUserPointer blob;
  
    sq_pushstring(vm, slot_name, -1);
    if(SQ_FAILED(sq_get(vm, idx)))
      return 1;
    if(SQ_FAILED(sqstd_getblob(vm, -1, &blob)))
      return 1;
    
    int size = sqstd_getblobsize(vm, -1);
    if(size % (sizeof(T) * mul) != 0)
      return 2;
    
    for(unsigned i = 0; i < size / sizeof(T); ++i)
      target->push_back(((T*)blob)[i]);
    
    sq_pop(vm, 1);
    
    return 0;
  }

  //Squirrel API here
  
  //general
  SQInteger quit(HSQUIRRELVM vm)
  {
    SQInteger* done;
    sq_getuserpointer(vm, -1, (SQUserPointer*)&done);
    *done = 1;
    return 1;
  }
  
  SQInteger loadNut(HSQUIRRELVM vm)
  {
    const SQChar* filename;
    sq_getstring(vm, 2, &filename);
    
    bool succeeded;
    
    try
    {
      succeeded = Sq::executeNut(filename);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("loadNut: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    sq_pushbool(vm, succeeded? SQTrue : SQFalse);
    return 1;
  }
  
  SQInteger reloadNut(HSQUIRRELVM vm)
  {
    const SQChar* filename;
    sq_getstring(vm, 2, &filename);
    
    bool succeeded;
    
    try
    {
      succeeded = Sq::executeNut(filename, true);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("reloadNut: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    sq_pushbool(vm, succeeded? SQTrue : SQFalse);
    return 1;
  }
  
  //viewer control
  SQInteger loadPipeline(HSQUIRRELVM vm)
  {
    const SQChar* filename;
    sq_getstring(vm, 2, &filename);
    
    try
    {
      H3D::setPipeline(filename);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("loadPipeline: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    return 0;
  }
  
  SQInteger setPipeline(HSQUIRRELVM vm)
  {
    static_assert(sizeof(SQChar*) == sizeof(SQUserPointer),
      "SQChar* and SQUserPointer do not have same size, union will be malformed");
    union
    {
      const SQChar* xml;
      SQUserPointer blob;
    };
    int size;
    
    if(sq_gettype(vm, 2) == OT_STRING)
    {
      sq_getstring(vm, 2, &xml);
      size = strlen(xml);
    }
    else if(SQ_SUCCEEDED(sqstd_getblob(vm, 2, &blob)))
    {
      size = sqstd_getblobsize(vm, 2);
    }
    else return sq_throwerror(vm,
      _SC("parameter 1 has invalid type; expected 'string' or 'blob'"));
    
    try
    {
      H3D::setPipeline(xml, size);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("setPipeline: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  SQInteger loadModel(HSQUIRRELVM vm)
  {
    const SQChar* filename;
    sq_getstring(vm, 2, &filename);
    
    try
    {
      H3D::setGeo(filename);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("loadModel: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    return 0;
  }
  
  SQInteger setModel(HSQUIRRELVM vm)
  {
    SQUserPointer blob;
    int size;
    
    if(sq_gettype(vm, 2) == OT_TABLE)
    {
      std::vector<float> verts, normals, tangents, binormals, texcoords1, texcoords2;
      std::vector<unsigned> indices;
      
      int ret = _readBlob<float, 3>(vm, 2, "vertices", &verts);
      if(ret == 1)
        return sq_throwerror(vm, "no slot 'vertices'");
      else if(ret == 2)
        return sq_throwerror(vm, _SC(
          "blob 'vertices' must contain an array of 32-bit float triplets"));
          
      ret = _readBlob<unsigned, 3>(vm, 2, "indices", &indices);
      if(ret == 1)
        return sq_throwerror(vm, "no slot 'indices'");
      else if(ret == 2)
        return sq_throwerror(vm, _SC(
        "blob 'indices' must contain an array of 32-bit integer triplets"));
      
      #define __readStream(_stream_, sizecomp, format, format_s, format_h) \
      ret = _readBlob<format, format_s>(vm, 2, #_stream_, &_stream_); \
      if(ret == 0){ \
        if(sizecomp) \
          return sq_throwerror(vm, _SC("stream '" #_stream_ "' has wrong length")); \
      }else if(ret == 2) return sq_throwerror(vm, _SC( \
        "blob '_stream_' must contain an array of 32-bit float format_h"));
      
      __readStream(normals, (verts.size()!=normals.size()), float, 3, triplets);
      __readStream(tangents, (verts.size()!=tangents.size()), float, 3, triplets);
      __readStream(binormals, (verts.size()!=binormals.size()), float, 3, triplets);
      __readStream(texcoords1, (verts.size()*2!=texcoords1.size()*3), float, 2, pairs);
      __readStream(texcoords2, (verts.size()*2!=texcoords2.size()*3), float, 2, pairs);
      
      #undef __readStream
      
      SQBool tanspace = false;
      sq_pushstring(vm, "tanspace", -1);
      if(SQ_SUCCEEDED(sq_get(vm, 2)))
      {
        if(sq_gettype(vm, -1) == OT_BOOL)
          sq_getbool(vm, -1, &tanspace);
        sq_pop(vm, 1);
      }
      
      try
      {
        H3D::setGeo(
          std::move(verts),
          std::move(normals),
          std::move(tangents),
          std::move(binormals),
          std::move(texcoords1),
          std::move(texcoords2),
          std::move(indices),
          tanspace);
      }
      catch(std::exception& e)
      {
        std::string error_str = std::string("setModel: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
      }
    }
    else if(SQ_SUCCEEDED(sqstd_getblob(vm, 2, &blob)))
    {
      //blob
      size = sqstd_getblobsize(vm, 2);
      
      try
      {
        H3D::setGeo((const char*)blob, size);
      }
      catch(std::exception& e)
      {
        std::string error_str = std::string("setModel: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
      }
    }
    else return sq_throwerror(vm,
      _SC("parameter 1 has invalid type; expected 'table' or 'blob'"));
    
    return 0;
  }
  
  SQInteger loadShader(HSQUIRRELVM vm)
  {
    const SQChar* filename;
    sq_getstring(vm, 2, &filename);
    
    try
    {
      H3D::setShader(filename);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("loadShader: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    return 0;
  }
  
  SQInteger setShader(HSQUIRRELVM vm)
  {
    static_assert(sizeof(SQChar*) == sizeof(SQUserPointer),
      "SQChar* and SQUserPointer do not have same size, union will be malformed");
    union
    {
      const SQChar* xml;
      SQUserPointer blob;
    };
    int size;
    
    if(sq_gettype(vm, 2) == OT_STRING)
    {
      sq_getstring(vm, 2, &xml);
      size = strlen(xml);
    }
    else if(SQ_SUCCEEDED(sqstd_getblob(vm, 2, &blob)))
    {
      size = sqstd_getblobsize(vm, 2);
    }
    else return sq_throwerror(vm,
      _SC("parameter 1 has invalid type; expected 'string' or 'blob'"));
    
    try
    {
      H3D::setShader(xml, size);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("setShader: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  SQInteger setUniform(HSQUIRRELVM vm)
  {
    const SQChar* name;
    sq_getstring(vm, 2, &name);
    
    SQFloat floats[4];
    sq_pushnull(vm);
    for(int i = 0; i < 4; ++i)
    {
      if(SQ_FAILED(sq_next(vm, -2)))
      {
        floats[i] = 0.;
        continue;
      }
      if(sq_gettype(vm, -1) != OT_FLOAT)
      {
        floats[i] = 0.;
      }
      else
      {
        sq_getfloat(vm, -1, &floats[i]);
      }
      sq_pop(vm, 2);
    }
    sq_pop(vm, 1);
    
    H3D::setUniform(name, floats[0], floats[1], floats[2], floats[3]);
    
    return 0;
  }
  
  SQInteger removeUniform(HSQUIRRELVM vm)
  {
    const SQChar* name;
    sq_getstring(vm, 2, &name);
    H3D::removeUniform(name);
    return 0;
  }
  
  SQInteger enableShaderFlag(HSQUIRRELVM vm)
  {
    SQInteger id;
    sq_getinteger(vm, 2, &id);
    if(id < 1 || id > 32)
      return sq_throwerror(vm, 
        _SC("invalid shader flag, id must be within range [1..32]"));
    H3D::enableShaderFlag(id);
    return 0;
  }
  
  SQInteger disableShaderFlag(HSQUIRRELVM vm)
  {
    SQInteger id;
    sq_getinteger(vm, 2, &id);
    if(id < 1 || id > 32)
      return sq_throwerror(vm, 
        _SC("invalid shader flag, id must be within range [1..32]"));
    H3D::disableShaderFlag(id);
    return 0;
  }
  
  SQInteger enableRenderStage(HSQUIRRELVM vm)
  {
    const SQChar* name;
    sq_getstring(vm, 2, &name);
    try
    {
      H3D::enableRenderStage(name);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("enableRenderStage: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    return 0;
  }
  
  SQInteger disableRenderStage(HSQUIRRELVM vm)
  {
    const SQChar* name;
    sq_getstring(vm, 2, &name);
    try
    {
      H3D::disableRenderStage(name);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("disableRenderStage: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    return 0;
  }
  
  //textures
  SQInteger loadTexture(HSQUIRRELVM vm)
  {
    SQInteger handle;
    
    if(sq_gettype(vm, 2) == OT_STRING)
    {
      const SQChar* name;
      sq_getstring(vm, 2, &name);
      
      try
      {
        handle = H3D::loadTexture(name);
      }
      catch(std::exception& e)
      {
        std::string error_str = std::string("loadTexture: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
      }
    }
    else
    {
      SQUserPointer blob;
      if(SQ_SUCCEEDED(sqstd_getblob(vm, 2, &blob)))
      {
        int size = sqstd_getblobsize(vm, 2);
        try
        {
          handle = H3D::loadTexture((const char*)blob, size);
        }
        catch(std::exception& e)
        {
          std::string error_str = std::string("loadTexture: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
        }
      }
      else return sq_throwerror(vm, 
        _SC("parameter 1 has invalid type; expected 'string' or 'blob'"));
    }
    
    sq_pushinteger(vm, handle);
    return 1;
  }
  
  SQInteger createTexture(HSQUIRRELVM vm)
  {
    SQInteger width, height;
    sq_getinteger(vm, 2, &width);
    sq_getinteger(vm, 3, &height);
    
    if(width < 0x10 || width > 0x1000 ||
      height < 0x10 || height > 0x1000 ||
      (width & (width - 1)) != 0 ||
      (height & (height - 1)) != 0)
      return sq_throwerror(vm,
        _SC("texture dimensions must be powers of 2 in interval 16..4096"));
    
    SQInteger tex = TextureManager::addTexture(width, height);
    sq_pushinteger(vm, tex);
    return 1;
  }
  
  SQInteger destroyTexture(HSQUIRRELVM vm)
  {
    SQInteger tex;
    sq_getinteger(vm, 2, &tex);
    
    try
    {
      TextureManager::destroyTexture(tex);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("destroyTexture: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    return 0;
  }
  
  SQInteger listTextures(HSQUIRRELVM vm)
  {
    std::vector<int> textures = TextureManager::listTextures();
    
    sq_newarray(vm, 0);
    for(int tex: textures)
    {
      sq_pushinteger(vm, tex);
      sq_arrayappend(vm, -2);
    }
    
    return 1;
  }
  
  SQInteger destroyTextures(HSQUIRRELVM vm)
  {
    SQBool keep = SQFalse;
    if(sq_gettop(vm) > 3)
      return sq_throwerror(vm, _SC("wrong number of parameters"));
    if(sq_gettop(vm) == 3)
      sq_getbool(vm, 3, &keep);
    
    std::vector<int> textures;
    sq_pushnull(vm);
    while(SQ_SUCCEEDED(sq_next(vm, 2)))
    {
      if(sq_gettype(vm, -1) == OT_INTEGER)
      {
        SQInteger val;
        sq_getinteger(vm, -1, &val);
        textures.push_back(val);
      }
      sq_pop(vm, 2);
    }
    sq_pop(vm, 1);
    
    std::sort(textures.begin(), textures.end());
    
    if(keep == SQTrue)
    {
      std::vector<int> all_textures = TextureManager::listTextures();
      auto it1 = all_textures.begin();
      auto it2 = textures.begin();
      for(;;)
      {
        if(*it1 < *it2)
        {
          ++it1;
          if(it1 == all_textures.end()) break;
        }
        else if(*it1 > *it2)
        {
          ++it2;
          if(it2 == textures.end()) break;
        }
        else
        {
          *it1 = -1;
          ++it1;
          ++it2;
          if(it1 == all_textures.end()) break;
          if(it2 == textures.end()) break;
        }
      }
      all_textures.erase(std::remove(all_textures.begin(), all_textures.end(), -1),
        all_textures.end());
      textures = std::move(all_textures);
    }
    TextureManager::destroyTextures(textures.begin(), textures.end());
    
    return 0;
  }
  
  SQInteger cloneTexture(HSQUIRRELVM vm)
  {
    SQInteger tex;
    sq_getinteger(vm, 2, &tex);
    
    SQInteger new_tex;
    
    try
    {
      new_tex = TextureManager::cloneTexture(tex);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("cloneTexture: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    sq_pushinteger(vm, new_tex);
    return 1;
  }
  
  SQInteger copyTexture(HSQUIRRELVM vm)
  {
    SQInteger dest, src, mask;
    
    if(sq_gettop(vm) > 4)
      return sq_throwerror(vm, _SC("wrong number of parameters"));
    
    sq_getinteger(vm, 2, &dest);
    sq_getinteger(vm, 3, &src);
    if(sq_gettop(vm) == 4)
      sq_getinteger(vm, 4, &mask);
    else mask = 0xf;
    
    try
    {
      TextureManager::copyTexture(dest, src, mask);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("copyTexture: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    return 0;
  }
  
  SQInteger swapTextures(HSQUIRRELVM vm)
  {
    SQInteger tex1, tex2, mask;
    
    if(sq_gettop(vm) > 4)
      return sq_throwerror(vm, _SC("wrong number of parameters"));
    
    sq_getinteger(vm, 2, &tex1);
    sq_getinteger(vm, 3, &tex2);
    if(sq_gettop(vm) == 4)
      sq_getinteger(vm, 4, &mask);
    else mask = 0xf;
    
    try
    {
      TextureManager::swapTextures(tex1, tex2, mask);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("swapTextures: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  SQInteger resizeTexture(HSQUIRRELVM vm)
  {
    SQInteger tex;
    SQInteger width, height;
    
    sq_getinteger(vm, 2, &tex);
    sq_getinteger(vm, 3, &width);
    sq_getinteger(vm, 4, &height);
    
    if(width < 0x10 || width > 0x1000 ||
      height < 0x10 || height > 0x1000 ||
      (width & (width - 1)) != 0 ||
      (height & (height - 1)) != 0)
      return sq_throwerror(vm,
        _SC("texture dimensions must be powers of 2 in interval 16..4096"));
    
    try
    {
      TextureManager::resizeTexture(tex, width, height);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("resizeTexture: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    return 0;
  }
  
  SQInteger getTextureDimensions(HSQUIRRELVM vm)
  {
    SQInteger tex;
    decltype(TextureManager::getTextureDimensions(0)) dims;
    sq_getinteger(vm, 2, &tex);
    
    try
    {
      dims = TextureManager::getTextureDimensions(tex);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("getTextureDimensions: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    sq_newarray(vm, 0);
    sq_pushinteger(vm, dims.second);
    sq_pushinteger(vm, dims.first);
    sq_arrayappend(vm, -3);
    sq_arrayappend(vm, -2);
    
    return 1;
  }
  
  //blobs
  template<class T>
  SQInteger writeTexture(HSQUIRRELVM vm)
  {
    SQInteger tex;
    SQUserPointer blob;
    if(SQ_FAILED(sqstd_getblob(vm, -1, &blob)))
      return sq_throwerror(vm, _SC("unable to retrieve blob"));
    SQInteger size = sqstd_getblobsize(vm, -1);
    
    sq_getinteger(vm, -2, &tex);
    
    decltype(TextureManager::getTextureDimensions(0)) dims;
    
    try
    {
      dims = TextureManager::getTextureDimensions(tex);
      unsigned tex_size = dims.first * dims.second;
      if(tex_size * 4 * sizeof(T) > (unsigned)size)
        return sq_throwerror(vm, _SC("blob it to small"));
      TextureManager::writeRawTexture(tex, (T*)blob);
    }
    catch(std::exception& e)
    {
      std::string error_str;
      if(std::is_floating_point<T>::value)
        error_str = std::string("writeTexturef: ") + e.what();
      else error_str = std::string("writeTextureb: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    return 0;
  }
  
  template<class T>
  SQInteger readTexture(HSQUIRRELVM vm)
  {
    SQInteger tex;
    sq_getinteger(vm, -1, &tex);
    
    decltype(TextureManager::getTextureDimensions(0)) dims;
    SQUserPointer ptr;
    
    try
    {
      dims = TextureManager::getTextureDimensions(tex);
      ptr = sqstd_createblob(vm, dims.first * dims.second * 4 * sizeof(T));
      TextureManager::readRawTexture((T*)ptr, tex);
    }
    catch(std::exception& e)
    {
      std::string error_str;
      if(std::is_floating_point<T>::value)
        error_str = std::string("readTexturef: ") + e.what();
      else error_str = std::string("readTextureb: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    return 1;
  }
  
  template<class T>
  SQInteger writeChannel(HSQUIRRELVM vm)
  {
    SQInteger tex;
    SQInteger ch;
    SQUserPointer blob;
    
    sq_getinteger(vm, 2, &tex);
    if(sq_gettop(vm) > 4)
      return sq_throwerror(vm, _SC("wrong number of parameters"));
    else if(sq_gettop(vm) == 4)
      sq_getinteger(vm, 4, &ch);
    else ch = 0xf;
    
    if(SQ_FAILED(sqstd_getblob(vm, 3, &blob)))
      return sq_throwerror(vm, _SC("unable to retrieve blob."));
    SQInteger size = sqstd_getblobsize(vm, 3);
    
    decltype(TextureManager::getTextureDimensions(0)) dims;
    
    try
    {
      dims = TextureManager::getTextureDimensions(tex);
      unsigned tex_size = dims.first * dims.second;
      if(tex_size * sizeof(T) > (unsigned)size)
        return sq_throwerror(vm, _SC("blob is to small"));
      TextureManager::writeRawChannel(tex, (T*)blob, ch);
    }
    catch(std::exception& e)
    {
      std::string error_str;
      if(std::is_floating_point<T>::value)
        error_str = std::string("writeChannelf: ") + e.what();
      else error_str = std::string("writeChannelb: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    return 0;
  }
  
  template<class T>
  SQInteger readChannel(HSQUIRRELVM vm)
  {
    SQInteger tex;
    SQInteger ch;
    sq_getinteger(vm, -2, &tex);
    sq_getinteger(vm, -1, &ch);
    
    if(!_isValidChannel(ch))
      return sq_throwerror(vm, _SC("invalid channel"));
    
    decltype(TextureManager::getTextureDimensions(0)) dims;
    SQUserPointer ptr;
    
    try
    {
      dims = TextureManager::getTextureDimensions(tex);
      ptr = sqstd_createblob(vm, dims.first * dims.second * sizeof(T));
      TextureManager::readRawChannel((T*)ptr, tex, ch);
    }
    catch(std::exception& e)
    {
      std::string error_str;
      if(std::is_floating_point<T>::value)
        error_str = std::string("readChannelf: ") + e.what();
      else error_str = std::string("readChannelb: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    return 1;
  }
  
  //clearing/blending
  SQInteger fillTexture(HSQUIRRELVM vm)
  {
    SQInteger tex;
    SQInteger mask;
    
    sq_getinteger(vm, 2, &tex);
    if(sq_gettop(vm) > 4)
      return sq_throwerror(vm, _SC("wrong number of parameters"));
    else if(sq_gettop(vm) == 4)
      sq_getinteger(vm, 4, &mask);
    else mask = 0xf;
    
    std::bitset<4> mask_bs(mask);
    std::array<float, 4> color;
    
    if(!_getArrayf<4>(vm, color, 3))
      return sq_throwerror(vm, _SC("malformed argument 2 in clearTexture"));
    
    try
    {
      TextureManager::fillTexture(tex, color, mask_bs);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("fillTexture: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    return 0;
  }
  
  SQInteger fillBlended(HSQUIRRELVM vm)
  {
    SQInteger tex, mask;
    
    std::array<float, 4> color, blend;
    
    if(sq_gettop(vm) > 5)
      return sq_throwerror(vm, _SC("wrong number of parameters"));
    
    sq_getinteger(vm, 2, &tex);
    if(!_getArrayf<4>(vm, color, 3))
      return sq_throwerror(vm, _SC("malformed argument 2 in fillBlended"));
    if(sq_gettype(vm, 4) == OT_FLOAT)
    {
      SQFloat val;
      sq_getfloat(vm, 4, &val);
      blend[0] = blend[1] = blend[2] = blend[3] = val;
    }
    else if(!_getArrayf<4>(vm, blend, 4))
      return sq_throwerror(vm, _SC("malformed argument 3 in fillBlended"));
    
    if(sq_gettop(vm) == 5)
      sq_getinteger(vm, 5, &mask);
    for(int i = 0; i < 4; ++i) if((mask & (1 << i)) == 0) blend[i] = 0.;
    
    try
    {
      TextureManager::fillBlended(tex, color, blend);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("fillBlended: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    return 0;
  }
  
  SQInteger fillBackground(HSQUIRRELVM vm)
  {
    SQInteger tex, mask;
    std::array<float, 4> color;
    
    if(sq_gettop(vm) > 4)
      return sq_throwerror(vm, _SC("wrong number of parameters"));
    
    sq_getinteger(vm, 2, &tex);
    if(!_getArrayf<4>(vm, color, 3))
      return sq_throwerror(vm, _SC("malformed argument 2 in fillBackground"));
    if(sq_gettop(vm) == 4)
      sq_getinteger(vm, 4, &mask);
    else mask = 0xf;
    
    try
    {
      TextureManager::fillBackground(tex, color, mask);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("fillBackground: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  SQInteger fillWithBlendChannel(HSQUIRRELVM vm)
  {
    SQInteger dest, mask, src, ch;
    std::array<float, 4> color;
    
    if(sq_gettop(vm) > 6)
      return sq_throwerror(vm, _SC("wrong number of parameters"));
    
    sq_getinteger(vm, 2, &dest);
    if(!_getArrayf<4>(vm, color, 3))
      return sq_throwerror(vm, _SC("malformed argument 2 in fillBackground"));
    
    if(sq_gettop(vm) == 6)
    {
      sq_getinteger(vm, 4, &mask);
      sq_getinteger(vm, 5, &src);
      sq_getinteger(vm, 6, &ch);
    }
    else
    {
      mask = 0xf;
      sq_getinteger(vm, 4, &src);
      sq_getinteger(vm, 5, &ch);
    }
    
    if(!_isValidChannel(ch))
      return sq_throwerror(vm, _SC("invalid channel"));
    
    try
    {
      TextureManager::fillWithAlphaCh(dest, color, mask, src, ch);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("fillWithBlendChannel: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  //filtering
  
  SQInteger clampTexels(HSQUIRRELVM vm)
  {
    SQInteger tex, mask;
    SQFloat min, max;
    
    if(sq_gettop(vm) > 5)
      return sq_throwerror(vm, _SC("wrong number of parameters"));
    else if(sq_gettop(vm) == 5)
      sq_getinteger(vm, 5, &mask);
    else mask = 0xf;
    
    sq_getinteger(vm, 2, &tex);
    sq_getfloat(vm, 3, &min);
    sq_getfloat(vm, 4, &max);
    
    if(min >= max)
      return 0;
    
    try
    {
      TextureManager::clampTexels(tex, min, max, mask);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("clampTexels: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    return 0;
  }
  
  SQInteger polyFilter(HSQUIRRELVM vm)
  {
    SQInteger tex, mask;
    if(sq_gettop(vm) > 4)
      return sq_throwerror(vm, _SC("wrong number of parameters"));
    else if(sq_gettop(vm) == 4)
      sq_getinteger(vm, 4, &mask);
    else mask = 0xf;
    sq_getinteger(vm, 2, &tex);
    
    std::array<float, 5> filter;
    if(!_getArrayf<5>(vm, filter, 3))
      return sq_throwerror(vm, _SC("malformed argument 2 in polyFilter"));
    
    try
    {
      TextureManager::filterTexture(tex,
        filter[0], filter[1], filter[2], filter[3], filter[4], mask);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("polyFilter: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    return 0;
  }
  
  SQInteger linFilter(HSQUIRRELVM vm)
  {
    SQInteger tex, mask;
    SQFloat from1, to1, from2, to2;
    
    if(sq_gettop(vm) > 7)
      return sq_throwerror(vm, _SC("wrong number of parameters"));
    else if(sq_gettop(vm) == 7)
      sq_getinteger(vm, 7, &mask);
    else mask = 0xf;
    
    sq_getinteger(vm, 2, &tex);
    sq_getfloat(vm, 3, &from1);
    sq_getfloat(vm, 4, &to1);
    sq_getfloat(vm, 5, &from2);
    sq_getfloat(vm, 6, &to2);
    
    if(from1 > to2)
    {
      std::swap(from1, to1);
      std::swap(from2, to2);
    }
    
    try
    {
      TextureManager::filterTextureLin(tex, from1, to1, from2, to2, mask);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("linFilter: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  SQInteger stencilFilter(HSQUIRRELVM vm)
  {
    SQInteger tex, mask;
    SQFloat cutof;
    SQBool rev;
    
    sq_getinteger(vm, 2, &tex);
    sq_getfloat(vm, 3, &cutof);
    
    if(cutof < 0. || cutof > 1.)
      return sq_throwerror(vm, _SC("malformed argument 2 in stencilFilter"));
    
    if(sq_gettop(vm) == 3)
    {
      rev = false;
      mask = 0xf;
    }
    else if(sq_gettop(vm) == 4)
    {
      if(sq_gettype(vm, 4) == OT_INTEGER)
      {
        sq_getinteger(vm, 4, &mask);
        rev = false;
      }
      else
      {
        sq_getbool(vm, 4, &rev);
        mask = 0xf;
      }
    }
    else if(sq_gettop(vm) == 5)
    {
      sq_getbool(vm, 4, &rev);
      sq_getinteger(vm, 5, &mask);
    }
    else return sq_throwerror(vm, _SC("wrong number of parameters"));
    
    try
    {
      TextureManager::filterTextureStencil(tex, cutof, rev, mask);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("stencilFilter: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  SQInteger downsampleFilter(HSQUIRRELVM vm)
  {
    SQInteger tex, levels, mask;
    
    sq_getinteger(vm, 2, &tex);
    sq_getinteger(vm, 3, &levels);
    
    if(levels < 2)
      return sq_throwerror(vm, _SC("malformed argument 2 in stencilFilter"));
    
    if(sq_gettop(vm) == 3)
      mask = 0xf;
    else if(sq_gettop(vm) == 4)
      sq_getinteger(vm, 4, &mask);
    else return sq_throwerror(vm, _SC("wrong number of parameters"));
    
    try
    {
      TextureManager::filterTextureDownsample(tex, levels, mask);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("downsampleFilter: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  //texture operations
  
  SQInteger blendTextures(HSQUIRRELVM vm)
  {
    SQInteger dest, src, mask, ch;
    
    if(sq_gettop(vm) > 5)
      return sq_throwerror(vm, _SC("wrong number of parameters"));
    
    sq_getinteger(vm, 2, &dest);
    sq_getinteger(vm, 3, &src);
    if(sq_gettop(vm) >= 4)
    {
      sq_getinteger(vm, 4, &mask);
      if(sq_gettop(vm) == 5)
        sq_getinteger(vm, 5, &ch);
      else ch = 8;
    }
    else
    {
      mask = 0xf;
      ch = 8;
    }
    
    if(!_isValidChannel(ch))
      return sq_throwerror(vm, _SC("invalid channel"));
    
    //printf("dest: %lli, src: %lli, mask: %lli, ch: %lli\n", dest, src, mask, ch);
    
    try
    {
      TextureManager::blendTextures(dest, src, mask, ch);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("blendTextures: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  SQInteger blendTexturesWithCh(HSQUIRRELVM vm)
  {
    SQInteger dest, src, blend_tex, mask, ch;
    
    if(sq_gettop(vm) > 6)
      return sq_throwerror(vm, _SC("wrong number of parameters"));
    
    sq_getinteger(vm, 2, &dest);
    sq_getinteger(vm, 3, &src);
    sq_getinteger(vm, 4, &blend_tex);
    sq_getinteger(vm, 5, &ch);
    
    if(!_isValidChannel(ch))
      return sq_throwerror(vm, _SC("invalid channel"));
    
    if(sq_gettop(vm) == 6)
      sq_getinteger(vm, 6, &mask);
    else mask = 0xf;
    
    try
    {
      TextureManager::blendTexturesWithCh(dest, src, mask, blend_tex, ch);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("blendTexturesWithCh: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  SQInteger mergeTextures(HSQUIRRELVM vm)
  {
    SQInteger dest, src, mask;
    SQFloat blend;
    
    sq_getinteger(vm, 2, &dest);
    sq_getinteger(vm, 3, &src);
    sq_getfloat(vm, 4, &blend);
    
    if(blend < 0. || blend > 1.)
      return sq_throwerror(vm, _SC("malformed argument 3 in mergeTextures"));
    
    if(sq_gettop(vm) == 4)
      mask = 0xf;
    else if(sq_gettop(vm) == 5)
      sq_getinteger(vm, 5, &mask);
    else return sq_throwerror(vm, _SC("wrong number of parameters"));
    
    try
    {
      TextureManager::mergeTextures(dest, src, blend, mask);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("mergeTextures: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  SQInteger channelDiff(HSQUIRRELVM vm)
  {
    SQInteger dest, src, ch, mask;
    
    sq_getinteger(vm, 2, &dest);
    sq_getinteger(vm, 3, &src);
    sq_getinteger(vm, 4, &ch);
    
    if(sq_gettop(vm) == 4)
      mask = 0xf;
    else if(sq_gettop(vm) == 5)
      sq_getinteger(vm, 5, &mask);
    else return sq_throwerror(vm, _SC("wrong number of parameters"));
    
    try
    {
      TextureManager::channelDiff(dest, src, ch, mask);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("channelDiff: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  SQInteger textureDiff(HSQUIRRELVM vm)
  {
    SQInteger dest, src, mask;
    
    sq_getinteger(vm, 2, &dest);
    sq_getinteger(vm, 3, &src);
    
    if(sq_gettop(vm) == 3)
      mask = 0xf;
    else if(sq_gettop(vm) == 4)
      sq_getinteger(vm, 4, &mask);
    else return sq_throwerror(vm, _SC("wrong number of parameters"));
    
    try
    {
      TextureManager::textureDiff(dest, src, mask);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("textureDiff: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  //shifting operations
  SQInteger warpTexture(HSQUIRRELVM vm)
  {
    SQInteger src, dmap, res;
    SQFloat mult;
    
    sq_getinteger(vm, 2, &src);
    sq_getinteger(vm, 3, &dmap);
    sq_getfloat(vm, 4, &mult);
    
    try
    {
      res = TextureManager::warpTexture(src, dmap, mult);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("warpTexture: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    sq_pushinteger(vm, res);
    return 1;
  }
  
  SQInteger warpInplace(HSQUIRRELVM vm)
  {
    SQInteger targ, dmap;
    SQFloat mult;
    
    sq_getinteger(vm, 2, &targ);
    sq_getinteger(vm, 3, &dmap);
    sq_getfloat(vm, 4, &mult);
    
    try
    {
      TextureManager::warpTextureInplace(targ, dmap, mult);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("warpInplace: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  SQInteger shiftTexels(HSQUIRRELVM vm)
  {
    SQInteger targ, res;
    SQFloat x_shift, y_shift;
    
    sq_getinteger(vm, 2, &targ);
    sq_getfloat(vm, 3, &x_shift);
    sq_getfloat(vm, 4, &y_shift);
    
    try
    {
      res = TextureManager::shiftTexels(targ, x_shift, y_shift);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("shiftTexels: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    sq_pushinteger(vm, res);
    return 1;
  }
  
  SQInteger shiftInplace(HSQUIRRELVM vm)
  {
    SQInteger targ;
    SQFloat x_shift, y_shift;
    
    sq_getinteger(vm, 2, &targ);
    sq_getfloat(vm, 3, &x_shift);
    sq_getfloat(vm, 4, &y_shift);
    
    try
    {
      TextureManager::shiftTexelsInplace(targ, x_shift, y_shift);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("shiftInplace: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  SQInteger applyLens(HSQUIRRELVM vm)
  {
    SQInteger lens, src, res;
    
    sq_getinteger(vm, 2, &lens);
    sq_getinteger(vm, 3, &src);
    
    try
    {
      res = TextureManager::applyLens(lens, src);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("applyLens: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    sq_pushinteger(vm, res);
    return 1;
  }
  
  SQInteger applyLensInplace(HSQUIRRELVM vm)
  {
    SQInteger dest, src;
    
    sq_getinteger(vm, 2, &dest);
    sq_getinteger(vm, 3, &src);
    
    try
    {
      TextureManager::applyLensInplace(dest, src);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("applyLensInplace: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  //blur operations
  SQInteger blurTexture(HSQUIRRELVM vm)
  {
    SQInteger tex, mask;
    SQFloat f;
    std::array<int, 2> dims;
    
    sq_getinteger(vm, 2, &tex);
    
    if(!_getArrayi<2>(vm, dims, 3))
      return sq_throwerror(vm, _SC("malformed argument 2 in clearTexture"));
    if(dims[0] < 0 || dims[1] < 0)
      return sq_throwerror(vm, _SC("malformed argument 2 in clearTexture"));
      
    switch(sq_gettop(vm))
    {
    case 3:
      f = 1.;
      mask = 0xf;
      break;
    case 4:
      if(sq_gettype(vm, 4) == OT_FLOAT)
      {
        sq_getfloat(vm, 4, &f);
        mask = 0xf;
      }
      else
      {
        sq_getinteger(vm, 4, &mask);
        f = 1.;
      }
      break;
    case 5:
      if(sq_gettype(vm, 4) == OT_FLOAT)
      {
        sq_getfloat(vm, 4, &f);
        sq_getinteger(vm, 5, &mask);
      }
      else return sq_throwerror(vm,
        _SC("parameter 4 has an invalid type 'integer' ; expected: 'float'"));
      break;
    }
    
    SQInteger ret;
    try
    {
      ret = TextureManager::blurTexture(tex, dims[0], dims[1], f, mask);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("blurTexture: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    sq_pushinteger(vm, ret);
    return 1;
  }
  
  SQInteger blurInplace(HSQUIRRELVM vm)
  {
    SQInteger tex, mask;
    SQFloat f;
    std::array<int, 2> dims;
    
    sq_getinteger(vm, 2, &tex);
    
    if(!_getArrayi<2>(vm, dims, 3))
      return sq_throwerror(vm, _SC("malformed argument 2 in clearTexture"));
    if(dims[0] < 0 || dims[1] < 0)
      return sq_throwerror(vm, _SC("malformed argument 2 in clearTexture"));
      
    switch(sq_gettop(vm))
    {
    case 3:
      f= 1.;
      mask = 0xf;
      break;
    case 4:
      if(sq_gettype(vm, 4) == OT_FLOAT)
      {
        sq_getfloat(vm, 4, &f);
        mask = 0xf;
      }
      else
      {
        sq_getinteger(vm, 4, &mask);
        f = 1.;
      }
      break;
    case 5:
      if(sq_gettype(vm, 4) == OT_FLOAT)
      {
        sq_getfloat(vm, 4, &f);
        sq_getinteger(vm, 5, &mask);
      }
      else return sq_throwerror(vm,
        _SC("parameter 4 has an invalid type 'integer' ; expected: 'float'"));
      break;
    }
    
    try
    {
      TextureManager::blurInplace(tex, dims[0], dims[1], f, mask);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("blurInplace: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  //channel operations
  SQInteger copyChannel(HSQUIRRELVM vm)
  {
    SQInteger dest, mask, src, ch;
    sq_getinteger(vm, 2, &dest);
    sq_getinteger(vm, 3, &mask);
    sq_getinteger(vm, 4, &src);
    sq_getinteger(vm, 5, &ch);
    
    if(!_isValidChannel(ch))
      return sq_throwerror(vm, _SC("malformed argument 4 in copyChannel"));
    
    try
    {
      TextureManager::copyChannel(dest, mask, src, ch);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("copyChannel: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    return 0;
  }
  
  SQInteger blendChannels(HSQUIRRELVM vm)
  {
    SQInteger dest, mask, src, ch;
    SQFloat blend;
    sq_getinteger(vm, 2, &dest);
    sq_getinteger(vm, 3, &mask);
    sq_getinteger(vm, 4, &src);
    sq_getinteger(vm, 5, &ch);
    sq_getfloat(vm, 6, &blend);
    
    if(!_isValidChannel(ch))
      return sq_throwerror(vm, _SC("malformed argument 4 in blendChannels"));
    
    try
    {
      TextureManager::blendChannels(dest, mask, src, ch, blend);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("blendChannels: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    return 0;
  }
  
  SQInteger swapChannels(HSQUIRRELVM vm)
  {
    SQInteger tex1, ch1, tex2, ch2;
    sq_getinteger(vm, 2, &tex1);
    sq_getinteger(vm, 3, &ch1);
    sq_getinteger(vm, 4, &tex2);
    sq_getinteger(vm, 5, &ch2);
    
    if(!_isValidChannel(ch1))
      return sq_throwerror(vm, _SC("malformed argument 2 in swapChannels"));
    if(!_isValidChannel(ch2))
      return sq_throwerror(vm, _SC("malformed argument 4 in swapChannels"));
    
    try
    {
      TextureManager::swapChannels(tex1, ch1, tex2, ch2);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("swapChannels: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    return 0;
  }
  
  //noise generation
  SQInteger generateNoise(HSQUIRRELVM vm)
  {
    SQInteger tex, dev, mask;
    sq_getinteger(vm, 2, &tex);
    sq_getinteger(vm, 3, &dev);
    if(sq_gettop(vm) == 4)
      sq_getinteger(vm, 4, &mask);
    else if(sq_gettop(vm) > 4)
      return sq_throwerror(vm, _SC("wrong number of parameters"));
    else mask = 0xf;
    
    try
    {
      TextureManager::generateNoise(tex, dev, mask);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("generateNoise: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  SQInteger generateWhiteNoise(HSQUIRRELVM vm)
  {
    SQInteger tex, dev, mask;
    sq_getinteger(vm, 2, &tex);
    sq_getinteger(vm, 3, &dev);
    if(sq_gettop(vm) == 4)
      sq_getinteger(vm, 4, &mask);
    else if(sq_gettop(vm) > 4)
      return sq_throwerror(vm, _SC("wrong number of parameters"));
    else mask = 0xf;
    
    try
    {
      TextureManager::generateWhiteNoise(tex, dev, mask);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("generateWhiteNoise: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  SQInteger makeTurbulence(HSQUIRRELVM vm)
  {
    SQInteger tex, levels, mask;
    SQFloat persistance;
    
    sq_getinteger(vm, 2, &tex);
    sq_getinteger(vm, 3, &levels);
    if(sq_gettop(vm) > 5)
      return sq_throwerror(vm, _SC("wrong number of parameters"));
    else if(sq_gettop(vm) == 5)
    {
      if(sq_gettype(vm, 4) == OT_INTEGER)
        return sq_throwerror(vm, 
          _SC("parameter 4 has an invalid type 'integer' ; expected: 'float'"));
      sq_getfloat(vm, 4, &persistance);
      sq_getinteger(vm, 5, &mask);
    }
    else if(sq_gettop(vm) == 4)
    {
      if(sq_gettype(vm, 4) == OT_INTEGER)
      {
        sq_getinteger(vm, 4, &mask);
        persistance = 2.;
      }
      else
      {
        sq_getfloat(vm, 4, &persistance);
        mask = 0xf;
      }
    }
    else
    {
      persistance = 2.;
      mask = 0xf;
    }
    
    if(levels <= 0)
      return sq_throwerror(vm, _SC("malformed argument 3 in makeTurbulence"));
    if(persistance <= 0.)
      return sq_throwerror(vm, _SC("malformed argument 4 in makeTurbulence"));
    
    SQInteger res;
    
    try
    {
      res = TextureManager::makeTurbulence(tex, levels, persistance, mask);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("makeTurbulence: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    sq_pushinteger(vm, res);
    return 1;
  }
  
  SQInteger makeTurbulenceInplace(HSQUIRRELVM vm)
  {
    SQInteger tex, levels, mask;
    SQFloat persistance;
    
    sq_getinteger(vm, 2, &tex);
    sq_getinteger(vm, 3, &levels);
    if(sq_gettop(vm) > 5)
      return sq_throwerror(vm, _SC("wrong number of parameters"));
    else if(sq_gettop(vm) == 5)
    {
      if(sq_gettype(vm, 4) == OT_INTEGER)
        return sq_throwerror(vm, 
          _SC("parameter 4 has an invalid type 'integer' ; expected: 'float'"));
      sq_getfloat(vm, 4, &persistance);
      sq_getinteger(vm, 5, &mask);
    }
    else if(sq_gettop(vm) == 4)
    {
      if(sq_gettype(vm, 4) == OT_INTEGER)
      {
        sq_getinteger(vm, 4, &mask);
        persistance = 2.;
      }
      else
      {
        sq_getfloat(vm, 4, &persistance);
        mask = 0xf;
      }
    }
    else
    {
      persistance = 2.;
      mask = 0xf;
    }
    
    if(levels <= 0)
      return sq_throwerror(vm, _SC("malformed argument 3 in makeTurbulence"));
    if(persistance <= 0.)
      return sq_throwerror(vm, _SC("malformed argument 4 in makeTurbulence"));
    
    try
    {
      TextureManager::makeTurbulenceInplace(tex, levels, persistance, mask);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("makeTurbulenceInplace: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  SQInteger makeCellNoise(HSQUIRRELVM vm)
  {
    SQInteger tex;
    std::vector<std::pair<double, double>> point_set;
    SQFloat range;
    std::array<int, 4> mask = {0, 0, 0, 0};
    
    sq_getinteger(vm, 2, &tex);
    
    point_set = _extractPointSet(vm, 3);
    if(point_set.size() <= 0)
      return sq_throwerror(vm, _SC("malformed argument 2 in makeCellNoise"));
    
    sq_getfloat(vm, 4, &range);
    if(range == 0.)
      return sq_throwerror(vm, _SC("malformed argument 3 in makeCellNoise"));
  
    if(sq_gettype(vm, 5) == OT_ARRAY)
    {
      if(sq_gettop(vm) != 5)
        return sq_throwerror(vm, _SC("wrong number of parameters"));
      if(!_getArrayi<4>(vm, mask, 5))
        return sq_throwerror(vm, _SC("malformed argument 4 in makeCellNoise"));
      
      for(auto i: mask) if(i < 0 || i > 4)
        return sq_throwerror(vm, _SC("malformed argument 4 in makeCellNoise"));
    }
    else
    {
      if(sq_gettop(vm) != 6)
        return sq_throwerror(vm, _SC("wrong number of parameters"));
      SQInteger m, o;
      sq_getinteger(vm, 5, &m);
      sq_getinteger(vm, 6, &o);
      
      if(o < 0 || o > 4)
        return sq_throwerror(vm, _SC("malformed argument 5 in makeCellNoise"));
      
      if(m & 1) mask[0] = o;
      if(m & 2) mask[1] = o;
      if(m & 4) mask[2] = o;
      if(m & 8) mask[3] = o;
    }
    
    try
    {
      TextureManager::makeCellNoise(tex, point_set, range, mask);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("makeCellNoise: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  SQInteger makeDelaunay(HSQUIRRELVM vm)
  {
    SQInteger tex;
    std::vector<std::pair<double, double>> point_set;
    SQFloat range;
    SQInteger mask;
    
    sq_getinteger(vm, 2, &tex);
    
    point_set = _extractPointSet(vm, 3);
    if(point_set.size() <= 0)
      return sq_throwerror(vm, _SC("malformed argument 2 in makeCellNoise"));
    
    sq_getfloat(vm, 4, &range);
    if(range == 0.)
      return sq_throwerror(vm, _SC("malformed argument 3 in makeCellNoise"));
    
    if(sq_gettop(vm) == 5)
      sq_getinteger(vm, 5, &mask);
    else mask = 0xf;
  
    try
    {
      TextureManager::makeDelaunay(tex, point_set, range, mask);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("makeDelaunay: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  SQInteger makeVoronoi(HSQUIRRELVM vm)
  {
    SQInteger tex;
    std::vector<std::pair<double, double>> point_set;
    SQFloat range;
    SQInteger mask;
    
    sq_getinteger(vm, 2, &tex);
    
    point_set = _extractPointSet(vm, 3);
    if(point_set.size() <= 0)
      return sq_throwerror(vm, _SC("malformed argument 2 in makeCellNoise"));
    
    sq_getfloat(vm, 4, &range);
    if(range == 0.)
      return sq_throwerror(vm, _SC("malformed argument 3 in makeCellNoise"));
    
    if(sq_gettop(vm) == 5)
      sq_getinteger(vm, 5, &mask);
    else mask = 0xf;
  
    try
    {
      TextureManager::makeVoronoi(tex, point_set, range, mask);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("makeVoronoi: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  SQInteger makeNormalMap(HSQUIRRELVM vm)
  {
    SQInteger tex;
    SQFloat mul;
    
    sq_getinteger(vm, 2, &tex);
    sq_getfloat(vm, 3, &mul);
    
    try
    {
      TextureManager::makeNormalMap(tex, mul);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("makeNormalMap: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  //sampler binding
  SQInteger bindSampler(HSQUIRRELVM vm)
  {
    const SQChar* name;
    SQInteger tex;
    sq_getstring(vm, 2, &name);
    sq_getinteger(vm, 3, &tex);
    try
    {
      H3D::bindSampler(tex, name);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("bindSampler: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    return 0;
  }
  
  SQInteger unbindSampler(HSQUIRRELVM vm)
  {
    const SQChar* name;
    sq_getstring(vm, 2, &name);
    H3D::unbindSampler(name);
    return 0;
  }
  
  //random devices
  SQInteger seedRandomDevice(HSQUIRRELVM vm)
  {
    SQInteger device, seed_i;
    const SQChar* seed_s;
    
    sq_getinteger(vm, 2, &device);
    if(sq_gettype(vm, 3) == OT_INTEGER)
    {
      sq_getinteger(vm, 3, &seed_i);
      Rand::seedDevice(device, seed_i);
    }
    else
    {
      sq_getstring(vm, 3, &seed_s);
      Rand::seedDevice(device, seed_s);
    }
    return 0;
  }
  
  //point sets
  SQInteger makePointSet(HSQUIRRELVM vm)
  {
    SQInteger size, device;
    SQInteger its;
    SQFloat mag;
    
    sq_getinteger(vm, 2, &device);
    sq_getinteger(vm, 3, &size);
    
    bool spread = false;
    
    if(sq_gettop(vm) == 5)
    {
      sq_getinteger(vm, 4, &its);
      sq_getfloat(vm, 5, &mag);
      
      if(its < 1)
        return sq_throwerror(vm, _SC("malformed argument 3 in makePointSet"));
      if(mag < 0. || mag > 1.)
        return sq_throwerror(vm, _SC("malformed argument 4 in makePointSet"));
      
      spread = true;
    }
    else if(sq_gettop(vm) != 3)
      return sq_throwerror(vm, _SC("wrong number of parameters"));
    
    if(size < 1)
      return sq_throwerror(vm, _SC("malformed argument 1 in makePointSet"));
    
    std::vector<std::pair<double, double>> set;
    
    try
    {
      set = PointSet::make(size, device);
      if(spread) PointSet::spread(set, its, mag);
    }
    catch(std::exception &e)
    {
      std::string error_str = std::string("makePointSet: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    //put into array
    sq_newarray(vm, 0);
    int idx = sq_gettop(vm);
    for(unsigned i = 0; i < set.size(); ++i)
    {
      sq_pushfloat(vm, set[i].first);
      sq_pushfloat(vm, set[i].second);
    }
    for(unsigned i = 0; i < set.size(); ++i)
    {
      sq_arrayappend(vm, idx);
      sq_arrayappend(vm, idx);
    }
    
    return 1;
  }
  
  SQInteger spreadPointSet(HSQUIRRELVM vm)
  {
    SQInteger its;
    SQFloat mag;
    
    std::vector<std::pair<double, double>> point_set;
    point_set = _extractPointSet(vm, 2);
    if(point_set.size() <= 0)
      return sq_throwerror(vm, _SC("malformed argument 2 in spreadPointSet"));
    
    sq_getinteger(vm, 3, &its);
    sq_getfloat(vm, 4, &mag);
    
    if(its < 1)
      return sq_throwerror(vm, _SC("malformed argument 2 in spreadPointSet"));
    if(mag < 0. || mag > 1.)
      return sq_throwerror(vm, _SC("malformed argument 2 in spreadPointSet"));
    
    try
    {
      PointSet::spread(point_set, its, mag);
    }
    catch(std::exception &e)
    {
      std::string error_str = std::string("spreadPointSet: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    //put into array
    sq_newarray(vm, 0);
    int idx = sq_gettop(vm);
    for(unsigned i = 0; i < point_set.size(); ++i)
    {
      sq_pushfloat(vm, point_set[i].first);
      sq_pushfloat(vm, point_set[i].second);
    }
    for(unsigned i = 0; i < point_set.size(); ++i)
    {
      sq_arrayappend(vm, idx);
      sq_arrayappend(vm, idx);
    }
    
    return 1;
  }
  
  //saving images
  SQInteger saveBMP(HSQUIRRELVM vm)
  {
    SQInteger tex;
    const SQChar* filename;
    
    sq_getinteger(vm, 2, &tex);
    sq_getstring(vm, 3, &filename);
    
    if(strlen(filename) == 0)
      return sq_throwerror(vm, _SC("malformed argument 2 in saveBMP"));
    
    try
    {
      TextureManager::writeToDisk(tex, filename);
    }
    catch(std::exception& e)
    {
      std::string error_str = std::string("saveBMP: ") + e.what();
      return sq_throwerror(vm, error_str.c_str());
    }
    
    return 0;
  }
  
  //reloading vm
  SQInteger reloadVM(HSQUIRRELVM vm)
  {
    Sq::reload = true;
    Terminal::stop();
    return sq_suspendvm(vm);
  }
}

namespace SqAPI
{
  void init()
  {
    using Sq::vm;
  
    sq_pushroottable(vm);
    
    #define NEW_CLOSURE(name, args, types) \
      sq_pushstring(vm, _SC(#name), -1); \
      sq_newclosure(vm, name, 0); \
      sq_setparamscheck(vm, args, types); \
      sq_newslot(vm, -3, SQFalse);
    #define NEW_CLOSURE_N(name, func, args, types) \
      sq_pushstring(vm, _SC(#name), -1); \
      sq_newclosure(vm, func, 0); \
      sq_setparamscheck(vm, args, types); \
      sq_newslot(vm, -3, SQFalse);
    
    //insert functions here
    sq_pushstring(vm, _SC("quit"), -1);
    sq_pushuserpointer(vm, &Common::done);
    sq_newclosure(vm, quit, 1);
    sq_setparamscheck(vm, 1, nullptr);
    sq_newslot(vm, -3, SQFalse);
    
    NEW_CLOSURE(loadNut, 2, "ts")
    NEW_CLOSURE(reloadNut, 2, "ts")
    NEW_CLOSURE(loadPipeline, 2, "ts")
    NEW_CLOSURE(setPipeline, 2, "t.")
    NEW_CLOSURE(loadModel, 2, "ts")
    NEW_CLOSURE(setModel, 2, "t.")
    NEW_CLOSURE(loadShader, 2, "ts")
    NEW_CLOSURE(setShader, 2, "t.")
    NEW_CLOSURE(setUniform, 3, "tsa")
    NEW_CLOSURE(removeUniform, 2, "ts")
    NEW_CLOSURE(enableShaderFlag, 2, "ti")
    NEW_CLOSURE(disableShaderFlag, 2, "ti")
    NEW_CLOSURE(enableRenderStage, 2, "ts")
    NEW_CLOSURE(disableRenderStage, 2, "ts")
    NEW_CLOSURE(loadTexture, 2, "t.")
    NEW_CLOSURE(createTexture, 3, "tii")
    NEW_CLOSURE(destroyTexture, 2, "ti")
    NEW_CLOSURE(cloneTexture, 2, "ti")
    NEW_CLOSURE(copyTexture, -3, "tiii")
    NEW_CLOSURE(swapTextures, -3, "tiii")
    NEW_CLOSURE(resizeTexture, 4, "tiii")
    NEW_CLOSURE(getTextureDimensions, 2, "ti")
    NEW_CLOSURE(bindSampler, 3, "tsi")
    NEW_CLOSURE(unbindSampler, 2, "ts")
    NEW_CLOSURE(listTextures, 1, "t")
    NEW_CLOSURE(destroyTextures, -2, "tab")
    NEW_CLOSURE_N(writeTextureb, writeTexture<uint8_t>, 3, "ti.")
    NEW_CLOSURE_N(writeTexturef, writeTexture<float>, 3, "ti.")
    NEW_CLOSURE_N(readTextureb, readTexture<uint8_t>, 2, "ti")
    NEW_CLOSURE_N(readTexturef, readTexture<float>, 2, "ti")
    NEW_CLOSURE_N(writeChannelb, writeChannel<uint8_t>, -3, "ti.i")
    NEW_CLOSURE_N(writeChannelf, writeChannel<float>, -3, "ti.i")
    NEW_CLOSURE_N(readChannelb, readChannel<uint8_t>, 3, "tii")
    NEW_CLOSURE_N(readChannelf, readChannel<float>, 3, "tii")
    NEW_CLOSURE(fillTexture, -3, "tiai")
    NEW_CLOSURE(fillBlended, -4, "tiaa|fi")
    NEW_CLOSURE(clampTexels, -4, "tiffi")
    NEW_CLOSURE(polyFilter, -3, "tiai")
    NEW_CLOSURE(linFilter, -6, "tiffffi")
    NEW_CLOSURE(stencilFilter, -3, "tifb|ii");
    NEW_CLOSURE(downsampleFilter, -3, "tiii");
    NEW_CLOSURE(copyChannel, 5, "tiiii")
    NEW_CLOSURE(swapChannels, 5, "tiiii")
    NEW_CLOSURE(blendChannels, 6, "tiiiif")
    NEW_CLOSURE(blendTextures, -3, "tiiii")
    NEW_CLOSURE(blendTexturesWithCh, -5, "tiiiii")
    NEW_CLOSURE(mergeTextures, -4, "tiifi");
    NEW_CLOSURE(channelDiff, -3, "tiiii")
    NEW_CLOSURE(textureDiff, -3, "tiii")
    NEW_CLOSURE(fillBackground, -3, "tiai")
    NEW_CLOSURE(fillWithBlendChannel, -5, "tiaiii")
    NEW_CLOSURE(warpTexture, 4, "tiif")
    NEW_CLOSURE(warpInplace, 4, "tiif")
    NEW_CLOSURE(shiftTexels, 4, "tiff")
    NEW_CLOSURE(shiftInplace, 4, "tiff")
    NEW_CLOSURE(applyLens, 3, "tii")
    NEW_CLOSURE(applyLensInplace, 3, "tii")
    NEW_CLOSURE(blurTexture, -3, "tiaf|ii");
    NEW_CLOSURE(blurInplace, -3, "tiaf|ii");
    NEW_CLOSURE(seedRandomDevice, 3, "tis|i");
    NEW_CLOSURE(generateNoise, -3, "tiii");
    NEW_CLOSURE(generateWhiteNoise, -3, "tiii");
    NEW_CLOSURE(makeTurbulence, -3, "tiif|ii");
    NEW_CLOSURE(makeTurbulenceInplace, -3, "tiif|ii");
    NEW_CLOSURE(makeCellNoise, -5, "tiafa|ii");
    NEW_CLOSURE(makeDelaunay, -4, "tiafi");
    NEW_CLOSURE(makeVoronoi, -4, "tiafi");
    NEW_CLOSURE(makeNormalMap, 3, "tif");
    NEW_CLOSURE(makePointSet, -3, "tiiif");
    NEW_CLOSURE(spreadPointSet, 4, "taif");
    NEW_CLOSURE(saveBMP, 3, "tis");
    NEW_CLOSURE(reloadVM, 1, "t");
    
    #undef NEW_CLOSURE
    #undef NEW_CLOSURE_N
    
    sq_pop(Sq::vm, 1);
  }
}
