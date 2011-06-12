#include "RenderSurface.hpp"

#include <algorithm>

template<typename Component>
RenderSurface<Component>::RenderSurface(int width, int height)
    : width_(width), height_(height), pixels_(new Component[width*height]) {
}

template<typename Component>
RenderSurface<Component>::~RenderSurface() {
}

template<typename Component>
int RenderSurface<Component>::GetWidth() const {
  return width_;
}

template<typename Component>
int RenderSurface<Component>::GetHeight() const {
  return height_;
}

template<typename Component>
void RenderSurface<Component>::Clear(const Component& clearColor) {
  std::fill(&pixels_[0], &pixels_[width_*height_], clearColor);
}

template<typename Component>
const Component* RenderSurface<Component>::GetPixels() const {
  return pixels_.get();
}

template<typename Component>
const Component& RenderSurface<Component>::operator()(int x, int y) const {
  return pixels_[width_*y+x];
}

template<typename Component>
Component& RenderSurface<Component>::operator()(int x, int y) {
  return pixels_[width_*y+x];
}

// Explicit template instantiations
template class RenderSurface<float>;
template class RenderSurface<Eigen::Vector2f>;
template class RenderSurface<Eigen::Vector3f>;
template class RenderSurface<Eigen::Vector4f>;
