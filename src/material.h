#ifndef MATERIAL_H_INCLUDED
#define MATERIAL_H_INCLUDED

#include <string>
#include <array>
#include <bitset>
#include <vector>
#include <algorithm>

#include <horde3d.h>

class MaterialBuilder
{
  std::string _shader;
  std::bitset<32> _shader_flags;
  std::vector<std::pair<std::string, std::string>> _samplers;
  std::vector<std::pair<std::string, std::array<float, 4>>> _uniforms;
  
  H3DRes res;
  
public:
  void setShader(const char* name)
  {
    _shader = name;
  }
  void setShaderFlag(int flag)
  {
    _shader_flags.set(flag);
  }
  void unsetShaderFlag(int flag)
  {
    _shader_flags.reset(flag);
  }
  void setSampler(const char* name, const char* source)
  {
    auto it = std::find_if(_samplers.begin(), _samplers.end(),
    [name](decltype(_samplers)::value_type& s)
    {
      return s.first == name;
    });
    if(it == _samplers.end())
      _samplers.push_back(std::make_pair(std::string(name), std::string(source)));
    else
    {
      it->second = source;
    }
  }
  const char* getSamplerRes(const char* name)
  {
    auto it = std::find_if(_samplers.begin(), _samplers.end(),
    [name](decltype(_samplers)::value_type& s)
    {
      return s.first == name;
    });
    if(it != _samplers.end())
      return it->second.c_str();
    else return nullptr;
  }
  void removeSampler(const char* name)
  {
    auto it = std::find_if(_samplers.begin(), _samplers.end(),
    [name](decltype(_samplers)::value_type& s)
    {
      return s.first == name;
    });
    if(it != _samplers.end())
      _samplers.erase(it);
  }
  void replaceSamplerRes(const char* old_name, const char* new_name)
  {
    for(auto& p: _samplers)
      if(p.second == old_name) p.second = new_name;
  }
  void setUniform(const char* name, float a, float b, float c, float d)
  {
    auto it = std::find_if(_uniforms.begin(), _uniforms.end(),
    [name](decltype(_uniforms)::value_type& p)
    {
      return p.first == name;
    });
    if(it == _uniforms.end())
      _uniforms.push_back(std::make_pair(
      std::string(name), std::array<float, 4>{a, b, c, d}));
    else
    {
      it->second = std::array<float, 4>{a, b, c, d};
    }
  }
  void removeUniform(const char* name)
  {
    auto it = std::find_if(_uniforms.begin(), _uniforms.end(),
    [name](decltype(_uniforms)::value_type& p)
    {
      return p.first == name;
    });
    if(it != _uniforms.end())
      _uniforms.erase(it);
  }
  
  MaterialBuilder(const char* name)
  {
    res = h3dAddResource(H3DResTypes::Material, name, 0);
    _shader = "";
  }
  ~MaterialBuilder()
  {
    if(h3dIsResLoaded(res))
      h3dUnloadResource(res);
    h3dRemoveResource(res);
  }
  H3DRes getRes()
  {
    return res;
  }
  
  void build();
};

#endif
