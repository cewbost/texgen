#include "common.h"

#include "h3d.h"
#include "rand.h"

#include "texture_manager.h"

#include "tex_op.h"

#include "blurfilter.h"

#include "pool_allocator.h"

#define __range(x) x.begin(),x.end()

constexpr int cexpr_objPerPool(int s)
{
  return 256 >> ((s + 1) / 2);
}

template<int I> using Pool =
  PoolAllocator<sizeof(Eigen::Array4f) * ((256 << I) + 1),
  cexpr_objPerPool(I), 16>;

void deleteTexture(Texture* tex)
{
  int w = tex->width;
  int h = tex->height;
  tex->~Texture();
  Texture::_deallocate(tex, w, h);
}

//pools:
namespace
{
  //dispatcher
  Pool<16>  _pool16;
  Pool<15>  _pool15;
  Pool<14>  _pool14;
  Pool<13>  _pool13;
  Pool<12>  _pool12;
  Pool<11>  _pool11;
  Pool<10>  _pool10;
  Pool<9>   _pool9;
  Pool<8>   _pool8;
  Pool<7>   _pool7;
  Pool<6>   _pool6;
  Pool<5>   _pool5;
  Pool<4>   _pool4;
  Pool<3>   _pool3;
  Pool<2>   _pool2;
  Pool<1>   _pool1;
  Pool<0>   _pool0;
  
  void* _allocate(int magnitude)
  {
    void* ptr;
    switch(magnitude)
    {
    case 0:   ptr = _pool0.allocate();  break;
    case 1:   ptr = _pool1.allocate();  break;
    case 2:   ptr = _pool2.allocate();  break;
    case 3:   ptr = _pool3.allocate();  break;
    case 4:   ptr = _pool4.allocate();  break;
    case 5:   ptr = _pool5.allocate();  break;
    case 6:   ptr = _pool6.allocate();  break;
    case 7:   ptr = _pool7.allocate();  break;
    case 8:   ptr = _pool8.allocate();  break;
    case 9:   ptr = _pool9.allocate();  break;
    case 10:  ptr = _pool10.allocate(); break;
    case 11:  ptr = _pool11.allocate(); break;
    case 12:  ptr = _pool12.allocate(); break;
    case 13:  ptr = _pool13.allocate(); break;
    case 14:  ptr = _pool14.allocate(); break;
    case 15:  ptr = _pool15.allocate(); break;
    case 16:  ptr = _pool16.allocate(); break;
    default:
      //*(int*)nullptr = 6;
      throw tgException("invalid texture size in allocate function");
    }
    return ptr;
  }
  void _deallocate(int magnitude, void* ptr)
  {
    switch(magnitude)
    {
    case 0:   _pool0.deallocate(ptr);   break;
    case 1:   _pool1.deallocate(ptr);   break;
    case 2:   _pool2.deallocate(ptr);   break;
    case 3:   _pool3.deallocate(ptr);   break;
    case 4:   _pool4.deallocate(ptr);   break;
    case 5:   _pool5.deallocate(ptr);   break;
    case 6:   _pool6.deallocate(ptr);   break;
    case 7:   _pool7.deallocate(ptr);   break;
    case 8:   _pool8.deallocate(ptr);   break;
    case 9:   _pool9.deallocate(ptr);   break;
    case 10:  _pool10.deallocate(ptr);  break;
    case 11:  _pool11.deallocate(ptr);  break;
    case 12:  _pool12.deallocate(ptr);  break;
    case 13:  _pool13.deallocate(ptr);  break;
    case 14:  _pool14.deallocate(ptr);  break;
    case 15:  _pool15.deallocate(ptr);  break;
    case 16:  _pool16.deallocate(ptr);  break;
    default:
      //*(int*)nullptr = 6;
      throw tgException("invalid texture size in deallocate function");
    }
  }
  
  using TexResPair = std::pair<Texture*, H3DRes>;
  TexResPair* _textures = nullptr;
  int _num_textures = 0;
  int _texture_stepper = 0;
  int _texture_capacity = 0;
  
  void _createResource(int, const char*);
  
