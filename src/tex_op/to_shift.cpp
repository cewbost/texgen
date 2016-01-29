
#include "../tex_op.h"

#include "to_common.h"

namespace
{
  void _displaceMap(
    Texture* dest, const Texture* src, const Texture* d_map,
    float multip, int from, int to)
  {
    for(int i = from; i < to; ++i)
    {
      unsigned frag_x = (i % src->width) * 64;
      unsigned frag_y = (i / src->width) * 64;
      frag_x += (int)((d_map->get(i)[0] * multip) * 64);
      frag_y += (int)((d_map->get(i)[1] * multip) * 64);
      dest->get(i) = src->sampleTrivial<64>
        (frag_x % (src->width * 64), frag_y % (src->height * 64));
    }
  }
  
  void _sampleMap(Texture* dest, const Texture* src, int from, int to)
  {
    unsigned w = src->width * 64;
    unsigned h = src->height * 64;
    for(int i = from; i < to; ++i)
    {
      float x = dest->get(i)[0];
      float y = dest->get(i)[1];
      x = x < 0.? 0. : (x > 1.? 1. : x);
      y = y < 0.? 0. : (y > 1.? 1. : y);
      dest->get(i) = src->sampleTrivial<64>((unsigned)(x * w), (unsigned)(y * h));
    }
  }
  
  void _shiftTexels(Texture* dest, const Texture* src,
    float x, float y, int from, int to)
  {
    int x_shift = (int)(x * src->width);
    int y_shift = (int)(y * src->height);
    for(int i = from; i < to; ++i)
    {
      dest->get(i) = src->get(
        ((i % src->width) + (unsigned)x_shift) % src->width,
        ((i / src->height) + (unsigned)y_shift) % src->height);
    }
  }
}

namespace TexOp
{
  void displaceMap(Texture* dest, const Texture* src, Texture* dmap, float fac)
  {
    int elements = dest->width * dest->height;
    _launchThreads(elements, _displaceMap, dest, src, dmap, fac);
  }
  
  void sampleMap(Texture* dest, const Texture* src)
  {
    int elements = dest->width * dest->height;
    _launchThreads(elements, _sampleMap, dest, src);
  }
  
  void shiftTexels(Texture* dest, const Texture* src, float x_shift, float y_shift)
  {
    int elements = dest->width * dest->height;
    _launchThreads(elements, _shiftTexels, dest, src, x_shift, y_shift);
  }
}
