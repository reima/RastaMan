#include "OpenGLRenderer.hpp"
#include "RastaManRenderer.hpp"
#include "Font.hpp"

#include "Eigen/Geometry"

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>

using namespace Eigen;

void LoadSimpleObj(const char* filename,
                   std::vector<Vector3f>& vertices,
                   std::vector<int>& indices,
                   Vector3f& min, Vector3f& max) {
  std::ifstream ifs(filename);
  std::string line;
  while (std::getline(ifs, line)) {
    if (line[0] == '#') continue;
    std::stringstream ss(line);
    std::string keyword;
    ss >> keyword;
    if (keyword == "v") {
      Eigen::Vector3f vec;
      ss >> vec.x() >> vec.y() >> vec.z();
      if (vertices.empty()) {
        min = max = vec;
      } else {
        min = min.cwiseMin(vec);
        max = max.cwiseMax(vec);
      }
      vertices.push_back(vec);
    } else if (keyword == "f") {
      int idx[3];
      for (int i = 0; i < 3; ++i) {
        ss >> idx[i];
        ss.ignore(1000, ' ');
        indices.push_back(idx[i] - 1);
      }
      idx[1] = idx[2];
      while (ss >> idx[2]) {
        ss.ignore(1000, ' ');
        indices.push_back(idx[0] - 1);
        indices.push_back(idx[1] - 1);
        indices.push_back(idx[2] - 1);
        idx[1] = idx[2];
      }
    }
  }
}

int width = 512;
int height = 512;

std::vector<Vector3f> vertices;
std::vector<int> indices;

Matrix4f projectionMatrix;
Matrix4f modelMatrix;
Matrix4f viewMatrix;
Matrix4f modelViewMatrix;

Quaternionf rotation(Quaternionf::Identity());
Vector3f translation(0.0f, 0.0f, -2.0f);

std::shared_ptr<RenderTarget> rt;
const int kRendererCount = 2;
std::unique_ptr<IRenderer> renderers[kRendererCount];

std::chrono::high_resolution_clock::time_point lastFrameTime;
std::unique_ptr<Font> font;

enum RenderMode {
  RM_OPENGL,
  RM_RASTAMAN,
  RM_DIFFERENCE
};

RenderMode renderMode = RM_OPENGL;

Matrix4f getPerspectiveMatrix(float fieldOfView, float aspect, float zNear,
                              float zFar) {
  const float f = 1.f/std::tan(fieldOfView/180.f * 3.14159265f * .5f);
  return (Matrix4f() <<
    f/aspect, 0, 0, 0,
    0, f, 0, 0,
    0, 0, (zFar + zNear)/(zNear - zFar), 2*zFar*zNear/(zNear - zFar),
    0, 0, -1, 0).finished();
}

Matrix4f getOrthographicMatrix(float left, float right, float bottom, float top,
                               float zNear, float zFar) {
  return (Matrix4f() <<
    2.0f/(right-left), 0, 0, -(right+left) / (right-left),
    0, 2.0f/(top-bottom), 0, -(top+bottom) / (top-bottom),
    0, 0, -2.0f/(zFar-zNear), -(zFar+zNear) / (zFar-zNear),
    0, 0, 0, 1).finished();
}

void resize(GLFWwindow* window, int width, int height) {
  ::width = width;
  ::height = height;
  for (auto& renderer : renderers) {
    renderer->SetViewport(0, 0, width, height);
  }
  rt.reset(new RenderTarget(width, height));
  dynamic_cast<RastaManRenderer*>(renderers[1].get())->SetRenderTarget(rt);

  const auto aspect = static_cast<float>(width)/height;
  projectionMatrix = getPerspectiveMatrix(60.0f, aspect, 0.1f, 100.0f);
  //projectionMatrix =
  //  getOrthographicMatrix(-aspect, aspect, -1.0f, 1.0f, 0.0f, 10.0f);
}

void special(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (action != GLFW_PRESS && action != GLFW_REPEAT) {
    return;
  }

  switch (key) {
    case GLFW_KEY_ESCAPE:
      glfwSetWindowShouldClose(window, GLFW_TRUE);
      break;
    case GLFW_KEY_LEFT:
      rotation = Quaternionf(AngleAxisf(-.05f, Vector3f::UnitY())) * rotation;
      break;
    case GLFW_KEY_RIGHT:
      rotation = Quaternionf(AngleAxisf(.05f, Vector3f::UnitY())) * rotation;
      break;
    case GLFW_KEY_UP:
      rotation = Quaternionf(AngleAxisf(-.05f, Vector3f::UnitX())) * rotation;
      break;
    case GLFW_KEY_DOWN:
      rotation = Quaternionf(AngleAxisf(.05f, Vector3f::UnitX())) * rotation;
      break;
    case GLFW_KEY_PAGE_UP:
      translation.z() /= 1.05f;
      break;
    case GLFW_KEY_PAGE_DOWN:
      translation.z() *= 1.05f;
      break;
    case GLFW_KEY_O:
      renderMode = RM_OPENGL;
      break;
    case GLFW_KEY_R:
      renderMode = RM_RASTAMAN;
      break;
    case GLFW_KEY_X:
      renderMode = RM_DIFFERENCE;
      break;
    case GLFW_KEY_W:
      translation.y() += 0.0625f;
      break;
    case GLFW_KEY_S:
      translation.y() -= 0.0625f;
      break;
    case GLFW_KEY_A:
      translation.x() -= 0.0625f;
      break;
    case GLFW_KEY_D:
      translation.x() += 0.0625f;
      break;
  }
}

void update() {
  Affine3f viewTransform;
  viewTransform = Translation3f(translation) * rotation;
  viewMatrix = viewTransform.matrix();
  modelViewMatrix = viewMatrix * modelMatrix;
}

