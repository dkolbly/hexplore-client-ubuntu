#include "ui.h"

static const char shifted[] = "aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ1!2@3#4$5%6^7&8*9(0)-_=+[{]}\\|;:'\",<.>/?`~";

int my_toascii(SDL_Event *event)
{
  char code = event->key.keysym.sym;

  // EVIL!  Why am I not getting TEXTINPUT events?
  // and why in the name of all that's holy do I have to
  // interpret shift modifiers myself?!?!
  if (event->key.keysym.mod & (KMOD_LSHIFT|KMOD_RSHIFT)) {
    for (int i=0; shifted[i*2]; i++) {
      if (shifted[i*2] == code) {
        return shifted[i*2+1];
      }
    }
  }
  return code;
}

InputBoxResult ui_inputbox_event(struct UserInterface *ui, InputBox *b, SDL_Event *event)
{
  b->popupTime = ui->frameTime;
  b->dirty = true;

  if (event->key.type == SDL_KEYDOWN) {
    switch (event->key.keysym.scancode) {
    case SDL_SCANCODE_RIGHT:
      if (b->cursor < b->text.size()) {
        b->cursor++;
      }
      return INPUTBOX_CONTINUE;
    case SDL_SCANCODE_LEFT:
      if (b->cursor > 0) {
        b->cursor--;
      }
      return INPUTBOX_CONTINUE;
      
    case SDL_SCANCODE_HOME:
      b->cursor = 0;
      return INPUTBOX_CONTINUE;

    case SDL_SCANCODE_END:
      b->cursor = b->text.size();
      return INPUTBOX_CONTINUE;

    case SDL_SCANCODE_DELETE:
      if (b->cursor < b->text.size()) {
        b->text.erase(b->cursor, 1);
      }
      return INPUTBOX_CONTINUE;

    default:
      // fall through
      ;
    }

    switch (event->key.keysym.sym) {
    case SDLK_RETURN:
      b->popupTime = 0;
      SDL_StopTextInput();
      return INPUTBOX_COMPLETE;

    case SDLK_BACKSPACE:
      if (b->cursor > 0) {
        b->text.erase(b->cursor-1, 1);
        b->cursor--;
      }
      break;

    default:
      if ((event->key.keysym.sym >= ' ') && (event->key.keysym.sym < 0x7F)) {
        char ch = my_toascii(event);
        //printf("keydown... '%c'\n", ch);
        b->text.insert(b->cursor, 1, ch);
        b->cursor++;
      } else {
        printf("keydown... %#x ignored\n", event->key.keysym.sym);
      }
    }
    return INPUTBOX_CONTINUE;
  }
  b->text.insert(b->cursor, event->text.text);
  b->cursor++;
  return INPUTBOX_CONTINUE;
}

void ui_update_an_inputbox(struct UserInterface *ui, InputBox *ib, bool focus, bool obscure)
{
  assert(ib->ib_image);
  OverlayImage *im = ib->ib_image;

  im->clear();
  if (obscure) {
    std::string obs(ib->text.size(), '*');
    im->write(*ui->textFont, obs, 0, 0);
  } else {
    im->write(*ui->textFont, ib->text, 0, 0);
  }
  if (focus) {
    // draw the cursor
    for (int dy=0; dy<8; dy++) {
      im->put_pixel(ib->cursor * 8, dy, 0xff888888);
    }
  }
  im->flush();
  ib->dirty = false;
}
                           
void ui_update_login_page(struct UserInterface *ui, double dt)
{
  if (ui->loginPage.username.dirty) {
    ui_update_an_inputbox(ui, &ui->loginPage.username, 
                          (ui->loginPage.focus == -1) ? true : false, 
                          false);
  }
  if (ui->loginPage.password.dirty) {
    ui_update_an_inputbox(ui, &ui->loginPage.password,
                          (ui->loginPage.focus == -2) ? true : false, 
                          true);
  }
}

void ui_render_login_page(struct UserInterface *ui)
{
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  float r = 0.5;
  float g = 0.5;
  float b = sin(((ui->frameTime % 10000000) / 1e6) * M_PI * 2 / 10.0);
  glClearColor(r, g, (b+1)/2, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glUseProgram(0);
  glActiveTexture(GL_TEXTURE0);
  // draw it centered
  int dx = (ui->window_w / 2) - (ui->loginPage.background->width / 2);
  int dy = (ui->window_h / 2) - (ui->loginPage.background->height / 2);
  ui_render_overlay(ui, ui->loginPage.background, dx, dy, 1.0);

  ui_render_overlay(ui, ui->loginPage.username.ib_image, dx+283, dy+155, 2.0f);
  ui_render_overlay(ui, ui->loginPage.password.ib_image, dx+283, dy+155+48, 2.0f);
}

void ui_keyevent_login_page(struct UserInterface *ui, SDL_Event *event)
{
  switch (event->key.keysym.sym) {
  case SDLK_TAB:
    printf("switch focus...\n");
    switch (ui->loginPage.focus) {
    case -1: ui->loginPage.focus = -2; break;
    case -2: ui->loginPage.focus = -1; break;
    }
    ui->loginPage.username.dirty = true;
    ui->loginPage.password.dirty = true;
    return;
  case SDLK_ESCAPE:
    // exit the app
    ui->done_flag = SDL_TRUE;
    return;
  }

  InputBoxResult r;
  switch (ui->loginPage.focus) {
  case -1:
    r = ui_inputbox_event(ui, &ui->loginPage.username, event);
    break;
  case -2:
    r = ui_inputbox_event(ui, &ui->loginPage.password, event);
    break;
  }

  switch (r) {
  case INPUTBOX_COMPLETE:
    ui->page = MODAL_IN_GAME;
    break;
  case INPUTBOX_CONTINUE:
    break;
  }
}

void ui_event_login_page(struct UserInterface *ui, SDL_Event *event)
{
  switch (event->type) {
  case SDL_WINDOWEVENT:
    ui_window_event(ui, event);
    break;

  case SDL_MOUSEBUTTONDOWN:
    ui->page = MODAL_IN_GAME;
    break;

  case SDL_KEYDOWN:
    ui_keyevent_login_page(ui, event);
    break;
  case SDL_KEYUP:
    break;
  case SDL_QUIT:
    ui->done_flag = SDL_TRUE;
    break;
  }
}

