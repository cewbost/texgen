#ifndef SMALL_VECTOR_H_INCLUDED
#define SMALL_VECTOR_H_INCLUDED

#include <new>
#include <cstddef>

#define RC(x) reinterpret_cast<Type>(x)

template<class Type, int Size>
class SmallVector
{
private:
  size_t _size;
  union
  {
    char _ss_buffer[sizeof(Type) * Size];
    struct
    {
      size_t capacity;
      char* buffer;
    }_ls_buffer;
  };
  
  #define s_buff reinterpret_cast<Type>(_ss_buffer)
  #define l_buff reinterpret_cast<Type>(_ls_buffer.buffer)
  
  void _destroyContent()
  {
    if(_size > Size)
    {
      for(int i = 0; i < _size; ++i)
        l_buff[i].~Type();
      delete[] l_buff;
    }
    else
    {
      for(int i = 0; i < _size; ++i)
        s_buff[i].~Type();
    }
  }

public:
  
  SmallVector(): _size(0){}
  SmallVector(const SmallVector& other)
  {
    _destroyContent();
    _size = other._size;
    if(_size > Size)
    {
      for(int i = 0; i < _size; ++i)
        new(_ss_buffer + sizeof(Type) * i) Type(RC(other._ss_buffer)[i]);
    }
    else
    {
      _ls_buffer.capacity = other._ls_buffer.capacity;
      _ls_buffer.buffer = new char[sizeof(Type) * _ls_buffer.capacity];
      for(int i = 0; i < _size; ++i)
        new(_ls_buffer.buffer + sizeof(Type) * i) Type(RC(other._ss_buffer)[i]);
    }
  }
  SmallVector(SmallVector&& other)
  {
    _destroyContent();
    _size = other._size;
    if(_size > Size)
    {
      _ls_buffer.capacity = other._ls_buffer.capacity;
      _ls_buffer.buffer = other._ls_buffer.buffer;
      other._size = 0;
      //delete[] other._ls_buffer.buffer;
      //warning: this leaves a dangling pointer in the moved from object
      //_size being zero will prevent deleting
    }
    else
    {
      for(int i = 0; i < _size; ++i)
        new(_ss_buffer + sizeof(Type) * i) Type(RC(other._ss_buffer)[i]);
      //same as copying for small objects
    }
  }
  
  ~SmallVector()
  {
    _destroyContent();
  }
  
  void clear()
  {
    _destroyContent();
    _size = 0;
  }
};

#undef s_buff
#undef l_buff

#undef RC

#endif
