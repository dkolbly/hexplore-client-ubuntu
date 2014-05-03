#ifndef _H_HEXPLORE_CLIENT_SKYMAP
#define _H_HEXPLORE_CLIENT_SKYMAP

#include <GL/gl.h>
#include <glm/glm.hpp>
#include "clientoptions.h"

struct SkyView {
  glm::vec3     location;
  glm::vec3     sun;
  glm::dvec2    up;
  glm::dvec2    home_planet;
  glm::mat4     starMatrix;
  
  double        time_of_day;
  double        time_of_year;

  void update(glm::dvec3 const& posn, double solar_time);
};

GLuint ui_skymap_create(ClientOptions const& opt, double solar_time);

#endif /* _H_HEXPLORE_CLIENT_SKYMAP */
