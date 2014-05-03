#include <stdlib.h>
#include "ui.h"
#include <hexcom/misc.h>
#include "world.h"
#include <hexcom/hex.h>
#include "wire/model.pb.h"

void show_axes(UserInterface *ui, glm::mat4 model);

#define TEXTURE_GRID_WIDTH      (16)
#define SIDE_TEXTURE_HEIGHT     (10)    // how many z-units the texture covers

  static inline double terrain_u(unsigned index) {
    return (index % TEXTURE_GRID_WIDTH) * (1.0 / TEXTURE_GRID_WIDTH);
  }
  static inline double terrain_v(unsigned index) {
    return (index / TEXTURE_GRID_WIDTH) * (1.0 / TEXTURE_GRID_WIDTH);
  }
  enum TextureIds {
    GRASSY_DIRT_TOP,
    GRASSY_DIRT_BOTTOM = GRASSY_DIRT_TOP,
    GRASSY_DIRT_SIDE_FIRST,
    GRASSY_DIRT_SIDE_REST,

    OAK_TREE_TOP,
    OAK_TREE_BOTTOM = OAK_TREE_TOP,
    OAK_TREE_SIDE_FIRST,
    OAK_TREE_SIDE_REST,

    ROCK_TOP = 16,
    ROCK_BOTTOM = ROCK_TOP,
    ROCK_SIDE_FIRST = ROCK_TOP,
    ROCK_SIDE_REST = ROCK_TOP,

    SAND_TOP = 18,
    SAND_BOTTOM = SAND_TOP,
    SAND_SIDE_FIRST = SAND_TOP,
    SAND_SIDE_REST = SAND_TOP,

    RAILWAY_1_TOP = (10*16)+8,
    WATER_PHASE_1 = (12*16)+13,

    GRASS_PHASE_0 = (5*16)+8
  };


Mesh::~Mesh()
{
}

struct TriangularMesh : Mesh {
  virtual void render(UserInterface *ui,
                      glm::mat4 const& model);
};

struct LineMesh : Mesh {
  virtual void render(UserInterface *ui,
                      glm::mat4 const& model);
};

struct MeshAccumulator {
  GLfloat *posnp, *uvp, *ambientp, *normalp;
  GLuint *indexp, *linep;
  unsigned count;
  unsigned top_texture_index;
  unsigned bottom_texture_index;
  unsigned side1_texture_index;
  unsigned side2_texture_index;
  Slab *current;
  GLfloat z0, z1;
  GLfloat hex[6][2];
  GLfloat hexnorm[6][2];
  GLfloat posn[3*10000000];
  GLfloat normal[3*10000000];
  GLfloat uv[2*10000000];
  GLfloat ambient[10000000];
  GLuint index[3*10000000];
  GLuint lines[2*10000000];

  GLuint transparent_index[3*100000];
  unsigned num_transparent;

  MeshAccumulator()
    : posnp( &posn[0] ),
      uvp( &uv[0] ),
      ambientp( &ambient[0] ),
      normalp( &normal[0] ),
      indexp( &index[0] ),
      linep( &lines[0] ),
      count(0),
      num_transparent(0)
  {
  }

  unsigned vertex(float x, float y, float z,
                  float u, float v,
                  float a,
                  float nx, float ny, float nz) {
    /*printf("new vertex %u : %.3f %.3f %.3f   %.3f %.3f\n",
      count, x, y, z, u, v);*/
    posnp[0] = x;
    posnp[1] = y;
    posnp[2] = z;
    uvp[0] = u;
    uvp[1] = v;
    ambientp[0] = a;
    normalp[0] = nx;
    normalp[1] = ny;
    normalp[2] = nz;
    normalp += 3;
    posnp += 3;
    uvp += 2;
    ambientp += 1;
    return count++;
  }

  void line(unsigned a, unsigned b) {
    linep[0] = a;
    linep[1] = b;
    linep += 2;
  }

  void triangle(unsigned a, unsigned b, unsigned c) {
    indexp[0] = a;
    indexp[1] = b;
    indexp[2] = c;
    indexp += 3;
  }

  void transparent_triangle(unsigned a, unsigned b, unsigned c) {
    transparent_index[3*num_transparent] = a;
    transparent_index[3*num_transparent+1] = b;
    transparent_index[3*num_transparent+2] = c;
    num_transparent++;
  }

