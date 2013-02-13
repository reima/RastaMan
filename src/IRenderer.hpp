#ifndef IRENDERER_HPP
#define IRENDERER_HPP

#include <Eigen/Core>

class IRenderer {
 public:
  virtual ~IRenderer() {}

  virtual void Clear(const float clearColor[4]) = 0;

  virtual void SetModelViewMatrix(const Eigen::Matrix4f& m) = 0;
  virtual void SetProjectionMatrix(const Eigen::Matrix4f& m) = 0;

  virtual void SetViewport(int x, int y, int width, int height) = 0;

  virtual void DrawTriangles(const Eigen::Vector3f* vertices,
                             const int* indices,
                             int count) = 0;
};

#endif
