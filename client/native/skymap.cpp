#include <GL/gl.h>
//#include <GL/glu.h>
#include <glm/ext.hpp>
#include "skymap.h"
#include <hexcom/hex.h>
#include <hexcom/randompixel.h>

double home_period = 200;                       // year in # of days
//double galactic_period = home_period*200;       // galactic year in # of days
double galactic_period = 2;       // galactic year in # of days



#define PI2  (2*M_PI)

#define SKYMAP_REZ      (1024)

struct SkyMapCube {
  unsigned char texdata[6][SKYMAP_REZ][SKYMAP_REZ][4];
};

struct ColorProfile {
  ColorProfile(glm::vec3 const&a,
               glm::vec3 const&b,
               glm::vec3 const&c,
               glm::vec3 const&d,
               glm::vec3 const&e) {
    color[0] = a;
    color[1] = b;
    color[2] = c;
    color[3] = d;
    color[4] = e;
  }

  glm::vec3 color[5];
  // x goes from 0.000 to 5.000
  glm::vec3 get(float x) const;
};


glm::vec3 ColorProfile::get(float x) const
{
  int index = floor(x);

  if (index >= 4) {
    return color[4];
  } else if (index < 0) {
    return color[0];
  } else {
    float r = (cos((x - index) * PI) + 1)/2.0;
    return color[index] * r + color[index+1] * (1-r);
  }
}

static const glm::vec3 sunset_orange(0xFC/255.0, 0x43/255.0, 0x0B/255.0);
static const glm::vec3 sunset_blue(0x5B/255.0, 0x43/255.0, 0x8F/255.0);
static const glm::vec3 sunset_dark(0x23/255.0, 0x23/255.0, 0x4F/255.0);
static const glm::vec3 sunset_black(0, 0, 0.01);

static const ColorProfile dawn(sunset_orange,
                               sunset_blue,
                               sunset_dark,
                               sunset_black,
                               sunset_black);

GLuint ui_skymap_create(ClientOptions const& opt, double solar_time)
{
  glEnable(GL_TEXTURE_CUBE_MAP);
  //glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

  GLuint id;
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_CUBE_MAP, id);

  SkyMapCube *cube = (SkyMapCube*)malloc(sizeof(SkyMapCube));
  memset(&cube->texdata[0], 0, sizeof(cube->texdata));

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  static GLuint face_id[6] = { GL_TEXTURE_CUBE_MAP_POSITIVE_X,
                               GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
                               GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
                               GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };

  glm::vec3 home_planet_in_galaxy(10000, 30000, 9000);

  // load stars
  FILE *starf = fopen("textures/galaxy.dat", "r");
  if (starf) {
    // load stars
    char line[100];
    while (fgets(line, sizeof(line), starf)) {
      unsigned id;
      double sx, sy, sz;
      char spectral_type;
      double abs_mag;
      if (sscanf(line, "star %u %lg %lg %lg %c %lg\n", &id, &sx, &sy, &sz, &spectral_type, &abs_mag) == 6) {
        glm::vec3 star(sx, sy, sz);
        star += home_planet_in_galaxy;

        float ax = fabs(star.x);
        float ay = fabs(star.y);
        float az = fabs(star.z);
        //float dx, dy;
        int face;
        /* See
         *    http://www.arcsynthesis.org/gltut/Texturing/Tut17%20Pointing%20Projections.html
         * for the secret decoder ring on cube maps
         *
         * Here's the executive summary:
         *     -----map------       --x--   --y--
         *     Positive X map        -Z      -Y
         *     Negative X map        +Z      -Y
         *     Positive Y map        +X      +Z
         *     Negative Y map        +X      -Z
         *     Positive Z map        +X      -Y
         *     Negative Z map        -X      -Y
         */

        float s, t;
        if (ax > ay) {
          if (ax > az) {
            // X map
            face = (star.x >= 0) ? 0 : 1;
            s = -star.z / ax;
            t = -star.y / ax;
            if (star.x < 0) {
              s = -s;
            }
          } else {
            goto z_map;
          }
        } else if (ay > az) {
          // Y map
          face = (star.y >= 0) ? 2 : 3;
          s = star.x / ay;
          t = star.z / ay;
          if (star.y < 0) {
            t = -t;
          }
        } else {
          // Z map
        z_map:
          face = (star.z >= 0) ? 4 : 5;
          s = star.x / az;
          t = -star.y / az;
          if (star.z < 0) {
            s = -s;
          }
        }
        int is = s * (SKYMAP_REZ/2) + SKYMAP_REZ/2;
        int it = t * (SKYMAP_REZ/2) + SKYMAP_REZ/2;
        if (is < 0) { is = 0; } else if (is >= SKYMAP_REZ) { is = SKYMAP_REZ-1; }
        if (it < 0) { it = 0; } else if (it >= SKYMAP_REZ) { it = SKYMAP_REZ-1; }
        int brightness = 255;
        int r = brightness, g = brightness, b = brightness;

        cube->texdata[face][it][is][0] = r;
        cube->texdata[face][it][is][1] = g;
        cube->texdata[face][it][is][2] = b;
        cube->texdata[face][it][is][3] = 255;
      }
    }
    fclose(starf);
  }
  for (int f=0; f<6; f++) {
    glTexImage2D(face_id[f], 0, GL_RGBA, 
                 SKYMAP_REZ,
                 SKYMAP_REZ,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 &cube->texdata[f][0][0][0]);
  }
  free(cube);
  glDisable(GL_TEXTURE_CUBE_MAP);
  return id;
}

