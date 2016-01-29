#include <algorithm>
#include <vector>
#include <memory>

#include <cassert>

#include "rand.h"

#include "point_set.h"

#define __range(x) x.begin(), x.end()

namespace PointSet
{
  std::vector<std::pair<double, double>> make(int num, int rand)
  {
    std::vector<std::pair<double, double>> set;
    set = Rand::producefp(rand, num, 0., 1.);
    
    bool retry;
    do
    {
      retry = false;
      std::sort(__range(set),
        [](const std::pair<double, double>& p1, const std::pair<double, double>& p2)
      {
        if(p1.first == p2.first)
          return p1.second < p2.second;
        else return p1.first < p2.first;
      });
      
      decltype(set.begin()) it1, it2;
      for(it1 = it2 = set.begin(), ++it2; it2 != set.end(); ++it2)
        if(it1->first == it2->first && it1->second == it2->second)
          it1->first = NAN;
      std::remove_if(__range(set), [](std::pair<double, double>& p)
      {return std::isnan(p.first);});
      
      if((signed)set.size() < num)
      {
        std::vector<std::pair<double, double>> new_set =
          Rand::producefp(rand, num - set.size(), 0., 1.);
        set.insert(set.end(), __range(new_set));
        retry = true;
      }
    }while(retry);
    
    return set;
  }
  
  void spread(std::vector<std::pair<double, double>>& ps, int its, double mag)
  {
    double mag2 = 1. / std::sqrt(ps.size());
    
    assert(its > 0);
    
    std::vector<std::pair<double, double>> ps2;
    ps2 = ps;
    
    for(int it = 0; it < its; ++it)
    {
      for(unsigned i = 0; i < ps.size(); ++i)
      for(unsigned j = i + 1; j < ps.size(); ++j)
      {
        //calculate diff
        std::pair<double, double> diff;
        diff.first = ps[i].first - ps[j].first;
        diff.second = ps[i].second - ps[j].second;
        if(diff.first > .5) diff.first -= 1.;
        else if(diff.first < -.5) diff.first += 1.;
        if(diff.second > .5) diff.second -= 1.;
        else if(diff.second < -.5) diff.second += 1.;
        
        double diff_len = std::sqrt(
          diff.first * diff.first + diff.second * diff.second);
        
        diff.first /= diff_len;
        diff.second /= diff_len;
        
        diff_len = (mag2 - diff_len) * mag;
        if(diff_len < 0.) continue;
        
        diff.first *= diff_len;
        diff.second *= diff_len;
        ps2[i].first += diff.first;
        ps2[i].second += diff.second;
        ps2[j].first -= diff.first;
        ps2[j].second -= diff.second;
      }
      for(auto& point: ps2)
      {
        if(point.first < 0.)
          point.first = ((int)(-point.first) + 1) + point.first;
        else if(point.first > 1.)
          point.first = point.first - (int)point.first;
        if(point.second < 0.)
          point.second = ((int)(-point.second) + 1) + point.second;
        else if(point.second > 1.)
          point.second = point.second - (int)point.second;
      }
      
      ps = ps2;
    }
  }
}
