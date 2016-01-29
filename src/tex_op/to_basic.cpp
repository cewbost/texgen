
#include "../tex_op.h"

#include "to_common.h"

namespace
{
  //resizing
  /*
    note:
      downsizing should be passed the dimensions to be. upsizeing the dimensions that
      are.
  */
  void _upsizeX(Texture* dest, Texture* src,
    int width, int ysrs, int ydest)
  {
    for(int y = ysrs; y < ydest; ++y)
    {
      for(int x = 0; x < width - 1; ++x)
      {
        dest->operator()(x * 2, y) = src->operator()(x, y);
        dest->operator()(x * 2 + 1, y) =
          src->operator()(x, y) * .5 + src->operator()(x + 1, y) * .5;
      }
      dest->operator()((width - 1) * 2, y) =
        src->operator()(width - 1, y);
      dest->operator()((width - 1) * 2 + 1, y) =
        src->operator()(width - 1, y) * .5 + src->operator()(0, y) * .5;
    }
  }
  void _downsizeX(Texture* dest, Texture* src,
    int width, int ysrs, int ydest)
  {
    for(int y = ysrs; y < ydest; ++y)
    for(int x = 0; x < width; ++x)
    {
      dest->operator()(x, y) =
        src->operator()(x * 2, y) * .5 + src->operator()(x * 2 + 1, y) * .5;
    }
  }
  void _upsizeY(Texture* dest, Texture* src,
    int width, int ysrs, int ydest)
  {
    for(int y = ysrs; y < ydest; ++y)
    for(int x = 0; x < width; ++x)
    {
      dest->operator()(x, y * 2) = src->operator()(x, y);
      dest->operator()(x, y * 2 + 1) =
        src->operator()(x, y) * .5 + src->operator()(x, y + 1) * .5;
    }
  }
  void _upsizeYsafe(Texture* dest, Texture* src,
    int width, int ysrs, int ydest)
  {
    for(int y = ysrs; y < ydest - 1; ++y)
    for(int x = 0; x < width; ++x)
    {
      dest->operator()(x, y * 2) = src->operator()(x, y);
      dest->operator()(x, y * 2 + 1) =
        src->operator()(x, y) * .5 + src->operator()(x, y + 1) * .5;
    }
    for(int x = 0; x < width; ++x)
    {
      dest->operator()(x, ydest * 2 - 2) = src->operator()(x, ydest - 1);
      dest->operator()(x, ydest * 2 - 1) =
        src->operator()(x, ydest - 1) * .5 + src->operator()(x, 0) * .5;
    }
  }
  void _downsizeY(Texture* dest, Texture* src,
    int width, int ysrs, int ydest)
  {
    for(int y = ysrs; y < ydest; ++y)
    for(int x = 0; x < width; ++x)
    {
      dest->operator()(x, y) =
        src->operator()(x, y * 2) * .5 + src->operator()(x, y * 2 + 1) * .5;
    }
  }

  template<int mask>
  class _copyTex
  {
  public:
    static void func(Texture* dest, Texture* src, int from, int to)
    {
      auto src_p = src->data() + from;
      auto dest_p = dest->data() + from;
      for(int i = from; i < to; ++i)
      {
        if(mask == 0xf) *dest_p = *src_p;
        else
        {
          if(mask & 1) (*dest_p)[0] = (*src_p)[0];
          if(mask & 2) (*dest_p)[1] = (*src_p)[1];
          if(mask & 4) (*dest_p)[2] = (*src_p)[2];
          if(mask & 8) (*dest_p)[3] = (*src_p)[3];
        }
        ++src_p;
        ++dest_p;
      }
    }
  };
  
  template<int mask>
  class _clearChannels
  {
  public:
    static void func(Texture* tex, const Eigen::Array4f& val, int from, int to)
    {
      for(int i = from; i < to; ++i)
      {
        if(mask == 0xf) tex->get(i) = val;
        else
        {
          if(mask & 0x1) tex->get(i)[0] = val[0];
          if(mask & 0x2) tex->get(i)[1] = val[1];
          if(mask & 0x4) tex->get(i)[2] = val[2];
          if(mask & 0x8) tex->get(i)[3] = val[3];
        }
      }
    }
  };
  
  void _clear(Texture* tex, const Eigen::Array4f &col, int from, int to)
  {
    for(int i = from; i < to; ++i)
      tex->get(i) = col;
  }
  
  template<int ch>
  class _copyChannel
  {
  public:
    template<int mask>
    class NextC
    {
    public:
      static void func(Texture* dest, Texture* src, int from, int to)
      {
        auto dest_p = dest->data() + from;
        auto src_p = src->data() + from;
        for(int i = from; i < to; ++i)
        {
          if(mask & (1 << (ch + 1) % 4)) (*dest_p)[(ch + 1) % 4] = (*src_p)[ch];
          if(mask & (1 << (ch + 2) % 4)) (*dest_p)[(ch + 2) % 4] = (*src_p)[ch];
          if(mask & (1 << (ch + 3) % 4)) (*dest_p)[(ch + 3) % 4] = (*src_p)[ch];
          if(mask & (1 << (ch    ) % 4)) (*dest_p)[(ch    ) % 4] = (*src_p)[ch];
          ++dest_p;
          ++src_p;
        }
      }
    };
  };
  
