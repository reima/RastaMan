#ifndef FONT_HPP
#define FONT_HPP

#include "GL/glew.h"

class Font {
 public:
  Font();
  ~Font();

  void Draw(const char* str, int x, int y);

 private:
  GLuint texture_;
};

#endif
