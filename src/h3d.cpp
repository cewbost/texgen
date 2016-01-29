#include <limits>

#include <horde3d.h>

#include "common.h"

#include "material.h"
#include "viewer.h"
#include "texture_manager.h"
#include "terminal.h"
#include "bin_serializer.h"

#include "h3d.h"

namespace
{
  H3DRes _pipeline  = 0;
  H3DRes _shader    = 0;
  H3DRes _geometry  = 0;
  
  std::unique_ptr<MaterialBuilder> _mat_builder;
  
  int _res_loaded = 0;
  
  H3DRes _loadResource(
    const char* res_name,
    int type,
    const char* filename)
  {
    auto buffer = Common::loadFile(Common::path(filename).c_str());
    H3DRes res = h3dAddResource(type, res_name, 0);
    if(!h3dLoadResource(res, buffer.first.get(), buffer.second))
    {
      h3dRemoveResource(res);
      throw tgException("Unable to load resource from file %s", filename);
    }
    return res;
  }
  
  H3DRes _loadResource(
    int type,
    const char* filename)
  {
    auto buffer = Common::loadFile(Common::path(filename).c_str());
    char res_name[64];
    snprintf(res_name, 64, "_res%i", _res_loaded++);
    H3DRes res = h3dAddResource(type, res_name, 0);
    if(!h3dLoadResource(res, buffer.first.get(), buffer.second))
    {
      h3dRemoveResource(res);
      throw tgException("Unable to load resource from file %s", filename);
    }
    return res;
  }
  
  void _loadXmlResource(
    H3DRes res,
    const char* filename)
  {
    auto buffer = Common::loadFile(Common::path(filename).c_str());
    if(!h3dLoadResource(res, buffer.first.get(), buffer.second))
      throw tgException("Unable to load resource from file %s", filename);
  }
  
  H3DRes _makeGeometry(
    int num_verts,
    const float* verts,
    const int16_t* normals,
    const int16_t* tangents,
    const int16_t* binormals,
    const float* texcoords1,
    const float* texcoords2,
    int num_triangles,
    const uint32_t* triangle_indices)
  {
    BinSerializer buffer;
    
    //header
    buffer.write("H3DG");
    buffer.write<uint32_t>(5);
    buffer.write<uint32_t>(0);
    
    //vert streams header
    buffer.write<uint32_t>(1
    + (normals? 1 : 0)
    + (tangents? 1 : 0)
    + (binormals? 1 : 0)
    + (texcoords1? 1 : 0)
    + (texcoords2? 1 : 0));
    buffer.write<uint32_t>(num_verts);
    
    //vertices
    buffer.write<uint32_t>(0);
    buffer.write<uint32_t>(12);
    buffer.write(verts, num_verts * 3);
    
    if(normals)
    {
      buffer.write<uint32_t>(1);
      buffer.write<uint32_t>(6);
      buffer.write(normals, num_verts * 3);
    }
    if(tangents)
    {
      buffer.write<uint32_t>(2);
      buffer.write<uint32_t>(6);
      buffer.write(tangents, num_verts * 3);
    }
    if(binormals)
    {
      buffer.write<uint32_t>(3);
      buffer.write<uint32_t>(6);
      buffer.write(binormals, num_verts * 3);
    }
    if(texcoords1)
    {
      buffer.write<uint32_t>(6);
      buffer.write<uint32_t>(8);
      buffer.write(texcoords1, num_verts * 2);
    }
    if(texcoords2)
    {
      buffer.write<uint32_t>(7);
      buffer.write<uint32_t>(8);
      buffer.write(texcoords2, num_verts * 2);
    }
    
    //triangle indices
    buffer.write<uint32_t>(num_triangles * 3);
    buffer.write(triangle_indices, num_triangles * 3);
    
    //morph targets
    buffer.write<uint32_t>(0);
  
    char res_name[64];
    snprintf(res_name, 64, "_res%i", _res_loaded++);
    H3DRes res = h3dAddResource(H3DResTypes::Geometry, res_name, 0);
    if(!h3dLoadResource(res, buffer.get(), buffer.tell()))
    {
      h3dRemoveResource(res);
      throw tgException("Unable to load resource %s", res_name);
    }
    return res;
  }
}

