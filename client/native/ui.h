#ifndef _H_HEXPLORE_CLIENT_UI   // -*-c++-*-
#define _H_HEXPLORE_CLIENT_UI

#include <SDL.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <unordered_map>
#include <hexcom/pick.h>
#include <hexcom/curve.h>
#include <hexcom/picture.h>

#include "connection.h"
#include "world.h"
#include "wirehandler.h"
#include "sound.h"
#include "animus.h"
#include "skymap.h"
#include "clientoptions.h"
#include "overlay.h"

struct UserInterface;
struct FieldBookWidget;

typedef std::unordered_map<std::string,wire::entity::EntityType*> EntityTypeMap;

struct ShaderRef {
  GLuint shaderId;
  GLuint shaderPgmMVPMatrixIndex;
  GLuint shaderPgmTextureIndex;
  GLuint fogColorIndex;
  GLuint fogDensityIndex;
  GLuint waterWiggleIndex;
  GLuint sunPositionIndex;
  GLuint starMatrixIndex;
  GLuint daylightAmbientIndex;
};

struct OverlayWindow {
  long                  ow_showtime;    // 0 for not being shown
  long                  ow_cleartime;
  OverlayImage         *ow_content;
  // nominal positions in window
  int                   ow_x;
  int                   ow_y;
};

struct Mesh {
  //Posn  region_posn;
  ShaderRef *shader;
  GLuint vertexBuffer;
  GLuint uvBufferId;
  GLuint indexBufferId;
  GLuint ambientBufferId;
  unsigned count;       // number of *triangles*
  unsigned textureUnit; // which texture unit to use
  virtual ~Mesh();
  virtual void render(UserInterface *ui,
                      glm::mat4 const& model) = 0;
};

struct SuperMesh {
  std::string                   name;
  glm::mat4                     matrix;
  Mesh                         *piece;
  std::vector<SuperMesh*>       sub;
  bool                          has_ref;
  glm::vec3                     ref;
  PickerPtr     picker;

  SuperMesh()
    : has_ref(false) {
  }
    
  virtual float *render(UserInterface *ui,
                        glm::mat4 const& model,
                        float *args) = 0;
protected:
  float *_render(UserInterface *ui, glm::mat4 const& model, float *args);
};

struct Entity;

typedef std::unordered_map<unsigned, Entity*> EntityMap;


struct InputBox {
  unsigned long       popupTime;      // 0=not popped up
  std::string         text;
  size_t              cursor;
  bool                dirty;
  OverlayImage       *ib_image;
};


struct ViewPoint {
  glm::vec3             eyes;
  float                 facing;
  float                 tilt;
  glm::mat4             vp_matrix;  // resulting view matrix
};

struct TerrainSection {
  Posn                  ts_posn;
  Mesh                 *ts_ground;
  Mesh                 *ts_water;
  void releaseContents();
};

enum ModalPage {
  MODAL_IN_GAME,
  MODAL_LOGIN_PAGE
};

/***
 *   Texture Unit Assignments
 *   ========================
 *   0  terrain and most cell textures
 *   1  character textures
 */

struct UserInterface {
  UserInterface(ClientOptions const& opt);

  enum ModalPage page;
  EntityMap     entities;
  EntityTypeMap etypes;
  float mouseScale;
  float walkSpeed;
  Span spanOnTopOf;             // the span we are above
  std::vector<PlayerAnimus*>    playerAnimae;
  unsigned                      activePlayerAnimae;// bitmap of active animae
  bool focus;
  bool need_settle;
  float walkingTime;
  float eyeHeight;
  int toolSlot;
  int toolHeight;
  int placeToolType;
  glm::vec3 skyColor;
  SkyView       skyView;

  SDL_Window *window;
  int window_w, window_h;
  SDL_Renderer *renderer;
  SDL_GLContext glcontext;

  OverlayImage *status_overlay;
  Picture *status_overlay_bg;

  SDL_Window *status_window;
  SDL_Renderer *status_renderer;

  Font *textFont;
  struct {
    OverlayImage *tip_overlay;
    Picture *tip_background;
  } typeinPopup;
  struct OverlayWindow  *toolPopup;
  struct OverlayWindow  *toolPopupSel;

  FieldBookWidget *fieldbook;

  unsigned long frame;
  struct {
    unsigned long frame;
    long time;
  } fpsReport;
  int done_flag;

  GLuint vertexArrayId; 
  GLuint textureId;

  struct ShaderRef terrainShader;
  struct ShaderRef skyboxShader;
  struct ShaderRef waterShader;
  struct ShaderRef robotShader;
  struct ShaderRef outlineShader;

  glm::mat4     projectionMatrix;   // optics

  Entity       *playerEntity;
  // TODO, these should be part of (*playerEntity)
  glm::vec3     location;           // camera location
  float         facing;                 // facing direction
  float         tilt;                   // head tilt
  Posn          currentRegionPosn;

  long frameTime;
  long launchTime;
  double solarTimeBase;
  long solarTimeReference;      // our frameTime for which we know solarTimeBase
  double solarTimeRate;         // number of solar day-units per realtime unit (usec)
  std::vector<TerrainSection> terrain;
  GLuint cursorTexture;
  Mesh *outlineMesh;

  ViewPoint current_viewpoint;
  struct {
    Curve      *anim;
    long        start_time;
    long        end_time;
    float       time_scale;
    float       value_scale;
    float       value_base;
  } vp_adj_eye;

  Connection *cnx;
  ClientWorld *world;

  struct {
    bool enable;
    int x, y, z;
    unsigned entity;
    unsigned selected_entity;
    uint64_t p_rpi;
    ClientRegion *p_rgn;
  } pick;

  struct {
    struct AudioStream *stream;
    SoundEffect *boink;
  } sounds;

  struct InputBox inputbox;


  struct {
    OverlayImage *background;
    InputBox username;
    InputBox password;
    int focus;
    std::string username_entry;
    std::string password_entry;
  } loginPage;

  void update_entity(wire::entity::EntityInfo const& info);

};

struct Mesh *build_mesh(struct UserInterface *ui);
struct Mesh *build_cursor_mesh(struct UserInterface *ui);
TerrainSection build_section_from_region(UserInterface *ui,
                                         ClientRegion *rgn);

void draw_mesh(struct UserInterface *ui, 
               ShaderRef const& shader, 
               struct Mesh *m, 
               glm::mat4 *model);

void flush_incoming(struct UserInterface *ui);
SuperMesh *load_mesh_from_model(const char *path, ShaderRef *shader);
SuperMesh *internalize_model_mesh(wire::model::Mesh const& mesh,
                                  ShaderRef *shader,
                                  frect *bbox,
                                  glm::mat4 const& parentModel);

void outline_hex_prism(struct UserInterface *ui, 
                       int x, int y, int z0, int z1);

void outline_hex_face(struct UserInterface *ui, 
                      int x, int y, int z0, int z1,
                      int face);

void ui_render_overlay(UserInterface *ui,
                       OverlayImage *img,
                       int window_x,
                       int window_y,
                       float scale);

/* handle window-manager layer events on the UI */

void ui_window_event(struct UserInterface *ui, SDL_Event *event);


enum InputBoxResult {
  INPUTBOX_CONTINUE,
  INPUTBOX_COMPLETE
};

InputBoxResult ui_inputbox_event(struct UserInterface *ui, InputBox *b, SDL_Event *event);

#endif /* _H_HEXPLORE_CLIENT_UI */
