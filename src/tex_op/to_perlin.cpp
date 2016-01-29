
#include "../rand.h"

#include "../tex_op.h"

#include "to_common.h"

#include <cstdio>

#include <algorithm>

#define __range(x) x.begin(), x.end()

namespace
{
  template<int mask>
  class _generateNoise
  {
  public:
    static void func(Texture* tex, uint32_t* rand, uint32_t rand_max,
      int from, int to)
    {
      for(int i = from; i < to; ++i)
      {
        if(mask & 1) tex->get(i)[0] = (float)rand[i * 4 + 0] / rand_max;
        if(mask & 2) tex->get(i)[1] = (float)rand[i * 4 + 1] / rand_max;
        if(mask & 4) tex->get(i)[2] = (float)rand[i * 4 + 2] / rand_max;
        if(mask & 8) tex->get(i)[3] = (float)rand[i * 4 + 3] / rand_max;
      }
    }
  };
  
  template<int mask>
  class _generateWhiteNoise
  {
  public:
    static void func(Texture* tex, uint32_t* rand, uint32_t rand_max,
      int from, int to)
    {
      for(int i = from; i < to; ++i)
      {
        if(mask & 1) tex->get(i)[0] = (float)rand[i] / rand_max;
        if(mask & 2) tex->get(i)[1] = (float)rand[i] / rand_max;
        if(mask & 4) tex->get(i)[2] = (float)rand[i] / rand_max;
        if(mask & 8) tex->get(i)[3] = (float)rand[i] / rand_max;
      }
    }
  };
  
  template<int mask>
  class _makeTurbulence
  {
  public:
    static void func(Texture* dest, const Texture* src,
      const std::vector<float>& levels, int from, int to)
    {
      Eigen::Array4f sample;
      float l_tot = 0.;
      for(float f: levels) l_tot += f;
      
      for(int i = from; i < to; ++i)
      {
        const int x = i % src->width;
        const int y = i / src->height;
        sample << 0., 0., 0., 0.;
        
        for(unsigned u = 0; u < levels.size(); ++u)
          sample += src->sampleBoxed(x, y, u) * levels[u];
        sample /= l_tot;
        
        if(mask == 0xf)
          dest->get(i) = sample;
        else
        {
          if(mask & 1) dest->get(i)[0] = sample[0];
          else dest->get(i)[0] = src->get(i)[0];
          if(mask & 2) dest->get(i)[1] = sample[1];
          else dest->get(i)[1] = src->get(i)[1];
          if(mask & 4) dest->get(i)[2] = sample[2];
          else dest->get(i)[2] = src->get(i)[2];
          if(mask & 8) dest->get(i)[3] = sample[3];
          else dest->get(i)[3] = src->get(i)[3];
        }
      }
    }
  };
}

namespace TexOp
{
  void generateNoise(Texture* tex, int rand_device, std::bitset<4> mask)
  {
    std::vector<uint32_t> rand = Rand::producei(
      rand_device, tex->width * tex->height * 4);
    uint32_t rand_max = Rand::getMax(rand_device);
    _launchThreadsMasked<_generateNoise>(mask.to_ulong(), tex, rand.data(), rand_max);
  }
  
  void generateWhiteNoise(Texture* tex, int rand_device, std::bitset<4> mask)
  {
    std::vector<uint32_t> rand = Rand::producei(
      rand_device, tex->width * tex->height);
    uint32_t rand_max = Rand::getMax(rand_device);
    _launchThreadsMasked<_generateWhiteNoise>
      (mask.to_ulong(), tex, rand.data(), rand_max);
  }
  
  void makeTurbulence(Texture* dest, const Texture* src, int levels,
    float persistance, std::bitset<4> mask)
  {
    std::vector<float> pers(levels);
    float f = 1.;
    std::for_each(__range(pers), [&f, persistance](float& p){p = f; f *= persistance;});
    _launchThreadsMasked<_makeTurbulence>(mask.to_ulong(), dest, src, std::cref(pers));
  }
}
