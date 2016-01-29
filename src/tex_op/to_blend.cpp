
#include "../tex_op.h"

#include "to_common.h"

namespace
{
  void _blend(
    Texture* tex,
    const Eigen::Array4f &chs,
    const Eigen::Array4f &blend,
    int from, int to)
  {
    Eigen::Array4f r_blend = Eigen::Array4f(1., 1., 1., 1.) - blend;
    for(int i = from; i < to; ++i)
      tex->get(i) = tex->get(i) * r_blend + chs * blend;
  }
  
  template<int ch>
  class _blendChannels
  {
  public:
    template<int mask>
    class NextC
    {
    public:
      static void func(Texture* dest, Texture* src, float blend, int from, int to)
      {
        auto dest_p = dest->data() + from;
        auto src_p = src->data() + from;
        for(int i = from; i < to; ++i)
        {
          if(mask & (1 << (ch + 1) % 4))
            (*dest_p)[(ch + 1) % 4] =
              (*src_p)[ch] * blend + (*dest_p)[(ch + 1) % 4] * (1. - blend);
          if(mask & (1 << (ch + 2) % 4))
            (*dest_p)[(ch + 2) % 4] =
              (*src_p)[ch] * blend + (*dest_p)[(ch + 2) % 4] * (1. - blend);
          if(mask & (1 << (ch + 3) % 4))
            (*dest_p)[(ch + 3) % 4] =
              (*src_p)[ch] * blend + (*dest_p)[(ch + 3) % 4] * (1. - blend);
          if(mask & (1 << (ch    ) % 4))
            (*dest_p)[(ch    ) % 4] =
              (*src_p)[ch] * blend + (*dest_p)[(ch    ) % 4] * (1. - blend);
          ++dest_p;
          ++src_p;
        }
      }
    };
  };
  
  template<int ch>
  class _blendTex
  {
  public:
    template<int mask>
    class NextC
    {
    public:
      static void func(Texture* dest, Texture* src, Texture* a_tex, int from, int to)
      {
        for(int i = from; i < to; ++i)
        {
          float blend_fac = a_tex->get(i)[ch];
          if(mask == 0xf)
          {
            dest->get(i) = dest->get(i) * (1. - blend_fac) + src->get(i) * blend_fac;
          }
          else
          {
            if(mask & 1)
              dest->get(i)[0] = 
                dest->get(i)[0] * (1. - blend_fac) + src->get(i)[0] * blend_fac;
            if(mask & 2)
              dest->get(i)[1] = 
                dest->get(i)[1] * (1. - blend_fac) + src->get(i)[1] * blend_fac;
            if(mask & 4)
              dest->get(i)[2] = 
                dest->get(i)[2] * (1. - blend_fac) + src->get(i)[2] * blend_fac;
            if(mask & 8)
              dest->get(i)[3] = 
                dest->get(i)[3] * (1. - blend_fac) + src->get(i)[3] * blend_fac;
          }
        }
      }
    };
  };
  
  template<int mask>
  class _mergeTex
  {
  public:
    static void func(Texture* dest, Texture* src, float blend, int from, int to)
    {
      for(int i = from; i < to; ++i)
      {
        if(mask == 0xf)
        {
          dest->get(i) = dest->get(i) * (1. - blend) * src->get(i) * blend;
        }
        else
        {
          if(mask & 1) dest->get(i)[0] =
            dest->get(i)[0] * (1. - blend) * src->get(i)[0] * blend;
          if(mask & 2) dest->get(i)[1] =
            dest->get(i)[1] * (1. - blend) * src->get(i)[1] * blend;
          if(mask & 4) dest->get(i)[2] =
            dest->get(i)[2] * (1. - blend) * src->get(i)[2] * blend;
          if(mask & 8) dest->get(i)[3] =
            dest->get(i)[3] * (1. - blend) * src->get(i)[3] * blend;
        }
      }
    }
  };
  
  template<int ch>
  class _blendCwA
  {
  public:
    template<int mask>
    class NextC
    {
    public:
      static void func(
        Texture* dest, Texture* src, const Eigen::Array4f& val, int from, int to)
      {
        auto dest_p = dest->data() + from;
        auto src_p = src->data() + from;
        for(int i = from; i < to; ++i)
        {
          if(mask & (1 << (ch + 1) % 4))
            (*dest_p)[(ch + 1) % 4] =
              (*src_p)[ch] * (*dest_p)[(ch + 1) % 4] +
              (1. - (*src_p)[ch]) * val[(ch + 1) % 4];
          if(mask & (1 << (ch + 2) % 4))
            (*dest_p)[(ch + 2) % 4] =
              (*src_p)[ch] * (*dest_p)[(ch + 2) % 4] +
              (1. - (*src_p)[ch]) * val[(ch + 2) % 4];
          if(mask & (1 << (ch + 3) % 4))
            (*dest_p)[(ch + 3) % 4] =
              (*src_p)[ch] * (*dest_p)[(ch + 3) % 4] +
              (1. - (*src_p)[ch]) * val[(ch + 3) % 4];
          if(mask & (1 << (ch    ) % 4))
            (*dest_p)[(ch    ) % 4] =
              (*src_p)[ch] * (*dest_p)[(ch    ) % 4] +
              (1. - (*src_p)[ch]) * val[(ch    ) % 4];
          ++dest_p;
          ++src_p;
        }
      }
    };
  };
  
