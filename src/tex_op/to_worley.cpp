
#include <cmath>
#include <cstdlib>

#include "to_common.h"
#include "../tex_op.h"

#include "../worley.h"

namespace
{
  float _distFunc(float x, float y)
  {
    return std::sqrt(x * x + y * y);
  }
  
  using Map = Worley<float, _distFunc, 4>;
  
  template<int mask, int oct>
  void _writeSDFMapWorker(Texture* dest, Map& map, float range, int from, int to)
  {
    int x_offset, y_offset;
    map.getResolution(&x_offset, &y_offset);
    x_offset /= 4;
    y_offset /= 4;
    for(int y = from; y < to; ++y)
    {
      for(int x = 0; x < (int)dest->width; ++x)
      {
        float read = map.get<oct>(x + x_offset, y + y_offset) / range;
        if(read > 1.) read = 1.;
        if(mask & 1) dest->get(x, y)[0] = read;
        if(mask & 2) dest->get(x, y)[1] = read;
        if(mask & 4) dest->get(x, y)[2] = read;
        if(mask & 8) dest->get(x, y)[3] = read;
      }
    }
  }
  
  template<int oct>
  void _writeSDFMapHelper(
    int mask, Texture* dest, Map& map, float range, int from, int to)
  {
    switch(mask)
    {
      case 0x1: _writeSDFMapWorker<0x1, oct>(dest, map, range, from, to); break;
      case 0x2: _writeSDFMapWorker<0x2, oct>(dest, map, range, from, to); break;
      case 0x3: _writeSDFMapWorker<0x3, oct>(dest, map, range, from, to); break;
      case 0x4: _writeSDFMapWorker<0x4, oct>(dest, map, range, from, to); break;
      case 0x5: _writeSDFMapWorker<0x5, oct>(dest, map, range, from, to); break;
      case 0x6: _writeSDFMapWorker<0x6, oct>(dest, map, range, from, to); break;
      case 0x7: _writeSDFMapWorker<0x7, oct>(dest, map, range, from, to); break;
      case 0x8: _writeSDFMapWorker<0x8, oct>(dest, map, range, from, to); break;
      case 0x9: _writeSDFMapWorker<0x9, oct>(dest, map, range, from, to); break;
      case 0xa: _writeSDFMapWorker<0xa, oct>(dest, map, range, from, to); break;
      case 0xb: _writeSDFMapWorker<0xb, oct>(dest, map, range, from, to); break;
      case 0xc: _writeSDFMapWorker<0xc, oct>(dest, map, range, from, to); break;
      case 0xd: _writeSDFMapWorker<0xd, oct>(dest, map, range, from, to); break;
      case 0xe: _writeSDFMapWorker<0xe, oct>(dest, map, range, from, to); break;
      case 0xf: _writeSDFMapWorker<0xf, oct>(dest, map, range, from, to); break;
    }
  }
  
  void _writeSDFMap(Texture* dest, Map& map, std::array<int, 4>& mask,
    float range, int from, int to)
  {
    //channel masks per octave
    int oct_masks[4] = {0, 0, 0, 0};
    
    for(int i = 0; i < 4; ++i) if(mask[i] != 0)
      oct_masks[mask[i] - 1] |= (1 << i);
    
    if(oct_masks[0] != 0)
      _writeSDFMapHelper<0>(oct_masks[0], dest, map, range, from, to);
    if(oct_masks[1] != 0)
      _writeSDFMapHelper<1>(oct_masks[1], dest, map, range, from, to);
    if(oct_masks[2] != 0)
      _writeSDFMapHelper<2>(oct_masks[2], dest, map, range, from, to);
    if(oct_masks[3] != 0)
      _writeSDFMapHelper<3>(oct_masks[3], dest, map, range, from, to);
  }
}

namespace TexOp
{
  void makeCellNoise(Texture* tex,
    std::vector<std::pair<double, double>>& point_set,
    float range,
    std::array<int, 4>& mask)
  {
    //create the map
    Map map(tex->width * 2, tex->height * 2);
    
    for(auto& point: point_set)
    {
      double x = point.first * tex->width;
      double y = point.second * tex->height;
      map.insertPoint(x, y);
      map.insertPoint(x + tex->width, y);
      map.insertPoint(x, y + tex->height);
      map.insertPoint(x + tex->width, y + tex->height);
    }
    
    map.generateDistances();
    
    if(range > 0)
      range *= tex->width < tex->height? tex->width : tex->height;
    else range = -range;
    
    //writing operations
    _launchThreads(tex->height, _writeSDFMap, tex, std::ref(map), std::ref(mask), range);
  }
}