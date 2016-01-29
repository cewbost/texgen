#include <vector>
#include <string>
#include <memory>
#include <stack>
#include <algorithm>

#include <cstring>
#include <cstdarg>
#include <cctype>

#include "ring_buffer.h"

#include "viewer.h"
#include "sq.h"
#include "common.h"

#include "terminal.h"

#define __range(x) x.begin(), x.end()

using namespace Terminal;

namespace
{
  bool _needs_update = false;
  bool _showing = false;
  bool _stopped = false;
  int _viewport_w, _viewport_h;
  
  constexpr int _transit_ticks = 10;
  int _transit = _transit_ticks;
  
  constexpr int _char_w = 8;
  constexpr int _char_h = 15;
  constexpr int _char_stride = 16;
  constexpr int _fontmap_size = 256;

  class __Vert
  {
    float _vals[4];
  public:
    operator float*()
    {
      return _vals;
    }
    float& operator[](int idx)
    {
      return _vals[idx];
    }
  };
  
  class __CharView: public std::unique_ptr<__Vert[]>
  {
  public:
    __Vert& operator()(int x, int y, int i)
    {
      return this->operator[]((x + y * _viewport_w) * 4 + i);
    }
  };
  
  void _rebuildText();
  inline void _updateCharView(int x, int y);
  void _updateCharViews();
  
  H3DRes _mat1, _mat2, _mat_common, _shader1, _shader2, _tex, _pipeline;
  
  constexpr unsigned _max_buffer_lines = 200;
  
  RingBuffer<std::string> _buffer;
  
  RingBuffer<std::string> _history;
  RingBuffer<std::string>::reverse_iterator _history_it;
  //std::vector<std::string> _buffer;
  
  __Vert _box_verts[2 * 4];
  __CharView _char_views;
  
  std::unique_ptr<char[]> _view_chars;
  
  int _caret_pos;
  int _caret_min;
  
  /*
    note:
      a line in _buffer can occupy several lines in _view_chars
  */
  
  inline void _setVertPos(float* vert, int x, int y)
  {
    vert[0] = (float)x / Viewer::screen_h;
    vert[1] = (float)y / Viewer::screen_h;
  }
  inline void _setVertPos(float* vert, double x, double y)
  {
    vert[0] = (float)x * Viewer::screen_aspect;
    vert[1] = (float)y;
  }
  inline void _setVertPos(float* vert, int x, double y)
  {
    vert[0] = (float)x / Viewer::screen_h;
    vert[1] = (float)y;
  }
  inline void _setVertPos(float* vert, double x, int y)
  {
    vert[0] = (float)x * Viewer::screen_aspect;
    vert[1] = (float)y / Viewer::screen_h;
  }
  
  inline void _setVertUV(float* vert, int u, int v)
  {
    vert[2] = (float)u / _fontmap_size;
    vert[3] = (float)v / _fontmap_size;
  }
  
  inline void _setCaretUV(float* vert)
  {
    if(vert[2] < 1.)
    {
      vert[2] += 1.;
      vert[3] += 1.;
    }
  }
  
  inline void _copyVertUV(float* dest, const float* src)
  {
    dest[2] = src[2];
    dest[3] = src[3];
  }
  
  //just for testing
  #ifndef NDEBUG
  inline void _printViewChars()
  {
    for(int y = _viewport_h - 1; y >= 0; --y)
    {
      printf("line: %2i:", y);
      for(int x = 0; x < _viewport_w; ++x)
      {
        putchar(_view_chars[x + y * _viewport_w]);
      }
      putchar('\n');
    }
  }
  #else
  #define _printViewChars(); /*nop*/
  #endif
  
  inline void _showOverlays()
  {
    h3dClearOverlays();
    h3dShowOverlays((float*)_box_verts[0], 4, .5, .5, .5, 1., _mat1, 0);
    h3dShowOverlays((float*)_box_verts[4], 4, 0., 0., 0., .7, _mat1, 0);
    h3dShowOverlays(
      (float*)_char_views[0], _viewport_w * _viewport_h * 4,
      .8, .8, .8, 1., _mat2, 0);
  }
  