  template<int ch>
  class _blendCwrA
  {
  public:
    template<int mask>
    class NextC
    {
    public:
      static void func(
        Texture* dest, Texture* src, const Eigen::Array4f& val, int from, int to)
      {
        auto dest_p = dest->data() + from;
        auto src_p = src->data() + from;
        for(int i = from; i < to; ++i)
        {
          if(mask & (1 << (ch + 1) % 4))
            (*dest_p)[(ch + 1) % 4] =
              (1. - (*src_p)[ch]) * (*dest_p)[(ch + 1) % 4] +
              (*src_p)[ch] * val[(ch + 1) % 4];
          if(mask & (1 << (ch + 2) % 4))
            (*dest_p)[(ch + 2) % 4] =
              (1. - (*src_p)[ch]) * (*dest_p)[(ch + 2) % 4] +
              (*src_p)[ch] * val[(ch + 2) % 4];
          if(mask & (1 << (ch + 3) % 4))
            (*dest_p)[(ch + 3) % 4] =
              (1. - (*src_p)[ch]) * (*dest_p)[(ch + 3) % 4] +
              (*src_p)[ch] * val[(ch + 3) % 4];
          if(mask & (1 << (ch    ) % 4))
            (*dest_p)[(ch    ) % 4] =
              (1. - (*src_p)[ch]) * (*dest_p)[(ch    ) % 4] +
              (*src_p)[ch] * val[(ch    ) % 4];
          ++dest_p;
          ++src_p;
        }
      }
    };
  };
  
  template<int ch>
  class _diffCh
  {
  public:
    template<int mask>
    class NextC
    {
    public:
      static void func(Texture* dest, Texture* src, int from, int to)
      {
        for(int i = from; i < to; ++i)
        {
          float s = src->get(i)[ch];
          if(mask & (1 << ((1 + ch) % 4)))
            dest->get(i)[(ch + 1) % 4] = fabs(dest->get(i)[ch] - s);
          if(mask & (1 << ((2 + ch) % 4)))
            dest->get(i)[(ch + 2) % 4] = fabs(dest->get(i)[ch] - s);
          if(mask & (1 << ((3 + ch) % 4)))
            dest->get(i)[(ch + 3) % 4] = fabs(dest->get(i)[ch] - s);
          if(mask & (1 << ch)) dest->get(i)[ch] = fabs(dest->get(i)[ch] - s);
        }
      }
    };
  };
  
  template<int mask>
  class _diffTex
  {
  public:
    static void func(Texture* dest, Texture* src, int from, int to)
    {
      for(int i = from; i < to; ++i)
      {
        if(mask & 0x1) dest->get(i)[0] = fabs(dest->get(i)[0] - src->get(i)[0]);
        if(mask & 0x2) dest->get(i)[1] = fabs(dest->get(i)[1] - src->get(i)[1]);
        if(mask & 0x4) dest->get(i)[2] = fabs(dest->get(i)[2] - src->get(i)[2]);
        if(mask & 0x8) dest->get(i)[3] = fabs(dest->get(i)[3] - src->get(i)[3]);
      }
    }
  };
}

namespace TexOp
{
  void blend(Texture* tex, const Eigen::Array4f &color, const Eigen::Array4f &mask)
  {
    int elements = tex->width * tex->height;
    _launchThreads(elements, _blend, tex, color, mask);
  }
  
  void blendChannels(
    Texture* dest,
    std::bitset<4> mask,
    Texture* src,
    int ch, float blend)
  {
    _launchThreadsCh2Masked<_blendChannels>(ch, mask.to_ulong(), dest, src, blend);
  }
  
  void blendTexturesWithAlpha(
    Texture* dest, Texture* src, std::bitset<4> mask, Texture* a_tex, int a_ch)
  {
    _launchThreadsCh2Masked<_blendTex>(a_ch, mask.to_ulong(), dest, src, a_tex);
  }
  
  void mergeTextures(Texture* dest, Texture* src, float blend, std::bitset<4> mask)
  {
    _launchThreadsMasked<_mergeTex>(mask.to_ulong(), dest, src, blend);
  }
  
  void fillWithBlendChannel(
    Texture* dest,
    std::bitset<4> mask,
    const Eigen::Array4f& color,
    Texture* blend_tex,
    int blend_ch)
  {
    _launchThreadsCh2Masked<_blendCwA>
      (blend_ch, mask.to_ulong(), dest, blend_tex, std::ref(color));
  }
  
  void fillWithRevBlendChannel(
    Texture* dest,
    std::bitset<4> mask,
    const Eigen::Array4f& color,
    Texture* blend_tex,
    int blend_ch)
  {
    _launchThreadsCh2Masked<_blendCwrA>
      (blend_ch, mask.to_ulong(), dest, blend_tex, std::ref(color));
  }
  
  void channelDiff(Texture* dest, Texture* src, int ch, std::bitset<4> mask)
  {
    _launchThreadsCh2Masked<_diffCh>(ch, mask.to_ulong(), dest, src);
  }
  
  void textureDiff(Texture* dest, Texture* src, std::bitset<4> mask)
  {
    _launchThreadsMasked<_diffTex>(mask.to_ulong(), dest, src);
  }
}
