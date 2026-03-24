#pragma once

#include <ctime>
#include <cstring>
#include <vector>
#include <sstream>
#include <string>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include <GL/glew.h>

#define _USE_MATH_DEFINES
#include <cmath>

// Compatibility typedefs for Windows types used throughout the codebase
#ifndef _WIN32
typedef unsigned int UINT;
typedef unsigned char BYTE;
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#endif

using namespace std;
