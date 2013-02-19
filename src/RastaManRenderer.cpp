#include "RastaManRenderer.hpp"

#include "FixedPoint.hpp"
#include "RenderSurface.hpp"

using namespace Eigen;

namespace {
typedef FixedPoint<int32_t, 8> FP;
typedef Matrix<FP, 2, 1> Vector2FP;
typedef Matrix<FP, 3, 1> Vector3FP;

template<typename T>
inline T Orient2D(const Array<T, 2, 1>& a, const Array<T, 2, 1>& b,
                  const Array<T, 2, 1>& c) {
  // Make sure computations have exactly the same order of operations for both
  // triangles of a shared edge.  This guarantees the same rounding behavior for
  // a consistent fill rule.
  if (a.x() < b.x() || (a.x() == b.x() && a.x() < b.x())) {
    return (a.x() - b.x())*(c.y() - a.y()) - (a.y() - b.y())*(c.x() - a.x());
  } else {
    return -((b.x() - a.x())*(c.y() - b.y()) - (b.y() - a.y())*(c.x() - b.x()));
  }
}

inline bool IsTopLeft(const Vector2FP& a, const Vector2FP& b) {
  return (a.y() == b.y() && a.x() > b.x()) || (a.y() < b.y());
}
}

RastaManRenderer::RastaManRenderer(std::shared_ptr<RenderTarget> rt)
  : modelViewMatrix_(Matrix4f::Identity()),
    projectionMatrix_(Matrix4f::Identity()),
    modelViewProjectionMatrix_(Matrix4f::Identity()),
    renderTarget_(rt) {
}

RastaManRenderer::~RastaManRenderer() {
}

void RastaManRenderer::Clear(const float clearColor[4]) {
  renderTarget_->GetBackBuffer()->Clear(Vector4f(clearColor));
  renderTarget_->GetZBuffer()->Clear(1.f);
}

void RastaManRenderer::SetModelViewMatrix(const Matrix4f& matrix) {
  modelViewMatrix_ = matrix;
  modelViewProjectionMatrix_ = projectionMatrix_ * modelViewMatrix_;
}

void RastaManRenderer::SetProjectionMatrix(const Matrix4f& matrix) {
  projectionMatrix_ = matrix;
  modelViewProjectionMatrix_ = projectionMatrix_ * modelViewMatrix_;
}

void RastaManRenderer::SetRenderTarget(std::shared_ptr<RenderTarget> rt) {
  renderTarget_ = rt;
}

void RastaManRenderer::SetViewport(int x, int y, int width, int height) {
  viewport_ = AlignedBox<int, 2>(Vector2i(x, y),
                                 Vector2i(x + width - 1, y + height - 1));
  viewportScale_ = Vector3f(0.5f * width, -0.5f * height, 0.5f);
  viewportBias_ = Vector3f(0.5f * width + x, 0.5f * height + y, 0.5f);
}

void RastaManRenderer::DrawTriangles(const Eigen::Vector3f* vertices,
                                     const int* indices,
                                     int count) {
  for (int i = 0; i < count; i += 3) {
    DrawTriangle(
      (Vector4f() << vertices[indices[i]], 1.0f).finished(),
      (Vector4f() << vertices[indices[i+1]], 1.0f).finished(),
      (Vector4f() << vertices[indices[i+2]], 1.0f).finished()
    );
  }
}

Vector4f RastaManRenderer::ProcessVertex(const Vector4f& position) {
  return modelViewProjectionMatrix_ * position;
}

Vector4f RastaManRenderer::ProcessFragment(const Vector3f& position) {
  return Vector4f(1, 1, 1, 1);
}

