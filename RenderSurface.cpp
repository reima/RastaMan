#include "RenderSurface.hpp"

#include <algorithm>

RenderSurface::RenderSurface(int width, int height)
    : width_(width), height_(height), pixels_(new Eigen::Vector4f[width*height]) {
}

RenderSurface::~RenderSurface() {
}

int RenderSurface::GetWidth() const {
  return width_;
}

int RenderSurface::GetHeight() const {
  return height_;
}

void RenderSurface::Clear(const Eigen::Vector4f& clearColor) {
  std::fill(&pixels_[0], &pixels_[width_*height_], clearColor);
}

const Eigen::Vector4f* RenderSurface::GetPixels() const {
  return pixels_.get();
}

const Eigen::Vector4f& RenderSurface::operator()(int x, int y) const {
  return pixels_[width_*y+x];
}

Eigen::Vector4f& RenderSurface::operator()(int x, int y) {
  return pixels_[width_*y+x];
}