  void _rebuildOverlays()
  {
    _viewport_w = (Viewer::screen_w / 2 + 4) / _char_w - 1;
    _viewport_h = (Viewer::screen_h - 2 + _char_h) / _char_h;
    
    //make box
    _setVertPos(_box_verts[0], Viewer::screen_w / 2, 0.);
    _setVertPos(_box_verts[1], Viewer::screen_w / 2 + 2, 0.);
    _setVertPos(_box_verts[2], Viewer::screen_w / 2 + 2, 1.);
    _setVertPos(_box_verts[3], Viewer::screen_w / 2, 1.);
    _setVertPos(_box_verts[4], Viewer::screen_w / 2 + 2, 0.);
    _setVertPos(_box_verts[5], 1., 0.);
    _setVertPos(_box_verts[6], 1., 1.);
    _setVertPos(_box_verts[7], Viewer::screen_w / 2 + 2, 1.);
    
    _char_views.reset(new __Vert[_viewport_w * _viewport_h * 4]);
    
    for(int y = 0; y < _viewport_h; ++y)
    for(int x = 0; x < _viewport_w; ++x)
    {
      _setVertPos(_char_views(x, y, 0),
        Viewer::screen_w / 2 + 4 + x * _char_w,
        (Viewer::screen_h - 2) - (y + 1) * _char_h);
      _setVertPos(_char_views(x, y, 1),
        Viewer::screen_w / 2 + 4 + (x + 1) * _char_w,
        (Viewer::screen_h - 2) - (y + 1) * _char_h);
      _setVertPos(_char_views(x, y, 2),
        Viewer::screen_w / 2 + 4 + (x + 1) * _char_w,
        (Viewer::screen_h - 2) - y * _char_h);
      _setVertPos(_char_views(x, y, 3),
        Viewer::screen_w / 2 + 4 + x * _char_w,
        (Viewer::screen_h - 2) - y * _char_h);
    }
    
    _rebuildText();
  }
  
  void _rebuildText()
  {
    //_buffer stays unmutated in this function
    _view_chars.reset(new char[_viewport_w * _viewport_h]);
    auto it = _buffer.rbegin();
    std::stack<
      std::pair<
        std::string::const_iterator,
        std::string::const_iterator>> str_stack;
    
    for(int y = 0; y < _viewport_h; ++y)
    {
      if(str_stack.empty())
      {
        if(it != _buffer.rend())
        {
          const auto& str = *it;
          int len = str.size();
          int lines = len / _viewport_w;
          
          auto sit = str.cbegin();
          while(lines > 0)
          {
            auto next_sit = sit + _viewport_w;
            str_stack.push(std::make_pair(sit, next_sit));
            sit = next_sit;
            --lines;
          }
          str_stack.push(std::make_pair(sit, str.cend()));
          ++it;
          //str_stack gets filled in this branch
        }
        else //if _buffer end reached, fill with empty
        {
          for(int x = 0; x < _viewport_w; ++x)
            _view_chars[x + y * _viewport_w] = ' ';
          continue;
          //this branch continues
        }
      }
      //str_stack is not empty here
      
      auto str_view = str_stack.top();
      str_stack.pop();
      int written = 0;
      while(str_view.first != str_view.second)
      {
        _view_chars[written + y * _viewport_w] = *str_view.first;
        ++str_view.first;
        ++written;
      }
      while(written < _viewport_w)
      {
        _view_chars[written + y * _viewport_w] = ' ';
        ++written;
      }
    }
    
    _updateCharViews();
    _showOverlays();
  }
  
  inline void _updateCharView(int x, int y)
  {
    unsigned char ch = _view_chars[x + y * _viewport_w];
    int charmap_x, charmap_y;
    if(ch < 32 || ch > 127)
    {
      charmap_x = 7;
      charmap_y = 11;
    }
    else
    {
      charmap_x = ch % 8;
      charmap_y = (ch - 32) / 8;
    }
    
    _setVertUV(_char_views(x, y, 0),
      charmap_x * _char_stride, charmap_y * _char_h);
    _setVertUV(_char_views(x, y, 1),
      charmap_x * _char_stride + _char_w, charmap_y * _char_h);
    _setVertUV(_char_views(x, y, 2),
      charmap_x * _char_stride + _char_w, charmap_y * _char_h + _char_h);
    _setVertUV(_char_views(x, y, 3),
      charmap_x * _char_stride, charmap_y * _char_h + _char_h);
  }
  