  GLuint gen_posn() {
    GLuint id;

    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER,
                 count * 3 * sizeof(GLfloat),
                 &posn[0],
                 GL_STATIC_DRAW);
    return id;
  }

  GLuint gen_normal() {
    GLuint id;

    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER,
                 count * 3 * sizeof(GLfloat),
                 &normal[0],
                 GL_STATIC_DRAW);
    return id;
  }

  GLuint gen_uv() {
    GLuint id;
    /*
    for (unsigned i=0; i<count; i++) {
      printf("uv[%d]  %.3f %.3f\n",
             i,
             TEXTURE_GRID_WIDTH*uv[i*2+0],
             TEXTURE_GRID_WIDTH*uv[i*2+1]);
    }
    */
    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER,
                 count * 2 * sizeof(GLfloat),
                 &uv[0],
                 GL_STATIC_DRAW);
    return id;
  }

  GLuint gen_ambient() {
    GLuint id;
    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER,
                 count * 1 * sizeof(GLfloat),
                 &ambient[0],
                 GL_STATIC_DRAW);
    return id;
  }

  unsigned size() {
    return (indexp - &index[0])/3;
  }

  GLuint gen_transparent_index() {
    GLuint id;
    glGenBuffers(1, &id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    unsigned num = num_transparent;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 num * 3 * sizeof(GLuint),
                 &transparent_index[0],
                 GL_STATIC_DRAW);
    return id;
  }

  GLuint gen_index() {
    GLuint id;
    glGenBuffers(1, &id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    unsigned num = (indexp - &index[0])/3;

    /*
    for (unsigned i=0; i<num; i++) {
      printf("index[%d]  %d %d %d\n",
             i, index[i*3+0], index[i*3+1], index[i*3+2]);
    }
    */
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 num * 3 * sizeof(GLuint),
                 &index[0],
                 GL_STATIC_DRAW);
    //printf("index size = %u\n", num);
    return id;
  }

  GLuint gen_lines(unsigned *nump) {
    GLuint id;
    glGenBuffers(1, &id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    unsigned num = (linep - &lines[0])/2;
    *nump = num;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 num * 2 * sizeof(GLuint),
                 &lines[0],
                 GL_STATIC_DRAW);
    return id;
  }

  /*
   *
   *       4
   *       /\
   *      /  \
   *    5/    \
   *    |      |3
   *    |      |
   *    |      |
   *    0\    /2
   *      \  /
   *       \/
   *        1
   *
   *
   *
   *
   *
   *
   *
   */

  void setup(Slab *slab) {
    current = slab;
    double x0 = slab->x * x_stride;
    if (slab->y & 1) {
      x0 += x_stride / 2;
    }
    double y0 = slab->y * y_stride;
    z0 = slab->z0 * z_scale;
    z1 = slab->z1 * z_scale;

    hex[0][0] = x0;
    hex[0][1] = y0;
    // face norms are labeled after the vertex # that precedes them,
    // so face[0] is between vertex 0 and vertex 1
    hexnorm[0][0] = cos(DEG_TO_RAD((210+270)/2.0));
    hexnorm[0][1] = sin(DEG_TO_RAD((210+270)/2.0));

    hex[1][0] = x0 + x_stride/2;
    hex[1][1] = y0 - a;
    hexnorm[1][0] = cos(DEG_TO_RAD((270+330)/2.0));
    hexnorm[1][1] = sin(DEG_TO_RAD((270+330)/2.0));

    hex[2][0] = x0 + x_stride;
    hex[2][1] = y0;
    hexnorm[2][0] = cos(DEG_TO_RAD(0));
    hexnorm[2][1] = sin(DEG_TO_RAD(0));

    hex[3][0] = x0 + x_stride;
    hex[3][1] = y0 + edge;
    hexnorm[3][0] = cos(DEG_TO_RAD((30+90)/2.0));
    hexnorm[3][1] = sin(DEG_TO_RAD((30+90)/2.0));

    hex[4][0] = x0 + x_stride/2;
    hex[4][1] = y0 + (edge+a);
    hexnorm[4][0] = cos(DEG_TO_RAD((90+150)/2.0));
    hexnorm[4][1] = sin(DEG_TO_RAD((90+150)/2.0));

    hex[5][0] = x0;
    hex[5][1] = y0 + edge;
    hexnorm[5][0] = cos(DEG_TO_RAD((150+210)/2.0));
    hexnorm[5][1] = sin(DEG_TO_RAD((150+210)/2.0));

    switch (slab->type) {
    case 1:
      top_texture_index = GRASSY_DIRT_TOP;
      bottom_texture_index = GRASSY_DIRT_BOTTOM;
      side1_texture_index = GRASSY_DIRT_SIDE_FIRST;
      side2_texture_index = GRASSY_DIRT_SIDE_REST;
      break;
    case 2:
      top_texture_index = OAK_TREE_TOP;
      bottom_texture_index = OAK_TREE_BOTTOM;
      side1_texture_index = OAK_TREE_SIDE_FIRST;
      side2_texture_index = OAK_TREE_SIDE_REST;
      break;
    case 3:
      top_texture_index = ROCK_TOP;
      bottom_texture_index = ROCK_BOTTOM;
      side1_texture_index = ROCK_SIDE_FIRST;
      side2_texture_index = ROCK_SIDE_REST;
      break;
    case 4:
      top_texture_index = SAND_TOP;
      bottom_texture_index = SAND_BOTTOM;
      side1_texture_index = SAND_SIDE_FIRST;
      side2_texture_index = SAND_SIDE_REST;
      break;

    case 5:
      top_texture_index = RAILWAY_1_TOP;
      bottom_texture_index = ROCK_BOTTOM;
      side1_texture_index = ROCK_SIDE_FIRST;
      side2_texture_index = ROCK_SIDE_REST;
      break;

    case 200:
      top_texture_index = GRASS_PHASE_0 + ((slab->flags >> 3) & 0x7);
      //top_texture_index = 6;
      break;

    case 240:
      top_texture_index = WATER_PHASE_1;
      bottom_texture_index = WATER_PHASE_1;
      side1_texture_index = WATER_PHASE_1;
      side2_texture_index = WATER_PHASE_1;
      break;
    }
  }

  void water_top() {
    double u0 = terrain_u(top_texture_index);
    double v0 = terrain_v(top_texture_index);
    const double t_s = 1.0/(4*a*TEXTURE_GRID_WIDTH);
    const double v_a = a * t_s;
    const double v_e = edge * t_s;
    const double u_w = x_stride * t_s;

    v0 += v_a;
    float ambient = 1.0;
    float nx = 0, ny = 0, nz = 1;
    unsigned k0 = vertex(hex[0][0], hex[0][1], z1, u0, v0, ambient, nx, ny, nz);
    unsigned k1 = vertex(hex[1][0], hex[1][1], z1, u0 + u_w/2, v0 - v_a, ambient, nx, ny, nz);
    unsigned k2 = vertex(hex[2][0], hex[2][1], z1, u0 + u_w, v0, ambient, nx, ny, nz);
    unsigned k3 = vertex(hex[3][0], hex[3][1], z1, u0 + u_w, v0 + v_e, ambient, nx, ny, nz);
    unsigned k4 = vertex(hex[4][0], hex[4][1], z1, u0 + u_w/2, v0 + (v_a+v_e), ambient, nx, ny, nz);
    unsigned k5 = vertex(hex[5][0], hex[5][1], z1, u0, v0 + v_e, ambient, nx, ny, nz);

    transparent_triangle(k0, k1, k2);
    transparent_triangle(k2, k3, k5);
    transparent_triangle(k3, k4, k5);
    transparent_triangle(k5, k0, k2);
  }

  void billboard() {
    // not really a billboard because it doesn't follow the user's view
    // a 3-way planar texture
    const double du = 1.0/TEXTURE_GRID_WIDTH;
    double u0 = terrain_u(top_texture_index);
    double v0 = terrain_v(top_texture_index) + du;
    double u1 = u0 + du;
    double v1 = v0 - du * 10 * (current->z1 - current->z0) / 64.0;

    /*
    printf("billboard================\n");
    printf("    u0 v0  %.3f %.3f\n", u0, v0);
    printf("    u1 v1  %.3f %.3f\n", u1, v1);
    */

    unsigned k[6][4];
    float a = 1.0;

    for (int i=0; i<6; i++) {
      /*float x0 = (hex[(i+5)%6][0] + hex[(i+1)%6][0])/2.0;
      float y0 = (hex[(i+5)%6][1] + hex[(i+1)%6][1])/2.0;
      float x1 = (hex[(i+2)%6][0] + hex[(i+4)%6][0])/2.0;
      float y1 = (hex[(i+2)%6][1] + hex[(i+4)%6][1])/2.0;
      */
      float x0 = hex[i][0];
      float y0 = hex[i][1];
      float x1 = hex[(i+3)%6][0];
      float y1 = hex[(i+3)%6][1];
      float nx = hexnorm[(i+1)%6][0];
      float ny = hexnorm[(i+1)%6][1];

      k[i][0] = vertex(x0, y0, z0, u0, v0, a,  nx, ny,  0.0f);
      k[i][1] = vertex(x1, y1, z0, u1, v0, a,  nx, ny,  0.0f);
      k[i][2] = vertex(x1, y1, z1, u1, v1, a,  nx, ny,  0.0f);
      k[i][3] = vertex(x0, y0, z1, u0, v1, a,  nx, ny,  0.0f);
      /*
      printf("    [%d] <%.3f %.3f %.3f> - <%.3f %.3f %.3f>\n", 
             i,
             x0, y0, z0,
             x1, y1, z1);
      */
      triangle(k[i][0], k[i][1], k[i][2]);
      triangle(k[i][2], k[i][3], k[i][0]);
    }
  }

  void hextop() {
    double u0 = terrain_u(top_texture_index);
    double v0 = terrain_v(top_texture_index);
    const double t_s = 1.0/(4*a*TEXTURE_GRID_WIDTH);
    const double v_a = a * t_s;
    const double v_e = edge * t_s;
    const double u_w = x_stride * t_s;

    v0 += v_a;
    float ambient = 1.0;
    switch (current->flags & DEEP_SHADOW) {
    case DEEP_SHADOW: ambient = 0.5; break;
    case MEDIUM_SHADOW: ambient = 0.667; break;
    case LIGHT_SHADOW: ambient = 0.9; break;
    }
    float nx = 0, ny = 0, nz = 1;
    unsigned k0 = vertex(hex[0][0], hex[0][1], z1, u0, v0, ambient, nx, ny, nz);
    unsigned k1 = vertex(hex[1][0], hex[1][1], z1, u0 + u_w/2, v0 - v_a, ambient, nx, ny, nz);
    unsigned k2 = vertex(hex[2][0], hex[2][1], z1, u0 + u_w, v0, ambient, nx, ny, nz);
    unsigned k3 = vertex(hex[3][0], hex[3][1], z1, u0 + u_w, v0 + v_e, ambient, nx, ny, nz);
    unsigned k4 = vertex(hex[4][0], hex[4][1], z1, u0 + u_w/2, v0 + (v_a+v_e), ambient, nx, ny, nz);
    unsigned k5 = vertex(hex[5][0], hex[5][1], z1, u0, v0 + v_e, ambient, nx, ny, nz);

    triangle(k0, k1, k2);
    triangle(k2, k3, k5);
    triangle(k3, k4, k5);
    triangle(k5, k0, k2);
    line(k0, k1);
    line(k1, k2);
    line(k2, k3);
    line(k3, k4);
    line(k4, k5);
    line(k5, k0);
  }

  void hexbottom() {
    double u0 = terrain_u(bottom_texture_index);
    double v0 = terrain_v(bottom_texture_index);
    const double t_s = 1.0/(4*a*TEXTURE_GRID_WIDTH);
    const double v_a = a * t_s;
    const double v_e = edge * t_s;
    const double u_w = x_stride * t_s;
    v0 += v_a;

    float nx = 0, ny = 0, nz = -1;

    unsigned k0 = vertex(hex[0][0], hex[0][1], z0, u0, v0, 1, nx, ny, nz);
    unsigned k1 = vertex(hex[1][0], hex[1][1], z0, u0 + u_w/2, v0 - v_a, 1, nx, ny, nz);
    unsigned k2 = vertex(hex[2][0], hex[2][1], z0, u0 + u_w, v0, 1, nx, ny, nz);
    unsigned k3 = vertex(hex[3][0], hex[3][1], z0, u0 + u_w, v0 + v_e, 1, nx, ny, nz);
    unsigned k4 = vertex(hex[4][0], hex[4][1], z0, u0 + u_w/2, v0 + (v_a+v_e), 1, nx, ny, nz);
    unsigned k5 = vertex(hex[5][0], hex[5][1], z0, u0, v0 + v_e, 1, nx, ny, nz);

    triangle(k2, k1, k0);
    triangle(k5, k3, k2);
    triangle(k5, k4, k3);
    triangle(k2, k0, k5);
    line(k0, k1);
    line(k1, k2);
    line(k2, k3);
    line(k3, k4);
    line(k4, k5);
    line(k5, k0);
  }

  void face(int f0) {
    int f1 = (f0+1)%6;
    int height = (current->z1 - current->z0);
    // generate the face fragment that's at the top
    double u0 = terrain_u(side1_texture_index);
    double v0 = terrain_v(side1_texture_index);
    const double u_w = 1.0/TEXTURE_GRID_WIDTH;
    int h = (height > SIDE_TEXTURE_HEIGHT) ? SIDE_TEXTURE_HEIGHT : height;
    double v_w = h / (double)(SIDE_TEXTURE_HEIGHT*TEXTURE_GRID_WIDTH);
    short zi = current->z1;
    double z = zi * z_scale;
    double dz = h * z_scale;

    float nx = hexnorm[f0][0];
    float ny = hexnorm[f0][1];
    float nz = 0;
    unsigned k0 = vertex(hex[f0][0], hex[f0][1], z, u0, v0, 1, nx, ny, nz);
    unsigned k1 = vertex(hex[f1][0], hex[f1][1], z, u0 + u_w, v0, 1, nx, ny, nz);
    unsigned k2 = vertex(hex[f1][0], hex[f1][1], z-dz, u0 + u_w, v0 + v_w, 0.5, nx, ny, nz);
    unsigned k3 = vertex(hex[f0][0], hex[f0][1], z-dz, u0, v0 + v_w, 0.5, nx, ny, nz);

    triangle(k0, k3, k1);
    triangle(k1, k3, k2);
    // generate the remaining face fragments
    height -= h;
    zi -= h;

    u0 = terrain_u(side2_texture_index);
    v0 = terrain_v(side2_texture_index);
    while (height > 0) {
      h = (height > SIDE_TEXTURE_HEIGHT) ? SIDE_TEXTURE_HEIGHT : height;
      v_w = h / (double)(SIDE_TEXTURE_HEIGHT*TEXTURE_GRID_WIDTH);
      z = zi * z_scale;
      dz = h * z_scale;

      unsigned k0 = vertex(hex[f0][0], hex[f0][1], z, u0, v0, 0.5, nx, ny, nz);
      unsigned k1 = vertex(hex[f1][0], hex[f1][1], z, u0 + u_w, v0, 0.5, nx, ny, nz);
      unsigned k2 = vertex(hex[f1][0], hex[f1][1], z-dz, u0 + u_w, v0 + v_w, 0.5, nx, ny, nz);
      unsigned k3 = vertex(hex[f0][0], hex[f0][1], z-dz, u0, v0 + v_w, 0.5, nx, ny, nz);

      triangle(k0, k3, k1);
      triangle(k1, k3, k2);
      height -= h;
      zi -= h;
    }
  }

  frect bbox(glm::mat4 xf) {
    frect b;
                   
    if (count == 0) {
      b.x0 = b.x1 = b.y0 = b.y1 = b.z0 = b.z1 = 0;
    } else {
      for (unsigned i=0; i<count; i++) {
        glm::vec4 p(posn[i*3+0], posn[i*3+1], posn[i*3+2], 1);
        p = xf * p;
        if (i == 0) {
          b.x0 = b.x1 = p[0];
          b.y0 = b.y1 = p[1];
          b.z0 = b.z1 = p[2];
        } else {
          b.x0 = (p[0] < b.x0) ? p[0] : b.x0;
          b.x1 = (p[0] > b.x1) ? p[0] : b.x1;
          b.y0 = (p[1] < b.y0) ? p[1] : b.y0;
          b.y1 = (p[1] > b.y1) ? p[1] : b.y1;
          b.z0 = (p[2] < b.z0) ? p[2] : b.z0;
          b.z1 = (p[2] > b.z1) ? p[2] : b.z1;
        }
      }
    }
    return b;
  }

  TriangularMesh *make_triangular(ShaderRef *shader) {
    TriangularMesh *m = new TriangularMesh();
    m->vertexBuffer = gen_posn();
    m->uvBufferId = gen_uv();
    m->ambientBufferId = gen_ambient();
    m->indexBufferId = gen_index();
    m->count = size();
    m->shader = shader;
    return m;
  }

  TriangularMesh *make_transparent_triangular(ShaderRef *shader, TriangularMesh *main) {
    if (num_transparent == 0) {
      return NULL;
    }
    TriangularMesh *m = new TriangularMesh();
    m->vertexBuffer = main->vertexBuffer;
    m->uvBufferId = main->uvBufferId;
    m->ambientBufferId = main->ambientBufferId;
    m->indexBufferId = gen_transparent_index();
    m->count = num_transparent;
    m->shader = shader;
    return m;
  }

  LineMesh *make_lines(ShaderRef *shader) {
    LineMesh *m = new LineMesh();
    m->vertexBuffer = gen_posn();
    m->uvBufferId = 0;
    m->ambientBufferId = 0;
    m->indexBufferId = gen_lines(&m->count);
    m->shader = shader;
    return m;
  }
};

