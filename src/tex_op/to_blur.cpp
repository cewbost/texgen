
#include "../tex_op.h"

#include "to_common.h"

namespace
{
  template<int mask>
  class _blur
  {
  public:
    static void func(
      Texture* dest, const Texture* src, BlurFilter2d* filt, int from, int to)
    {
      for(int i = from; i < to; ++i)
      {
        unsigned x = i % src->width;
        unsigned y = i / src->width;
        
        Eigen::Array4f collector(0., 0., 0., 0.);
        for(unsigned _y = 0; _y < filt->height; ++_y)
        for(unsigned _x = 0; _x < filt->width; ++_x)
        {
          collector += src->getBounded(
            x - filt->width / 2 + _x,
            y - filt->height / 2 + _y)
            * filt->get(_x, _y);
        }
        collector /= filt->weight;
        if(mask == 0xf)
          dest->get(i) = collector;
        else
        {
          dest->get(i) = src->get(i);
          if(mask & 1) dest->get(i)[0] = collector[0];
          if(mask & 2) dest->get(i)[1] = collector[1];
          if(mask & 4) dest->get(i)[2] = collector[2];
          if(mask & 8) dest->get(i)[3] = collector[3];
        }
      }
    }
  };
  
  template<int mask>
  class _blurf
  {
  public:
    static void func(
      Texture* dest, const Texture* src, BlurFilter2d* filt, float f, int from, int to)
    {
      for(int i = from; i < to; ++i)
      {
        unsigned x = i % src->width;
        unsigned y = i / src->width;
        
        Eigen::Array4f collector(0., 0., 0., 0.);
        for(unsigned _y = 0; _y < filt->height; ++_y)
        for(unsigned _x = 0; _x < filt->width; ++_x)
        {
          collector += src->getBounded(
            x - filt->width / 2 + _x,
            y - filt->height / 2 + _y)
            * filt->get(_x, _y);
        }
        collector /= filt->weight;
        collector *= f;
        dest->get(i) *= 1. - f;
        if(mask == 0xf)
          dest->get(i) += collector;
        else
        {
          Eigen::Array4f current = src->get(i);
          if(mask & 1) dest->get(i)[0] += collector[0];
          else dest->get(i)[0] = current[0];
          if(mask & 2) dest->get(i)[1] += collector[1];
          else dest->get(i)[1] = current[1];
          if(mask & 4) dest->get(i)[2] += collector[2];
          else dest->get(i)[2] = current[2];
          if(mask & 8) dest->get(i)[3] += collector[3];
          else dest->get(i)[3] = current[3];
        }
      }
    }
  };
  
  template<int mask>
  class _blurHoriz
  {
  public:
    static void func(
      Texture* dest, const Texture* src, BlurFilter1d* filt, int from, int to)
    {
      for(int i = from; i < to; ++i)
      {
        unsigned x = i % src->width;
        unsigned y = i / src->width;
        
        Eigen::Array4f collector(0., 0., 0., 0.);
        for(unsigned j = 0; j < filt->size; ++j)
        {
          collector += src->getBounded(
            x - filt->size / 2 + j, y)
            * filt->get(j);
        }
        collector /= filt->weight;
        if(mask == 0xf)
          dest->get(i) = collector;
        else
        {
          dest->get(i) = src->get(i);
          if(mask & 1) dest->get(i)[0] = collector[0];
          if(mask & 2) dest->get(i)[1] = collector[1];
          if(mask & 4) dest->get(i)[2] = collector[2];
          if(mask & 8) dest->get(i)[3] = collector[3];
        }
      }
    }
  };
  