  inline void _setCaretView(int x, int y)
  {
    _setCaretUV(_char_views(x, y, 0));
    _setCaretUV(_char_views(x, y, 1));
    _setCaretUV(_char_views(x, y, 2));
    _setCaretUV(_char_views(x, y, 3));
  }
  
  void _updateCharViews()
  {
    for(int y = 0; y < _viewport_h; ++y)
    for(int x = 0; x < _viewport_w; ++x)
    {
      _updateCharView(x, y);
    }
    _setCaretView(_caret_pos % _viewport_w,
      _buffer.back().size() / _viewport_w - _caret_pos / _viewport_w);
  }
  
  inline void _loadResource(H3DRes res, const char* filename)
  {
    auto buffer = Common::loadFile(Common::path(filename).c_str());
    if(!h3dLoadResource(res, buffer.first.get(), buffer.second))
      throw h3dException("unable to load resource \'%s\'", filename);
  }
  
  inline void _tokenizeIntoBuffer(const char* str)
  {
    std::string temp = "";
    const char* ch = str;
    do
    {
      if(*ch == '\n')
      {
        _buffer.back() += temp;
        _buffer.emplace_back("");
        temp = "";
      }
      else
      {
        temp += *ch;
      }
    }while(*(++ch) != '\0');
    _buffer.back() += temp;
  }
  
  inline void _setPromptLine(const char* line)
  {
    if(_stopped) return;
    _buffer.back() += line;
    _caret_min = _buffer.back().size();
  }
  
  //execution
  std::stack<char> _braces;
  std::string _entry;
  
  void _enterLine(const char* line)
  {
    int len = strlen(line);
    if(len == 0)
    {
      if(_braces.empty())
        _setPromptLine(">>");
      else _setPromptLine("..");
    }
    else
    {
      _history.push_back(line);
      _history_it = _history.rend();
      
      for(int i = 0; i < len; ++i)
      {
        switch(line[i])
        {
        case '(':
          _braces.push(')');
          break;
        case '[':
          _braces.push(']');
          break;
        case '{':
          _braces.push('}');
          break;
        
        case ')':
        case ']':
        case '}':
          if(_braces.top() != line[i])
          {
            i = len;
            while(!_braces.empty()) _braces.pop();
          }
          else _braces.pop();
        }
      }
      
      _entry += line;
      _entry += '\n';
      if(!_braces.empty())
      {
        _setPromptLine("..");
      }
      else
      {
        try
        {
          Sq::executeCode(_entry.c_str());
        }catch(...){} //this blocks exceptions for some reason
        _entry.clear();
        _setPromptLine(">>");
        updateSymbols();
      }
    }
  }
  
  //auto-complete stuff
  std::vector<std::string> _symbols;
  
  std::vector<std::string*> _matchSymbol(const char* word, int len)
  {
    std::vector<std::string*> matches;
    for(auto& str: _symbols)
    {
      int i;
      for(i = 0; i < len; ++i)
        if(str[i] != word[i])
          break;
      if(i == len)
        matches.push_back(&str);
    }
    
    return matches;
  }
  
  inline std::string _partialComplete(std::vector<std::string*>& match)
  {
    int matc = 0;
    char c;
    while(c = (*match[0])[matc], std::all_of(match.begin() + 1, match.end(),
      [=](std::string* str)->bool {return (*str)[matc] == c;})) ++matc;
    return match[0]->substr(0, matc);
  }
}

