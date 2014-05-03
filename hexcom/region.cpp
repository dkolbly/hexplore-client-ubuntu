#include <string.h>
#include "region.h"

#include "spanvec.cpp"

void expandSpan(Region *rgn, SpanVector const& vec, int z_start, int h,
                unsigned char *types,
                unsigned char *flags)
{
  int z0 = rgn->basement;
  int z_limit = z_start + h;
  if (types) {
    memset(types, 0, h);
  }
  if (flags) {
    memset(flags, 0, h);
  }
  for (SpanVector::const_iterator i = vec.begin();
       i != vec.end();
       ++i) {
    int z1 = z0 + i->height;
    if ((z0 < z_limit) && (z1 > z_start)) {
      // there is some overlap
      int j0 = z0 - z_start;
      if (j0 < 0) {
        j0 = 0;
      }
      int j1 = z1 - z_start;
      if (j1 >= h) {
        j1 = h;
      }
      /*printf("span %d:%d  covers %d:%d of the %d/%d expansion\n",
             z0, z1,
             j0, j1,
             z_start, h);*/
      assert((j0 >= 0) && (j0 < h));
      assert((j1 > 0) && (j1 <= h));
      if (types) {
        memset(&types[j0], i->type, (j1-j0));
      }
      if (flags) {
        memset(&flags[j0], i->flags, (j1-j0));
      }
    }
    z0 = z1;
    if (z0 >= z_limit) {
      // early exit if there's more stuff but all above our area of interest
      break;
    }
  }
}

static inline char blocktypechar(uint8_t t)
{
  switch (t) {
  case 0: return ' ';
  case 1: case 2: case 3: case 4: case 5:
  case 6: case 7: case 8: case 9:
    return t + '0';
  case 240: return '~';
  case 255: return '?';
  default: return '#';
  }
}

int Region::set(int x, int y, int z, int height, int type, int flags)
{
  int ix = x - origin.x;
  int iy = y - origin.y;
  int iz = z - basement;

  printf("dset(%d,%d) at %d for %d to %d (flags %d)\n",
         ix, iy, iz, height, type, flags);
  if ((ix < 0) || (ix >= REGION_SIZE) || (iy < 0) || (iy >= REGION_SIZE)) {
    return -1;
  }

  {
    unsigned char buf[64];
    expandSpan(this, columns[iy][ix], z-32, 64, &buf[0], NULL);

    printf("        ");
    for (int i=0; i<64; i++) {
      putchar((i==32) ? '|' : ' ');
    }
    printf("\n");
    printf("BEFORE: ");
    for (int i=0; i<64; i++) {
      putchar(blocktypechar(buf[i]));
    }
    printf("\n");
  }

  Span s;
  s.height = height;
  s.type = type;
  s.flags = flags;

  insert_span(&columns[iy][ix], iz, s);

  {
    unsigned char buf[64];
    expandSpan(this, columns[iy][ix], z-32, 64, &buf[0], NULL);

    printf(" AFTER: ");
    for (int i=0; i<64; i++) {
      putchar(blocktypechar(buf[i]));
    }
    printf("\n");
  }

  return 0;
}

void Region::unpack(std::string const& encoded)
{
  static bool verbose = 0;
  // Decode the span array
  unsigned char *p0 = (unsigned char *)&encoded[0];
  unsigned char *p = p0;

  for (int y=0; y<REGION_SIZE; y++) {
    for (int x=0; x<REGION_SIZE; x++) {
      bool cverbose = (verbose && (x == 0) && (y == 2));
      std::vector<Span>& col = columns[y][x];
      while (*p) {
        if (cverbose) {
          printf(": %02x %02x %02x %02x %02x %02x\n", p[0], p[1], p[2], p[3], p[4], p[5]);
        }
        unsigned h = *p++;
        if (h == 0x7F) {
          h = ((unsigned)p[0] << 8) + p[1];
          p += 2;
        }
        Span s;
        s.height = h;
        s.type = p[0];
        s.flags = p[1];
        p += 2;
        if (cverbose) {
          printf("decoded (%d,%d) - h=%d t=%d f=%d\n", x, y, s.height, s.type, s.flags);
        }
        col.push_back(s);
      }
      if (cverbose) {
        printf("  col has %ld spans\n", col.size());
      }
      p++;
    }
  }
}

void Region::pack(std::string *encoded)
{
  encoded->clear();
  encoded->reserve(100000);

  // encode the region's terrain data into the span array
  //unsigned char *p0 = (unsigned char *)&(*encoded)[0];
  //unsigned char *p = p0;

  for (int y=0; y<REGION_SIZE; y++) {
    for (int x=0; x<REGION_SIZE; x++) {
      std::vector<Span> const& col = columns[y][x];
      for (std::vector<Span>::const_iterator i=col.begin(); i<col.end(); ++i) {
        //printf("** h=%d t=%d f=%d\n", i->height, i->type, i->flags);
        unsigned h = i->height;
        uint8_t buf[8];

        assert(h>0);

        buf[6] = i->type;
        buf[7] = i->flags;
        if (h > 0x7E) {
          buf[3] = 0x7F;
          buf[4] = h >> 8;
          buf[5] = h;
          encoded->append((char*)&buf[3], 5);
        } else {
          buf[5] = h;
          encoded->append((char*)&buf[5], 3);
        }
      }
      //*p++ = 0;
      encoded->append(1, '\0');
    }
  }
  //assert((p-p0) < 100000);
  //  encoded->resize(p-p0);
}

