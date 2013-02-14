#ifndef RASTAMANRENDERER_HPP
#define RASTAMANRENDERER_HPP

#include "IRenderer.hpp"
#include "RenderTarget.hpp"

#include "Eigen/Core"
#include "Eigen/Geometry"

#include "boost/noncopyable.hpp"

#include <memory>
#include <vector>

class RastaManRenderer : public boost::noncopyable, public IRenderer {
 public:
  RastaManRenderer(std::shared_ptr<RenderTarget> rt);
  ~RastaManRenderer();

  void Clear(const float clearColor[4]);

  void SetModelViewMatrix(const Eigen::Matrix4f& matrix);
  void SetProjectionMatrix(const Eigen::Matrix4f& matrix);

  void SetViewport(int x, int y, int width, int height);

  void SetRenderTarget(std::shared_ptr<RenderTarget> rt);

  void DrawTriangles(const Eigen::Vector3f* vertices,
                     const int* indices,
                     int count);

  void DrawTriangle(const Eigen::Vector4f& v0,
                    const Eigen::Vector4f& v1,
                    const Eigen::Vector4f& v2);

 protected:
  Eigen::Vector4f ProcessVertex(const Eigen::Vector4f& position);
  Eigen::Vector4f ProcessFragment(const Eigen::Vector3f& position);
  void RasterizeTriangle(const Eigen::Vector3f& v0,
                         const Eigen::Vector3f& v1,
                         const Eigen::Vector3f& v2,
                         std::function<void(const Eigen::Vector3f&)> callback);

 private:
  Eigen::Matrix4f modelViewMatrix_;
  Eigen::Matrix4f projectionMatrix_;
  Eigen::Matrix4f modelViewProjectionMatrix_;

  Eigen::AlignedBox<int, 2> viewport_;
  Eigen::Vector3f viewportScale_;
  Eigen::Vector3f viewportBias_;

  std::shared_ptr<RenderTarget> renderTarget_;
};

#endif

