#ifndef RAND_H_INCLUDED
#define RAND_H_INCLUDED

#include <string>
#include <random>
#include <cstdint>
#include <vector>
#include <utility>

namespace Rand
{
  using DeviceType = std::mt19937;
  
  void seedDevice(int, int);
  void seedDevice(int, std::string);
  
  DeviceType* getDevice(int);
  uint32_t getMax(int);
  std::vector<uint32_t> producei(int, int);
  std::vector<std::pair<uint32_t, uint32_t>> produceip(int, int);
  std::vector<double>
    producef(int dev, int num, double min, double max);
  std::vector<std::pair<double, double>>
    producefp(int dev, int num, double min, double max);
}

#endif
