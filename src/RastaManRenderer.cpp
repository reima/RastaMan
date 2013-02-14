#include "RastaManRenderer.hpp"
#include "RenderSurface.hpp"

using namespace Eigen;

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

inline void SetupEdgeEquation(const Vector3f& v1, const Vector3f& v2,
                              float* a, float* b, float* c) {
  *a = v2.y() - v1.y();
  *b = v1.x() - v2.x();
  *c = (*a * (v1.x() + v2.x()) + *b * (v1.y() + v2.y())) * -.5f;
}

void RastaManRenderer::RasterizeTriangle(
    const Vector3f& v0,
    const Vector3f& v1,
    const Vector3f& v2,
    std::function<void(const Vector3f&)> callback) {
  // Bounding box
  AlignedBox<int, 2> box(v0.head<2>().cast<int>());
  box.extend(v1.head<2>().cast<int>())
     .extend(v2.head<2>().cast<int>());

  box = box.intersection(viewport_);
  if (box.isEmpty()) return;

  // Edge equations setup
  Array4f a, b, c;
  SetupEdgeEquation(v0, v1, &a[0], &b[0], &c[0]);
  SetupEdgeEquation(v1, v2, &a[1], &b[1], &c[1]);
  SetupEdgeEquation(v2, v0, &a[2], &b[2], &c[2]);

  // Cull back faces
  const auto doubleArea = c[0] + c[1] + c[2];
  if (doubleArea <= 0) return;

  // Interpolation equation setup
  const auto doubleAreaInv = 1.f/doubleArea;
  a[3] = (v0.z()*a[1] + v1.z()*a[2] + v2.z()*a[0])*doubleAreaInv;
  b[3] = (v0.z()*b[1] + v1.z()*b[2] + v2.z()*b[0])*doubleAreaInv;
  c[3] = (v0.z()*c[1] + v1.z()*c[2] + v2.z()*c[0])*doubleAreaInv;

  // Encode half pixel offsets in c
  c.head<3>() += (a.head<3>() + b.head<3>()) * .5f;

  for (auto y = box.min().y(); y <= box.max().y(); ++y) {
    for (auto x = box.min().x(); x <= box.max().x(); ++x) {
      const Vector4f e = a*static_cast<float>(x) + b*static_cast<float>(y) + c;
      if (e[0] > 0 && e[1] > 0 && e[2] > 0) {
        const auto z = e[3];
        callback(Vector3f(static_cast<float>(x), static_cast<float>(y), z));
      }
    }
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
    float& zBuffer = (*renderTarget_->GetZBuffer())(
      static_cast<int>(fragment.x()), static_cast<int>(fragment.y()));

    if (fragment.z() < zBuffer && 0 <= fragment.z() && fragment.z() <= 1) {
      const Vector4f color = ProcessFragment(fragment);
      (*renderTarget_->GetBackBuffer())(
        static_cast<int>(fragment.x()), static_cast<int>(fragment.y())) =
          // color;
          (Vector4f() << normal, 1.f).finished();
      zBuffer = fragment.z();
    }
  });
}