// has nothing to contribute (used to skip space in the subject column)
static inline bool is_space(int t)
{
  return (t==0) || (t==240) || (t==200);
}

// might be seen through (used to skip solids in the neighbor column)
static inline bool is_solid(int t)
{
  return (t!=0) && (t!=240) && (t!=200);
}

#define MAX_Z_DEPTH     (10000)
static void explosive_merge(ClientRegion *rgn,
                            SpanVector const& me, 
                            SpanVector const& neighbor,
                            MeshAccumulator *ma,
                            int x, int y, 
                            int face)
{
  uint8_t me_type[MAX_Z_DEPTH];
  uint8_t neighbor_type[MAX_Z_DEPTH];
  uint8_t me_flags[MAX_Z_DEPTH];

  int me_z = 0;
  for (SpanVector::const_iterator i=me.begin(); i!=me.end(); ++i) {
    memset(&me_flags[me_z], i->flags, i->height);
    if (is_space(i->type)) {
      memset(&me_type[me_z], 0, i->height);
    } else {
      memset(&me_type[me_z], i->type, i->height);
    }
    me_z += i->height;
  }

  int neighbor_z = 0;
  for (SpanVector::const_iterator i=neighbor.begin(); i!=neighbor.end(); ++i) {
    if (is_solid(i->type)) {
      memset(&neighbor_type[neighbor_z], 1, i->height);
    } else {
      memset(&neighbor_type[neighbor_z], 0, i->height);
    }
    neighbor_z += i->height;
  }
  if (me_z > neighbor_z) {
    memset(&neighbor_type[neighbor_z], 0, me_z - neighbor_z);
  }

  int i=0;
  Slab s;
  s.x = x;
  s.y = y;
  while (i < me_z) {
    if (me_type[i] && !neighbor_type[i]) {
      int i0 = i;
      s.type = me_type[i];
      s.flags = me_flags[i];
      while ((i < me_z)
             && (me_type[i] == s.type) 
             && (me_flags[i] == s.flags)
             && !neighbor_type[i]) {
        i++;
      }
      s.z0 = i0 + rgn->basement;
      s.z1 = i + rgn->basement;
      ma->setup(&s);
      ma->face(face);
    } else {
      i++;
    }
  }
}