  inline const char* _newTexResName()
  {
    static int num_tex_res = 0;
    static char buffer[16];
    snprintf(buffer, 16, "tex%i", num_tex_res);
    ++num_tex_res;
    return buffer;
  }
  
  inline int _findEmptyTexSlot()
  {
    if(_num_textures == _texture_capacity)
    {
      TexResPair* more_textures = new TexResPair[_texture_capacity << 1];
      memcpy(more_textures, _textures, sizeof(TexResPair) * _texture_capacity);
      delete[] _textures;
      _textures = more_textures;
      _texture_capacity <<= 1;
    }
    while(_textures[_texture_stepper].first != nullptr)
      _texture_stepper = (_texture_stepper + 1) % _texture_capacity;
    return _texture_stepper;
  }
  inline int _findResourceIdx(H3DRes res)
  {
    for(int i = 0; i < _texture_capacity; ++i)
      if(_textures[i].second == res) return i;
    return -1;
  }
  
  inline void _validateTextureHandle(int idx)
  {
    if(idx < 0 || idx > _texture_capacity || _textures[idx].first == nullptr)
      throw tgException("invalid texture handle: %i", idx);
  }
  
  int _storeTexture(Texture* tex)
  {
    int idx = _findEmptyTexSlot();
    _textures[idx].first = tex;
    _textures[idx].second = 0;
    ++_num_textures;
    return idx;
  }
  void _replaceTexture(Texture* tex, int idx)
  {
    if(_textures[idx].first != nullptr)
      deleteTexture(_textures[idx].first);
    _textures[idx].first = tex;
    if(_textures[idx].second != 0)
    {
      const char* old_res_name = h3dGetResName(_textures[idx].second);
      h3dUnloadResource(_textures[idx].second);
      h3dRemoveResource(_textures[idx].second);
      _textures[idx].second = 0;
      const char* new_res_name = _newTexResName();
      _createResource(idx, new_res_name);
      H3D::replaceSamplerRes(old_res_name, new_res_name);
    }
  }
  void _deleteTexture(int idx)
  {
    _validateTextureHandle(idx);
    deleteTexture(_textures[idx].first);
    _textures[idx].first = nullptr;
    if(_textures[idx].second != 0)
    {
      h3dUnloadResource(_textures[idx].second);
      h3dRemoveResource(_textures[idx].second);
      _textures[idx].second = 0;
    }
    --_num_textures;
  }
  Texture* _stealTexture(int idx)
  {
    _validateTextureHandle(idx);
    Texture* tex = _textures[idx].first;
    _textures[idx].first = nullptr;
    return tex;
  }
  
  void _updateResource(int idx)
  {
    //note: this function provides no checking for float values outside of
    //range [0:1]
    char* stream = (char*)h3dMapResStream(
      _textures[idx].second, H3DTexRes::ImageElem, 0,
      H3DTexRes::ImgPixelStream, false, true);
    
    int size = _textures[idx].first->width * _textures[idx].first->height;
    
    auto data = _textures[idx].first->data();
    
    for(int i = 0; i < size; ++i)
    {
      stream[0] = (char)((*data)[0] * 255.5);
      stream[1] = (char)((*data)[1] * 255.5);
      stream[2] = (char)((*data)[2] * 255.5);
      stream[3] = (char)((*data)[3] * 255.5);
      stream += 4;
      ++data;
    }
    
    h3dUnmapResStream(_textures[idx].second);
  }
  
  inline void _updateResourceMaybe(int idx)
  {
    if(_textures[idx].second != 0)
      _updateResource(idx);
  }
  
  void _createResource(int idx, const char* res_name)
  {
    static int tex_num = 0;
    ++tex_num;
    if(_textures[idx].second != 0)
    {
      h3dUnloadResource(_textures[idx].second);
      h3dRemoveResource(_textures[idx].second);
    }
    H3DRes tex = h3dCreateTexture(res_name,
      _textures[idx].first->width, _textures[idx].first->height,
      H3DFormats::TEX_BGRA8, H3DResFlags::NoTexMipmaps);
    
    _textures[idx].second = tex;
    
    _updateResource(idx);
  }
  
