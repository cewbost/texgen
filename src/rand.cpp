
#include <random>
#include <string>
#include <utility>
#include <unordered_map>

#include "rand.h"

#include "common.h"

constexpr Rand::DeviceType::result_type _div =
  Rand::DeviceType::max() - Rand::DeviceType::min();

namespace
{
  std::unordered_map<int, Rand::DeviceType> _devices;
}

namespace Rand
{
  void seedDevice(int device, int seed)
  {
    auto it = _devices.find(device);
    if(it == _devices.end())
      _devices.emplace(device, DeviceType(seed));
    else _devices[device].seed(seed);
  }
  
  void seedDevice(int device, std::string seed)
  {
    std::seed_seq seq(seed.begin(), seed.end());
    auto it = _devices.find(device);
    if(it == _devices.end())
      _devices.emplace(device, DeviceType(seq));
    else _devices[device].seed(seq);
  }
  
  DeviceType* getDevice(int device)
  {
    auto it = _devices.find(device);
    if(it == _devices.end())
      return nullptr;
    else return &(it->second);
  }
  
  uint32_t getMax(int device)
  {
    return DeviceType::max() - DeviceType::min();
  }
  
  std::vector<uint32_t> producei(int device, int num)
  {
    std::vector<uint32_t> res;
    auto it = _devices.find(device);
    if(it != _devices.end())
    {
      auto& dev = it->second;
      res.reserve(num);
      for(int i = 0; i < num; ++i)
        res.push_back(dev() - DeviceType::min());
    }
    else throw tgException("invalid random device");
    
    return res;
  }
  
  std::vector<std::pair<uint32_t, uint32_t>> prodiceip(int device, int num)
  {
    std::vector<std::pair<uint32_t, uint32_t>> res;
    auto it = _devices.find(device);
    if(it != _devices.end())
    {
      auto& dev = it->second;
      res.reserve(num);
      for(int i = 0; i < num; ++i)
      {
        uint32_t a, b;
        a = dev() - DeviceType::min();
        b = dev() - DeviceType::min();
        res.push_back(std::make_pair(a, b));
      }
    }
    else throw tgException("invalid random device");
    
    return res;
  }
  
  std::vector<double> producef(int device, int num, double min, double max)
  {
    std::vector<double> res;
    
    auto it = _devices.find(device);
    if(it != _devices.end())
    {
      auto& dev = it->second;
      res.reserve(num);
      for(int i = 0; i < num; ++i)
      {
        double produced = (double)(dev() - DeviceType::min()) / _div;
        produced = produced < 0.? 0. : (produced > 1.? 1. : produced);
        res.push_back(produced);
      }
    }
    else throw tgException("invalid random device");
    
    return res;
  }
  
  std::vector<std::pair<double, double>> producefp(
    int device, int num, double min, double max)
  {
    std::vector<std::pair<double, double>> res;
    
    auto it = _devices.find(device);
    if(it != _devices.end())
    {
      auto& dev = it->second;
      res.reserve(num);
      for(int i = 0; i < num; ++i)
      {
        double a, b;
        a = (double)(dev() - DeviceType::min()) / (double)_div;
        b = (double)(dev() - DeviceType::min()) / (double)_div;
        res.push_back(std::make_pair(a, b));
      }
    }
    else throw tgException("invalid random device");
    
    return res;
  }
}