TerrainSection build_section_from_region(UserInterface *ui,
                                         ClientRegion *rgn)
{
  MeshAccumulator *ma = new MeshAccumulator();
  uint64_t t0 = real_time();

  for (int y=0; y<REGION_SIZE; y++) {
    for (int x=0; x<REGION_SIZE; x++) {
      std::vector<Span>& col = rgn->columns[y][x];
      //printf("build mesh (%d,%d) %lu spans: ", rgn->origin.x + x, rgn->origin.y + y, col.size());
      std::vector<Span> *col_e = NULL;
      std::vector<Span> *col_ne = NULL;
      std::vector<Span> *col_nw = NULL;
      std::vector<Span> *col_w = NULL;
      std::vector<Span> *col_sw = NULL;
      std::vector<Span> *col_se = NULL;

      if (x > 0) {
        col_w = &rgn->columns[y][x-1];
        //printf(" W");
      }
      if (x < (REGION_SIZE-1)) {
        col_e = &rgn->columns[y][x+1];
        //printf(" E");
      }
      if (y > 0) {
        {
          int se_x = x, se_y = y;
          hex_se(&se_x, &se_y);
          if (se_x < REGION_SIZE) {
            assert(se_y >= 0);
            assert(se_x >= 0);
            col_se = &rgn->columns[se_y][se_x];
            //printf(" SE");
          }
        }
        {
          int sw_x = x, sw_y = y;
          hex_sw(&sw_x, &sw_y);
          if (sw_x > 0) {
            assert(sw_y >= 0);
            assert(sw_x < REGION_SIZE);
            col_sw = &rgn->columns[sw_y][sw_x];
            //printf(" SW");
          }
        }
      }
      if (y < (REGION_SIZE-1)) {
        {
          int ne_x = x, ne_y = y;
          hex_ne(&ne_x, &ne_y);
          if (ne_x < REGION_SIZE) {
            assert(ne_y >= 0);
            assert(ne_x >= 0);
            col_ne = &rgn->columns[ne_y][ne_x];
            //printf(" NE");
          }
        }

        {
          int nw_x = x, nw_y = y;
          hex_nw(&nw_x, &nw_y);
          if (nw_x > 0) {
            assert(nw_y >= 0);
            assert(nw_x < REGION_SIZE);
            col_nw = &rgn->columns[nw_y][nw_x];
            //printf(" NW");
          }
        }
      }
      //printf("\n");

      //printf("bmr (%d,%d)  %u\n", x, y, ma->size());
      Slab s;
      s.x = rgn->origin.x + x;
      s.y = rgn->origin.y + y;
      s.z0 = rgn->basement;
      bool wantbottom = false;

      for (std::vector<Span>::iterator i=col.begin(); i<col.end(); ++i) {
        s.z1 = s.z0 + i->height;
        s.type = i->type;
        s.flags = i->flags;
        if (i->type == 240) {
          // special handling for water
          ma->setup(&s);
          ma->water_top();
        } else if (i->type == 200) {
          // special handling for grass... not a hex prism model
          ma->setup(&s);
          ma->billboard();
          wantbottom = true;
        } else if (i->type != 0) {
          ma->setup(&s);
          ma->hextop();
          if (wantbottom) {
            ma->hexbottom();
          }
          if (!col_sw) {
            ma->face(0);
          }
          if (!col_se) {
            ma->face(1);
          }
          if (!col_e) {
            ma->face(2);
          }
          if (!col_ne) {
            ma->face(3);
          }
          if (!col_nw) {
            ma->face(4);
          }
          if (!col_w) {
            ma->face(5);
          }
        }
        s.z0 = s.z1;
        wantbottom = true;
      }
      if (col_sw) {
        explosive_merge(rgn, col, *col_sw, ma, rgn->origin.x + x, rgn->origin.y + y, 0);
      }
      if (col_se) {
        explosive_merge(rgn, col, *col_se, ma, rgn->origin.x + x, rgn->origin.y + y, 1);
      }
      if (col_e) {
        explosive_merge(rgn, col, *col_e, ma, rgn->origin.x + x, rgn->origin.y + y, 2);
      }
      if (col_ne) {
        explosive_merge(rgn, col, *col_ne, ma, rgn->origin.x + x, rgn->origin.y + y, 3);
      }
      if (col_nw) {
        explosive_merge(rgn, col, *col_nw, ma, rgn->origin.x + x, rgn->origin.y + y, 4);
      }
      if (col_w) {
        explosive_merge(rgn, col, *col_w, ma, rgn->origin.x + x, rgn->origin.y + y, 5);
      }
    }
  }

  TriangularMesh *m = ma->make_triangular(&ui->terrainShader);
  Mesh *tm = ma->make_transparent_triangular(&ui->waterShader, m);
  delete ma;
  uint64_t t1 = real_time();
  // report it as slow if it takes longer than 100 ms
  if ((t1-t0) > 100000) {
    fprintf(stderr, "warning: slow build for region (%d,%d); %d+%d triangles in %.4f sec\n", 
            rgn->origin.x, rgn->origin.y,
            m->count, tm ? tm->count : 0, 
            (t1-t0) * 1.0e-6);
  }
  rgn->picker = makeRegionPicker(rgn);

  TerrainSection s;
  s.ts_posn = rgn->origin;
  s.ts_ground = m;
  s.ts_water = tm;
  return s;
}

