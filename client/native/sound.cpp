#include "sound.h"
#include "ui.h"
#include <hexcom/hex.h>
#include <vector>
#include <string>
#include <sys/time.h>

#define MAX_VOLUME_R2   (20)

static inline u_int64_t 
rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (u_int64_t)lo)|( ((u_int64_t)hi)<<32 );
}

#define INTERNAL_FREQUENCY      (44100)
#define INTERNAL_FORMAT         (AUDIO_S16SYS)

struct Sample {
  int16_t left;
  int16_t right;
};

struct SoundEffect {
  std::string   name;
  int16_t      *data;
  unsigned      num_samples;
};

SoundEffect *sound_load(const char *path)
{
  SDL_AudioSpec spec, *ret;
  unsigned char *data;
  unsigned len;

  ret = SDL_LoadWAV(path, &spec, &data, &len);
  if (ret == NULL) {
    printf("%s: could not load (%s)\n", path, SDL_GetError());
    return NULL;
  }

  printf("%s: yay!  loaded %u bytes\n", path, len);
  printf("  loaded format=%d (%d bits)\n", ret->format, 
         SDL_AUDIO_BITSIZE(ret->format) );
  printf("         channels=%d\n", ret->channels);
  printf("         buffer=%d\n", ret->samples);
  printf("         freq/rate=%d\n", ret->freq);
  unsigned src_samples = (len * 8) / SDL_AUDIO_BITSIZE(ret->format);
  printf("         samples=%u (%.3f sec)\n", 
         src_samples,
         src_samples / (double)ret->freq);

  SDL_AudioCVT conversion;
  int conv_rc;
  conv_rc = SDL_BuildAudioCVT(&conversion,
                              ret->format,
                              ret->channels,
                              ret->freq,
                              AUDIO_S16SYS,
                              1,
                              INTERNAL_FREQUENCY);
  if (conv_rc == 0) {
    printf("no conversion necessary\n");
    SoundEffect *s = new SoundEffect();
    s->data = (int16_t *)data;
    s->num_samples = src_samples;
    s->name = path;
    return s;
  } else if (conv_rc < 0) {
    printf("no conversion possible\n");
    return NULL;
  } else {
    SDL_assert(conversion.needed);
    conversion.len = len;

    int nsamp = (len * 8) / SDL_AUDIO_BITSIZE(ret->format);
    int tmplen = len * conversion.len_mult;
    printf("allocating %d bytes for conversion buffer of %d samples\n", 
           tmplen, nsamp);
    conversion.buf = (unsigned char *)malloc(tmplen);
    /*for (int i=0; i<nsamp; i++) {
      printf("[%d] %3d\n", i, ((unsigned char *)data)[i]);
      }*/

    memcpy(conversion.buf, data, len);
    int rc = SDL_ConvertAudio(&conversion);
    int outlen = len * conversion.len_ratio;
    printf("rc %d ; cvt_len %d outlen %d\n", rc, conversion.len_cvt, outlen);
    int outsamp = (outlen * 8) / SDL_AUDIO_BITSIZE(INTERNAL_FORMAT);
    printf("output is %d bytes (%d samples or %.2f sec)\n", 
           outlen,
           outsamp,
           outsamp / (double)INTERNAL_FREQUENCY);

    SoundEffect *s = new SoundEffect();
    s->data = (int16_t *)conversion.buf;
    s->num_samples = outsamp;
    s->name = path;
    return s;
  }
}

static inline uint16_t volume_scaler(float v)
{
  if (v < 0.0) {
    return 0;
  } else if (v >= 1.0) {
    return 32768;
  } else {
    return floor(v * 32768.0);
  }
}

struct Source;

struct AudioStream {
  UserInterface         *ui;
  std::vector<Source*> sources;
  float x, y, z;
  float facing;
  SDL_AudioDeviceID device;
};


struct SourceLocation {
  SourceLocation(float _x, float _y, float _z)
    : x(_x), y(_y), z(_z)
  {
  }

  float         x, y, z;

  void mixin(AudioStream *mic, 
             float volume,
             Sample *dest,
             int16_t *source, 
             unsigned count) {
    double dx = x - mic->x;
    double dy = y - mic->y;
    double dz = z - mic->z;
    double a = atan2(dy, dx) - mic->facing;
    double r2 = dx*dx + dy*dy + dz*dz;
    if (r2 <= MAX_VOLUME_R2) {
      r2 = 1;
    } else {
      r2 = MAX_VOLUME_R2 / r2;
    }
    double balance = sin(a);
    float left = volume * (1.0 + balance) * r2;
    float right = volume * (1.0 - balance) * r2;

    int left_volume = volume_scaler(left);
    int right_volume = volume_scaler(right);
    /*printf("mixin angle (%.1f,%.1f) %.4f+%.4f %.4f degrees %d <--> %d\n", 
           dx, dy,
           atan2(dy,dx),
           mic->facing,
           a,
           left_volume, right_volume);*/

    for (unsigned i=0; i<count; i++) {
      dest[i].left += ((int)source[i] * left_volume) >> 16;
      dest[i].right += ((int)source[i] * right_volume) >> 16;
    }
  }
};