namespace Terminal
{
  void printfm(const char* format, ...)
  {
    va_list args;
    va_start(args, format);
    std::unique_ptr<char[]> temp(new char[256]);
    int size = vsnprintf(temp.get(), 256, format, args);
    if(size >= 256)
    {
      va_end(args);
      va_start(args, format);
      temp.reset(new char[size + 1]);
      vsnprintf(temp.get(), size + 1, format, args);
    }
    va_end(args);
    
    _tokenizeIntoBuffer(temp.get());
    _needs_update = true;
  }
  
  void print(const char* arg)
  {
    _tokenizeIntoBuffer(arg);
    _needs_update = true;
  }
  
  void println(const char* arg)
  {
    _tokenizeIntoBuffer(arg);
    _buffer.emplace_back("");
    _needs_update = true;
  }
  
  void rebuild()
  {
    _rebuildOverlays();
  }
  
  void update()
  {
    if(_showing && _transit > 0)
    {
      --_transit;
      h3dSetMaterialUniform(_mat_common, "translation",
        (Viewer::screen_aspect / 2) * ((float)_transit / _transit_ticks),
        0., 0., 0.);
    }
    else if(!_showing && _transit < _transit_ticks)
    {
      ++_transit;
      h3dSetMaterialUniform(_mat_common, "translation",
        (Viewer::screen_aspect / 2) * ((float)_transit / _transit_ticks),
        0., 0., 0.);
    }
  
    if(_needs_update)
    {
      _rebuildText();
      _needs_update = false;
    }
  }
  
  void feedCharacter(char ch)
  {
    if(!_showing) return;
  
    switch(ch)
    {
    case '\b':
      if(_caret_pos > _caret_min)
      {
        --_caret_pos;
        _buffer.back().erase(_caret_pos, 1);
      }
      break;
    case '\t':
      {
        ////auto-complete
        if(_caret_pos == _caret_min)
          break;
        char ch = _buffer.back()[_caret_pos - 1];
        if(!isalnum(ch) && ch != '_')
          break;
        
        int c;
        for(c = _caret_pos - 2; c >= _caret_min; --c)
        {
          ch = _buffer.back()[c];
          if(!isalnum(ch) && ch != '_')
            break;
        }
        ++c;
        
        std::vector<std::string*> matches =
          _matchSymbol(_buffer.back().c_str() + c, _caret_pos - c);
        
        if(matches.size() > 1)
        {
          std::string temp = "";
          for(auto str: matches)
          {
            temp += *str;
            int tab_size = 
              (temp.size() - (_viewport_w % 8) * (temp.size() / _viewport_w)) % 8;
            temp += ("        ") + tab_size;
          }
          _buffer.insert(_buffer.rbegin(), temp);
          
          int fill_from = _caret_pos - c;
          temp = _partialComplete(matches);
          _buffer.back().insert(_caret_pos, temp, fill_from, temp.size());
          _caret_pos += temp.size() - fill_from;
        }
        else if(matches.size() == 1)
        {
          int fill_from = _caret_pos - c;
          int fill_num = matches[0]->size() - fill_from;
          _buffer.back().insert(_caret_pos, *matches[0], fill_from, fill_num);
          _caret_pos += fill_num;
        }
      }
      break;
    case '\n':
      {
        auto& line = _buffer.back();
        _buffer.push_back(std::string(""));
        _enterLine(line.substr(_caret_min).c_str());
        _caret_pos = _caret_min;
        break;
      }
    default:
      _buffer.back().insert(_buffer.back().begin() + _caret_pos, ch);
      ++_caret_pos;
    }
    _needs_update = true;
  }
  
