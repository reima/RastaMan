#include "RenderPipeline.hpp"
#include "RenderSurface.hpp"

#include <cmath>
#include <fstream>
#include <iostream>

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

std::vector<Vector3f> vertices;
std::vector<int> indices;

const int width = 1024;
const int height = 1024;

#include <GL/glut.h>

void resize(int width, int height) {
  glViewport(0, 0, width, height);
}

void render() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glBegin(GL_TRIANGLES);
  for (int i = 0; i < indices.size(); ++i) {
    glVertex3fv(vertices[indices[i]].data());
  }
  glEnd();

  boost::scoped_array<char> data(new char[width*height*3]);
  glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data.get());

  std::ofstream ofs("imageGL.ppm");
  ofs << "P6\n";
  ofs << width << " " << height << "\n";
  ofs << 255 << "\n";
  for (int y = height - 1; y >= 0; --y) {
    ofs.write(&data[width*y*3], width*3);
  }

  exit(0);
}

int main(int argc, char* argv[]) {
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
  Matrix4f modelView;
  modelView << scale, 0.0f, 0.0f, -scale*mid.x(),
               0.0f, scale, 0.0f, -scale*mid.y(),
               0.0f, 0.0f, scale, 0.0f,
               0.0f, 0.0f, 0.0f, 1.0f;


  if (argc > 2) {
    glutInit(&argc, argv);
    glutInitWindowSize(width, height);
    glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE | GLUT_DEPTH);
    glutCreateWindow("OpenGL ground trouth");
    glutDisplayFunc(render);
    glutReshapeFunc(resize);

    glClearColor(0, 0, 0, 0);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(modelView.data());

    glutMainLoop();
  } else {
    RenderPipeline pipeline;
    boost::shared_ptr<RenderSurface> surface(new RenderSurface(width, height));
    surface->Clear(Vector4f(0, 0, 0, 0));

    pipeline.SetRenderSurface(surface);
    pipeline.SetViewport(0, 0, width, height);

    pipeline.SetModelViewMatrix(modelView);

//    for (int j = 0; j < 100; ++j) {
      for (int i = 0; i < indices.size(); i += 3) {
        pipeline.DrawTriangle(
          (Vector4f() << vertices[indices[i]], 1.0f).finished(),
          (Vector4f() << vertices[indices[i+1]], 1.0f).finished(),
          (Vector4f() << vertices[indices[i+2]], 1.0f).finished()
        );
      }
//    }

    std::ofstream ofs("image.ppm");
    ofs << "P6\n";
    ofs << surface->GetWidth() << " " << surface->GetHeight() << "\n";
    ofs << 255 << "\n";
    for (int y = 0; y < surface->GetHeight(); ++y) {
      for (int x = 0; x < surface->GetWidth(); ++x) {
        Matrix<unsigned char, 3, 1> col =
          ((*surface)(x, y).head<3>() * 255).eval().cast<unsigned char>();
        ofs.write(reinterpret_cast<char *>(col.data()), 3);
      }
    }
  }

  return 0;
}

