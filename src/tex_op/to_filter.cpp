
#include "../tex_op.h"

#include "to_common.h"

constexpr double PI = 3.141592653589793;
constexpr double PI_halved = 1.5707963267948966;

class FilterFunctor
{
  float _constant;
  float _linear;
  float _squared;
  float _rev_squared;
  float _smoothstep;
public:
  FilterFunctor(float co, float li, float sq, float rs, float sm):
    _constant(co), _linear(li), _squared(sq), _rev_squared(rs), _smoothstep(sm){}
  float operator()(float in) const
  {
    float f = _constant;
    f += _linear * in;
    f += _squared * (in * in);
    f += _rev_squared * (1. - (1. - in) * (1. - in));
    f += _smoothstep * ((sin((in - .5) * PI) + 1.) / 2.);
    return f < 0.? 0.0 : (f > 1.? 1. : f);
  }
};

class ClampFunctor
{
  float _min;
  float _max;
public:
  ClampFunctor(float min, float max): _min(min), _max(max){}
  float operator()(float in) const
  {
    return in < _min? _min : (in > _max? _max : in);
  }
};

template<int low_range, int hi_range>
class LinearFunctor
{
  static_assert(low_range <= 1 && low_range >= -1,
    "template arguments in LinearFunctor must be in range [-1, 1] (first)");
  static_assert(hi_range <= 1 && hi_range >= -1,
    "template arguments in LinearFunctor must be in range [-1, 1] (second)");
  float _from1, _rise1, _from2, _rise2;
public:
  LinearFunctor(float f1, float t1, float f2, float t2):
    _from1(f1), _rise1(t1 - f1), _from2(f2), _rise2(t2 - f2){}
  float operator()(float in) const
  {
    if(low_range == -1 && hi_range == -1)
      return 0.;
    if(low_range == 1 && hi_range == 1)
      return 1.;
    
    in -= _from1;
    if(in < 0.)
    {
      if(low_range == -1)
        return 0.;
      else if(low_range == 1)
        return 1.;
      else return _from2;
    }
    in /= _rise1;
    if(in > 1.)
    {
      if(hi_range == -1)
        return 0.;
      else if(hi_range == 1)
        return 1.;
      else return _from2 + _rise2;
    }
    
    in = _from2 + _rise2 * in;
    if(low_range == -1 || hi_range == -1)
      in = in < 0.? 0. : in;
    if(low_range == 1 || hi_range == 1)
      in = in > 1.? 1. : in;
    
    return in;
  }
};

template<bool rev>
class StencilFunctor
{
  float _cutof;
public:
  StencilFunctor(float c): _cutof(c){}
  float operator()(float in) const
  {
    if(rev) return in > _cutof? 0.: 1.;
    else return in < _cutof? 1.: 0.;
    return 0.0;
  }
};

class DownsampleFunctor
{
  unsigned _levels;
public:
  DownsampleFunctor(unsigned l): _levels(l){}
  float operator()(float in) const
  {
    return (unsigned)(in * _levels) * (1. / (_levels - 1));
  }
};

template<class T>
class _filter
{
public:
  template<int mask>
  class NextC
  {
  public:
    static void func(Texture* tex, const T& functor, int from, int to)
    {
      for(int i = from; i < to; ++i)
      {
        if(mask & 0x1) tex->get(i)[0] = functor(tex->get(i)[0]);
        if(mask & 0x2) tex->get(i)[1] = functor(tex->get(i)[1]);
        if(mask & 0x4) tex->get(i)[2] = functor(tex->get(i)[2]);
        if(mask & 0x8) tex->get(i)[3] = functor(tex->get(i)[3]);
      }
    }
  };
};

namespace
{
  template<class Functor>
  inline void _linearFilterHelper(
    Texture* tex, float f1, float t1, float f2, float t2, std::bitset<4> mask)
  {
    Functor lin(f1, t1, f2, t2);
    _launchThreadsMasked<_filter<Functor>::template NextC>
      (mask.to_ulong(), tex, std::cref(lin));
  }
}

namespace TexOp
{
  void clamp(Texture* tex, float min, float max, std::bitset<4> mask)
  {
    ClampFunctor clamp(min, max);
    _launchThreadsMasked<_filter<ClampFunctor>::NextC>
      (mask.to_ulong(), tex, std::cref(clamp));
  }
  
  void filter(
    Texture* tex,
    float co,
    float li,
    float sq,
    float rs,
    float sm,
    std::bitset<4> mask)
  {
    FilterFunctor filter(co, li, sq, rs, sm);
    _launchThreadsMasked<_filter<FilterFunctor>::NextC>
      (mask.to_ulong(), tex, std::cref(filter));
  }
  
  void linearFilter(
    Texture* tex, float f1, float t1, float f2, float t2, std::bitset<4> mask)
  {
    int lo, hi;
    lo = f2 < 0.? 1 : (f2 > 1.? 2 : 0);
    hi = t2 < 0.? 1 : (t2 > 1.? 2 : 0);
    
    switch(lo + (hi << 2))
    {
    case 0x0:
      _linearFilterHelper<LinearFunctor<0, 0>>(tex, f1, t1, f2, t2, mask);
      break;
    case 0x1:
      _linearFilterHelper<LinearFunctor<-1, 0>>(tex, f1, t1, f2, t2, mask);
      break;
    case 0x2:
      _linearFilterHelper<LinearFunctor<1, 0>>(tex, f1, t1, f2, t2, mask);
      break;
    case 0x4:
      _linearFilterHelper<LinearFunctor<0, -1>>(tex, f1, t1, f2, t2, mask);
      break;
    case 0x5:
      _linearFilterHelper<LinearFunctor<-1, -1>>(tex, f1, t1, f2, t2, mask);
      break;
    case 0x6:
      _linearFilterHelper<LinearFunctor<1, -1>>(tex, f1, t1, f2, t2, mask);
      break;
    case 0x8:
      _linearFilterHelper<LinearFunctor<0, 1>>(tex, f1, t1, f2, t2, mask);
      break;
    case 0x9:
      _linearFilterHelper<LinearFunctor<-1, 1>>(tex, f1, t1, f2, t2, mask);
      break;
    case 0xa:
      _linearFilterHelper<LinearFunctor<1, 1>>(tex, f1, t1, f2, t2, mask);
      break;
    }
  }
  
  void stencilFilter(Texture* tex, float cutof, bool rev, std::bitset<4> mask)
  {
    if(rev)
    {
      StencilFunctor<true> fil(cutof);
      _launchThreadsMasked<_filter<StencilFunctor<true>>::NextC>
        (mask.to_ulong(), tex, std::cref(fil));
    }
    else
    {
      StencilFunctor<false> fil(cutof);
      _launchThreadsMasked<_filter<StencilFunctor<false>>::NextC>
        (mask.to_ulong(), tex, std::cref(fil));
    }
  }
  
  void downsampleFilter(Texture* tex, unsigned levels, std::bitset<4> mask)
  {
    DownsampleFunctor fil(levels);
    _launchThreadsMasked<_filter<DownsampleFunctor>::NextC>
      (mask.to_ulong(), tex, std::cref(fil));
  }
}
