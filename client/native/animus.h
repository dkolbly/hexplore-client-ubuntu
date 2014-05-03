#ifndef _H_HEXPLORE_ANIMUS      // -*-c++-*-
#define _H_HEXPLORE_ANIMUS

#include <hexcom/curve.h>
#include <memory>

struct Entity;
struct UserInterface;

struct Animus {
  // the subject is who we are animating
  Entity       *a_subject;
  long          a_startTime;
  long          a_endTime;
  unsigned      a_uid;

  virtual ~Animus();
  // this is invoked for intermediate updates
  //   dt varies from 0 at the beginning to 1.0 at the endTime.
  //  returns false to cancel the animus
  virtual bool update(double dt) = 0;
  // this is invoked for the final completion update
  virtual void complete() = 0;
};

/**
 *  A PlayerAnimus is an animus controlled locally by the user;
 *  they are kept in a separate list in the UI object, and are
 *  associated with user controls of various sorts
 */
struct PlayerAnimus : Animus {
  unsigned              pa_keyFlags;
  UserInterface        *pa_ui;
};

struct PlayerCrouchAnimus : PlayerAnimus {
  bool                  pca_crouchingDown;      // false=>standing back up
  float                 pca_initialEyeHeight;
  virtual bool update(double dt);
  virtual void complete();
};

struct PlayerWalkingAnimus : PlayerAnimus {
  float                 pwa_direction;
  float                 pwa_speed;
  glm::vec3             pwa_startloc;
  virtual bool update(double dt);
  virtual void complete();
};

struct PlayerJumpAnimus : PlayerAnimus {
  glm::vec3             pja_initialVelocity;
  glm::vec3             pja_initialPosition;
  virtual bool update(double dt);
  virtual void complete();
};

/**
 *  this is SWIM_FLOAT, and controls vertical motion while swimming
 */
struct PlayerSwimAnimus : PlayerAnimus {
  virtual bool update(double dt);
  virtual void complete();
  static void start(UserInterface *ui);
  float psa_verticalSpeed;
};

/**
 *  this is for SWIM_FORWARD, SWIM_BACKWARD, etc. and controls
 *  horizontal motion while swimming
 */

struct PlayerSwimAroundAnimus : PlayerAnimus {
  virtual bool update(double dt);
  virtual void complete();
  static void start(UserInterface *ui, unsigned keyFlags, float dir, float speed);
};

#endif /* _H_HEXPLORE_ANIMUS */
