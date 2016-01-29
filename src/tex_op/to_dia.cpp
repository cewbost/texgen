
#include "../tex_op.h"

#include "to_common.h"

#include "../point_set.h"
#include "../worley.h"
#include "../delaunay.h"

namespace
{
  float _distFunc(float x, float y)
  {
    return std::sqrt(x * x + y * y);
  }

  using Map = Worley<float, _distFunc>;
  
  void _placeLine(Map* map, float x1, float y1, float x2, float y2)
  {
    int divs = floor(fabs(x1 - x2));
    int divs2 = floor(fabs(y1 - y2));
    divs = (divs > divs2? divs : divs2) + 1;
    if(divs == 1) return;
    
    double inc_x, inc_y;
    inc_x = (x2 - x1) / divs;
    inc_y = (y2 - y1) / divs;
    
    for(int i = 1; i < divs; ++i)
      map->insertPoint(x1 + inc_x * i, y1 + inc_y * i);
  }
  
  template<int mask>
  class _writeSDFMap
  {
  public:
    static void func(Texture* dest, const Map& map, float range, int from, int to)
    {
      int x_offset, y_offset;
      map.getResolution(&x_offset, &y_offset);
      x_offset /= 4;
      y_offset /= 4;
      for(int i = from; i < to; ++i)
      {
        int x = i % dest->width;
        int y = i / dest->width;
        float read = map.get(x + x_offset, y + y_offset) / range;
        if(read > 1.) read = 1.;
        if(mask & 1) dest->get(x, y)[0] = read;
        if(mask & 2) dest->get(x, y)[1] = read;
        if(mask & 4) dest->get(x, y)[2] = read;
        if(mask & 8) dest->get(x, y)[3] = read;
      }
    }
  };
}

namespace TexOp
{
  void makeDelaunay(Texture* tex,
    std::vector<std::pair<double, double>>& point_set,
    float range,
    std::bitset<4> mask)
  {
    //create the map
    Map map(tex->width * 2, tex->height * 2);
    std::vector<float> verts;
    verts.reserve(point_set.size() * 8);
    
    for(auto& point: point_set)
    {
      double x = point.first * tex->width;
      double y = point.second * tex->height;
      map.insertPoint(x, y);
      map.insertPoint(x + tex->width, y);
      map.insertPoint(x, y + tex->height);
      map.insertPoint(x + tex->width, y + tex->height);
      
      verts.push_back(x);
      verts.push_back(y);
      verts.push_back(x + tex->width);
      verts.push_back(y);
      verts.push_back(x);
      verts.push_back(y + tex->height);
      verts.push_back(x + tex->width);
      verts.push_back(y + tex->height);
    }
    
    Delaunay<float> del(verts);
    
    auto edges = del.triangulate().edges();
    
    for(unsigned i = 0; i < edges.size(); i += 2)
      _placeLine(&map, 
        verts[edges[i] * 2],
        verts[edges[i] * 2 + 1],
        verts[edges[i + 1] * 2],
        verts[edges[i + 1] * 2 + 1]);
    
    map.generateDistances();
    
    if(range > 0)
      range *= tex->width < tex->height? tex->width : tex->height;
    else range = -range;
    
    _launchThreadsMasked<_writeSDFMap>(mask.to_ulong(), tex, std::move(map), range);
  }
  
  void makeVoronoi(Texture* tex,
    std::vector<std::pair<double, double>>& point_set,
    float range,
    std::bitset<4> mask)
  {
    //create the map
    Map map(tex->width * 2, tex->height * 2);
    std::vector<float> verts;
    verts.reserve(point_set.size() * 8);
    
    for(auto& point: point_set)
    {
      double x = point.first * tex->width;
      double y = point.second * tex->height;
      
      verts.push_back(x);
      verts.push_back(y);
      verts.push_back(x + tex->width);
      verts.push_back(y);
      verts.push_back(x);
      verts.push_back(y + tex->height);
      verts.push_back(x + tex->width);
      verts.push_back(y + tex->height);
    }
    
    Delaunay<float> del(verts);
    
    std::vector<int> edges;
    std::vector<float> points;
    del.triangulate().dual(&points, &edges);
    
    for(unsigned i = 0; i < edges.size(); i += 2)
      _placeLine(&map,
        points[edges[i] * 2],
        points[edges[i] * 2 + 1],
        points[edges[i + 1] * 2],
        points[edges[i + 1] * 2 + 1]);
    for(unsigned i = 0; i < points.size(); i += 2)
      map.insertPoint(points[i], points[i + 1]);
    
    map.generateDistances();
    
    if(range > 0)
      range *= tex->width < tex->height? tex->width : tex->height;
    else range = -range;
    
    _launchThreadsMasked<_writeSDFMap>(mask.to_ulong(), tex, std::cref(map), range);
  }
}