void TerrainSection::releaseContents()
{
  if (ts_ground) {
    delete ts_ground;
    ts_ground = NULL;
  }
  if (ts_water) {
    delete ts_water;
    ts_water = NULL;
  }
}

struct PlainSuperMesh : SuperMesh {
  virtual float *render(UserInterface *ui,
                        glm::mat4 const& model,
                        float *args);
};

struct SingleAxisMesh : SuperMesh {
  glm::vec3     axis;
  SingleAxisMesh(glm::vec3 const& _axis)
    : axis(_axis) {
  }
};

struct SingleAxisRotationMesh : SingleAxisMesh {
  SingleAxisRotationMesh(glm::vec3 const& axis)
    : SingleAxisMesh(axis) {
  }

  virtual float *render(UserInterface *ui,
                        glm::mat4 const& model,
                        float *args);
};

struct SingleAxisTranslationMesh : SingleAxisMesh {
  SingleAxisTranslationMesh(glm::vec3 const& axis)
    : SingleAxisMesh(axis) {
  }

  virtual float *render(UserInterface *ui,
                        glm::mat4 const& model,
                        float *args);
};


float *SingleAxisTranslationMesh::render(UserInterface *ui,
                                         glm::mat4 const& model,
                                         float *args)
{
  //printf("TRANSLATE <%.3f %.3f %3f> * %.3f\n", axis[0], axis[1], axis[2], args[0]);
  float distance = *args++;
  glm::mat4 m = glm::translate(model * matrix, axis*distance);
  return _render(ui, m, args);
}