namespace H3D
{
  void setPipeline(const char* filename)
  {
    char buffer[64] = "";
    snprintf(buffer, 64, "_res%i", _res_loaded++);
    H3DRes res = h3dAddResource(H3DResTypes::Pipeline, buffer, 0);
    try
    {
      _loadXmlResource(res, filename);
    }
    catch(...)
    {
      dumpMessages();
      throw;
    }
    if(_pipeline) h3dUnloadResource(_pipeline);
    _pipeline = res;
    
    Viewer::newCamera(res);
    dumpMessages();
  }
  
  void setPipeline(const char* buffer, size_t size)
  {
    char name[64] = "";
    snprintf(name, 64, "_res%i", _res_loaded++);
    H3DRes res = h3dAddResource(H3DResTypes::Pipeline, name, 0);
    if(!h3dLoadResource(res, buffer, size))
    {
      dumpMessages();
      throw tgException("unable to load pipeline resource");
    }
    else
    {
      if(_pipeline) h3dUnloadResource(_pipeline);
      _pipeline = res;
    
      Viewer::newCamera(res);
      dumpMessages();
    }
  }
  
  void setGeo(const char* filename)
  {
    H3DRes res;
    try
    {
      res = _loadResource(H3DResTypes::Geometry, filename);
    }
    catch(...)
    {
      dumpMessages();
      throw;
    }
    if(_geometry) h3dUnloadResource(_geometry);
    _geometry = res;
    
    if(_geometry != 0 && _shader != 0)
      Viewer::newModel(_geometry, _mat_builder->getRes());
    dumpMessages();
  }
  
  void setGeo(const char* buffer, size_t size)
  {
    char name[64] = "";
    snprintf(name, 64, "_res%i", _res_loaded++);
    H3DRes res = h3dAddResource(H3DResTypes::Geometry, name, 0);
    if(!h3dLoadResource(res, buffer, size))
    {
      dumpMessages();
      throw tgException("unable to load shader resource");
    }
    else
    {
      if(_geometry) h3dUnloadResource(_geometry);
      _geometry = res;
      
      if(_geometry != 0 && _shader != 0)
        Viewer::newModel(_geometry, _mat_builder->getRes());
      dumpMessages();
    }
  }
  
