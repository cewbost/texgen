#ifndef TEXTURE_H_INCLUDED
#define TEXTURE_H_INCLUDED

#include <new>
#include <bitset>

#include <eigen3/Eigen/Dense>

#include <horde3d.h>

struct alignas(16) TexHeader
{
  unsigned width;
  unsigned height;
  
  std::pair<int, int> getDimensions() const
  {
    return std::make_pair(width, height);
  }
  
  typedef Eigen::Array4f ElementType;
};

class Texture: public TexHeader
{
  /*
    note:
      objects of this class must never be allocated with new/delete
      use makeTexture or deleteTexture
  */
  static void* _allocate(int, int);
  static void _deallocate(void*, int, int);

public:
  Eigen::Array4f* data()
  {return (Eigen::Array4f*)this + 1;}
  const Eigen::Array4f* data() const
  {return (Eigen::Array4f*)this + 1;}
  Eigen::Array4f& operator[](int idx)
  {return data()[idx];}
  const Eigen::Array4f& operator[](int idx) const
  {return data()[idx];}
  Eigen::Array4f& operator()(int x, int y)
  {return data()[x + y * width];}
  const Eigen::Array4f& operator()(int x, int y) const
  {return data()[x + y * width];}
  Eigen::Array4f& get(int idx)
  {return data()[idx];}
  const Eigen::Array4f& get(int idx) const
  {return data()[idx];}
  Eigen::Array4f& get(int x, int y)
  {return data()[x + y * width];}
  const Eigen::Array4f& get(int x, int y) const
  {return data()[x + y * width];}
  Eigen::Array4f& getBounded(unsigned x, unsigned y)
  {return data()[(x % width) + (y % height) * width];}
  const Eigen::Array4f& getBounded(unsigned x, unsigned y) const
  {return data()[(x % width) + (y % height) * width];}
  
  template<int P>
  Eigen::Array4f sample(unsigned x, unsigned y) const
  {
    //this function relies on unsigned roll-over for correct results
    //this function has not been tested due to being dead code
    const unsigned frag_x = x / P;
    const unsigned frag_y = y / P;
    const double x_ratio = (double)(x % P) / P;
    const double y_ratio = (double)(y % P) / P;
    
    Eigen::Array4f samples[4];
    samples[0] = this->get(frag_x % width, frag_y % height);
    samples[1] = this->get((frag_x + 1) % width, frag_y % height);
    samples[2] = this->get(frag_x % width, (frag_y + 1) % height);
    samples[3] = this->get((frag_x + 1) % width, (frag_y + 1) % height);
    
    samples[0] = samples[0] * (1. - x_ratio) + samples[1] * x_ratio;
    samples[2] = samples[2] * (1. - x_ratio) + samples[3] * x_ratio;
    samples[0] = samples[0] * (1. - y_ratio) + samples[2] * y_ratio;
    
    return samples[0];
  }
  
  template<int P>
  Eigen::Array4f sampleTrivial(unsigned x, unsigned y) const
  {
    const unsigned frag_x = x / P;
    const unsigned frag_y = y / P;
    
    double x_ratio, y_ratio;
    
    Eigen::Array4f samples[4];
    samples[0] = this->get(frag_x, frag_y);
    
    switch((x % P == 0? 0 : 1) + (y % P == 0? 0 : 2))
    {
    case 1:
      x_ratio = (double)(x % P) / P;
      samples[1] = this->get((frag_x + 1) % width, frag_y);
      samples[0] = samples[0] * (1. - x_ratio) + samples[1] * x_ratio;
      break;
    case 2:
      y_ratio = (double)(y % P) / P;
      samples[1] = this->get(frag_x, (frag_y + 1) % height);
      samples[0] = samples[0] * (1. - y_ratio) + samples[1] * y_ratio;
      break;
    case 3:
      x_ratio = (double)(x % P) / P;
      y_ratio = (double)(y % P) / P;
      samples[1] = this->get((frag_x + 1) % width, frag_y);
      samples[2] = this->get(frag_x, (frag_y + 1) % height);
      samples[3] = this->get((frag_x + 1) % width, (frag_y + 1) % height);
      samples[0] = samples[0] * (1. - x_ratio) + samples[1] * x_ratio;
      samples[2] = samples[2] * (1. - x_ratio) + samples[3] * x_ratio;
      samples[0] = samples[0] * (1. - y_ratio) + samples[2] * y_ratio;
      break;
    default:
      break;
    }
    
    return samples[0];
  }
  
  Eigen::Array4f sampleBoxed(unsigned x, unsigned y, int level) const
  {
    const unsigned div = 1 << level;
    const unsigned w = this->width / div;
    const unsigned h = this->height / div;
    
    Eigen::Array4f samples[4];
    samples[0] = this->get(x / div, y / div);
    samples[1] = this->get(((x / div) + 1) % w, y / div);
    samples[2] = this->get(x / div, ((y / div) + 1) % h);
    samples[3] = this->get(((x / div) + 1) % w, ((y / div) + 1) % h);
    
    x %= div;
    y %= div;
    
    samples[0] *= (div - x) * (div - y);
    samples[1] *= x * (div - y);
    samples[2] *= (div - x) * y;
    samples[3] *= x * y;
    
    samples[0] += samples[1];
    samples[0] += samples[2];
    samples[0] += samples[3];
    
    samples[0] /= div * div;
    
    return samples[0];
  }

  
  void createResource();
  
  Texture(unsigned w, unsigned h): TexHeader{w, h}
  {
    new(data()) Eigen::Array4f[w * h];
  }
  Texture(unsigned, unsigned, H3DRes);
  Texture(Texture&);
  Texture(const Texture&) = delete;
  Texture(Texture&&) = delete;
  
  ~Texture(){}
  
  template<class... T>
  friend Texture* makeTexture(unsigned, unsigned, T...);
  template<class... T>
  friend Texture* makeTexture(Texture&, T...);
  friend void deleteTexture(Texture*);
};

template<class... T>
Texture* makeTexture(unsigned w, unsigned h, T... t)
{
  return new(Texture::_allocate(w, h)) Texture(w, h, t...);
}
template<class... T>
Texture* makeTexture(Texture& other, T... t)
{
  unsigned w, h;
  w = other.width;
  h = other.height;
  return new(Texture::_allocate(w, h)) Texture(other, t...);
}

#endif
