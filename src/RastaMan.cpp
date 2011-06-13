#include "OpenGLRenderer.hpp"
#include "RastaManRenderer.hpp"

#include <boost/scoped_ptr.hpp>

#include <cmath>
#include <fstream>
#include <iostream>

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <Eigen/Geometry>

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
      int idx[4];
      for (int i = 0; i < 3; ++i) {
        ss >> idx[i];
        indices.push_back(idx[i] - 1);
        ss.ignore(1000, ' ');
      }
      if (ss >> idx[3]) {
        // Split quad
        indices.push_back(idx[0] - 1);
        indices.push_back(idx[2] - 1);
        indices.push_back(idx[3] - 1);
      }
    }
  }
}

const int initialWidth = 512;
const int initialHeight = 512;

std::vector<Vector3f> vertices;
std::vector<int> indices;

Matrix4f projectionMatrix;
Matrix4f modelMatrix;
Matrix4f viewMatrix;
Matrix4f modelViewMatrix;

Quaternionf rotation(Quaternionf::Identity());
float camDistance = 2.f;

boost::shared_ptr<RenderTarget> rt(
  new RenderTarget(initialWidth, initialHeight));
const int kRendererCount = 2;
boost::scoped_ptr<IRenderer> renderer[kRendererCount];

enum {
  RM_OPENGL,
  RM_RASTAMAN,
  RM_DIFFERENCE
} renderMode = RM_OPENGL;

int frames = 0;
int fps = -1;

void resize(int width, int height) {
  for (int i = 0; i < kRendererCount; ++i) {
    renderer[i]->SetViewport(0, 0, width, height);
  }
  rt.reset(new RenderTarget(width, height));
  dynamic_cast<RastaManRenderer*>(renderer[1].get())->SetRenderTarget(rt);

  float fieldOfView = 60.f;
  float aspect = static_cast<float>(width)/height;
  float zNear = .1f;
  float zFar = 100.f;

  float f = 1.f/std::tan(fieldOfView/180.f * 3.14159265f * .5f);

  projectionMatrix <<
    f/aspect, 0, 0, 0,
    0, f, 0, 0,
    0, 0, (zFar + zNear)/(zNear - zFar), 2*zFar*zNear/(zNear - zFar),
    0, 0, -1, 0;
}

void keyboard(unsigned char key, int x, int y) {
  switch (key) {
    case 'o':
      renderMode = RM_OPENGL;
      break;
    case 'r':
      renderMode = RM_RASTAMAN;
      break;
    case 'd':
      renderMode = RM_DIFFERENCE;
      break;
    case '+':
      camDistance /= 1.05f;
      break;
    case '-':
      camDistance *= 1.05f;
      break;
    case 27:
      glutLeaveMainLoop();
      break;
  }
}

void specialKey(int key, int x, int y) {
  switch (key) {
    case GLUT_KEY_LEFT:
      rotation = Quaternionf(AngleAxisf(-.05f, Vector3f::UnitY())) * rotation;
      break;
    case GLUT_KEY_RIGHT:
      rotation = Quaternionf(AngleAxisf(.05f, Vector3f::UnitY())) * rotation;
      break;
    case GLUT_KEY_UP:
      rotation = Quaternionf(AngleAxisf(-.05f, Vector3f::UnitX())) * rotation;
      break;
    case GLUT_KEY_DOWN:
      rotation = Quaternionf(AngleAxisf(.05f, Vector3f::UnitX())) * rotation;
      break;
  }
}

void timer(int) {
  fps = frames;
  frames = 0;
  glutTimerFunc(1000, timer, 0);
}

void update() {
  Affine3f viewTransform;
  viewTransform = Translation3f(0, 0, -camDistance) * rotation;
  viewMatrix = viewTransform.matrix();
  modelViewMatrix = viewMatrix * modelMatrix;
  glutPostRedisplay();
}

void drawScene(IRenderer* renderer) {
  const float clearColor[4] = { 0, 0, 0, 0 };
  renderer->Clear(clearColor);
  renderer->SetProjectionMatrix(projectionMatrix);
  renderer->SetModelViewMatrix(modelViewMatrix);
  renderer->DrawTriangles(&vertices[0], &indices[0], indices.size());
}

void render() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (renderMode == RM_OPENGL || renderMode == RM_DIFFERENCE) {
    drawScene(renderer[0].get());
  }

  glDisable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  if (renderMode == RM_RASTAMAN || renderMode == RM_DIFFERENCE) {
    drawScene(renderer[1].get());

    glRasterPos2f(-1.f, 1.f);
    glPixelZoom(1.f, -1.f);
    if (renderMode == RM_DIFFERENCE) {
      glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
      glBlendFunc(GL_ONE, GL_ONE);
      glEnable(GL_BLEND);
    }
    glDrawPixels(rt->GetBackBuffer()->GetWidth(),
                 rt->GetBackBuffer()->GetHeight(), GL_RGBA, GL_FLOAT,
                 reinterpret_cast<const GLvoid*>(
                   rt->GetBackBuffer()->GetPixels()));
    glDisable(GL_BLEND);
  }

  const GLubyte* text;
  switch (renderMode) {
    case RM_OPENGL: text = (const GLubyte*)"OpenGL"; break;
    case RM_RASTAMAN: text = (const GLubyte*)"RastaMan"; break;
    case RM_DIFFERENCE: text = (const GLubyte*)"Difference"; break;
  }
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glRasterPos2i(-1, -1);
  glutBitmapString(GLUT_BITMAP_HELVETICA_10, text);

  char fpsText[100];
  sprintf(fpsText, " %d FPS", fps);
  glutBitmapString(GLUT_BITMAP_HELVETICA_10, (const GLubyte*)fpsText);

  glEnable(GL_DEPTH_TEST);

  //boost::scoped_array<char> data(new char[width*height*3]);
  //glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data.get());

  //std::ofstream ofs("image.ppm");
  //ofs << "P6\n";
  //ofs << width << " " << height << "\n";
  //ofs << 255 << "\n";
  //for (int y = height - 1; y >= 0; --y) {
  //  ofs.write(&data[width*y*3], width*3);
  //}

  ++frames;
  glutSwapBuffers();
}

int main(int argc, char* argv[]) {
  renderer[0].reset(new OpenGLRenderer());
  renderer[1].reset(new RastaManRenderer(rt));

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

  glutInit(&argc, argv);
  glutInitWindowSize(initialWidth, initialHeight);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
  glutCreateWindow("RastaMan");
  glutDisplayFunc(render);
  glutReshapeFunc(resize);
  glutKeyboardFunc(keyboard);
  glutSpecialFunc(specialKey);
  glutIdleFunc(update);
  glutTimerFunc(1000, timer, 0);

  glewInit();

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);

  glutMainLoop();

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

