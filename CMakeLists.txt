CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(RastaMan)

SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

FIND_PACKAGE(Boost 1.49 REQUIRED)
FIND_PACKAGE(Eigen3 REQUIRED)
FIND_PACKAGE(GLEW REQUIRED)
FIND_PACKAGE(GLFW REQUIRED)
FIND_PACKAGE(OpenGL REQUIRED)

SET(RastaMan_SOURCE
  src/FixedPoint.hpp
  src/Font.cpp
  src/Font.hpp
  src/IRenderer.hpp
  src/OpenGLRenderer.cpp
  src/OpenGLRenderer.hpp
  src/RastaMan.cpp
  src/RastaManRenderer.cpp
  src/RastaManRenderer.hpp
  src/RenderSurface.cpp
  src/RenderSurface.hpp
  src/RenderTarget.cpp
  src/RenderTarget.hpp
)

IF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  SET(CMAKE_CXX_FLAGS "-std=c++11")
ENDIF()

INCLUDE_DIRECTORIES(
  ${Boost_INCLUDE_DIR}
  ${EIGEN3_INCLUDE_DIR}
  ${GLEW_INCLUDE_DIR}
  ${GLFW_INCLUDE_DIR}
  ${OPENGL_INCLUDE_DIR}
)

ADD_EXECUTABLE(RastaMan ${RastaMan_SOURCE})
TARGET_LINK_LIBRARIES(RastaMan
  ${GLEW_LIBRARY}
  ${GLFW_LIBRARY}
  ${OPENGL_LIBRARIES}
)
