#include "OpenGLRenderer.hpp"

#include <GL/glew.h>
#include <Eigen/Geometry>

using namespace Eigen;

OpenGLRenderer::OpenGLRenderer() {
}

OpenGLRenderer::~OpenGLRenderer() {
}

void OpenGLRenderer::Clear(const float clearColor[4]) {
  glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRenderer::SetModelViewMatrix(const Eigen::Matrix4f& m) {
  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf(m.data());
}

void OpenGLRenderer::SetProjectionMatrix(const Eigen::Matrix4f& m) {
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(m.data());
}

void OpenGLRenderer::SetViewport(int x, int y, int width, int height) {
  glViewport(x, y, width, height);
}

void OpenGLRenderer::DrawTriangles(const Vector3f* vertices,
                                   const int* indices,
                                   int count) {
  glBegin(GL_TRIANGLES);
  for (int i = 0; i < count; i += 3) {
    const Vector3f& v0 = vertices[indices[i]];
    const Vector3f& v1 = vertices[indices[i+1]];
    const Vector3f& v2 = vertices[indices[i+2]];
    Vector3f normal = (v1 - v0).cross(v2 - v0).normalized();
    normal = normal * .5f + Vector3f::Constant(.5f);
    glColor3fv(normal.data());
    glVertex3fv(v0.data());
    glVertex3fv(v1.data());
    glVertex3fv(v2.data());
  }
  glEnd();
}