float *SingleAxisRotationMesh::render(UserInterface *ui,
                                         glm::mat4 const& model,
                                         float *args)
{
  //printf("ROTATE <%.3f %.3f %3f> * %.3f\n", axis[0], axis[1], axis[2], args[0]);
  float angle = *args++;
  glm::mat4 m = glm::rotate(model * matrix, angle, axis);
  return _render(ui, m, args);
}

float *SuperMesh::_render(UserInterface *ui, glm::mat4 const& model, float *args)
{
  piece->render(ui, model);
  for (std::vector<SuperMesh*>::iterator i=sub.begin(); i != sub.end(); ++i) {
    args = (*i)->render(ui, model, args);
  }
  return args;
}

float *PlainSuperMesh::render(UserInterface *ui,
                              glm::mat4 const& model,
                              float *args)
{
#if 0
  printf("[ [ %5.2f %5.2f %5.2f %5.2f ]   THIS MATRIX<%s>\n"
         "  [ %5.2f %5.2f %5.2f %5.2f ]\n"
         "  [ %5.2f %5.2f %5.2f %5.2f ]\n"
         "  [ %5.2f %5.2f %5.2f %5.2f ] ]\n",
         matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3], 
         name.c_str(),
         matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3], 
         matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3], 
         matrix[3][0], matrix[3][1], matrix[3][2], matrix[3][3] );

  printf("[ [ %5.2f %5.2f %5.2f %5.2f ]   PARENT MODEL\n"
         "  [ %5.2f %5.2f %5.2f %5.2f ]\n"
         "  [ %5.2f %5.2f %5.2f %5.2f ]\n"
         "  [ %5.2f %5.2f %5.2f %5.2f ] ]\n",
         model[0][0], model[0][1], model[0][2], model[0][3], 
         model[1][0], model[1][1], model[1][2], model[1][3], 
         model[2][0], model[2][1], model[2][2], model[2][3], 
         model[3][0], model[3][1], model[3][2], model[3][3] );
#endif

  glm::mat4 m = model * matrix;

  /*glm::vec4 org = m * glm::vec4(0,0,0,1);
  printf("---------------------------------------> (%5.2f %5.2f %5.2f)\n",
         org[0], org[1], org[2]);
  */
  args = _render(ui, m, args);
  //show_axes(ui, m);
  return args;
}

SuperMesh *internalize_model_mesh(wire::model::Mesh const& mesh,
                                  ShaderRef *shader,
                                  frect *bbox,
                                  glm::mat4 const& parentMatrix)
{
  MeshAccumulator *ma = new MeshAccumulator();
  wire::model::Vertices const& vertices = mesh.vertices();

  size_t n = vertices.x_size();
  bool has_norm = (vertices.nx_size() == (int)n);
  bool has_texture = (vertices.u_size() == (int)n);
  
  for (size_t i=0; i<n; i++) {
    double x = vertices.x(i);
    double y = vertices.y(i);
    double z = vertices.z(i);
    double u = has_texture ? vertices.u(i) : 0;
    double v = has_texture ? vertices.v(i) : 0;
    if (has_norm) {
      double nx = vertices.nx(i);
      double ny = vertices.ny(i);
      double nz = vertices.nz(i);
      ma->vertex(x, y, z, u, v, 1, nx, ny, nz);
    } else {
      ma->vertex(x, y, z, u, v, 1, 0, 0, 1);
    }
  }
  //printf("mesh<%s> |matrix|=%d\n", mesh.name().c_str(), mesh.matrix_size());
  glm::mat4 xform(1);

  if (mesh.matrix_size() == 16) {
    for (int j=0; j<4; j++) {
      for (int k=0; k<4; k++) {
        xform[j][k] = mesh.matrix(j*4+k);
      }
    }
  }

  n = mesh.faces_size();
  for (size_t i=0; i<n; i++) {
    wire::model::Face const& face(mesh.faces(i));
    size_t m = face.vertex_size();
    for (unsigned k=0; k<(m-2); k++) {
      ma->triangle(face.vertex(k), face.vertex(k+1), face.vertex(k+2));
    }
  }

  //
  Mesh *m = ma->make_triangular(shader);
  if (bbox) {
    *bbox = ma->bbox(parentMatrix * xform);
  }
  delete ma;

  SuperMesh *here;

  if (mesh.has_style()) {
    switch (mesh.style()) {
    case wire::model::SINGLE_AXIS_ROTATION:
      {
        assert(mesh.basis_size() == 3);
        glm::vec3 b(mesh.basis(0), mesh.basis(1), mesh.basis(2));
        SingleAxisRotationMesh *s = new SingleAxisRotationMesh(b);
        here = s;
      }
      break;
    case wire::model::SINGLE_AXIS_TRANSLATION:
      {
        assert(mesh.basis_size() == 3);
        glm::vec3 b(mesh.basis(0), mesh.basis(1), mesh.basis(2));
        SingleAxisTranslationMesh *s = new SingleAxisTranslationMesh(b);
        here = s;
      }
      break;
    }
  } else {
    here = new PlainSuperMesh();
  }
  
  here->piece = m;
  here->matrix = xform;
  here->name = mesh.name();
  
  if (mesh.refpoint_size() == 3) {
    here->has_ref = true;
    here->ref[0] = mesh.refpoint(0);
    here->ref[1] = mesh.refpoint(1);
    here->ref[2] = mesh.refpoint(2);
  }

  for (int i=0; i<mesh.children_size(); i++) {
    frect subbox;
    SuperMesh *p = internalize_model_mesh(mesh.children(i), shader, 
                                          &subbox,
                                          parentMatrix * here->matrix);
    if (bbox) {
      bbox->x0 = fmin(bbox->x0, subbox.x0);
      bbox->x1 = fmax(bbox->x1, subbox.x1);
      bbox->y0 = fmin(bbox->y0, subbox.y0);
      bbox->y1 = fmax(bbox->y1, subbox.y1);
      bbox->z0 = fmin(bbox->z0, subbox.z0);
      bbox->z1 = fmax(bbox->z1, subbox.z1);
    }
    here->sub.push_back(p);
  }

  return here;
}

