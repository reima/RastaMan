#ifndef RENDERSURFACE_HPP
#define RENDERSURFACE_HPP

#include <boost/noncopyable.hpp>
#include <boost/scoped_array.hpp>
#include <Eigen/Core>

class RenderSurface : public boost::noncopyable {
 public:
  RenderSurface(int width, int height);
  ~RenderSurface();

  int GetWidth() const;
  int GetHeight() const;

  void Clear(const Eigen::Vector4f& clearColor);

  const Eigen::Vector4f* GetPixels() const;

  const Eigen::Vector4f& operator()(int x, int y) const;
  Eigen::Vector4f& operator()(int x, int y);

 private:
  int width_;
  int height_;
  boost::scoped_array<Eigen::Vector4f> pixels_;
};

#endif

