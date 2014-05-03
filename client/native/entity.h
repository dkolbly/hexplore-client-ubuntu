#ifndef _H_HEXPLORE_ENTITY
#define _H_HEXPLORE_ENTITY

#include <string>

struct UserInterface;

struct EntityHandler {
  virtual void draw(UserInterface *ui, Entity *ent) = 0;
  static EntityHandler *get(UserInterface *ui,
                            std::string const& type,
                            std::string const& subtype);
  virtual int pick(glm::vec3 const& origin,
                   glm::vec3 const& direction,
                   PickPoint *pickat) = 0;
  frect bbox;
};

struct Entity {
  unsigned      id;
  glm::vec3     location;
  float         facing;         // facing direction, in degrees (0-360) (0=east, 90=north, 180=west)
  float         tilt;           // head tilt degrees (0=horizontal, +=looking up, -=looking down)
  EntityHandler *handler;
  struct {
    glm::vec3   neg_velocity;
    float       neg_facevel;
    float       time_scale;
    long        final_time;
  } smooth;
  frect bbox();
  int pick(glm::vec3 const& origin,
           glm::vec3 const& direction,
           PickPoint *pickat);
};

void tell_entity(UserInterface *ui, std::string const& msg);

#endif /* _H_HEXPLORE_ENTITY */