/**
 *    solar_time = days since epoch
 *    home_period = days in year
 *    galactic_period = days in galactic orbit
 */

void SkyView::update(glm::dvec3 const& posn, double solar_time)
{
  float longitude = posn[0] * longitude_radians_per_x;

  location = posn;
  time_of_year = fmod(solar_time / home_period, 1.0);
  time_of_day = fmod(solar_time, 1.0);

  //fprintf(stderr, "time of year: %.3f\n", time_of_year);
  time_of_year *= PI2;

  // dvec2's are in solar system coordinates,
  //
  //                  ^
  //                 y|
  //                  |
  //                  |              ..O
  //                  |           ...
  //                  |        ... ^
  //                  |     ...     \  planet angle ~= time of year
  //                 *** ...         \     distance ::= 1.0 AU
  //                *sun*--------------->
  //                 ***                x
  //
  //
  home_planet.x = cos(time_of_year);
  home_planet.y = sin(time_of_year);
  //fprintf(stderr, "home planet at %.3f %.3f\n", home.x, home.y);
  up.x = cos(longitude + time_of_day * PI2);
  up.y = sin(longitude + time_of_day * PI2);
  //fprintf(stderr, "facing up is %.3f %.3f\n", up.x, up.y);

  //fprintf(stderr, "sun %.3f  ==> %.3f\n", d, acos(d) * 180/PI);
  glm::dvec2 east(-up.y, up.x);

  // Figure out the sun position in local coordinates (up=up, east=facing east)
#if 0
  sun.x = glm::dot(-home_planet, east);
  sun.y = 0;
  sun.z = glm::dot(-home_planet, up);
#endif
  sun.x = up.y;
  sun.y = 0;
  sun.z = -up.x;

  // Figure out the direction to the center of the galaxy in local coordinates
  //glm::vec3 galaxy_

  // this is a little buggy because it neglects the Z coordinate of the
  // solar system in galactic coordinates, and aligns the sun's coordinate
  // system to the galaxy's
  glm::vec3 up_in_galaxy(up.x, up.y, 0);
  glm::vec3 east_in_galaxy(east.x, east.y, 0);
  glm::vec3 west_in_galaxy(glm::cross(up_in_galaxy, east_in_galaxy));
  
  starMatrix[0][0] = east_in_galaxy.x;
  starMatrix[0][1] = east_in_galaxy.y;
  starMatrix[0][2] = east_in_galaxy.z;
  starMatrix[0][3] = 0;

  starMatrix[1][0] = west_in_galaxy.x;
  starMatrix[1][1] = west_in_galaxy.y;
  starMatrix[1][2] = west_in_galaxy.z;
  starMatrix[1][3] = 0;

  starMatrix[2][0] = up_in_galaxy.x;
  starMatrix[2][1] = up_in_galaxy.y;
  starMatrix[2][2] = up_in_galaxy.z;
  starMatrix[2][3] = 0;

  starMatrix[3] = glm::vec4(0,0,0,1);
}
