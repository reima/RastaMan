#include "RastaManRenderer.hpp"
#include "RenderSurface.hpp"

using namespace Eigen;

RastaManRenderer::RastaManRenderer(boost::shared_ptr<RenderTarget> rt)
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

void RastaManRenderer::SetRenderTarget(boost::shared_ptr<RenderTarget> rt) {
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

void RastaManRenderer::RasterizeTriangle(const Vector3f& v0,
                                         const Vector3f& v1,
                                         const Vector3f& v2,
                                         std::vector<Vector3f>& fragments) {
  // Bounding box
  AlignedBox<int, 2> box(v0.head<2>().cast<int>());
  box.extend(v1.head<2>().cast<int>())
     .extend(v2.head<2>().cast<int>());

  box = box.intersection(viewport_);
  if (box.isEmpty()) return;

  // Edge equations setup
  float a0, b0, c0;
  a0 = v0.y() - v1.y();
  b0 = v1.x() - v0.x();
  c0 = (a0*(v0.x() + v1.x()) + b0*(v0.y() + v1.y())) * -.5f;
  float a1, b1, c1;
  a1 = v1.y() - v2.y();
  b1 = v2.x() - v1.x();
  c1 = (a1*(v1.x() + v2.x()) + b1*(v1.y() + v2.y())) * -.5f;
  float a2, b2, c2;
  a2 = v2.y() - v0.y();
  b2 = v0.x() - v2.x();
  c2 = (a2*(v0.x() + v2.x()) + b2*(v0.y() + v2.y())) * -.5f;

  // Cull back faces
  const float doubleArea = c0 + c1 + c2;
  if (doubleArea <= 0) return;

  // Interpolation equation setup
  const float doubleAreaInv = 1.f/doubleArea;
  float az = (v0.z()*a1 + v1.z()*a2 + v2.z()*a0)*doubleAreaInv;
  float bz = (v0.z()*b1 + v1.z()*b2 + v2.z()*b0)*doubleAreaInv;
  float cz = (v0.z()*c1 + v1.z()*c2 + v2.z()*c0)*doubleAreaInv;

  for (int y = box.min().y(); y <= box.max().y(); ++y) {
    for (int x = box.min().x(); x <= box.max().x(); ++x) {
      float e0 = a0*(x+.5f) + b0*(y+.5f) + c0;
      float e1 = a1*(x+.5f) + b1*(y+.5f) + c1;
      float e2 = a2*(x+.5f) + b2*(y+.5f) + c2;
      if (e0 > 0 && e1 > 0 && e2 > 0) {
        float z = az*(x+.5f) + bz*(y+.5f) + cz;
        fragments.push_back(Vector3f(x, y, z));
      }
    }
  }
}

void RastaManRenderer::DrawTriangle(const Vector4f& v0,
                                    const Vector4f& v1,
                                    const Vector4f& v2) {
  Vector3f normal =
    (v1 - v0).head<3>().cross((v2 - v0).head<3>()).normalized();
  normal = normal*.5f + Vector3f::Constant(.5f);
  
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
  std::vector<Vector3f> fragments;
  RasterizeTriangle(c[0], c[1], c[2], fragments);

  for (std::vector<Vector3f>::iterator it = fragments.begin();
       it != fragments.end(); ++it) {
    float& zBuffer = (*renderTarget_->GetZBuffer())(it->x(), it->y());

    if (it->z() < zBuffer && 0 <= it->z() && it->z() <= 1) {
      Vector4f color = ProcessFragment(*it);
      (*renderTarget_->GetBackBuffer())(it->x(), it->y()) =// color;
        (Vector4f() << normal, 1.f).finished();
      zBuffer = it->z();
    }
  }
}