void RastaManRenderer::RasterizeTriangle(
    const Vector3f& v0,
    const Vector3f& v1,
    const Vector3f& v2,
    std::function<void(const Vector3f&)> callback) {
  // Double triangle area for interpolation and backface culling
  const auto doubleArea = Orient2D<float>(v0.head<2>(), v1.head<2>(),
                                          v2.head<2>());

  // Backface culling
  if (doubleArea <= 0.0f) {
    return;
  }

  // Precomputation for z interpolation formula:
  // z = v0.z + w1/doubleArea*(v1.z-v0.z) + w2/doubleArea*(v2.z-v0.z)
  const float zz[3] = {
    v0.z(),
    (v1.z() - v0.z()) / doubleArea,
    (v2.z() - v0.z()) / doubleArea,
  };

  // Convert to fixed point
  const Vector2FP iv0 = (v0.head<2>() - Vector2f(0.5f, 0.5f)).cast<FP>();
  const Vector2FP iv1 = (v1.head<2>() - Vector2f(0.5f, 0.5f)).cast<FP>();
  const Vector2FP iv2 = (v2.head<2>() - Vector2f(0.5f, 0.5f)).cast<FP>();

  // Bounding box
  AlignedBox<int, 2> box;
  box.extend(Vector2i(iv0.x(), iv0.y()));
  box.extend(Vector2i(iv1.x(), iv1.y()));
  box.extend(Vector2i(iv2.x(), iv2.y()));
  box = box.intersection(viewport_);

  if (box.isEmpty()) {
    return;
  }

  // Bias for fill rule
  static const FP eps = std::numeric_limits<FP>::epsilon();
  const Vector3FP bias(
    IsTopLeft(iv1, iv2) ? FP(0) : eps,
    IsTopLeft(iv2, iv0) ? FP(0) : eps,
    IsTopLeft(iv0, iv1) ? FP(0) : eps);

  // Increments
  const Vector3FP pixelInc(
    iv2.y() - iv1.y(),
    iv0.y() - iv2.y(),
    iv1.y() - iv0.y());
  const Vector3FP rowInc(
    iv1.x() - iv2.x(),
    iv2.x() - iv0.x(),
    iv0.x() - iv1.x());

  // Base values
  Vector2i p(box.min());
  Vector3FP row(
    Orient2D<FP>(iv1, iv2, p.cast<FP>()),
    Orient2D<FP>(iv2, iv0, p.cast<FP>()),
    Orient2D<FP>(iv0, iv1, p.cast<FP>()));

  for (; p.y() <= box.max().y(); ++p.y()) {
    auto w = row;
    for (p.x() = box.min().x(); p.x() <= box.max().x(); ++p.x()) {
      if (w[0] >= bias[0] && w[1] >= bias[1] && w[2] >= bias[2]) {
        // p is in triangle
        const float z = v0.z() + w[1].GetAs<float>()*zz[1]
          + w[2].GetAs<float>()*zz[2];
        callback((Vector3f() << p.cast<float>(), z).finished());
      }
      w += pixelInc;
    }
    row += rowInc;
  }
}

void RastaManRenderer::DrawTriangle(const Vector4f& v0,
                                    const Vector4f& v1,
                                    const Vector4f& v2) {
  auto normal =
    (v1 - v0).head<3>().cross((v2 - v0).head<3>()).normalized();
  normal = normal*.5f + Vector3f::Constant(.5f);

  /*
   * Projection
   */
  const Vector4f clip[3] = {
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
  const Vector4f ndc[3] = {
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
  RasterizeTriangle(c[0], c[1], c[2], [&] (const Vector3f& fragment) {
    Vector2i pixel = fragment.head<2>().cast<int>();
    float& zBuffer = (*renderTarget_->GetZBuffer())(pixel.x(), pixel.y());

    if (fragment.z() < zBuffer && 0 <= fragment.z() && fragment.z() <= 1) {
      Vector4f& backBuffer = (*renderTarget_->GetBackBuffer())(pixel.x(),
                                                               pixel.y());
      const auto color = ProcessFragment(fragment);
      //backBuffer = color;
      backBuffer = (Vector4f() << normal, 1.0f).finished();
      zBuffer = fragment.z();
    }
  });
}
