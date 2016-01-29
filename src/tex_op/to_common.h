#ifndef TEX_OP_COMMON_H_INCLUDED
#define TEX_OP_COMMON_H_INCLUDED

#include <thread>

extern int _num_threads;
extern std::unique_ptr<std::thread[]> _thread_pool;

//helper functions (with bonus crazy metaprogramming)

/*
  this module makes extensive use of metaprogramming.
  _launchThreads runs all available threads in paralell to iterate over elements.
    arguments:
      int x: number of elements to iterate over
      F func: function to execute
      T&&... t: any other arguments are forwarded
    the function to execute must take two integers as it's final arguments
    these will be supplied the beginning and end indices of the elements for that
    thread
  _launchThreadsAnd takes another function as it's third argument and executes this
    function for the last range of elements.
  
  there are helper templates for code generation.
  to use a channel or channel mask as a template argument, allowing compiletime
  removal of conditionals in tight loops, the tasks should be implemented as
  template classes with a static void func(...) member. Arguments to this function
  are forwarded from _launchThreads, hence the two last arguments must be integers.
  the first argument must be a Texture*, since it is forwarded by the helpers, which
  all take a Texture* as first argument in order to compute the range of elements
  from the textures width and height attributes.
  
  all the code-generation helpers take one template template argument for the task
  to perform. for nested templates the member template should be named NextC.
  
  _launchThreadsMasked: branches for mask (15 options)
    <template<int> class Task>(int mask, Texture* tex, ...)
  _launchThreadsCh: branches for single channel (4 options)
    <template<int> class Task>(int ch, Texture* tex, ...)
  _launchThreads2Ch: branches for two channels (16 options)
    <template<int, int> class Task>(int ch1, ch2, Texture* tex, ...)
    note: ch1 and ch2 are passed as template arguments in the same order they're
    given.
  _launchThreadsCh2Masked: branches for channel and mask (60 options)
    <template<int> class Task>(int ch, int mask, Texture* tex, ...)
    note: Task should be a nested template
  
  note:
    - long ass compiler errors probably means you screwed up const-correctness
    - arguments are forwarded through multiple functions as template r-value
      reference arguments so arguments that should arrive as references to task
      handlers should be passed with std::ref
*/

template<class F, class... T>
void _launchThreads(int x, F func, T&&... t)
{
  int x_per_thread = x / _num_threads;
  for(int i = 0; i < _num_threads - 1; ++i)
    _thread_pool[i] = std::thread(func, std::forward<T>(t)...,
      x_per_thread * i, x_per_thread * (i + 1));
  func(std::forward<T>(t)..., x_per_thread * (_num_threads - 1), x);
  for(int i = 0; i < _num_threads - 1; ++i)
    _thread_pool[i].join();
}
template<class F1, class F2, class... T>
void _launchThreadsAnd(int x, F1 func1, F2 func2, T&&... t)
{
  int x_per_thread = x / _num_threads;
  for(int i = 0; i < _num_threads - 1; ++i)
    _thread_pool[i] = std::thread(func1, std::forward<T>(t)...,
      x_per_thread * i, x_per_thread * (i + 1));
  func2(std::forward<T>(t)..., x_per_thread * (_num_threads - 1), x);
  for(int i = 0; i < _num_threads - 1; ++i)
    _thread_pool[i].join();
}

template<template<int> class F, class E, class... T>
void _launchThreadsMasked(
  int mask, E tex, T&&... t)
{
  int e = tex->width * tex->height;
  switch(mask)
  {
  case 0x1: _launchThreads(e, F<0x1>::func, tex, std::forward<T>(t)...); break;
  case 0x2: _launchThreads(e, F<0x2>::func, tex, std::forward<T>(t)...); break;
  case 0x3: _launchThreads(e, F<0x3>::func, tex, std::forward<T>(t)...); break;
  case 0x4: _launchThreads(e, F<0x4>::func, tex, std::forward<T>(t)...); break;
  case 0x5: _launchThreads(e, F<0x5>::func, tex, std::forward<T>(t)...); break;
  case 0x6: _launchThreads(e, F<0x6>::func, tex, std::forward<T>(t)...); break;
  case 0x7: _launchThreads(e, F<0x7>::func, tex, std::forward<T>(t)...); break;
  case 0x8: _launchThreads(e, F<0x8>::func, tex, std::forward<T>(t)...); break;
  case 0x9: _launchThreads(e, F<0x9>::func, tex, std::forward<T>(t)...); break;
  case 0xa: _launchThreads(e, F<0xa>::func, tex, std::forward<T>(t)...); break;
  case 0xb: _launchThreads(e, F<0xb>::func, tex, std::forward<T>(t)...); break;
  case 0xc: _launchThreads(e, F<0xc>::func, tex, std::forward<T>(t)...); break;
  case 0xd: _launchThreads(e, F<0xd>::func, tex, std::forward<T>(t)...); break;
  case 0xe: _launchThreads(e, F<0xe>::func, tex, std::forward<T>(t)...); break;
  case 0xf: _launchThreads(e, F<0xf>::func, tex, std::forward<T>(t)...); break;
  default: break;
  }
}

