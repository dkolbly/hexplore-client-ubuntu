#include "ico.h"

static const double phi = (1.0 + sqrt(5)) / 2.0;

Icosahedron ico;

static inline double neg0(int k, double x) {
  return (k & 1) ? -x : x;
}

static inline double neg1(int k, double x) {
  return (k & 2) ? -x : x;
}


static inline glm::vec3 face_normal(int i, int j, int k)
{
  glm::vec3 da = ico.vertex[j] - ico.vertex[i];
  glm::vec3 db = ico.vertex[k] - ico.vertex[j];
  glm::vec3 x(glm::cross(da, db));
  return x;
}

Icosahedron::Icosahedron() 
{
  for (int i=0; i<4; i++) {
    vertex[i] = glm::vec3(0, neg1(i,1), neg0(i,phi));
  }
  for (int i=0; i<4; i++) {
    vertex[i+4] = glm::vec3(neg1(i,1), neg0(i,phi), 0);
  }
  for (int i=0; i<4; i++) {
    vertex[i+8] = glm::vec3(neg0(i,phi), 0, neg1(i,1));
  }
  for (int f=0; f<20; f++) {
    int i = index[f][0];
    int j = index[f][1];
    int k = index[f][2];
    normal[i] = face_normal(i, j, k);
    center[i] = (ico.vertex[i] + ico.vertex[j] + ico.vertex[k])/3.0f;
  }
}

unsigned char Icosahedron::index[20][3] = {
  {  0,  2,  8 },
  {  9,  2,  0 },
  {  0,  4,  6 },
  {  8,  4,  0 },
  {  0,  6,  9 },
  { 10,  3,  1 },
  {  1,  3, 11 },
  {  6,  4,  1 },
  {  1,  4, 10 },
  { 11,  6,  1 },
  {  7,  5,  2 },
  {  2,  5,  8 },
  {  9,  7,  2 },
  {  3,  5,  7 },
  { 10,  5,  3 },
  {  3,  7, 11 },
  {  4,  8, 10 },
  { 10,  8,  5 },
  { 11,  9,  6 },
  {  7,  9, 11 }
};



#if UNIT_TEST
void consider_face(int i, int j, int k)
{
  printf(" consider %2d %2d %2d :", i, j, k);
  glm::vec3 n = face_normal(i, j, k);
  glm::vec3 c = (ico.vertex[i] + ico.vertex[j] + ico.vertex[k])/3.0f;
  printf("  % .3f % .3f % .3f", n.x, n.y, n.z);
  printf("  ; %.3f\n", glm::dot(c,n));
}

int main()
{
  for (int i=0; i<12; i++) {
    printf("%2d.  % .3f % .3f % .3f\n",
           i,
           ico.vertex[i].x,
           ico.vertex[i].y,
           ico.vertex[i].z);
  }
  bool edge[12][12];
  
  for (int i=0; i<12; i++) {
    for (int j=0; j<12; j++) {
      edge[i][j] = false;
    }
  }
    
  for (int i=0; i<12; i++) {
    printf("%2d : ", i);
    for (int j=0; j<12; j++) {
      float d = glm::distance(ico.vertex[i], ico.vertex[j]);
      if (fabs(d-2.0) < 0.001) {
        printf(" %2d", j);
        edge[i][j] = true;
        edge[j][i] = true;
      }
    }
    printf("\n");
  }
  for (int i=0; i<12; i++) {
    for (int j=i; j<12; j++) {
      for (int k=j; k<12; k++) {
        if (edge[i][j] && edge[j][k] && edge[k][i]) {
          //printf(" consider %2d %2d %2d :", i, j, k);
          glm::vec3 n = face_normal(i, j, k);
          glm::vec3 c = (ico.vertex[i] + ico.vertex[j] + ico.vertex[k])/3.0f;
          //printf("  % .3f % .3f % .3f", n.x, n.y, n.z);
          //printf("  ; %.3f\n", glm::dot(c,n));
          if (glm::dot(c,n) < 0) {
            consider_face(k, j, i);
          } else {
            consider_face(i, j, k);
          }
        }
      }
    }
  }
  return 0;
}

#endif
