#ifndef _H_HEXPLORE_SOUND
#define _H_HEXPLORE_SOUND

struct SoundEffect;
struct UserInterface;

int sound_init(UserInterface *ui);
SoundEffect *sound_load(const char *path);
void sound_play(UserInterface *ui, SoundEffect *base, float vol, double x, double y, double z);


#endif /* _H_HEXPLORE_SOUND */
