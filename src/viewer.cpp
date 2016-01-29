#include <SDL.h>
#include <horde3d.h>
#include <GL/gl.h>

#include <eigen3/Eigen/Dense>
#include <eigen3/Eigen/Geometry>

#include "common.h"

#include "h3d.h"
#include "terminal.h"
#include "sq.h"

#include "viewer.h"

namespace
{
  bool _inited = false;

  SDL_Window* _screen;
  SDL_GLContext _gl_context;
  
  double _camera_zoom   = 30.;
  int _camera_scew      = 0;
  bool _show_terminal   = false;
  
  H3DNode _camera       = 0;
  H3DNode _term_camera  = 0;
  H3DNode _model        = 0;
  H3DNode _light        = 0;

  H3DRes _main_pipeline = 0;
  H3DRes _term_pipeline = 0;
  
  Eigen::Affine3f _orient;
  Eigen::Vector3f _x_axis, _y_axis;
  
  struct
  {
    double yaw, pitch;
  }_model_orientation{0., 0.}, _light_orientation{0., 0.};
  
  int _buttondown = 0;
  
  void _updateModelOrientation()
  {
    using namespace Eigen;
    Affine3f temp = _orient;
    temp.rotate(
      AngleAxis<float>(.05 * _model_orientation.yaw, _x_axis) *
      AngleAxis<float>(.05 * _model_orientation.pitch, _y_axis));
    h3dSetNodeTransMat(_model, temp.data());
  }
  void _applyModelOrientation()
  {
    using namespace Eigen;
    Affine3f temp = _orient;
    temp.rotate(
      AngleAxis<float>(.05 * _model_orientation.yaw, _x_axis) *
      AngleAxis<float>(.05 * _model_orientation.pitch, _y_axis));
    h3dSetNodeTransMat(_model, temp.data());
    _orient = temp;
    auto normalMat = _orient.linear().inverse();
    _x_axis = (normalMat * Vector3f::UnitX()).normalized();
    _y_axis = (normalMat * Vector3f::UnitY()).normalized();
    
    _model_orientation.yaw = _model_orientation.pitch = 0.;
  }
  
  void _updateLightOrientation()
  {
    using namespace Eigen;
    if(_light)
    {
      Affine3f orient;
      orient.setIdentity();
      orient.rotate(
        AngleAxis<float>(.05 * _light_orientation.yaw, Vector3f::UnitX()) *
        AngleAxis<float>(.05 * _light_orientation.pitch, Vector3f::UnitY()));
      orient.translate(Vector3f(0., 0., 10.));
      h3dSetNodeTransMat(_light, orient.data());
    }
  }
  
  inline void _render()
  {
    if(_camera)
      h3dRender(_camera);
    
    h3dRender(_term_camera);
    
    h3dFinalizeFrame();
    SDL_GL_SwapWindow(_screen);
  }
  
  inline void _updateCameraView()
  {
    if(!_camera)
      return;
    h3dSetupCameraView(_camera, _camera_zoom, Viewer::screen_aspect, .01, 100.);
    if(_camera_scew)
    {
      float left = h3dGetNodeParamF(_camera, H3DCamera::LeftPlaneF, 0);
      float scew = (left / 2) * ((float)_camera_scew / 10);
      h3dSetNodeParamF(_camera, H3DCamera::LeftPlaneF, 0, left - scew);
      h3dSetNodeParamF(_camera, H3DCamera::RightPlaneF, 0, -left - scew);
    }
  }
}

namespace Viewer
{
  int screen_w, screen_h;
  double screen_aspect;

  void init()
  {
    if(SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) != 0)
      throw tgException("error: SDL_Init failed: %s", SDL_GetError());
    
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    
    SDL_DisplayMode dm;
    if(SDL_GetDesktopDisplayMode(0, &dm) != 0)
      throw tgException("error: SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
    
    screen_w = dm.w;
    screen_h = dm.h;
    
    screen_w /= 2;
    screen_h /= 2;

    screen_aspect = (float)screen_w / (float)screen_h;

    if((_screen = SDL_CreateWindow("TexGen",
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      screen_w, screen_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE)) == nullptr)
      throw tgException("error: SDL_CreateWindow failed: %s", SDL_GetError());
      
    if((_gl_context = SDL_GL_CreateContext(_screen)) == nullptr)
      throw tgException("error: SDL_GL_CreateContext failed: %s", SDL_GetError());
    
    //clear the screen before anything else
    glClearColor(0., 0., 0., 0.);
    glClear(GL_COLOR_BUFFER_BIT);

    //horde3d shit
    if(!h3dInit())
      throw tgException("error: h3dInit failed.");
    
    H3D::init();
    H3D::dumpMessagesToStdout();
    
    _light = h3dAddLightNode(H3DRootNode, "light", 0,
      "LIGHT_CONTEXT", "SHADOW_CONTEXT");
    
    _light_orientation.yaw = 0.;
    _light_orientation.pitch = 0.;
    
    h3dSetNodeTransform(
      _light,
      0., 0., 10.,
      0., 0., 0.,
      1., 1., 1.);
    
    _inited = true;
    
    _term_pipeline = Terminal::init();
    Terminal::rebuild();
    
    _term_camera = h3dAddCameraNode(H3DRootNode, "term_camera", _term_pipeline);
    
    h3dSetupCameraView(_term_camera, 90., screen_aspect, .01, 100.);
    h3dSetNodeParamI(_term_camera, H3DCamera::ViewportWidthI, screen_w);
    h3dSetNodeParamI(_term_camera, H3DCamera::ViewportHeightI, screen_h);
  }
  