  template<int mask>
  class _blurHorizf
  {
  public:
    static void func(
      Texture* dest, const Texture* src, BlurFilter1d* filt, float f, int from, int to)
    {
      for(int i = from; i < to; ++i)
      {
        unsigned x = i % src->width;
        unsigned y = i / src->width;
        
        Eigen::Array4f collector(0., 0., 0., 0.);
        for(unsigned j = 0; j < filt->size; ++j)
        {
          collector += src->getBounded(
            x - filt->size / 2 + j, y)
            * filt->get(j);
        }
        collector /= filt->weight;
        collector *= f;
        dest->get(i) *= 1. - f;
        if(mask == 0xf)
          dest->get(i) += collector;
        else
        {
          Eigen::Array4f current = src->get(i);
          if(mask & 1) dest->get(i)[0] += collector[0];
          else dest->get(i)[0] = current[0];
          if(mask & 2) dest->get(i)[1] += collector[1];
          else dest->get(i)[1] = current[1];
          if(mask & 4) dest->get(i)[2] += collector[2];
          else dest->get(i)[2] = current[2];
          if(mask & 8) dest->get(i)[3] += collector[3];
          else dest->get(i)[3] = current[3];
        }
      }
    }
  };
  
  template<int mask>
  class _blurVertic
  {
  public:
    static void func(
      Texture* dest, const Texture* src, BlurFilter1d* filt, int from, int to)
    {
      for(int i = from; i < to; ++i)
      {
        unsigned x = i % src->width;
        unsigned y = i / src->width;
        
        Eigen::Array4f collector(0., 0., 0., 0.);
        for(unsigned j = 0; j < filt->size; ++j)
        {
          collector += src->getBounded(
            x, y - filt->size / 2 + j)
            * filt->get(j);
        }
        collector /= filt->weight;
        if(mask == 0xf)
          dest->get(i) = collector;
        else
        {
          dest->get(i) = src->get(i);
          if(mask & 1) dest->get(i)[0] = collector[0];
          if(mask & 2) dest->get(i)[1] = collector[1];
          if(mask & 4) dest->get(i)[2] = collector[2];
          if(mask & 8) dest->get(i)[3] = collector[3];
        }
      }
    }
  };
  
  template<int mask>
  class _blurVerticf
  {
  public:
    static void func(
      Texture* dest, const Texture* src, BlurFilter1d* filt, float f, int from, int to)
    {
      for(int i = from; i < to; ++i)
      {
        unsigned x = i % src->width;
        unsigned y = i / src->width;
        
        Eigen::Array4f collector(0., 0., 0., 0.);
        for(unsigned j = 0; j < filt->size; ++j)
        {
          collector += src->getBounded(
            x, y - filt->size / 2 + j)
            * filt->get(j);
        }
        collector /= filt->weight;
        collector *= f;
        dest->get(i) *= 1. - f;
        if(mask == 0xf)
          dest->get(i) += collector;
        else
        {
          Eigen::Array4f current = src->get(i);
          if(mask & 1) dest->get(i)[0] += collector[0];
          else dest->get(i)[0] = current[0];
          if(mask & 2) dest->get(i)[1] += collector[1];
          else dest->get(i)[1] = current[1];
          if(mask & 4) dest->get(i)[2] += collector[2];
          else dest->get(i)[2] = current[2];
          if(mask & 8) dest->get(i)[3] += collector[3];
          else dest->get(i)[3] = current[3];
        }
      }
    }
  };
}

namespace TexOp
{
  void blur(Texture* dest, const Texture* src, BlurFilter2d* filter,
    float f, std::bitset<4> mask)
  {
    if(f == 1.)
      _launchThreadsMasked<_blur>(mask.to_ulong(), dest, src, filter);
    else _launchThreadsMasked<_blurf>(mask.to_ulong(), dest, src, filter, f);
  }
  
  void blurHoriz(Texture* dest, const Texture* src, BlurFilter1d* filter,
    float f, std::bitset<4> mask)
  {
    if(f == 1.)
      _launchThreadsMasked<_blurHoriz>(mask.to_ulong(), dest, src, filter);
    else _launchThreadsMasked<_blurHorizf>(mask.to_ulong(), dest, src, filter, f);
  }
  
  void blurVertic(Texture* dest, const Texture* src, BlurFilter1d* filter,
    float f, std::bitset<4> mask)
  {
    if(f == 1.)
      _launchThreadsMasked<_blurVertic>(mask.to_ulong(), dest, src, filter);
    else _launchThreadsMasked<_blurVerticf>(mask.to_ulong(), dest, src, filter, f);
  }
}