  void setGeo(
    std::vector<float> verts,
    std::vector<float> normals,
    std::vector<float> tangents,
    std::vector<float> binormals,
    std::vector<float> texcoords1,
    std::vector<float> texcoords2,
    std::vector<unsigned> indices,
    bool get_tanspace)
  {
    if(indices.size() <= 0)
      throw tgException("no triangle indices supplied");
    else if(indices.size() % 3 != 0)
      throw tgException("triangle indices must come in packs of three");
  
    unsigned num_verts = verts.size();
    if(num_verts <= 0)
      throw tgException("no vertices supplied");
    else if(num_verts % 3 != 0)
      throw tgException("vertices must have three components each");
    num_verts /= 3;
    
    std::unique_ptr<int16_t[]> norms, tangs, binorms;
    float *tex1, *tex2;
    
    if(normals.empty())
    {
      norms = nullptr;
      tangs = nullptr;
      binorms = nullptr;
    }
    else if(normals.size() != num_verts * 3)
      throw tgException("number of normals does not match number of vertices");
    else
    {
      norms.reset(new int16_t[num_verts * 3]);
      for(unsigned i = 0; i < num_verts * 3; ++i)
      {
        norms[i] = (int16_t)(normals[i] * std::numeric_limits<int16_t>::max());
      }
      
      if(tangents.empty())
        tangs = nullptr;
      else if(tangents.size() != num_verts * 3)
        throw tgException("number of tangents does not match number of vertices");
      else
      {
        tangs.reset(new int16_t[num_verts * 3]);
        for(unsigned i = 0; i < num_verts * 3; ++i)
        {
          tangs[i] = (int16_t)(tangents[i] * std::numeric_limits<int16_t>::max());
        }
      }
      
      if(binormals.empty())
        binorms = nullptr;
      else if(binormals.size() != num_verts * 3)
        throw tgException("number of binormals does not match number of vertices");
      else
      {
        binorms.reset(new int16_t[num_verts * 3]);
        for(unsigned i = 0; i < num_verts * 3; ++i)
        {
          binorms[i] = (int16_t)(binormals[i] * std::numeric_limits<int16_t>::max());
        }
      }
      
      if((bool)binorms != (bool)tangs && get_tanspace)
      {
        if((bool)tangs)
        {
          binorms.reset(new int16_t[num_verts * 3]);
          for(unsigned i = 0; i < num_verts * 3; i += 3)
          {
            Eigen::Vector3f norm(normals[i], normals[i + 1], normals[i + 2]);
            Eigen::Vector3f tang(tangents[i], tangents[i + 1], tangents[i + 2]);
            Eigen::Vector3f bin = norm.cross(tang);
            binorms[i] = (int16_t)(bin[0] * std::numeric_limits<int16_t>::max());
            binorms[i + 1] = (int16_t)(bin[1] * std::numeric_limits<int16_t>::max());
            binorms[i + 2] = (int16_t)(bin[2] * std::numeric_limits<int16_t>::max());
          }
        }
        else
        {
          tangs.reset(new int16_t[num_verts * 3]);
          for(unsigned i = 0; i < num_verts * 3; i += 3)
          {
            Eigen::Vector3f norm(normals[i], normals[i + 1], normals[i + 2]);
            Eigen::Vector3f bin(binormals[i], binormals[i + 1], binormals[i + 2]);
            Eigen::Vector3f tang = bin.cross(norm);
            tangs[i] = (int16_t)(tang[0] * std::numeric_limits<int16_t>::max());
            tangs[i + 1] = (int16_t)(tang[1] * std::numeric_limits<int16_t>::max());
            tangs[i + 2] = (int16_t)(tang[2] * std::numeric_limits<int16_t>::max());
          }
        }
      }
    }
    
    if(texcoords1.empty())
      tex1 = nullptr;
    else if(texcoords1.size() != num_verts * 2)
      throw tgException(
        "number if texcoords stream 1 does not match number of vertices");
    else tex1 = texcoords1.data();
    
    if(texcoords2.empty())
      tex2 = nullptr;
    else if(texcoords2.size() != num_verts * 2)
      throw tgException(
        "number if texcoords stream 2 does not match number of vertices");
    else tex2 = texcoords2.data();
    
    H3DRes res;
    try
    {
      res = _makeGeometry(num_verts, verts.data(), norms.get(), tangs.get(),
        binorms.get(), tex1, tex2, indices.size() / 3, indices.data());
    }
    catch(...)
    {
      dumpMessages();
      throw;
    }
    
    if(_geometry) h3dUnloadResource(_geometry);
    _geometry = res;
    
    if(_geometry != 0 && _shader != 0)
      Viewer::newModel(_geometry, _mat_builder->getRes());
    dumpMessages();
  }
  
  void setShader(const char* filename)
  {
    H3DRes res;
    char res_name[64];
    snprintf(res_name, 64, "_res%i", _res_loaded++);
    try
    {
      res = _loadResource(res_name, H3DResTypes::Shader, filename);
    }
    catch(...)
    {
      dumpMessages();
      throw;
    }
    if(_shader) h3dUnloadResource(_shader);
    _shader = res;
    
    _mat_builder->setShader(res_name);
    _mat_builder->build();
    dumpMessages();
  }
  
  void setShader(const char* buffer, size_t size)
  {
    char name[64] = "";
    snprintf(name, 64, "_res%i", _res_loaded++);
    H3DRes res = h3dAddResource(H3DResTypes::Shader, name, 0);
    if(!h3dLoadResource(res, buffer, size))
    {
      dumpMessages();
      throw tgException("unable to load shader resource");
    }
    else
    {
      if(_shader) h3dUnloadResource(_shader);
      _shader = res;
      
      _mat_builder->setShader(name);
      _mat_builder->build();
      dumpMessages();
    }
  }
  
  
  void setUniform(const char* name, float a, float b, float c, float d)
  {
    _mat_builder->setUniform(name, a, b, c, d);
    _mat_builder->build();
    dumpMessages();
  }
  
  void removeUniform(const char* name)
  {
    _mat_builder->removeUniform(name);
    _mat_builder->build();
    dumpMessages();
  }
  
  void enableShaderFlag(int flag)
  {
    _mat_builder->setShaderFlag(flag);
    _mat_builder->build();
    dumpMessages();
  }
  
  void disableShaderFlag(int flag)
  {
    _mat_builder->unsetShaderFlag(flag);
    _mat_builder->build();
    dumpMessages();
  }
  
