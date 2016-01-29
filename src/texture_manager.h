#ifndef TEXTURE_MANAGER_H_INCLUDED
#define TEXTURE_MANAGER_H_INCLUDED

#include "texture.h"

namespace TextureManager
{
  void init();
  void deinit();

  int addTexture(H3DRes);
  int addTexture(int, int);
  int cloneTexture(int);
  void copyTexture(int, int, std::bitset<4>);
  void swapTextures(int, int, std::bitset<4>);
  void destroyTexture(int);
  void destroyTextures(std::vector<int>::iterator, std::vector<int>::iterator);
  
  std::vector<int> listTextures();
  
  void removeResource(H3DRes);
  
  void writeRawTexture(int, const uint8_t*);
  void writeRawTexture(int, const float*);
  void readRawTexture(uint8_t*, int);
  void readRawTexture(float*, int);
  void writeRawChannel(int, const uint8_t*, std::bitset<4>);
  void writeRawChannel(int, const float*, std::bitset<4>);
  void readRawChannel(uint8_t*, int, int);
  void readRawChannel(float*, int, int);
  
  void fillTexture(int, const std::array<float, 4>&, std::bitset<4>);
  void fillBlended(int, const std::array<float, 4>&, const std::array<float, 4>&);
  void filterTexture(int, float, float, float, float, float, std::bitset<4>);
  void clampTexels(int, float, float, std::bitset<4>);
  void filterTextureLin(int, float, float, float, float, std::bitset<4>);
  void filterTextureStencil(int, float, bool, std::bitset<4>);
  void filterTextureDownsample(int, int, std::bitset<4>);
  
  void channelDiff(int, int, int, std::bitset<4>);
  void textureDiff(int, int, std::bitset<4>);
  
  void fillBackground(int, const std::array<float, 4>&, std::bitset<4>);
  void fillWithAlphaCh(int, const std::array<float, 4>&, std::bitset<4>, int, int);
  
  void blendTextures(int, int, std::bitset<4>, int);
  void blendTexturesWithCh(int, int, std::bitset<4>, int, int);
  void mergeTextures(int, int, float, std::bitset<4>);
  
  void copyChannel(int, std::bitset<4>, int, int);
  void swapChannels(int, int, int, int);
  void blendChannels(int, std::bitset<4>, int, int, float);
  
  int warpTexture(int, int, float);
  void warpTextureInplace(int, int, float);
  int shiftTexels(int, float, float);
  void shiftTexelsInplace(int, float, float);
  int applyLens(int, int);
  void applyLensInplace(int, int);
  
  int blurTexture(int, int, int, float, std::bitset<4>);
  void blurInplace(int, int, int, float, std::bitset<4>);
  
  void resizeTexture(int, unsigned, unsigned);
  decltype(TexHeader().getDimensions()) getTextureDimensions(int);
  
  void generateNoise(int, int, std::bitset<4>);
  void generateWhiteNoise(int, int, std::bitset<4>);
  int makeTurbulence(int, int, float, std::bitset<4>);
  void makeTurbulenceInplace(int, int, float, std::bitset<4>);
  
  void makeCellNoise(
    int, std::vector<std::pair<double, double>>&, float, std::array<int, 4>&);
    
  void makeDelaunay(
    int, std::vector<std::pair<double, double>>&, float, std::bitset<4>);
  void makeVoronoi(
    int, std::vector<std::pair<double, double>>&, float, std::bitset<4>);
  
  void makeNormalMap(int, double);
  
  H3DRes getTexRes(int);
  
  void writeToDisk(int, const char*);
}

#endif
