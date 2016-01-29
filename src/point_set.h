#ifndef POINT_SET_H_INCLUDED
#define POINT_SET_H_INCLUDED

#include <vector>
#include <utility>

namespace PointSet
{
  std::vector<std::pair<double, double>> make(int, int);
  
  //this function does not preserve order
  void spread(std::vector<std::pair<double, double>>&, int, double = 1.);
}

#endif
