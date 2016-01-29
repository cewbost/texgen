
#include <cstdio>
#include <cstdlib>

#include "common.h"

#include "sq.h"
#include "viewer.h"
#include "h3d.h"
#include "terminal.h"

int main(int argc, char** argv)
{
  //set app_path
  using Common::app_path;
  app_path = argv[0];

  if( app_path.find( "/" ) != std::string::npos )
    app_path.erase(app_path.rfind("/") + 1);
  else if( app_path.find( "\\" ) != std::string::npos )
    app_path.erase(app_path.rfind("\\") + 1);
  else
    app_path.clear();
  
  #ifdef NDEBUG
  freopen(Common::path("log.txt").c_str(), "wb", stdout);
  #endif
  
  freopen(Common::path("log.out").c_str(), "wb", stderr);
  
  try
  {
    Viewer::init();
    Sq::init();
    
    for(int i = 1; i < argc; ++i)
    {
      try
      {
        Sq::executeNutRawPath(argv[i]);
      }
      catch(std::exception& e)
      {
        Terminal::printfm("%s\n", e.what());
      }
    }
    
    Terminal::updateSymbols();
    Terminal::start();
    
    while(!Common::done)
    {
      Viewer::update();
    }
  }
  catch(h3dException& e)
  {
    printf("\nunrecoverable error(h3d): %s\n\n", e.what());
    H3D::dumpMessagesToStdout();
    printf("exiting\n");
  }
  catch(std::exception& e)
  {
    printf("\nunrecoverable error: %s\nexiting\n", e.what());
  }
  
  Sq::deinit();
  Viewer::deinit();
  
  #ifdef NDEBUG
  fflush(stdout);
  fclose(stdout);
  #endif
  
  return 0;
}