  void enableRenderStage(const char* id)
  {
    if(_pipeline == 0)
      throw tgException("no pipeline active");
    
    int elem = h3dFindResElem(
      _pipeline,
      H3DPipeRes::StageElem,
      H3DPipeRes::StageNameStr,
      id);
    if(elem == -1)
      throw tgException("stage %s not found", id);
    h3dSetResParamI(
      _pipeline,
      H3DPipeRes::StageElem,
      elem,
      H3DPipeRes::StageActivationI,
      1);
  }
  
  void disableRenderStage(const char* id)
  {
    if(_pipeline == 0)
      throw tgException("no pipeline active");
    
    int elem = h3dFindResElem(
      _pipeline,
      H3DPipeRes::StageElem,
      H3DPipeRes::StageNameStr,
      id);
    if(elem == -1)
      throw tgException("stage %s not found", id);
    h3dSetResParamI(
      _pipeline,
      H3DPipeRes::StageElem,
      elem,
      H3DPipeRes::StageActivationI,
      0);
  }
  
  int loadTexture(const char* filename)
  {
    H3DRes res;
    try
    {
      res = _loadResource(H3DResTypes::Texture, filename);
    }
    catch(...)
    {
      dumpMessages();
      throw;
    }
    int handle = TextureManager::addTexture(res);
    dumpMessages();
    return handle;
  }
  
  int loadTexture(const char* buffer, size_t size)
  {
    char name[64] = "";
    snprintf(name, 64, "_res%i", _res_loaded++);
    H3DRes res = h3dAddResource(H3DResTypes::Texture, name, 0);
    if(!h3dLoadResource(res, buffer, size))
    {
      dumpMessages();
      throw tgException("unable to load texture resource");
    }
    
    int handle = TextureManager::addTexture(res);
    dumpMessages();
    return handle;
  }
  
  void bindSampler(int tex, const char* sampler_name)
  {
    const char* res_name = _mat_builder->getSamplerRes(sampler_name);
    if(res_name != nullptr)
    {
      H3DRes res = h3dFindResource(H3DResTypes::Texture, res_name);
      if(res != 0)
      {
        TextureManager::removeResource(res);
        if(h3dIsResLoaded(res))
          h3dUnloadResource(res);
        h3dRemoveResource(res);
      }
    }
    H3DRes res = TextureManager::getTexRes(tex);
    res_name = h3dGetResName(res);
    _mat_builder->setSampler(sampler_name, res_name);
    _mat_builder->build();
    dumpMessages();
  }
  
  void unbindSampler(const char* sampler_name)
  {
    const char* res_name = _mat_builder->getSamplerRes(sampler_name);
    _mat_builder->removeSampler(sampler_name);
    _mat_builder->build();
    if(res_name != nullptr)
    {
      H3DRes res = h3dFindResource(H3DResTypes::Texture, res_name);
      if(res != 0)
      {
        TextureManager::removeResource(res);
        if(h3dIsResLoaded(res))
          h3dUnloadResource(res);
        h3dRemoveResource(res);
      }
    }
    dumpMessages();
  }
  
  void replaceSamplerRes(const char* old_name, const char* new_name)
  {
    _mat_builder->replaceSamplerRes(old_name, new_name);
    _mat_builder->build();
    dumpMessages();
  }
  
  H3DRes createTexture(int w, int h)
  {
    char res_name[64];
    snprintf(res_name, 64, "_res%i", _res_loaded++);
    return h3dCreateTexture(res_name, w, h, H3DFormats::TEX_BGRA8, 0);
  }
  
  void dumpMessagesToStdout()
  {
    std::string message;

    for(message = h3dGetMessage(nullptr, nullptr); message.length() > 0;
      message = h3dGetMessage(nullptr, nullptr))
      std::printf("%s\n", message.c_str());

    std::fflush(stdout);
  }
  
  void dumpMessages()
  {
    const char* message;

    for(message = h3dGetMessage(nullptr, nullptr); strlen(message) > 0;
      message = h3dGetMessage(nullptr, nullptr))
      Terminal::println(message);
  }
  
  void init()
  {
    _mat_builder.reset(new MaterialBuilder("material"));
    TextureManager::init();
  }
  void deinit() noexcept
  {
    _mat_builder = nullptr;
    TextureManager::deinit();
  }
}