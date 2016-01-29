#ifndef H3D_H_INCLUDED
#define H3D_H_INCLUDED

#include <vector>

#include <horde3d.h>

namespace H3D
{
  void setPipeline(const char*);
  void setPipeline(const char*, size_t);
  void setGeo(const char*);
  void setGeo(const char*, size_t);
  void setGeo(
    std::vector<float> verts,
    std::vector<float> norms,
    std::vector<float> tangs,
    std::vector<float> binorms,
    std::vector<float> tcoords1,
    std::vector<float> tcoords2,
    std::vector<unsigned> idx,
    bool);
  void setShader(const char*);
  void setShader(const char*, size_t);
  
  void setUniform(const char*, float, float, float, float);
  void removeUniform(const char*);
  void enableShaderFlag(int);
  void disableShaderFlag(int);
  void enableRenderStage(const char*);
  void disableRenderStage(const char*);
  
  
  int loadTexture(const char*);
  int loadTexture(const char*, size_t);
  void bindSampler(int, const char*);
  void unbindSampler(const char*);
  void replaceSamplerRes(const char*, const char*);
  
  H3DRes createTexture(int, int);
  
  void dumpMessages();
  void dumpMessagesToStdout();
  
  void init();
  void deinit() noexcept;
}

#endif
