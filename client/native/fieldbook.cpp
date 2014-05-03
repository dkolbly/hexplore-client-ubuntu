#include "ui.h"
#include "fieldbook.h"

struct FieldPage {
  std::string           title;
  Picture              *picture;
  std::string           text;
};

FieldBookWidget::FieldBookWidget(UserInterface *owner)
  : ui(owner),
    fieldbook_overlay(NULL),
    fieldbook_state(0),
    fieldbook_open(NULL),
    fieldbook_closed(NULL),
    fieldbook_dirty(false),
    page_num(0)
{
  pages.resize(10);
  {
    FieldPage *p = new FieldPage();
    p->title = "Contents";
    pages[0] = p;
  }

  {
    FieldPage *p = new FieldPage();
    p->title = "Bluebonnet";
    p->picture = Picture::load_png("textures/bluebonnet2.png");
    p->text = "Scientific Name:\n  Lupinus texensis\n\nthe state flower of\nTexas";
    pages[2] = p;
  }
}

void FieldBookWidget::widget_deactivate()
{
  printf("Fieldbook DEACTIVATE\n");
  fieldbook_state = 0;
}

void FieldBookWidget::widget_activate()
{
  printf("Fieldbook ACTIVATE\n");
  if (!fieldbook_overlay) {
    fieldbook_overlay = new OverlayImage(800, 600);
    fieldbook_open = Picture::load_png("textures/field-book-open.png");
    fieldbook_closed = Picture::load_png("textures/field-book-closed.png");
  }
  fieldbook_state = 1;
  fieldbook_dirty = true;
}

void FieldBookWidget::widget_render()
{
  if (fieldbook_state != 0) {
    ui_render_overlay(ui, fieldbook_overlay, 0, 0, 1.0);
  }
}

bool FieldBookWidget::widget_keypress(SDL_Event const& event)
{
  printf("Fieldbook KEY\n");
  if (fieldbook_state == 0) {
    // we are not active, don't consume the event
    return false;
  }

  if (event.key.type != SDL_KEYDOWN) {
    return true;
  }
  if (event.key.keysym.sym == SDLK_ESCAPE) {
    widget_deactivate();
    return true;
  }
  if (fieldbook_state != 2) {
    // we are not open, ignore the event
    return true;
  }

  switch (event.key.keysym.sym) {
  case SDLK_HOME:
    page_num = 0;
    fieldbook_dirty = true;
    break;

  case SDLK_PAGEDOWN:
    page_num += 2;
    fieldbook_dirty = true;
    break;

  case SDLK_PAGEUP:
    if (page_num >= 2) {
      page_num -= 2;
      fieldbook_dirty = true;
    }
    break;
  }
  // even if we ignored it, we consumed the event
  return true;
}

bool FieldBookWidget::widget_click(SDL_Event const& event)
{
  if (fieldbook_state == 0) {
    // hmm, we shouldn't be here... we should have been activated first
    return false;
  }
  if (fieldbook_state == 1) {
    fieldbook_state = 2;
    fieldbook_dirty = true;
    return true;
  }
  return true;
}

void FieldBookWidget::draw_page(unsigned pagenum)
{
  FieldPage *p = NULL;
  printf("draw_page(%u)\n", pagenum);
  if (pagenum < pages.size()) {
    p = pages[pagenum];
  }
  if (!p) {
    return;
  }

  int x, y, w;
  if (pagenum & 1) {
    // recto
    x = 293;
    y = 44;
    w = 168;
  } else {
    // verso
    x = 68;
    y = 56;
    w = 183;
  }
  if (p->title.size() > 0) {
    int x0 = x + w/2 - ui->textFont->stringwidth(p->title)/2;
    fieldbook_overlay->write(*ui->textFont, p->title, x0, y);
    y += 12;
  }
  if (p->picture) {
    int x0 = x + w/2 - p->picture->width / 2;
    fieldbook_overlay->paint(*p->picture, x0, y);
    y += p->picture->height + 4;
  }
  if (pagenum == 0) {
    // it's the table of contents
    
    printf("TOC\n");
    unsigned p = 1;
    for (std::vector<FieldPage*>::iterator i = pages.begin()+1;
         i != pages.end();
         ++i, ++p) {
      if (*i && !(*i)->title.empty()) {
        printf("TOC %d\n", p);
        fieldbook_overlay->write(*ui->textFont, (*i)->title, x, y);
        char page[20];
        snprintf(&page[0], sizeof(page), "%u", p);
        int x0 = x + w - ui->textFont->stringwidth(page);
        fieldbook_overlay->write(*ui->textFont, page, x0, y);
        y += 8;
      }
    }
         
  } else {
    FontCursor c(fieldbook_overlay, *ui->textFont, x, y);
    c.write(p->text);
  }
}

void FieldBookWidget::widget_refresh()
{
  if (!fieldbook_dirty) {
    return;
  }
  fieldbook_overlay->clear();

  switch (fieldbook_state) {
  case 1:
    fieldbook_overlay->paint(*fieldbook_closed, 0, 0);
    break;
  case 2:
    fieldbook_overlay->paint(*fieldbook_open, 0, 0);
    break;
  }
  if (fieldbook_state == 2) {
    if (page_num > 0) {
      char verso_num[10];
      snprintf(&verso_num[0], sizeof(verso_num), "%u", page_num);
      fieldbook_overlay->write(*ui->textFont, verso_num, 45, 385);
    }
    char recto_num[10];
    snprintf(&recto_num[0], sizeof(recto_num), "%u", page_num+1);
    fieldbook_overlay->write(*ui->textFont, recto_num, 490, 372);

    // draw the text of the page(s)
    draw_page(page_num);
    draw_page(page_num+1);
  }
  fieldbook_overlay->flush();
  fieldbook_dirty = false;
}