SuperMesh *load_mesh_from_model(const char *path, ShaderRef *shader)
{
  size_t len;
  const char *data = load_file(path, &len);
  printf("%s: %zu bytes\n", path, len);
  std::string buf(data, len);
  wire::model::Mesh mesh;
  if (mesh.ParseFromString(buf)) {
    printf("%s: Loaded mesh <%s> with %d faces and %d vertices\n",
           path,
           mesh.name().c_str(),
           mesh.faces_size(),
           mesh.vertices().x_size());
    if (mesh.children_size() > 0) {
      printf("   and %d children\n", mesh.children_size());
    }
  } else {
    printf("%s: Could not load mesh\n", path);
  }
  return internalize_model_mesh(mesh, shader, NULL, glm::mat4(1));
}

struct Mesh *build_mesh(struct UserInterface *ui)
{
  Slab s0;
  s0.x = 0;
  s0.y = 0;
  s0.z0 = 0;
  s0.z1 = 8;
  s0.flags = 0;
  s0.type = 1;

  Slab s1;
  s1.x = 1;
  s1.y = 0;
  s1.z0 = 0;
  s1.z1 = 16;
  s1.flags = 0;
  s1.type = 1;

  MeshAccumulator *map = new MeshAccumulator();
  MeshAccumulator& ma(*map);

  ma.setup(&s0);
  ma.hextop();
  ma.hexbottom();
  ma.face(0);
  ma.face(1);
  ma.face(2);
  ma.face(3);
  ma.face(4);
  ma.face(5);

  ma.setup(&s1);
  ma.hextop();
  ma.hexbottom();
  ma.face(0);
  ma.face(1);
  ma.face(2);
  ma.face(3);
  ma.face(4);
  ma.face(5);

  for (unsigned x=2; x<10; x++) {
    for (unsigned y=2; y<10; y++) {
      Slab s;
      s.x = x;
      s.y = y;
      s.z0 = 0;
      s.z1 = x+y;
      s.type = 1;
      s.flags = 0;
      ma.setup(&s);
      ma.hextop();
      ma.face(0);
      ma.face(1);
      ma.face(2);
      ma.face(3);
      ma.face(4);
      ma.face(5);
    }
  }

  Mesh *m = ma.make_triangular(&ui->terrainShader);
  return m;
}

void outline_hex_face(struct UserInterface *ui,
                      int x, int y, int iz0, int iz1,
                      int face)
{
  glUseProgram(0);
  glLineWidth(1);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(-5, -3);
  glm::mat4 mx = ui->projectionMatrix * ui->current_viewpoint.vp_matrix;
  glLoadMatrixf(&mx[0][0]);

  glColor3f(1,0,0);

  double x0 = hex_x(x, y);
  double y0 = hex_y(x, y);

  glBegin(GL_LINE_LOOP);
  if (face < 6) {
    double z0 = iz0 * z_scale;
    double z1 = iz1 * z_scale;

    int i = face;
    glVertex3f(hex_edges[i].online[0] + x0,
               hex_edges[i].online[1] + y0,
               z0);
    glVertex3f(hex_edges[(i+1)%6].online[0] + x0,
               hex_edges[(i+1)%6].online[1] + y0,
               z0);
    glVertex3f(hex_edges[(i+1)%6].online[0] + x0,
               hex_edges[(i+1)%6].online[1] + y0,
               z1);
    glVertex3f(hex_edges[i].online[0] + x0,
               hex_edges[i].online[1] + y0,
               z1);
  } else {
    double z = ((face == PICK_INDEX_FACE_TOP) ? iz1 : iz0) * z_scale;
    for (int i=0; i<6; i++) {
      glVertex3f(hex_edges[i].online.x + x0,
                 hex_edges[i].online.y + y0,
                 z);
    }
  }
  glEnd();
  glDisable(GL_POLYGON_OFFSET_FILL);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
}

void outline_hex_prism(struct UserInterface *ui,
                       int x, int y, int iz0, int iz1)
{
  glUseProgram(0);
  glLineWidth(1);
  glDisable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(-3, -3);
  glm::mat4 mx = ui->projectionMatrix * ui->current_viewpoint.vp_matrix;
  glLoadMatrixf(&mx[0][0]);

  glColor3f(1,1,1);

  double x0 = hex_x(x, y);
  double y0 = hex_y(x, y);
  double z0 = iz0 * z_scale;
  double z1 = iz1 * z_scale;

  glBegin(GL_LINES);
  for (int i=0; i<6; i++) {
    glVertex3f(hex_edges[i].online[0] + x0,
               hex_edges[i].online[1] + y0,
               z0);
    glVertex3f(hex_edges[i].online[0] + x0,
               hex_edges[i].online[1] + y0,
               z1);
  }
  for (int i=0; i<6; i++) {
    glVertex3f(hex_edges[i].online[0] + x0,
               hex_edges[i].online[1] + y0,
               z0);
    glVertex3f(hex_edges[(i+1)%6].online[0] + x0,
               hex_edges[(i+1)%6].online[1] + y0,
               z0);
    glVertex3f(hex_edges[i].online[0] + x0,
               hex_edges[i].online[1] + y0,
               z1);
    glVertex3f(hex_edges[(i+1)%6].online[0] + x0,
               hex_edges[(i+1)%6].online[1] + y0,
               z1);
  }
  glEnd();

  glDisable(GL_POLYGON_OFFSET_FILL);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
}

