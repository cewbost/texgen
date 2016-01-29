#ifndef TERMINAL_H_INCLUDED
#define TERMINAL_H_INCLUDED

#include <cstdarg>

#include <horde3d.h>

namespace Terminal
{
  void printfm(const char*, ...);
  void print(const char*);
  void println(const char*);
  
  void rebuild();
  void update();

  void feedCharacter(char);
  
  struct Caret
  {
    enum
    {
      left,
      right,
      back,
      forward,
      home,
      end,
      erase_word
    };
  };
  void moveCaret(int);
  
  struct History
  {
    enum
    {
      back,
      forward
    };
  };
  void exploreHistory(int);
  
  void updateSymbols();
  
  void toggle();
  
  void start();
  void stop();

  H3DRes init();
}

#endif
