#ifndef BLURFILTER_H_INCLUDED
#define BLURFILTER_H_INCLUDED

#include <cassert>

#include <memory>

class BlurFilter2d
{
  std::unique_ptr<unsigned[]> _filter;
  
  BlurFilter2d(): _filter(nullptr), width(0), height(0){}
  
  void _adjustWeight()
  {
    weight = 0;
    for(unsigned i = 0; i < width * height; ++i)
      weight += _filter[i];
  }
  
public:

  unsigned width, height, weight;

  unsigned& get(unsigned x, unsigned y)
  {
    return _filter[x + y * width];
  }
  double getWeighted(unsigned x, unsigned y)
  {
    return (double)_filter[x + y * width] / weight;
  }
  
  friend class BlurFilter1d;
};

class BlurFilter1d
{
  std::unique_ptr<unsigned[]> _filter;
  
  void _blurOut()
  {
    unsigned new_size = size + 2;
    std::unique_ptr<unsigned[]> new_filter(new unsigned[new_size]);
    for(unsigned i = 0; i < new_size; ++i)
      new_filter[i] = 0;
    for(unsigned i = 0; i < size; ++i)
    {
      new_filter[i    ] += _filter[i];
      new_filter[i + 1] += _filter[i];
      new_filter[i + 2] += _filter[i];
    }
    
    size = new_size;
    _filter = std::move(new_filter);
  }
  
  void _adjustWeight()
  {
    weight = 0;
    for(unsigned i = 0; i < size; ++i)
      weight += _filter[i];
  }
  
public:

  unsigned size, weight;

  BlurFilter1d& make(unsigned level)
  {
    assert(level >= 0);
    _filter.reset(new unsigned[1]);
    _filter[0] = 1;
    size = 1;
    for(unsigned i = 0; i < level; ++i)
      _blurOut();
    _adjustWeight();
      
    return *this;
  }
  
  BlurFilter2d square()
  {
    BlurFilter2d filt;
    
    filt.width = size;
    filt.height = size;
    filt._filter.reset(new unsigned[filt.width * filt.height]);
    
    for(unsigned y = 0; y < filt.height; ++y)
    for(unsigned x = 0; x < filt.width; ++x)
      filt.get(x, y) = _filter[x] * _filter[y];
      
    filt._adjustWeight();
    
    return filt;
  }
  
  BlurFilter2d operator* (const BlurFilter1d& other)
  {
    BlurFilter2d filt;
    
    filt.width = size;
    filt.height = other.size;
    filt._filter.reset(new unsigned[filt.width * filt.height]);
    
    for(unsigned y = 0; y < filt.height; ++y)
    for(unsigned x = 0; x < filt.width; ++x)
      filt.get(x, y) = _filter[x] * other._filter[y];
    
    filt._adjustWeight();
    
    return filt;
  }
  
  unsigned& get(unsigned idx)
  {
    return _filter[idx];
  }
  double getWeighted(unsigned idx)
  {
    return (double)_filter[idx] / weight;
  }

  BlurFilter1d(): _filter(nullptr), size(0){}
  BlurFilter1d(unsigned level)
  {
    make(level);
  }
};

#endif
