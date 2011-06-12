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

const int width = 512;
const int height = 512;

std::vector<Vector3f> vertices;
std::vector<int> indices;
Matrix4f modelView;
boost::shared_ptr<RenderTarget> rt(new RenderTarget(width, height));

enum {
  RM_OPENGL,
  RM_RASTAMAN,
  RM_DIFFERENCE
} renderMode = RM_OPENGL;

const int kRendererCount = 2;
boost::scoped_ptr<IRenderer> renderer[kRendererCount];

int frames = 0;
int fps = -1;

void resize(int width, int height) {
  for (int i = 0; i < kRendererCount; ++i) {
    renderer[i]->SetViewport(0, 0, width, height);
  }
}

void keyboard(unsigned char key, int x, int y) {
  if (key == 'o') {
    renderMode = RM_OPENGL;
    glutPostRedisplay();
  } else if (key == 'r') {
    renderMode = RM_RASTAMAN;
    glutPostRedisplay();
  } else if (key == 'd') {
    renderMode = RM_DIFFERENCE;
    glutPostRedisplay();
  } else if (key == 27) {
    glutLeaveMainLoop();
  }
}

void timer(int) {
  fps = frames;
  frames = 0;
  glutTimerFunc(1000, timer, 0);
}

void update() {
  Matrix4f rotation;
  rotation << AngleAxisf(.003f, Vector3f::UnitY()).matrix(),
              Vector3f::Zero(), RowVector4f::UnitW();
  modelView *= rotation;
  glutPostRedisplay();
}

void drawScene(IRenderer* renderer) {
  const float clearColor[4] = { 0, 0, 0, 0 };
  renderer->Clear(clearColor);
  renderer->SetModelViewMatrix(modelView);
  renderer->DrawTriangles(&vertices[0], &indices[0], indices.size());
}

void render() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (renderMode == RM_OPENGL || renderMode == RM_DIFFERENCE) {
    drawScene(renderer[0].get());
  }

  glDisable(GL_DEPTH_TEST);
  if (renderMode == RM_RASTAMAN || renderMode == RM_DIFFERENCE) {
    drawScene(renderer[1].get());

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRasterPos2f(-1.f, 1.f);
    glPixelZoom(1.f, -1.f);
    if (renderMode == RM_DIFFERENCE) {
      glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
      glBlendFunc(GL_ONE, GL_ONE);
      glEnable(GL_BLEND);
    }
    glDrawPixels(width, height, GL_RGBA, GL_FLOAT,
      reinterpret_cast<const GLvoid*>(rt->GetBackBuffer()->GetPixels()));
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
  Vector3f mid = (max + min) * 0.5f;
  std::cout << "Mid: (" << mid.x() << " " << mid.y() << " " << mid.z() << ")" << std::endl;
  const float scale = 1.9f / (std::max(extents[0], extents[1]));
  std::cout << "Scale: " << scale << std::endl;
  modelView << scale, 0.0f, 0.0f, -scale*mid.x(),
               0.0f, scale, 0.0f, -scale*mid.y(),
               0.0f, 0.0f, scale, 0.0f,
               0.0f, 0.0f, 0.0f, 1.0f;

  glutInit(&argc, argv);
  glutInitWindowSize(width, height);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
  glutCreateWindow("RastaMan");
  glutDisplayFunc(render);
  glutReshapeFunc(resize);
  glutKeyboardFunc(keyboard);
  glutIdleFunc(update);
  glutTimerFunc(1000, timer, 0);

  glewInit();

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CW);

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

