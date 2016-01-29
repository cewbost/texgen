#ifndef VIEWER_H_INCLUDED
#define VIEWER_H_INCLUDED

#include <horde3d.h>

namespace Viewer
{
  extern int screen_w, screen_h;
  extern double screen_aspect;

  void init();
  void deinit() noexcept;
  
  void update();
  
  void newCamera(H3DRes);
  void newModel(H3DRes, H3DRes);
  void newLightMat(H3DRes);
}

#endif