  inline bool _areTexturesSameSize(int tex1, int tex2)
  {
    return
      _textures[tex1].first->width == _textures[tex2].first->width &&
      _textures[tex1].first->height == _textures[tex2].first->height;
  }
}

void* Texture::_allocate(int w, int h)
{
  int magnitude = 0;
  for(int i = 16; i < w; i <<= 1, ++magnitude);
  for(int i = 16; i < h; i <<= 1, ++magnitude);
  return ::_allocate(magnitude);
}

void Texture::_deallocate(void* ptr, int w, int h)
{
  int magnitude = 0;
  for(int i = 16; i < w; i <<= 1, ++magnitude);
  for(int i = 16; i < h; i <<= 1, ++magnitude);
  ::_deallocate(magnitude, ptr);
}

Texture::Texture(unsigned w, unsigned h, H3DRes res): TexHeader{w, h}
{
  new(data()) Eigen::Array4f[w * h];
  const uint8_t* stream = (const uint8_t*)h3dMapResStream(
    res, H3DTexRes::ImageElem, 0, H3DTexRes::ImgPixelStream, true, false);
  
  for(unsigned i = 0; i < w * h; ++i)
  {
    data()[i][0] = (float)stream[i * 4] / 255.5;
    data()[i][1] = (float)stream[i * 4 + 1] / 255.5;
    data()[i][2] = (float)stream[i * 4 + 2] / 255.5;
    data()[i][3] = (float)stream[i * 4 + 3] / 255.5;
  }
  
  h3dUnmapResStream(res);
}

Texture::Texture(Texture& other): TexHeader{other.width, other.height}
{
  TexOp::copyTexture(this, &other, 0xf);
}

namespace TextureManager
{
  //helper classes
  class LinearInterpTexture
  {
  public:
    Texture* handle;
    bool did_interp;
    LinearInterpTexture(int tex1, int tex2)
    {
      if(!_areTexturesSameSize(tex1, tex2))
      {
        did_interp = true;
        handle = makeTexture(*_textures[tex1].first);
        handle = TexOp::resizeTexture(handle,
          _textures[tex2].first->width,
          _textures[tex2].first->height);
      }
      else
      {
        did_interp = false;
        handle = _textures[tex1].first;
      }
    }
    ~LinearInterpTexture()
    {
      if(did_interp)
        deleteTexture(handle);
    }
    operator Texture*()
    {
      return handle;
    }
  };
  
  //functions

  void init()
  {
    _textures = new TexResPair[16];
    _texture_capacity = 16;
    for(int i = 0; i < _texture_capacity; ++i)
    {
      _textures[i].first = nullptr;
      _textures[i].second = 0;
    }
    
    TexOp::init();
  }
  
  void deinit()
  {
    delete[] _textures;
    TexOp::deinit();
  }

  int addTexture(H3DRes res)
  {
    int tex_w = h3dGetResParamI(res, H3DTexRes::ImageElem, 0, H3DTexRes::ImgWidthI);
    int tex_h = h3dGetResParamI(res, H3DTexRes::ImageElem, 0, H3DTexRes::ImgHeightI);
    
    int idx = _storeTexture(makeTexture(tex_w, tex_h, res));
    _textures[idx].second = res;
    return idx;
  }
  
  int addTexture(int w, int h)
  {
    return _storeTexture(makeTexture(w, h));
  }
  
  void destroyTexture(int tex)
  {
    _deleteTexture(tex);
  }
  
  void destroyTextures(std::vector<int>::iterator beg, std::vector<int>::iterator end)
  {
    while(beg != end)
    {
      _deleteTexture(*beg);
      ++beg;
    }
  }
  
  int cloneTexture(int tex)
  {
    _validateTextureHandle(tex);
    Texture* new_tex = makeTexture(*_textures[tex].first);
    return _storeTexture(new_tex);
  }
  
