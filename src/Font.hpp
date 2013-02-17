#ifndef FONT_HPP
#define FONT_HPP

#include "GL/glew.h"

#include "boost/noncopyable.hpp"

class Font : boost::noncopyable {
 public:
  Font();
  ~Font();

  void Draw(const char* str, int x, int y);

  int GetLineHeight() const;
  int GetCharWidth() const;

 private:
  GLuint texture_;
};

#endif