  template<int DC, int SC>
  class _swapChannels
  {
  public:
    static void func(Texture* dest, Texture* src, int from, int to)
    {
      auto dest_p = dest->data() + from;
      auto src_p = src->data() + from;
      for(int i = from; i < to; ++i)
      {
        float temp = (*dest_p)[DC];
        (*dest_p)[DC] = (*src_p)[SC];
        (*src_p)[SC] = temp;
        ++dest_p;
        ++src_p;
      }
    }
  };
  template<int mask>
  class _swapChannelsPairwise
  {
  public:
    static void func(Texture* tex1, Texture* tex2, int from, int to)
    {
      float temp;
      for(int i = from; i < to; ++i)
      {
        if(mask & 1)
        {
          temp = tex2->get(i)[0];
          tex2->get(i)[0] = tex1->get(i)[0];
          tex1->get(i)[0] = temp;
        }
        if(mask & 2)
        {
          temp = tex2->get(i)[1];
          tex2->get(i)[1] = tex1->get(i)[1];
          tex1->get(i)[1] = temp;
        }
        if(mask & 4)
        {
          temp = tex2->get(i)[2];
          tex2->get(i)[2] = tex1->get(i)[2];
          tex1->get(i)[2] = temp;
        }
        if(mask & 8)
        {
          temp = tex2->get(i)[3];
          tex2->get(i)[3] = tex1->get(i)[3];
          tex1->get(i)[3] = temp;
        }
      }
    }
  };
}

namespace TexOp
{
  Texture* resizeTexture(Texture* tex, int width, int height)
  {
    /*
      note:
        This might not be the most efficient solution.
        An iterative approach was taken since the logic for bilinear texturing is more
        complex, however it instead performs many allocations/deallocations. Those are
        pooled so might not be very heavy, but they do thrash the cache.
    */
  
    Texture* old_tex;
    
    int x_samp, y_samp;
    if((unsigned)width >= tex->width) x_samp = (width / tex->width) / 2;
    else x_samp = -((tex->width / width) / 2);
    if((unsigned)height >= tex->height) y_samp = (height / tex->height) / 2;
    else y_samp = -((tex->height / height) / 2);
    
    while(x_samp > 0)
    {
      old_tex = tex;
      int width = old_tex->width;
      int height = old_tex->height;
      tex = makeTexture(width * 2, height);
      
      _launchThreads(height, _upsizeX, tex, old_tex, width);
      
      deleteTexture(old_tex);
      x_samp /= 2;
    }
    while(x_samp < 0)
    {
      old_tex = tex;
      int width = old_tex->width;
      int height = old_tex->height;
      tex = makeTexture(width / 2, height);
      
      _launchThreads(height, _downsizeX, tex, old_tex, width);
      
      deleteTexture(old_tex);
      x_samp /= 2;
    }
    while(y_samp > 0)
    {
      old_tex = tex;
      int width = old_tex->width;
      int height = old_tex->height;
      tex = makeTexture(width, height * 2);
      
      _launchThreadsAnd(height, _upsizeY, _upsizeYsafe, tex, old_tex, width);
      
      deleteTexture(old_tex);
      y_samp /= 2;
    }
    while(y_samp < 0)
    {
      old_tex = tex;
      int width = old_tex->width;
      int height = old_tex->height;
      tex = makeTexture(width, height / 2);
      
      _launchThreads(height, _downsizeY, tex, old_tex, width);
      
      deleteTexture(old_tex);
      y_samp /= 2;
    }
    
    return tex;
  }
  
  void copyTexture(Texture* dest, Texture* src, std::bitset<4> mask)
  {
    _launchThreadsMasked<_copyTex>(mask.to_ulong(), dest, src);
  }
  
  void clear(
    Texture* tex,
    const Eigen::Array4f& color,
    std::bitset<4> mask)
  {
    _launchThreadsMasked<_clearChannels>(mask.to_ulong(), tex, color);
  }
  void clear(Texture* tex, const Eigen::Array4f &color)
  {
    int elements = tex->width * tex->height;
    _launchThreads(elements, _clear, tex, color);
  }
  
  void copyChannel(Texture* dest, std::bitset<4> mask, Texture* src, int ch)
  {
    _launchThreadsCh2Masked<_copyChannel>(ch, mask.to_ulong(), dest, src);
  }
  
  void swapChannels(Texture* dest, int dch, Texture* src, int sch)
  {
    _launchThreads2Ch<_swapChannels>(dch, sch, dest, src);
  }
  void swapChannels(Texture* tex1, Texture* tex2, std::bitset<4> mask)
  {
    _launchThreadsMasked<_swapChannelsPairwise>(mask.to_ulong(), tex1, tex2);
  }
}