void LineMesh::render(UserInterface *ui,
                      glm::mat4 const& model)
{
  glUseProgram(shader->shaderId);
  //glDisable(GL_DEPTH_TEST);
  glEnable(GL_POLYGON_OFFSET_LINE);
  glPolygonOffset(1, 1);

  //============================================================

  // Model matrix : an identity matrix (model will be at the origin)

  // Our ModelViewProjection : multiplication of our 3 matrices
  glm::mat4 MVP        = ui->projectionMatrix 
    * ui->current_viewpoint.vp_matrix 
    * model;

  ShaderRef const& shader = ui->outlineShader;

  // Send our transformation to the currently bound shader, in the
  // "MVP" uniform For each model you render, since the MVP will be
  // different (at least the M part)
  glUniformMatrix4fv(shader.shaderPgmMVPMatrixIndex, 1, GL_FALSE, &MVP[0][0]);

  // First attribute buffer -- vertices
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glVertexAttribPointer(0,              // attribute 0
                        3,              // size  len([x,y,z])
                        GL_FLOAT,       // type
                        GL_FALSE,       // normalized?
                        0,              // stride
                        (void*)0);      // array buffer offset;

  // Draw the mesh
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferId);
  glDrawElements(GL_LINES, count*2, GL_UNSIGNED_INT, (void*)0);
  glDisableVertexAttribArray(0);
  glDisable(GL_POLYGON_OFFSET_LINE);
}

void TriangularMesh::render(UserInterface *ui,
                            glm::mat4 const& model)
{
  glUseProgram(shader->shaderId);

  // Our ModelViewProjection : multiplication of our 3 matrices
  glm::mat4 MVP        = ui->projectionMatrix * ui->current_viewpoint.vp_matrix * model;

  // configure the variable 'theTextureSampler' to use texture unit 0
  glUniform1i(shader->shaderPgmTextureIndex, 0);

  // Send our transformation to the currently bound shader, in the
  // "MVP" uniform For each model you render, since the MVP will be
  // different (at least the M part)
  glUniformMatrix4fv(shader->shaderPgmMVPMatrixIndex, 1, GL_FALSE, &MVP[0][0]);

  if (shader->waterWiggleIndex) {
    float dx = 0.3 * cos(ui->frameTime * 1.0e-6) / TEXTURE_GRID_WIDTH;
    float dy = 0.1 * sin(ui->frameTime * 1.0e-6) / TEXTURE_GRID_WIDTH;
    glUniform2f(shader->waterWiggleIndex, dx, dy);
  }
  
  if (shader->fogColorIndex) {
    glUniform4f(shader->fogColorIndex, 
                ui->skyColor.r,
                ui->skyColor.g,
                ui->skyColor.b, 
                1);
  }
  if (shader->fogDensityIndex) {
    glUniform1f(shader->fogDensityIndex, 0.007);
  }
  if (shader->daylightAmbientIndex) {
    float v = glm::smoothstep(-0.2f, 0.0f, ui->skyView.sun.z);
    glUniform1f(shader->daylightAmbientIndex, v);
  }


  // First attribute buffer -- vertices
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glVertexAttribPointer(0,              // attribute 0
                        3,              // size  len([x,y,z])
                        GL_FLOAT,       // type
                        GL_FALSE,       // normalized?
                        0,              // stride
                        (void*)0);      // array buffer offset;


  // 2nd attribute -- texture UV coordinates
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, uvBufferId);
  glVertexAttribPointer(1,              // attribute 1
                        2,              // size  len([U,V])
                        GL_FLOAT,       // type
                        GL_FALSE,       // normalized?
                        0,              // stride
                        (void*)0);      // array buffer offset;

  // 3rd attribute -- surface ambient lighting value
  glEnableVertexAttribArray(2);
  glBindBuffer(GL_ARRAY_BUFFER, ambientBufferId);
  glVertexAttribPointer(2,              // attribute 2
                        1,              // size  len([A])
                        GL_FLOAT,       // type
                        GL_FALSE,       // normalized?
                        0,              // stride
                        (void*)0);      // array buffer offset;

  // Draw the mesh
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferId);
  glDrawElements(GL_TRIANGLES, count*3, GL_UNSIGNED_INT, (void*)0);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(2);
}


#if 0
SuperMesh *build_door_supermesh(UserInterface *ui)
{
  MeshAccumulator *ma = new MeshAccumulator();
  const float door_model_width = x_stride;
  const float door_model_thick = 4.0 * (door_model_width / 28.0);

  const float door_texture_scale = (1.0/TEXTURE_GRID_WIDTH) / door_model_width;
  const float x0 = 0;
  const float y0 = edge - door_thick/2;
  const float z0 = 0;
  const float u0 = terrain_u(12*16);
  const float v0 = terrain_v(12*16);
  const float tp = 1.0/(TEXTURE_GRID_WIDTH*64); // texture pixel

  unsigned k000 = ma->vertex(x0, y0, z0, u0 + 4*tp, v0 + (4+56)*tp, 1);
  unsigned k100 = ma->vertex(x0+door_model_width, y0, z0, u0 + (4+28)*tp, v0 + (4+56)*tp, 1);
  unsigned k010 = ma->vertex(x0, y0, z0+, u0 + 4*tp, v0 + (4+56)*tp, 1);
}
#endif
