#ifndef RENDERTARGET_HPP
#define RENDERTARGET_HPP

#include "RenderSurface.hpp"

#include <memory>

class RenderTarget {
 public:
  RenderTarget(std::shared_ptr<RenderSurface4f> backBuffer,
               std::shared_ptr<RenderSurface1f> zBuffer);
  RenderTarget(int width, int height);
  ~RenderTarget();

  std::shared_ptr<RenderSurface4f> GetBackBuffer();
  std::shared_ptr<RenderSurface1f> GetZBuffer();

 private:
  std::shared_ptr<RenderSurface4f> backBuffer_;
  std::shared_ptr<RenderSurface1f> zBuffer_;
};

#endif

