#ifndef TEX_OP_H
#define TEX_OP_H

#include <bitset>
#include <vector>

#include "texture.h"
#include "blurfilter.h"

#ifdef SQUSEDOUBLE
typedef double SQFloat;
#else
typedef float SQFloat;
#endif

namespace TexOp
{
  //note: This function destroys the old texture.
  Texture* resizeTexture(Texture*, int, int);
  void copyTexture(Texture*, Texture*, std::bitset<4>);
  
  void writeRawTexture(Texture*, const uint8_t*);
  void writeRawTexture(Texture*, const float*);
  void readRawTexture(uint8_t*, const Texture*);
  void readRawTexture(float*, const Texture*);
  void writeRawChannel(Texture*, const uint8_t*, std::bitset<4>);
  void writeRawChannel(Texture*, const float*, std::bitset<4>);
  void readRawChannel(uint8_t*, const Texture*, int);
  void readRawChannel(float*, const Texture*, int);
  
  void clear(Texture*, const Eigen::Array4f&, std::bitset<4>);
  void clear(Texture*, const Eigen::Array4f&);
  void blend(Texture*, const Eigen::Array4f&, const Eigen::Array4f&);
  
  void copyChannel(Texture*, std::bitset<4>, Texture*, int);
  void swapChannels(Texture*, int, Texture*, int);
  void swapChannels(Texture*, Texture*, std::bitset<4>);
  void blendChannels(Texture*, std::bitset<4>, Texture*, int, float);
  
  void clamp(Texture*, float, float, std::bitset<4>);
  void filter(Texture*, float, float, float, float, float, std::bitset<4>);
  void linearFilter(Texture*, float, float, float, float, std::bitset<4>);
  void stencilFilter(Texture*, float, bool, std::bitset<4>);
  void downsampleFilter(Texture*, unsigned, std::bitset<4>);
  
  void channelDiff(Texture*, Texture*, int, std::bitset<4>);
  void textureDiff(Texture*, Texture*, std::bitset<4>);
  
  void fillWithBlendChannel(Texture*, std::bitset<4>, const Eigen::Array4f&,
    Texture*, int);
  void fillWithRevBlendChannel(Texture*, std::bitset<4>, const Eigen::Array4f&,
    Texture*, int);
  
  void blendTexturesWithAlpha(Texture*, Texture*, std::bitset<4>, Texture*, int);
  
  void mergeTextures(Texture*, Texture*, float, std::bitset<4>);
  
  void displaceMap(Texture* targ, const Texture* src, Texture* dmap, float mult);
  void sampleMap(Texture*, const Texture*);
  void shiftTexels(Texture*, const Texture*, float, float);
  
  void blur(Texture*, const Texture*, BlurFilter2d*, float, std::bitset<4>);
  void blurHoriz(Texture*, const Texture*, BlurFilter1d*, float, std::bitset<4>);
  void blurVertic(Texture*, const Texture*, BlurFilter1d*, float, std::bitset<4>);
  
  void generateNoise(Texture*, int, std::bitset<4>);
  void generateWhiteNoise(Texture*, int, std::bitset<4>);
  void makeTurbulence(Texture*, const Texture*, int, float, std::bitset<4>);
  
  void makeCellNoise(Texture*, std::vector<std::pair<double, double>>&, float,
    std::array<int, 4>&);
    
  void makeDelaunay(Texture*, std::vector<std::pair<double, double>>&,
    float, std::bitset<4>);
  void makeVoronoi(Texture*, std::vector<std::pair<double, double>>&,
    float, std::bitset<4>);
    
  void makeNormalMap(Texture*, double);
  
  void init();
  void deinit() noexcept;
}

#endif
