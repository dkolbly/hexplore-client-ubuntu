#ifndef _H_HEXCOM_ICO
#define _H_HEXCOM_ICO

#include <glm/glm.hpp>

struct Icosahedron {
  glm::vec3     vertex[4*3];
  glm::vec3     normal[20];
  glm::vec3     center[20];
  static unsigned char index[20][3];
  Icosahedron();
};

extern Icosahedron ico;

#endif /* _H_HEXCOM_ICO */
