#ifndef RENDERSURFACE_HPP
#define RENDERSURFACE_HPP

#include <boost/noncopyable.hpp>
#include <boost/scoped_array.hpp>
#include <Eigen/Core>

template<typename Component>
class RenderSurface : public boost::noncopyable {
 public:
  typedef Component ComponentType;

  RenderSurface(int width, int height);
  ~RenderSurface();

  int GetWidth() const;
  int GetHeight() const;

  void Clear(const Component& clearColor);

  const Component* GetPixels() const;

  const Component& operator()(int x, int y) const;
  Component& operator()(int x, int y);

 private:
  int width_;
  int height_;
  boost::scoped_array<Component> pixels_;
};

typedef RenderSurface<float> RenderSurface1f;
typedef RenderSurface<Eigen::Vector2f> RenderSurface2f;
typedef RenderSurface<Eigen::Vector3f> RenderSurface3f;
typedef RenderSurface<Eigen::Vector4f> RenderSurface4f;

#endif

