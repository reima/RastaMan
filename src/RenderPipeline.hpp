#ifndef RENDERPIPELINE_HPP
#define RENDERPIPELINE_HPP

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

class RenderSurface;

class RenderPipeline : public boost::noncopyable {
 public:
  RenderPipeline();
  ~RenderPipeline();

  void SetModelViewMatrix(const Eigen::Matrix4f& matrix);
  void SetProjectionMatrix(const Eigen::Matrix4f& matrix);

  void SetViewport(int x, int y, int width, int height);

  void SetRenderSurface(boost::shared_ptr<RenderSurface> surface);

  void DrawTriangle(const Eigen::Vector4f& v0,
                    const Eigen::Vector4f& v1,
                    const Eigen::Vector4f& v2);

 protected:
  Eigen::Vector4f ProcessVertex(const Eigen::Vector4f& position);
  Eigen::Vector4f ProcessFragment(const Eigen::Vector2i& position);
  void RasterizeTriangle(const Eigen::Vector2f& v0,
                         const Eigen::Vector2f& v1,
                         const Eigen::Vector2f& v2,
                         std::vector<Eigen::Vector2i>& fragments);

 private:
  Eigen::Matrix4f modelViewMatrix_;
  Eigen::Matrix4f projectionMatrix_;
  Eigen::Matrix4f modelViewProjectionMatrix_;

  Eigen::AlignedBox<int, 2> viewport_;
  Eigen::Vector3f viewportScale_;
  Eigen::Vector3f viewportBias_;

  boost::shared_ptr<RenderSurface> renderSurface_;
};

#endif