  void copyTexture(int dest, int src, std::bitset<4> mask)
  {
    _validateTextureHandle(dest);
    _validateTextureHandle(src);
    
    if(mask.all())
    {
      Texture* dest_t = _textures[dest].first;
      Texture* src_t = _textures[src].first;
      Texture* new_tex = makeTexture(*src_t);
      if(dest_t->width != src_t->width || dest_t->height != src_t->height)
        new_tex = TexOp::resizeTexture(new_tex, dest_t->width, dest_t->height);
      _replaceTexture(new_tex, dest);
    }
    else
    {
      LinearInterpTexture src_c(src, dest);
      TexOp::copyTexture(_textures[dest].first, src_c, mask);
      _updateResourceMaybe(dest);
    }
  }
  
  void swapTextures(int tex1, int tex2, std::bitset<4> mask)
  {
    _validateTextureHandle(tex1);
    _validateTextureHandle(tex2);
    
    if(mask.all())
    {
      auto temp = _textures[tex1].first;
      _textures[tex1].first = _textures[tex2].first;
      _textures[tex2].first = temp;
    }
    else
    {
      TexOp::swapChannels(_textures[tex1].first, _textures[tex2].first, mask);
    }
    _updateResourceMaybe(tex1);
    _updateResourceMaybe(tex2);
  }
  
  void resizeTexture(int tex, unsigned width, unsigned height)
  {
    _validateTextureHandle(tex);
    if(_textures[tex].first->width == width && _textures[tex].first->height == height)
      return;
    
    Texture* new_tex = TexOp::resizeTexture(_stealTexture(tex), width, height);
    _replaceTexture(new_tex, tex);
  }
  
  decltype(TexHeader().getDimensions()) getTextureDimensions(int tex)
  {
    _validateTextureHandle(tex);
    return _textures[tex].first->getDimensions();
  }
  
  void removeResource(H3DRes res)
  {
    int idx = _findResourceIdx(res);
    if(idx >= 0)
      _textures[idx].second = 0;
  }
  
  std::vector<int> listTextures()
  {
    std::vector<int> textures;
    for(int i = 0; i < _texture_capacity; ++i)
    if(_textures[i].first != nullptr)
      textures.push_back(i);
    return textures;
  }
  
  H3DRes getTexRes(int tex)
  {
    if(_texture_capacity <= tex || _textures[tex].first == nullptr)
      throw tgException("%i is not a valid texture id", tex);
    if(_textures[tex].second == 0)
    {
      _createResource(tex, _newTexResName());
    }
    return _textures[tex].second;
  }
  
  //raw access
  void writeRawTexture(int tex, const uint8_t* data)
  {
    _validateTextureHandle(tex);
    TexOp::writeRawTexture(_textures[tex].first, data);
    _updateResourceMaybe(tex);
  }
  void writeRawTexture(int tex, const float* data)
  {
    _validateTextureHandle(tex);
    TexOp::writeRawTexture(_textures[tex].first, data);
    _updateResourceMaybe(tex);
  }
  void readRawTexture(uint8_t* dest, int tex)
  {
    _validateTextureHandle(tex);
    TexOp::readRawTexture(dest, _textures[tex].first);
  }
  void readRawTexture(float* dest, int tex)
  {
    _validateTextureHandle(tex);
    TexOp::readRawTexture(dest, _textures[tex].first);
  }
  void writeRawChannel(int tex, const uint8_t* data, std::bitset<4> mask)
  {
    _validateTextureHandle(tex);
    TexOp::writeRawChannel(_textures[tex].first, data, mask);
    _updateResourceMaybe(tex);
  }
  void writeRawChannel(int tex, const float* data, std::bitset<4> mask)
  {
    _validateTextureHandle(tex);
    TexOp::writeRawChannel(_textures[tex].first, data, mask);
    _updateResourceMaybe(tex);
  }
  void readRawChannel(uint8_t* dest, int tex, int ch)
  {
    _validateTextureHandle(tex);
    TexOp::readRawChannel(dest, _textures[tex].first, ch);
  }
  void readRawChannel(float* dest, int tex, int ch)
  {
    _validateTextureHandle(tex);
    TexOp::readRawChannel(dest, _textures[tex].first, ch);
  }
  