struct Source {
  SoundEffect          *base;
  unsigned              index;
  float                 volume;
  SourceLocation        loc;

  Source(SoundEffect *base, float vol, SourceLocation const& l)
    : base(base),
      index(0),
      volume(vol),
      loc(l)
  {
    /*
    float balance = sin(angle);
    float left = vol * (1.0 + balance) / 2.0;
    float right = vol * (1.0 - balance) / 2.0;
    left_volume = volume_scaler(left);
    right_volume = volume_scaler(right);
    */
    /*printf("Balance at angle %.3f deg ==> %.3f--%.3f   %5d--%5d\n",
           (angle * 180.0/PI),
           left,
           right,
           left_volume,
           right_volume);*/
  }
    
};

void audioCallback(void *userdata,
                   unsigned char *stream,
                   int len)
{
  //uint64_t t0 = rdtsc();
  //  printf("%lu\n", t0);
  struct AudioStream *as = (struct AudioStream *)userdata;
  struct Sample *p = (struct Sample *)stream;
  int n = len / sizeof(struct Sample);

  as->x = as->ui->location[0];
  as->y = as->ui->location[1];
  as->z = as->ui->location[2];
  as->facing = DEG_TO_RAD(as->ui->facing);

  memset(stream, 0, len);

  for (std::vector<Source*>::iterator i=as->sources.begin();
       i != as->sources.end();) {
    Source *s = *i;
    int use = n;
    bool kill = false;
    if (s->index + use > s->base->num_samples) {
      use = s->base->num_samples - s->index;
      kill = true;
    }
    int16_t *src = &s->base->data[s->index];
    s->loc.mixin(as, s->volume, &p[0], src, use);
    s->index += use;

    if (kill) {
      // remove it from the list
      as->sources.erase(i);
      // and destroy it
      delete s;
    } else {
      // move on to the next one
      ++i;
    }
  }
  /*
  uint64_t t1 = rdtsc();
  struct timeval tv;
  gettimeofday(&tv, NULL);
  double rt = tv.tv_sec + tv.tv_usec * 1.0e-6;
  double usec = (t1-t0)/2.0e9/1.0e-6;
  printf("%.3f %.3f usec ~~ %.1f%%\n", rt, usec, usec / (0.021*1.0e6));
  */
}

/*unsigned turn_facing(unsigned interval, void *param)
{
  AudioStream *as = (AudioStream *)param;
  as->facing += 0.1;
  return interval;
}
*/
int sound_init(UserInterface *ui)
{
  int num = SDL_GetNumAudioDevices(0);
  printf("device 0 (of %d) = <%s>\n", num, SDL_GetAudioDeviceName(0,0));
  const char *devname = SDL_GetAudioDeviceName(0,0);
  SDL_AudioSpec want, have;
  SDL_zero(want);
  want.freq = 44100;
  want.format = AUDIO_S16SYS;
  want.channels = 2;
  want.samples = 2048;
  want.callback = audioCallback;

  struct AudioStream *stream = new AudioStream();
  stream->x = 10;
  stream->y = 10;
  stream->z = 0;
  stream->facing = PI/2;
  stream->ui = ui;
  ui->sounds.stream = stream;

  want.userdata = stream;

  stream->device = SDL_OpenAudioDevice(devname,
                                       0,
                                       &want,
                                       &have,
                                       // we're lazy... let SDL do any
                                       // converting necessary
                                       0);
  if (stream->device <= 0) {
    fprintf(stderr, "Sound open failed: %s\n", SDL_GetError());
    return -1;
  }
  printf("format want=%#x have=%#x\n", want.format, have.format);
  printf("freq want=%d have=%d\n", want.freq, have.freq);
 
  SDL_PauseAudioDevice(stream->device, 0);
 
  return 0;
}

void sound_play(UserInterface *ui, SoundEffect *base, float vol, double x, double y, double z)
{
  ui->sounds.stream->sources.push_back(new Source(base, vol, SourceLocation(x,y,z)));
}

/* Conversion...

avconv -i ../explosion_x.wav /tmp/temp.mp3
avconv -i /tmp/temp.mp3 testx.wav
*/
