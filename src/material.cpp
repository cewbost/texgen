#include <sstream>

#include "common.h"

#include "material.h"

void MaterialBuilder::build()
{
  std::stringstream stream;
  
  stream << "<Material>";
  
  //shader
  stream  
    << "<Shader source=\""
    << _shader
    << "\" />";
  //shader flags
  for(int i = 1; i <= 32; ++i)
  if(_shader_flags[i])
    stream
      << "<ShaderFlag name=\"_F"
      << (char)('0' + (i / 10))
      << (char)('0' + (i % 10))
      << "_\" />";
  //samplers
  for(auto& samp: _samplers)
    stream 
      << "<Sampler name=\""
      << samp.first
      << "\" map=\""
      << samp.second
      << "\" />";
  //uniforms
  for(auto& uni: _uniforms)
    stream 
      << "<Uniform name=\""
      << uni.first
      << "\" a=\""
      << uni.second[0]
      << "\" b=\""
      << uni.second[1]
      << "\" c=\""
      << uni.second[2]
      << "\" d=\""
      << uni.second[3]
      << "\" />";
  
  stream << "</Material>";
  
  std::string buffer = stream.str();
  
  //this is for debugging purposes
  //printf("%s\n", buffer.c_str());
  
  if(h3dIsResLoaded(res))
    h3dUnloadResource(res);
  if(!h3dLoadResource(res, buffer.data(), buffer.size()))
    throw tgException("MaterialBuilder failed! material: %s", buffer.c_str());
}