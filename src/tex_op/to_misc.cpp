
#include "../tex_op.h"

#include "to_common.h"

namespace
{
  void _makeNormalMap(Texture* tex, double mul, int from, int to)
  {
    for(int i = from; i < to; ++i)
    {
      int x = i % tex->width;
      int y = i / tex->width;
      
      double x_diff = tex->get((x + tex->width - 1) % tex->width, y)[3];
      x_diff -= tex->get((x + 1) % tex->width, y)[3];
      double y_diff = tex->get(x, (y + tex->height - 1) % tex->height)[3];
      y_diff -= tex->get(x, (y + 1) % tex->height)[3];
      
      x_diff *= mul;
      y_diff *= mul;
      
      double len = sqrt(1 + x_diff * x_diff + y_diff * y_diff);
      x_diff /= len;
      y_diff /= len;
      double z_diff = 1. / len;
      
      tex->get(i)[2] = (x_diff + 1.) / 2;
      tex->get(i)[1] = (y_diff + 1.) / 2;
      tex->get(i)[0] = (z_diff + 1.) / 2;
    }
  }
}

namespace TexOp
{
  void makeNormalMap(Texture* tex, double mul)
  {
    _launchThreads(tex->width * tex->height, _makeNormalMap, tex, mul);
  }
}