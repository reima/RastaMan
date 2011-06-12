#ifndef RENDERTARGET_HPP
#define RENDERTARGET_HPP

#include <boost/shared_ptr.hpp>

#include "RenderSurface.hpp"

class RenderTarget {
 public:
  RenderTarget(boost::shared_ptr<RenderSurface4f> backBuffer,
               boost::shared_ptr<RenderSurface1f> zBuffer);
  RenderTarget(int width, int height);
  ~RenderTarget();

  boost::shared_ptr<RenderSurface4f> GetBackBuffer();
  boost::shared_ptr<RenderSurface1f> GetZBuffer();

 private:
  boost::shared_ptr<RenderSurface4f> backBuffer_;
  boost::shared_ptr<RenderSurface1f> zBuffer_;
};

#endif

