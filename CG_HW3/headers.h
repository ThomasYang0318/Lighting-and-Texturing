#ifndef HEADERS_H
#define HEADERS_H
//
// ---- OpenGL Loader + Windowing ----
#if defined(__APPLE__)
  // 如果你用 Homebrew 的 GLEW / freeglut，通常會是這樣
  #include <OpenGL/gl.h>
  #include <GLUT/glut.h>
  // 如果你遇到 deprecation 警告，可加：
  // #define GL_SILENCE_DEPRECATION
#else
  #include <GL/glew.h>
  #include <GL/freeglut.h>
#endif

/*
// OpenGL and FreeGlut headers.
#include <glew.h>
#include <freeglut.h>
*/

// GLM.
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// OpenCV.
#include <opencv2/opencv.hpp>

// C++ STL headers.
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <fstream>
#include <sstream>

#endif
