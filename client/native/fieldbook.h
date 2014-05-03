#ifndef _H_HEXPLORE_FIELDBOOK
#define _H_HEXPLORE_FIELDBOOK

#include <SDL.h>

struct UserInterface;
struct FieldPage;

struct FieldBookWidget {
  FieldBookWidget(UserInterface*);
  void widget_activate();
  void widget_deactivate();
  // return true if the widget consumes the event
  bool widget_click(SDL_Event const& event);
  bool widget_keypress(SDL_Event const& event);
  void widget_render();
  void widget_refresh();

private:
  UserInterface                *ui;
  OverlayImage                 *fieldbook_overlay;
  int                           fieldbook_state;
  Picture                      *fieldbook_open;
  Picture                      *fieldbook_closed;
  bool                          fieldbook_dirty;
  unsigned                      page_num;
  std::vector<FieldPage*>       pages;
  
  void draw_page(unsigned);
};

void ui_fieldbook_command(UserInterface *ui);
void ui_render_fieldbook(UserInterface *ui);
void ui_render_click(UserInterface *ui);
void ui_render_key(UserInterface *ui);

#endif /* _H_HEXPLORE_FIELDBOOK */
