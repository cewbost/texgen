#include <thread>

#include "common.h"

#include "texture.h"

#include "tex_op.h"
#include "tex_op/to_common.h"

#include "blurfilter.h"

int _num_threads;
std::unique_ptr<std::thread[]> _thread_pool;

namespace TexOp
{
  void init()
  {
    //#ifndef NDEBUG
    //_num_threads = 1;
    //#else
    _num_threads = std::thread::hardware_concurrency();
    //#endif
    _thread_pool.reset(new std::thread[_num_threads - 1]);
  }
  
  void deinit() noexcept
  {
    _thread_pool = nullptr;
  }
}