  //clearing/blending/filtering
  void fillTexture(int tex, const std::array<float, 4> &color, std::bitset<4> mask)
  {
    _validateTextureHandle(tex);
    Eigen::Array4f col(color[0], color[1], color[2], color[3]);
    if(mask.none())
    {
      return;
    }
    else if(mask.all())
    {
      TexOp::clear(_textures[tex].first, col);
    }
    else
    {
      TexOp::clear(_textures[tex].first, col, mask);
    }
    _updateResourceMaybe(tex);
  }
  void fillBlended(int tex,
    const std::array<float, 4> &color, const std::array<float, 4> &blend)
  {
    _validateTextureHandle(tex);
    Eigen::Array4f col(color[0], color[1], color[2], color[3]);
    Eigen::Array4f bld(blend[0], blend[1], blend[2], blend[3]);
    TexOp::blend(_textures[tex].first, col, bld);
    _updateResourceMaybe(tex);
  }
  
  void fillBackground(int tex, const std::array<float, 4>& color, std::bitset<4> mask)
  {
    _validateTextureHandle(tex);
    Eigen::Array4f col(color[0], color[1], color[2], color[3]);
    TexOp::fillWithBlendChannel(
      _textures[tex].first, mask, col, _textures[tex].first, 8);
    _updateResourceMaybe(tex);
  }
  void fillWithAlphaCh(
    int tex, const std::array<float, 4>& color, std::bitset<4> mask, int src, int ch)
  {
    _validateTextureHandle(tex);
    _validateTextureHandle(src);
    LinearInterpTexture src_c(src, tex);
    Eigen::Array4f col(color[0], color[1], color[2], color[3]);
    TexOp::fillWithRevBlendChannel(
      _textures[tex].first, mask, col, src_c, ch);
    _updateResourceMaybe(tex);
  }
  
  void filterTexture(int tex,
    float co,
    float li,
    float sq,
    float rs,
    float sm,
    std::bitset<4> mask)
  {
    _validateTextureHandle(tex);
    TexOp::filter(_textures[tex].first, co, li, sq, rs, sm, mask);
    _updateResourceMaybe(tex);
  }
  void filterTextureLin(int tex,
    float from1,
    float to1,
    float from2,
    float to2,
    std::bitset<4> mask)
  {
    _validateTextureHandle(tex);
    TexOp::linearFilter(_textures[tex].first, from1, to1, from2, to2, mask);
    _updateResourceMaybe(tex);
  }
  
  void filterTextureStencil(
    int tex,
    float cutof,
    bool rev,
    std::bitset<4> mask)
  {
    _validateTextureHandle(tex);
    TexOp::stencilFilter(_textures[tex].first, cutof, rev, mask);
    _updateResourceMaybe(tex);
  }
  
  void filterTextureDownsample(
    int tex,
    int levels,
    std::bitset<4> mask)
  {
    _validateTextureHandle(tex);
    TexOp::downsampleFilter(_textures[tex].first, levels, mask);
    _updateResourceMaybe(tex);
  }
  
  void channelDiff(int dest, int src, int ch, std::bitset<4> mask)
  {
    _validateTextureHandle(dest);
    if(dest == src)
    {
      TexOp::channelDiff(_textures[dest].first, _textures[src].first, ch, mask);
    }
    else
    {
      _validateTextureHandle(src);
      LinearInterpTexture src_t(src, dest);
      TexOp::channelDiff(_textures[dest].first, src_t, ch, mask);
    }
    _updateResourceMaybe(dest);
  }
  
  void textureDiff(int dest, int src, std::bitset<4> mask)
  {
    _validateTextureHandle(dest);
    if(dest == src)
    {
      TexOp::textureDiff(_textures[dest].first, _textures[src].first, mask);
    }
    else
    {
      _validateTextureHandle(src);
      LinearInterpTexture src_t(src, dest);
      TexOp::textureDiff(_textures[dest].first, src_t, mask);
    }
    _updateResourceMaybe(dest);
  }
  
  void clampTexels(int tex, float min, float max, std::bitset<4> mask)
  {
    _validateTextureHandle(tex);
    TexOp::clamp(_textures[tex].first, min, max, mask);
    _updateResourceMaybe(tex);
  }
  
  //texture operations
  void blendTextures(int dest, int src, std::bitset<4> mask, int ch)
  {
    _validateTextureHandle(dest);
    _validateTextureHandle(src);
    LinearInterpTexture src_t(src, dest);
    TexOp::blendTexturesWithAlpha(_textures[dest].first, src_t, mask, src_t, ch);
    _updateResourceMaybe(dest);
  }
  
