#ifndef OPENGLRENDERER_HPP
#define OPENGLRENDERER_HPP

#include "IRenderer.hpp"

class OpenGLRenderer : public IRenderer {
 public:
  OpenGLRenderer();
  ~OpenGLRenderer();

  void Clear(const float clearColor[4]);

  void SetModelViewMatrix(const Eigen::Matrix4f& m);
  void SetProjectionMatrix(const Eigen::Matrix4f& m);  

  void SetViewport(int x, int y, int width, int height);

  void DrawTriangles(const Eigen::Vector3f* vertices,
                     const int* indices,
                     int count);
};

#endif
