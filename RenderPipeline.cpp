#include "RenderPipeline.hpp"
#include "RenderSurface.hpp"

using namespace Eigen;

RenderPipeline::RenderPipeline()
  : modelViewMatrix_(Matrix4f::Identity()),
    projectionMatrix_(Matrix4f::Identity()),
    modelViewProjectionMatrix_(Matrix4f::Identity()) {
}

RenderPipeline::~RenderPipeline() {
}

void RenderPipeline::SetModelViewMatrix(const Matrix4f& matrix) {
  modelViewMatrix_ = matrix;
  modelViewProjectionMatrix_ = projectionMatrix_ * modelViewMatrix_;
}

void RenderPipeline::SetProjectionMatrix(const Matrix4f& matrix) {
  projectionMatrix_ = matrix;
  modelViewProjectionMatrix_ = projectionMatrix_ * modelViewMatrix_;
}

void RenderPipeline::SetRenderSurface(boost::shared_ptr<RenderSurface> surface) {
  renderSurface_ = surface;
}

void RenderPipeline::SetViewport(int x, int y, int width, int height) {
  viewport_ = AlignedBox<int, 2>(Vector2i(x, y), Vector2i(x + width, y + height));
  viewportScale_ = Vector3f(0.5f * width, -0.5f * height, 0.5f);
  viewportBias_ = Vector3f(0.5f * width + x, 0.5f * height + y, 0.5f);
}

Vector4f RenderPipeline::ProcessVertex(const Vector4f& position) {
  return modelViewProjectionMatrix_ * position;
}

Vector4f RenderPipeline::ProcessFragment(const Vector2i& position) {
  return Vector4f(1, 1, 1, 1);
}

void RenderPipeline::RasterizeTriangle(const Vector2f& v0,
                                       const Vector2f& v1,
                                       const Vector2f& v2,
                                       std::vector<Vector2i>& fragments) {
  // Bounding box
  AlignedBox<int, 2> box(v0.cast<int>());
  box.extend(v1.cast<int>()).extend(v2.cast<int>());

  box = box.intersection(viewport_);
  if (box.isEmpty()) return;

  // Edge equations setup
  float a0, b0, c0;
  a0 = v0.y() - v1.y();
  b0 = v1.x() - v0.x();
  c0 = -(a0*(v0.x() + v1.x()) + b0*(v0.y() + v1.y())) * 0.5f;
  float a1, b1, c1;
  a1 = v1.y() - v2.y();
  b1 = v2.x() - v1.x();
  c1 = -(a1*(v1.x() + v2.x()) + b1*(v1.y() + v2.y())) * 0.5f;
  float a2, b2, c2;
  a2 = v2.y() - v0.y();
  b2 = v0.x() - v2.x();
  c2 = -(a2*(v0.x() + v2.x()) + b2*(v0.y() + v2.y())) * 0.5f;

  for (int y = box.min().y(); y <= box.max().y(); ++y) {
    for (int x = box.min().x(); x <= box.max().x(); ++x) {
      float e0 = a0*(x+0.5f) + b0*(y+0.5f) + c0;
      float e1 = a1*(x+0.5f) + b1*(y+0.5f) + c1;
      float e2 = a2*(x+0.5f) + b2*(y+0.5f) + c2;
      if (e0 > 0 && e1 > 0 && e2 > 0) {
        fragments.push_back(Vector2i(x, y));
      }
    }
  }
}

void RenderPipeline::DrawTriangle(const Vector4f& v0,
                                  const Vector4f& v1,
                                  const Vector4f& v2) {
  /*
   * Projection
   */
  Vector4f clip[3] = {
    ProcessVertex(v0),
    ProcessVertex(v1),
    ProcessVertex(v2)
  };

  /*
   * TODO: Clipping?
   */

  /*
   * Dehomogenization
   */
  Vector4f ndc[3] = {
    clip[0] / clip[0].w(),
    clip[1] / clip[1].w(),
    clip[2] / clip[2].w()
  };

  /*
   * Viewport transform
   */
  Vector3f c[3];
  for (int i = 0; i < 3; ++i) {
    c[i] = ndc[i].head<3>().cwiseProduct(viewportScale_) + viewportBias_;
  }

  /*
   * Rasterization
   */
  std::vector<Vector2i> fragments;
  RasterizeTriangle(c[0].head<2>(), c[1].head<2>(), c[2].head<2>(), fragments);

  for (std::vector<Vector2i>::iterator it = fragments.begin(); it != fragments.end(); ++it) {
    Vector4f color = ProcessFragment(*it);
    (*renderSurface_)(it->x(), it->y()) = color;
  }
}