  void deinit() noexcept
  {
    if(!_inited)
      return;
    H3D::deinit();
    h3dRelease();
    
    SDL_StopTextInput();
    SDL_GL_DeleteContext(_gl_context);
    SDL_DestroyWindow(_screen);
    SDL_Quit();
  }
  
  void update()
  {
    static unsigned ticks = 0;
    unsigned new_ticks = SDL_GetTicks() / 10;
    unsigned tick_diff = new_ticks - ticks;
    ticks = new_ticks;
  
    static int camera_zoom_event = 0;
  
    SDL_Event event;
    SDL_PumpEvents();
    
    while(SDL_PollEvent(&event))
    {
      switch(event.type)
      {
      case SDL_KEYDOWN:
        switch(event.key.keysym.sym)
        {
        case SDLK_ESCAPE:
          Common::done = 1;
          return;
        case SDLK_KP_PLUS:
          camera_zoom_event |= 2;
          break;
        case SDLK_KP_MINUS:
          camera_zoom_event |= 1;
          break;
        case SDLK_BACKSPACE:
          if(event.key.keysym.mod & KMOD_CTRL)
            Terminal::moveCaret(Terminal::Caret::erase_word);
          else Terminal::feedCharacter('\b');
          break;
        case SDLK_TAB:
          if(event.key.keysym.mod & KMOD_CTRL)
          {
            Terminal::toggle();
            _show_terminal = !_show_terminal;
            if(_show_terminal)
              SDL_StartTextInput();
            else SDL_StopTextInput();
          }
          else Terminal::feedCharacter('\t');
          break;
        case SDLK_RETURN:
          Terminal::feedCharacter('\n');
          break;
        case SDLK_LEFT:
          if(event.key.keysym.mod & KMOD_CTRL)
            Terminal::moveCaret(Terminal::Caret::back);
          else Terminal::moveCaret(Terminal::Caret::left);
          break;
        case SDLK_RIGHT:
          if(event.key.keysym.mod & KMOD_CTRL)
            Terminal::moveCaret(Terminal::Caret::forward);
          else Terminal::moveCaret(Terminal::Caret::right);
          break;
        case SDLK_UP:
          Terminal::exploreHistory(Terminal::History::back);
          break;
        case SDLK_DOWN:
          Terminal::exploreHistory(Terminal::History::forward);
          break;
        case SDLK_HOME:
          Terminal::moveCaret(Terminal::Caret::home);
          break;
        case SDLK_END:
          Terminal::moveCaret(Terminal::Caret::end);
          break;
        }
        break;
      
      case SDL_KEYUP:
        switch(event.key.keysym.sym)
        {
        case SDLK_KP_PLUS:
          camera_zoom_event &= ~2;
          break;
        case SDLK_KP_MINUS:
          camera_zoom_event &= ~1;
          break;
        default:
          break;
        }
        break;
      
      case SDL_MOUSEBUTTONDOWN:
        if(event.button.button == SDL_BUTTON_LEFT)
        {
          _buttondown = 1;
        }
        else if(event.button.button == SDL_BUTTON_RIGHT)
        {
          _buttondown = 2;
        }
        else _buttondown = 0;
        break;
      
      case SDL_MOUSEBUTTONUP:
        if(_buttondown == 1)
          _applyModelOrientation();
        _buttondown = 0;
        break;
      
      case SDL_MOUSEMOTION:
        if(_buttondown == 1)
        {
          _model_orientation.yaw += event.motion.yrel * .1;
          _model_orientation.pitch += event.motion.xrel * .1;
          _updateModelOrientation();
        }
        else if(_buttondown == 2)
        {
          _light_orientation.yaw += event.motion.yrel * .1;
          _light_orientation.pitch += event.motion.xrel * .1;
          if(_light_orientation.yaw < -30.)
            _light_orientation.yaw = -30.;
          else if(_light_orientation.yaw > 30.)
            _light_orientation.yaw = 30.;
          if(_light_orientation.pitch < -30.)
            _light_orientation.pitch = -30.;
          else if(_light_orientation.pitch > 30.)
            _light_orientation.pitch = 30.;
          _updateLightOrientation();
        }
        break;
      
      case SDL_WINDOWEVENT:
        switch(event.window.event)
        {
        case SDL_WINDOWEVENT_ENTER:
        case SDL_WINDOWEVENT_LEAVE:
          _buttondown = 0;
          camera_zoom_event = 0;
          break;
          
        case SDL_WINDOWEVENT_SIZE_CHANGED:
          
          screen_w = event.window.data1;
          screen_h = event.window.data2;
          
          screen_aspect = (float)screen_w / (float)screen_h;
          
          h3dResizePipelineBuffers(_main_pipeline, screen_w, screen_h);
          h3dResizePipelineBuffers(_term_pipeline, screen_w, screen_h);
          _updateCameraView();
          h3dSetNodeParamI(_camera, H3DCamera::ViewportWidthI, screen_w);
          h3dSetNodeParamI(_camera, H3DCamera::ViewportHeightI, screen_h);
          h3dSetNodeParamI(_term_camera, H3DCamera::ViewportWidthI, screen_w);
          h3dSetNodeParamI(_term_camera, H3DCamera::ViewportHeightI, screen_h);
          
          Terminal::rebuild();
          
          break;
        }
        break;
      
      case SDL_TEXTINPUT:
        {
          auto ch = event.text.text[0];
          if(ch >= 32 && ch < 128)
            Terminal::feedCharacter((char)ch);
        }
        break;
      }
    }
    
    if(Sq::reload)
    {
      Sq::reloadVM();
      Sq::reload = false;
    }
    
    if(tick_diff)
    {
      bool update_camera = false;
      for(; tick_diff > 0; --tick_diff)
      {
        if(_show_terminal && _camera_scew < 10)
        {
          ++_camera_scew;
          update_camera = true;
        }
        else if(!_show_terminal && _camera_scew > 0)
        {
          --_camera_scew;
          update_camera = true;
        }
        if(camera_zoom_event)
        {
          if(camera_zoom_event == 2)
          {
            if(_camera_zoom > 5.)
              _camera_zoom -= .4;
            else _camera_zoom = 5.;
          }
          else if(camera_zoom_event == 1)
          {
            if(_camera_zoom < 75.)
              _camera_zoom += .4;
            else _camera_zoom = 75.;
          }
          update_camera = true;
        }
        
        Terminal::update();
      }
      if(update_camera)
      {
        _updateCameraView();
      }
    }
  
    _render();
  }
  