void drawScene(IRenderer* renderer) {
  const float clearColor[4] = { 0, 0, 0, 0 };
  renderer->Clear(clearColor);
  renderer->SetProjectionMatrix(projectionMatrix);
  renderer->SetModelViewMatrix(modelViewMatrix);
  renderer->DrawTriangles(&vertices[0], &indices[0], indices.size());
}

void drawOverlay(GLFWwindow* window) {
  const auto now = std::chrono::high_resolution_clock::now();
  const auto elapsedMillis =
    std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrameTime)
    .count();
  lastFrameTime = now;

  std::stringstream ss;
  switch (renderMode) {
    case RM_OPENGL: ss << "OpenGL"; break;
    case RM_RASTAMAN: ss << "RastaMan"; break;
    case RM_DIFFERENCE: ss << "Diff"; break;
  }
  ss << " " << elapsedMillis << "ms";

  glDisable(GL_DEPTH_TEST);
  glColor3f(1.0f, 1.0f, 1.0f);
  int y = height;
  font->Draw(ss.str().c_str(), 0, y -= font->GetLineHeight());

  Vector2d mouse;
  glfwGetCursorPos(window, &mouse.x(), &mouse.y());
  if (mouse.x() < 0 || mouse.x() >= width || mouse.y() < 0 || mouse.y() >= height) {
    return;
  }

  ss.str("");
  ss << "Position: (" << mouse << ")";
  font->Draw(ss.str().c_str(), 0, y -= font->GetLineHeight());
  
  const auto color = (*rt->GetBackBuffer())(mouse.x(), mouse.y());
  ss.str("");
  ss << "Color: (" << color << ")";
  font->Draw(ss.str().c_str(), 0, y -= font->GetLineHeight());

  const auto z = (*rt->GetZBuffer())(mouse.x(), mouse.y());
  ss.str("");
  ss << "Depth: " << z;
  font->Draw(ss.str().c_str(), 0, y -= font->GetLineHeight());
}

void render(GLFWwindow* window) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (renderMode == RM_OPENGL || renderMode == RM_DIFFERENCE) {
    glEnable(GL_DEPTH_TEST);
    drawScene(renderers[0].get());
  }

  glDisable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  if (renderMode == RM_RASTAMAN || renderMode == RM_DIFFERENCE) {
    drawScene(renderers[1].get());

    glRasterPos2f(-1.f, 1.f);
    glPixelZoom(1.f, -1.f);
    if (renderMode == RM_DIFFERENCE) {
      glLogicOp(GL_XOR);
      glEnable(GL_COLOR_LOGIC_OP);
    }
    glDrawPixels(rt->GetBackBuffer()->GetWidth(),
                 rt->GetBackBuffer()->GetHeight(), GL_RGBA, GL_FLOAT,
                 reinterpret_cast<const GLvoid*>(
                   rt->GetBackBuffer()->GetPixels()));
    glDisable(GL_COLOR_LOGIC_OP);
  }

  drawOverlay(window);
  
  //boost::scoped_array<char> data(new char[width*height*3]);
  //glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data.get());

  //std::ofstream ofs("image.ppm");
  //ofs << "P6\n";
  //ofs << width << " " << height << "\n";
  //ofs << 255 << "\n";
  //for (int y = height - 1; y >= 0; --y) {
  //  ofs.write(&data[width*y*3], width*3);
  //}
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Missing argument." << std::endl;
    return 1;
  }

  Vector3f min, max;
  LoadSimpleObj(argv[1], vertices, indices, min, max);
  std::cout << vertices.size() << " vertices, "
            << indices.size()/3 << " faces" << std::endl;
  std::cout << "Bounds: (" << min.x() << " " << min.y() << " " << min.z()
            << ")-(" << max.x() << " " << max.y() << " " << max.z() << ")"
            << std::endl;

  Vector3f extents = max - min;
  const float scale = 1.f / extents.maxCoeff();
  Vector3f mid = (max + min) * .5f;
  modelMatrix = (Scaling(scale) * Translation3f(-mid)).matrix();

  renderers[0].reset(new OpenGLRenderer());

  rt.reset(new RenderTarget(width, height));
  renderers[1].reset(new RastaManRenderer(rt));

  if (!glfwInit()) {
    std::cerr << "Error initializing GLEW" << std::endl;
    return 1;
  }

  const auto window = glfwCreateWindow(width, height, "RastaMan", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    std::cerr << "Error opening window" << std::endl;
    return 1;
  }

  glfwMakeContextCurrent(window);

  glfwSetKeyCallback(window, special);
  glfwSetWindowSizeCallback(window, resize);
  glfwSwapInterval(0);

  glewInit();

  font.reset(new Font);

  GLint subpixelBits = 0;
  glGetIntegerv(GL_SUBPIXEL_BITS, &subpixelBits);
  std::cerr << subpixelBits << std::endl;

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);

  bool running = true;

  while (!glfwWindowShouldClose(window)) {
    update();
    render(window);
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();

  //std::ofstream ofs("image.ppm");
  //ofs << "P6\n";
  //ofs << surface->GetWidth() << " " << surface->GetHeight() << "\n";
  //ofs << 255 << "\n";
  //for (int y = 0; y < surface->GetHeight(); ++y) {
  //  for (int x = 0; x < surface->GetWidth(); ++x) {
  //    Matrix<unsigned char, 3, 1> col =
  //      ((*surface)(x, y).head<3>() * 255).eval().cast<unsigned char>();
  //    ofs.write(reinterpret_cast<char *>(col.data()), 3);
  //  }
  //}

  return 0;
}