  void blendTexturesWithCh(int dest, int src, std::bitset<4> mask, int b_tex, int ch)
  {
    _validateTextureHandle(dest);
    _validateTextureHandle(src);
    _validateTextureHandle(b_tex);
    LinearInterpTexture src_t(src, dest);
    LinearInterpTexture blend_t(b_tex, dest);
    TexOp::blendTexturesWithAlpha(_textures[dest].first, src_t, mask, blend_t, ch);
    _updateResourceMaybe(dest);
  }
  
  void mergeTextures(int dest, int src, float blend, std::bitset<4> mask)
  {
    _validateTextureHandle(dest);
    _validateTextureHandle(src);
    LinearInterpTexture src_t(src, dest);
    TexOp::mergeTextures(_textures[dest].first, src_t, blend, mask);
    _updateResourceMaybe(dest);
  }
  
  //channel operations
  void copyChannel(int dest, std::bitset<4> mask, int src, int ch)
  {
    _validateTextureHandle(dest);
    _validateTextureHandle(src);
    LinearInterpTexture src_t(src, dest);
    TexOp::copyChannel(_textures[dest].first, mask, src_t, ch);
    _updateResourceMaybe(dest);
  }
  
  void blendChannels(int dest, std::bitset<4> mask, int src, int ch, float blend)
  {
    _validateTextureHandle(dest);
    _validateTextureHandle(src);
    LinearInterpTexture src_t(src, dest);
    TexOp::blendChannels(_textures[dest].first, mask, src_t, ch, blend);
    _updateResourceMaybe(dest);
  }
  
  void swapChannels(int dest, int dch, int src, int sch)
  {
    _validateTextureHandle(dest);
    _validateTextureHandle(src);
    LinearInterpTexture src_t(src, dest);
    TexOp::swapChannels(_textures[dest].first, dch, src_t, sch);
    _updateResourceMaybe(dest);
  }
  
  int warpTexture(int src, int dmap, float mult)
  {
    _validateTextureHandle(src);
    _validateTextureHandle(dmap);
    LinearInterpTexture dmap_t(dmap, src);
    Texture* targ = makeTexture(
      _textures[src].first->width, _textures[src].first->height);
    TexOp::displaceMap(targ, _textures[src].first, dmap_t, mult);
    return _storeTexture(targ);
  }
  
  void warpTextureInplace(int tex, int dmap, float mult)
  {
    _validateTextureHandle(tex);
    _validateTextureHandle(dmap);
    LinearInterpTexture dmap_t(dmap, tex);
    Texture* targ = makeTexture(
      _textures[tex].first->width, _textures[tex].first->height);
    TexOp::displaceMap(targ, _textures[tex].first, dmap_t, mult);
    _replaceTexture(targ, tex);
  }
  
  int shiftTexels(int tex, float x_shift, float y_shift)
  {
    _validateTextureHandle(tex);
    Texture* targ = makeTexture(
      _textures[tex].first->width, _textures[tex].first->height);
    TexOp::shiftTexels(targ, _textures[tex].first, x_shift, y_shift);
    return _storeTexture(targ);
  }
  
  void shiftTexelsInplace(int tex, float x_shift, float y_shift)
  {
    _validateTextureHandle(tex);
    Texture* targ = makeTexture(
      _textures[tex].first->width, _textures[tex].first->height);
    TexOp::shiftTexels(targ, _textures[tex].first, x_shift, y_shift);
    _replaceTexture(targ, tex);
  }
  
  int applyLens(int lens, int tex)
  {
    _validateTextureHandle(lens);
    _validateTextureHandle(tex);
    Texture* targ = makeTexture(*_textures[lens].first);
    TexOp::sampleMap(targ, _textures[tex].first);
    return _storeTexture(targ);
  }
  
  void applyLensInplace(int lens, int tex)
  {
    _validateTextureHandle(lens);
    _validateTextureHandle(tex);
    TexOp::sampleMap(_textures[lens].first, _textures[tex].first);
    _updateResourceMaybe(lens);
  }
  