  void newCamera(H3DRes pipe_res)
  {
    if(_camera)
      h3dRemoveNode(_camera);

    _main_pipeline = pipe_res;
    _camera = h3dAddCameraNode(H3DRootNode, "camera", pipe_res);

    _updateCameraView();
    h3dSetNodeParamI(_camera, H3DCamera::ViewportWidthI, screen_w);
    h3dSetNodeParamI(_camera, H3DCamera::ViewportHeightI, screen_h);
    
    h3dSetNodeTransform(
      _camera,
      0., 0., 10.,
      0., 0., 0.,
      1., 1., 1.);
    
    int res_elem = h3dFindResElem(_term_pipeline,
      H3DPipeRes::StageElem, H3DPipeRes::StageNameStr, "Clear");
    if(res_elem != -1)
      h3dSetResParamI(_term_pipeline, H3DPipeRes::StageElem, res_elem,
        H3DPipeRes::StageActivationI, 0);
  }
  
  void newModel(H3DRes geo_res, H3DRes mat_res)
  {
    if(_model)
      h3dRemoveNode(_model);
    
    _model = h3dAddModelNode(H3DRootNode, "model", geo_res);
    int verts = h3dGetResParamI(
      geo_res,
      H3DGeoRes::GeometryElem,
      0,
      H3DGeoRes::GeoVertexCountI);
    int indices = h3dGetResParamI(
      geo_res,
      H3DGeoRes::GeometryElem,
      0,
      H3DGeoRes::GeoIndexCountI);
    H3DNode mesh = h3dAddMeshNode(_model, "mesh", mat_res, 0, indices, 0, verts - 1);
    
    _model_orientation.yaw = 0.;
    _model_orientation.pitch = 0.;
    
    _orient.setIdentity();
    _x_axis = Eigen::Vector3f::UnitX();
    _y_axis = Eigen::Vector3f::UnitY();
    h3dSetNodeTransMat(_model, _orient.data());
    
    h3dSetNodeTransform(
      mesh,
      0., 0., 0.,
      0., 0., 0.,
      1., 1., 1.);
  }
}