template<template<int> class F, class E, class... T>
void _launchThreadsCh2Masked(
  int ch, int mask, E tex, T&&... t)
{
  switch(ch)
  {
  case 1: 
    _launchThreadsMasked<F<0>::template NextC>(mask, tex, std::forward<T>(t)...);
    break;
  case 2: 
    _launchThreadsMasked<F<1>::template NextC>(mask, tex, std::forward<T>(t)...);
    break;
  case 4: 
    _launchThreadsMasked<F<2>::template NextC>(mask, tex, std::forward<T>(t)...);
    break;
  case 8: 
    _launchThreadsMasked<F<3>::template NextC>(mask, tex, std::forward<T>(t)...);
    break;
  default: break;
  }
}

template<template<int> class F, class E, class... T>
void _launchThreadsCh(
  int ch, E tex, T&&... t)
{
  int elements = tex->width * tex->height;
  switch(ch)
  {
  case 1: _launchThreads(elements, F<0>::func, tex, std::forward<T>(t)...); break;
  case 2: _launchThreads(elements, F<1>::func, tex, std::forward<T>(t)...); break;
  case 4: _launchThreads(elements, F<2>::func, tex, std::forward<T>(t)...); break;
  case 8: _launchThreads(elements, F<3>::func, tex, std::forward<T>(t)...); break;
  default: break;
  }
}

template<template<int, int> class F, class E, class... T>
void _launchThreads2Ch(
  int ch1, int ch2, E tex, T&&... t)
{
  int e = tex->width * tex->height;
  switch((ch1 << 4) | ch2)
  {
  case 0x11: _launchThreads(e, F<0, 0>::func, tex, std::forward<T>(t)...); break;
  case 0x21: _launchThreads(e, F<1, 0>::func, tex, std::forward<T>(t)...); break;
  case 0x41: _launchThreads(e, F<2, 0>::func, tex, std::forward<T>(t)...); break;
  case 0x81: _launchThreads(e, F<3, 0>::func, tex, std::forward<T>(t)...); break;
  case 0x12: _launchThreads(e, F<0, 1>::func, tex, std::forward<T>(t)...); break;
  case 0x22: _launchThreads(e, F<1, 1>::func, tex, std::forward<T>(t)...); break;
  case 0x42: _launchThreads(e, F<2, 1>::func, tex, std::forward<T>(t)...); break;
  case 0x82: _launchThreads(e, F<3, 1>::func, tex, std::forward<T>(t)...); break;
  case 0x14: _launchThreads(e, F<0, 2>::func, tex, std::forward<T>(t)...); break;
  case 0x24: _launchThreads(e, F<1, 2>::func, tex, std::forward<T>(t)...); break;
  case 0x44: _launchThreads(e, F<2, 2>::func, tex, std::forward<T>(t)...); break;
  case 0x84: _launchThreads(e, F<3, 2>::func, tex, std::forward<T>(t)...); break;
  case 0x18: _launchThreads(e, F<0, 3>::func, tex, std::forward<T>(t)...); break;
  case 0x28: _launchThreads(e, F<1, 3>::func, tex, std::forward<T>(t)...); break;
  case 0x48: _launchThreads(e, F<2, 3>::func, tex, std::forward<T>(t)...); break;
  case 0x88: _launchThreads(e, F<3, 3>::func, tex, std::forward<T>(t)...); break;
  }
}

#endif
