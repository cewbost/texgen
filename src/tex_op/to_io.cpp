
#include "../tex_op.h"
#include "to_common.h"

namespace
{
  void _copyc2t(Texture* dest, const uint8_t* src, int from, int to)
  {
    auto dest_p = dest->data() + from;
    src += from * 4;
    for(int i = from; i < to; ++i)
    {
      (*dest_p)[0] = (float)src[0] / 255;
      (*dest_p)[1] = (float)src[1] / 255;
      (*dest_p)[2] = (float)src[2] / 255;
      (*dest_p)[3] = (float)src[3] / 255;
      ++dest_p;
      src += 4;
    }
  }
  void _copyt2c(uint8_t* dest, const Texture* src, int from, int to)
  {
    auto src_p = src->data() + from;
    dest += from * 4;
    for(int i = from; i < to; ++i)
    {
      dest[0] = (uint8_t)((*src_p)[0] * 255.5);
      dest[1] = (uint8_t)((*src_p)[1] * 255.5);
      dest[2] = (uint8_t)((*src_p)[2] * 255.5);
      dest[3] = (uint8_t)((*src_p)[3] * 255.5);
      dest += 4;
      ++src_p;
    }
  }
  
  void _copyf2t(Texture* dest, const float* src, int from, int to)
  {
    auto src_p = src + from * 4;
    for(int i = from; i < to; ++i)
    {
      dest->get(i)[0] = (float)src_p[0];
      dest->get(i)[1] = (float)src_p[1];
      dest->get(i)[2] = (float)src_p[2];
      dest->get(i)[3] = (float)src_p[3];
      src_p += 4;
    }
  }
  void _copyt2f(float* dest, const Texture* src, int from, int to)
  {
    auto dest_p = dest + from * 4;
    for(int i = from; i < to; ++i)
    {
      dest_p[0] = (float)src->get(i)[0];
      dest_p[1] = (float)src->get(i)[1];
      dest_p[2] = (float)src->get(i)[2];
      dest_p[3] = (float)src->get(i)[3];
      dest_p += 4;
    }
  }
  
  template<int mask>
  class _copyChannelc2t
  {
  public:
    static void func(Texture* dest, const uint8_t* src, int from, int to)
    {
      auto dest_p = dest->data() + from;
      src += from;
      for(int i = from; i < to; ++i)
      {
        if(mask & 0x1) (*dest_p)[0] = (float)(*src) / 255;
        if(mask & 0x2) (*dest_p)[1] = (float)(*src) / 255;
        if(mask & 0x4) (*dest_p)[2] = (float)(*src) / 255;
        if(mask & 0x8) (*dest_p)[3] = (float)(*src) / 255;
        ++dest_p;
        ++src;
      }
    }
  };
  
  template<int mask>
  class _copyChannelf2t
  {
  public:
    static void func(Texture* dest, const float* src, int from, int to)
    {
      for(int i = from; i < to; ++i)
      {
        if(mask & 0x1) dest->get(i)[0] = (float)src[i];
        if(mask & 0x2) dest->get(i)[1] = (float)src[i];
        if(mask & 0x4) dest->get(i)[2] = (float)src[i];
        if(mask & 0x8) dest->get(i)[3] = (float)src[i];
      }
    }
  };
  
  template<int ch>
  class _copyChannelt2c
  {
  public:
    static void func(const Texture* src, uint8_t* dest, int from, int to)
    {
      auto src_p = src->data() + from;
      dest += from;
      for(int i = from; i < to; ++i)
      {
        *dest = (uint8_t)((*src_p)[ch] * 255.5);
        ++dest;
        ++src_p;
      }
    }
  };
  
  template<int ch>
  class _copyChannelt2f
  {
  public:
    static void func(const Texture* src, float* dest, int from, int to)
    {
      for(int i = from; i < to; ++i)
        dest[i] = (float)src->get(i)[ch];
    }
  };
}

namespace TexOp
{
  void writeRawTexture(Texture* dest, const uint8_t* src)
  {
    int elements = dest->width * dest->height;
    _launchThreads(elements, _copyc2t, dest, src);
  }
  void writeRawTexture(Texture* dest, const float* src)
  {
    int elements = dest->width * dest->height;
    _launchThreads(elements, _copyf2t, dest, src);
  }
  void writeRawChannel(Texture* dest, const uint8_t* src, std::bitset<4> mask)
  {
    _launchThreadsMasked<_copyChannelc2t>(mask.to_ulong(), dest, src);
  }
  void writeRawChannel(Texture* dest, const float* src, std::bitset<4> mask)
  {
    _launchThreadsMasked<_copyChannelf2t>(mask.to_ulong(), dest, src);
  }
  void readRawTexture(uint8_t* dest, const Texture* src)
  {
    int elements = src->width * src->height;
    _launchThreads(elements, _copyt2c, dest, src);
  }
  void readRawTexture(float* dest, const Texture* src)
  {
    int elements = src->width * src->height;
    _launchThreads(elements, _copyt2f, dest, src);
  }
  void readRawChannel(uint8_t* dest, const Texture* src, int ch)
  {
    _launchThreadsCh<_copyChannelt2c>(ch, src, dest);
  }
  void readRawChannel(float* dest, const Texture* src, int ch)
  {
    _launchThreadsCh<_copyChannelt2f>(ch, src, dest);
  }
}