  int blurTexture(int tex, int f_width, int f_height, float f, std::bitset<4> mask)
  {
    _validateTextureHandle(tex);
    Texture* targ = makeTexture(
      _textures[tex].first->width, _textures[tex].first->height);
    
    switch((f_width == 0? 1 : 0) + (f_height == 0? 2 : 0))
    {
    case 0:
      {
        BlurFilter2d filter = BlurFilter1d(f_width) * BlurFilter1d(f_height);
        TexOp::blur(targ, _textures[tex].first, &filter, f, mask);
      }
      break;
    case 1:
      {
        BlurFilter1d filter(f_height);
        TexOp::blurVertic(targ, _textures[tex].first, &filter, f, mask);
      }
      break;
    case 2:
      {
        BlurFilter1d filter(f_width);
        TexOp::blurHoriz(targ, _textures[tex].first, &filter, f, mask);
      }
      break;
    case 3:
      TexOp::copyTexture(targ, _textures[tex].first, 0xf);
    }
    
    return _storeTexture(targ);
  }
  
  void blurInplace(int tex, int f_width, int f_height, float f, std::bitset<4> mask)
  {
    _validateTextureHandle(tex);
    Texture* targ = makeTexture(
      _textures[tex].first->width, _textures[tex].first->height);
    
    switch((f_width == 0? 1 : 0) + (f_height == 0? 2 : 0))
    {
    case 0:
      {
        BlurFilter2d filter = BlurFilter1d(f_width) * BlurFilter1d(f_height);
        TexOp::blur(targ, _textures[tex].first, &filter, f, mask);
      }
      break;
    case 1:
      {
        BlurFilter1d filter(f_height);
        TexOp::blurVertic(targ, _textures[tex].first, &filter, f, mask);
      }
      break;
    case 2:
      {
        BlurFilter1d filter(f_width);
        TexOp::blurHoriz(targ, _textures[tex].first, &filter, f, mask);
      }
      break;
    case 3:
      deleteTexture(targ);
      return;
    }
    
    _replaceTexture(targ, tex);
  }
  
  void generateNoise(int tex, int dev, std::bitset<4> mask)
  {
    _validateTextureHandle(tex);
    if(Rand::getDevice(dev) == nullptr)
      throw tgException("random device %i not initialized", dev);
    TexOp::generateNoise(_textures[tex].first, dev, mask);
    _updateResourceMaybe(tex);
  }
  
  void generateWhiteNoise(int tex, int dev, std::bitset<4> mask)
  {
    _validateTextureHandle(tex);
    if(Rand::getDevice(dev) == nullptr)
      throw tgException("random device %i not initialized", dev);
    TexOp::generateWhiteNoise(_textures[tex].first, dev, mask);
    _updateResourceMaybe(tex);
  }
  
  int makeTurbulence(int tex, int levels, float persistance, std::bitset<4> mask)
  {
    assert(levels > 0);
    assert(persistance > 0.);
    _validateTextureHandle(tex);
    Texture* targ = makeTexture(_textures[tex].first->width,
      _textures[tex].first->height);
    TexOp::makeTurbulence(targ, _textures[tex].first, levels, persistance, mask);
    return _storeTexture(targ);
  }
  
  void makeTurbulenceInplace(int tex, int levels, float persistance, std::bitset<4> mask)
  {
    assert(levels > 0);
    assert(persistance > 0.);
    _validateTextureHandle(tex);
    Texture* targ = makeTexture(_textures[tex].first->width,
      _textures[tex].first->height);
    TexOp::makeTurbulence(targ, _textures[tex].first, levels, persistance, mask);
    _replaceTexture(targ, tex);
  }
  
  void makeCellNoise(int tex,
    std::vector<std::pair<double, double>>& ps,
    float range,
    std::array<int, 4>& mask)
  {
    _validateTextureHandle(tex);
    if(std::any_of(__range(ps), [](std::pair<double, double>& p)
    {return p.first < 0. || p.first > 1. || p.second < 0. || p.second > 1.;}))
      throw tgException("malformed point-set");
    TexOp::makeCellNoise(_textures[tex].first, ps, range, mask);
    _updateResourceMaybe(tex);
  }
  