  void moveCaret(int arg)
  {
    if(!_showing) return;
  
    switch(arg)
    {
    case Caret::right:
      if(_caret_pos < (int)_buffer.back().size())
        ++_caret_pos;
      break;
    case Caret::left:
      if(_caret_pos > _caret_min)
        --_caret_pos;
      break;
    case Caret::back:
      {
        bool found_word = false;
        int c;
        for(c = _caret_pos - 1; c >= _caret_min; --c)
        {
          char ch = _buffer.back()[c];
          if(isalnum(ch) || ch == '_')
            found_word = true;
          else if(found_word)
          {
            _caret_pos = c + 1;
            break;
          }
        }
        if(c < _caret_min)
          _caret_pos = _caret_min;
      }
      break;
    case Caret::forward:
      {
        bool found_word = false;
        int c;
        for(c = _caret_pos; c < (int)_buffer.back().size(); ++c)
        {
          char ch = _buffer.back()[c];
          if(isalnum(ch) || ch == '_')
            found_word = true;
          else if(found_word)
            break;
        }
        _caret_pos = c;
      }
      break;
    case Caret::home:
      _caret_pos = _caret_min;
      break;
    case Caret::end:
      _caret_pos = _buffer.back().size();
      break;
    case Caret::erase_word:
      {
        bool found_word = false;
        int c;
        int new_caret_pos;
        for(c = _caret_pos - 1; c >= _caret_min; --c)
        {
          char ch = _buffer.back()[c];
          if(isalnum(ch) || ch == '_')
            found_word = true;
          else if(found_word)
          {
            new_caret_pos = c + 1;
            break;
          }
        }
        if(c < _caret_min)
          new_caret_pos = _caret_min;
        
        _buffer.back().erase(new_caret_pos, _caret_pos - new_caret_pos);
        _caret_pos = new_caret_pos;
      }
      break;
    }
    _needs_update = true;
  }
  
  void exploreHistory(int dir)
  {
    if(!_showing) return;
  
    switch(dir)
    {
    case History::back:
      if(_history_it != _history.rend())
        ++_history_it;
      if(_history_it == _history.rend())
        _history_it = _history.rbegin();
      break;
    case History::forward:
      if(_history_it == _history.rend())
        return;
      if(_history_it != _history.rbegin())
        --_history_it;
      else _history_it = _history.rend();
      break;
    }
    
    _buffer.back().erase(_caret_min);
    if(_history_it != _history.rend())
      _buffer.back() += *_history_it;
    _caret_pos = _buffer.back().size();
    _needs_update = true;
  }
  
  void updateSymbols()
  {
    _symbols = Sq::getRootTableSymbols();
  }
  
  void toggle()
  {
    _showing = !_showing;
  }
  
  H3DRes init()
  {
    _buffer.lockCapacity(400);
    _history.lockCapacity(200);
    _history_it = _history.rend();
  
    _buffer.emplace_back("");
    
    //load assets
    _mat1 = h3dAddResource(H3DResTypes::Material, "terminal_back_mat", 0);
    _mat2 = h3dAddResource(H3DResTypes::Material, "terminal_font_mat", 0);
    _mat_common = h3dAddResource(H3DResTypes::Material, "term_common", 0);
    _shader1 = h3dAddResource(H3DResTypes::Shader, "terminal_back_shader", 0);
    _shader2 = h3dAddResource(H3DResTypes::Shader, "terminal_font_shader", 0);
    _tex = h3dAddResource(H3DResTypes::Texture, "terminal_font_texture", 0);
    _pipeline = h3dAddResource(H3DResTypes::Pipeline, "terminal_pipeline", 0);
    
    const char* mat1_filename = "assets/term1.xml";
    const char* mat2_filename = "assets/term2.xml";
    const char* mat_common_filename = "assets/term_common.xml";
    const char* shader1_filename = "assets/term1.shader";
    const char* shader2_filename = "assets/term2.shader";
    const char* tex_filename = "assets/font.bmp";
    const char* pipeline_filename = "assets/term_pipeline.xml";
    
    _loadResource(_mat1, mat1_filename);
    _loadResource(_mat2, mat2_filename);
    _loadResource(_mat_common, mat_common_filename);
    _loadResource(_shader1, shader1_filename);
    _loadResource(_shader2, shader2_filename);
    _loadResource(_tex, tex_filename);
    _loadResource(_pipeline, pipeline_filename);
    
    h3dSetMaterialUniform(_mat_common, "translation",
      (Viewer::screen_aspect / 2) * ((float)_transit / _transit_ticks),
      0., 0., 0.);
    
    return _pipeline;
  }
  
  void start()
  {
    _stopped = false;
    _setPromptLine(">>");
    _caret_pos = _caret_min;
  }
  void stop()
  {
    _stopped = true;
  }
}
