#include "RenderTarget.hpp"

#include <cassert>

RenderTarget::RenderTarget(boost::shared_ptr<RenderSurface4f> backBuffer,
                           boost::shared_ptr<RenderSurface1f> zBuffer)
    : backBuffer_(backBuffer), zBuffer_(zBuffer) {
  assert(backBuffer_->GetWidth() == zBuffer_->GetWidth());
  assert(backBuffer_->GetHeight() == zBuffer_->GetHeight());
}

RenderTarget::RenderTarget(int width, int height)
  : backBuffer_(new RenderSurface4f(width, height)),
    zBuffer_(new RenderSurface1f(width, height)) {
}

RenderTarget::~RenderTarget() {
}

boost::shared_ptr<RenderSurface4f> RenderTarget::GetBackBuffer() {
  return backBuffer_;
}

boost::shared_ptr<RenderSurface1f> RenderTarget::GetZBuffer() {
  return zBuffer_;
}