  void makeDelaunay(int tex,
    std::vector<std::pair<double, double>>& ps,
    float range,
    std::bitset<4> mask)
  {
    _validateTextureHandle(tex);
    if(std::any_of(__range(ps), [](std::pair<double, double>& p)
    {return p.first < 0. || p.first > 1. || p.second < 0. || p.second > 1.;}))
      throw tgException("malformed point-set");
    TexOp::makeDelaunay(_textures[tex].first, ps, range, mask);
    _updateResourceMaybe(tex);
  }
  
  void makeVoronoi(int tex,
    std::vector<std::pair<double, double>>& ps,
    float range,
    std::bitset<4> mask)
  {
    _validateTextureHandle(tex);
    if(std::any_of(__range(ps), [](std::pair<double, double>& p)
    {return p.first < 0. || p.first > 1. || p.second < 0. || p.second > 1.;}))
      throw tgException("malformed point-set");
    TexOp::makeVoronoi(_textures[tex].first, ps, range, mask);
    _updateResourceMaybe(tex);
  }
  
  void makeNormalMap(int tex, double mul)
  {
    _validateTextureHandle(tex);
    TexOp::makeNormalMap(_textures[tex].first, mul);
    _updateResourceMaybe(tex);
  }
  
  void writeToDisk(int tex, const char* filename)
  {
    _validateTextureHandle(tex);
    
    FILE* file = fopen(filename, "w");
    if(file == nullptr)
      throw tgException("unable to open file \'%s\'", filename);
    
    Texture* texh = _textures[tex].first;
    char *d_ref;
    uint32_t size = texh->width * texh->height;

    int buff_size = size * 4 + 54;

    std::unique_ptr<char[]> buffer(new char[buff_size]);
    d_ref = buffer.get();

    union
    {
      char ub[4];
      uint32_t ul;
      uint16_t us[2];
    };

    //bmp header
    strncpy(ub, "BM", 2);   //magic number
    memcpy(d_ref, ub, 2); d_ref += 2;

    ul = buff_size;     //file size
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = 0;                 //unused
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = 54;                //offset of pixel array
    memcpy(d_ref, ub, 4); d_ref += 4;

    //dib header
    ul = 40;                //size of dib header
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = texh->width;                 //image width
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = texh->height;                 //image height
    memcpy(d_ref, ub, 4); d_ref += 4;
    us[0] = 1; us[1] = 32;  //number of color planes/color depth
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = 0;                 //pixel array compression used
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = size * 4;          //size of pixel array
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = 1000;              //pixels/meter
    memcpy(d_ref, ub, 4); d_ref += 4;
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = 0;                 //palette size
    memcpy(d_ref, ub, 4); d_ref += 4;
    memcpy(d_ref, ub, 4); d_ref += 4;

    //pixel array
    for(unsigned y = texh->height; y > 0; --y)
    for(unsigned x = 0; x < texh->width; ++x)
    {
      auto texel = texh->get(x, y - 1);
      if(texel[0] < 0.) texel[0] = 0.;
      else if(texel[0] > 1.) texel[0] = 1.;
      if(texel[1] < 0.) texel[0] = 0.;
      else if(texel[1] > 1.) texel[0] = 1.;
      if(texel[2] < 0.) texel[0] = 0.;
      else if(texel[2] > 1.) texel[0] = 1.;
      if(texel[3] < 0.) texel[0] = 0.;
      else if(texel[3] > 1.) texel[0] = 1.;
      
      ub[0] = (int)(texel[0] * 255);
      ub[1] = (int)(texel[1] * 255);
      ub[2] = (int)(texel[2] * 255);
      ub[3] = (int)(texel[3] * 255);
      
      memcpy(d_ref, ub, 4);
      d_ref += 4;
    }
    
    int errors = 0;
    if(fwrite(buffer.get(), 1, buff_size, file) < size)
      errors = 1;
    if(fflush(file) != 0)
      errors |= 2;
    fclose(file);
    
    if(errors) throw tgException("failed to write file \'%s\' to disk", filename);
  }
}