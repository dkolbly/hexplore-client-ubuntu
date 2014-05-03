#ifdef UNIT_TEST
#include <stdio.h>
#include <string.h>
#include <vector>
#include <assert.h>

struct Span {
  unsigned short height;
  unsigned char type;
  unsigned char flags;
};

#include <vector>
typedef std::vector<Span> SpanVector;
#endif

////////////////////////////////////////////////////////////////////////

static inline bool mergeable(SpanVector::iterator const& a, SpanVector::iterator const& b)
{
  return ((a->type == b->type) && (a->flags == b->flags));
}

void span_merge(SpanVector *vec, SpanVector::iterator const& j)
{
#if SPAN_INSERT_DEBUG
  printf("span_merge at [%zd]\n", j - vec->begin());
#endif
  bool left_merge = false;
  bool right_merge = false;

  if (j != vec->begin()) {
    SpanVector::iterator l = j-1;
    left_merge = mergeable(l, j);
  }

  if ((j+1) != vec->end()) {
    SpanVector::iterator r = j+1;
    right_merge = mergeable(j, r);
  }

  if (left_merge) {
    if (right_merge) {
#if SPAN_INSERT_DEBUG
      printf("double merge\n");
#endif
      j[-1].height += j[0].height + j[1].height;
      vec->erase(j, j+2);
    } else {
#if SPAN_INSERT_DEBUG
      printf("left merge\n");
#endif
      j[-1].height += j[0].height;
      vec->erase(j, j+1);
    }
  } else if (right_merge) {
#if SPAN_INSERT_DEBUG
    printf("right merge\n");
#endif
    j[0].height += j[1].height;
    vec->erase(j+1, j+2);
  }
}

void insert_span(SpanVector *vec, unsigned short bottom, Span const& span)
{
  SpanVector::iterator i0;

  for (i0=vec->begin(); i0!=vec->end(); ++i0) {
#if SPAN_INSERT_DEBUG
    // bottom is the Z coordinate (relative to the Z of i0) that is
    // the bottom of the span we're inserting.  Hence, it is always
    // non-negative because we break out before it becomes negative
    // (that's "case 2")
    printf("top of loop [%zu] bottom=%u height=%u\n",
           i0-vec->begin(),
           bottom,
           i0->height);
#endif
    if (bottom < i0->height) {
      // Case 2: we found a span that starts before where we are trying to insert
      // and ends after that; the span needs to be split
#if SPAN_INSERT_DEBUG
      printf("case 2 @ %u\n", bottom);
#endif
      int remain = i0->height - bottom;
      remain -= span.height;
#if SPAN_INSERT_DEBUG
      printf("h=%d - b=%d - sh=%d = %d\n", i0->height, bottom, span.height, remain);
#endif
      Span rest = *i0;
      SpanVector::iterator i1;
      if (bottom == 0) {
        // there's nothing left of the part of the span below the bottom, so replace
        // it with the new one
        *i0 = span;
        i1 = i0;
#if SPAN_INSERT_DEBUG
        printf("  replaced span because nothing below it\n");
#endif
      } else {
#if SPAN_INSERT_DEBUG
        printf("  adjusting span to height=%u, inserting new span\n", bottom);
#endif
        i0->height = bottom;
        i1 = vec->insert(i0+1, span);
      }
      // At this point, i1 refers to the new span we're inserting.
      // It might be the same as i0 if we replaced i0, otherwise it's a
      // new one.  i0 has been invalidated
      if (remain > 0) {
#if SPAN_INSERT_DEBUG
        printf("case 2a %d\n", remain);
#endif
        // the new span completely covers the remainder
        //           |<---------OLD--------------------------->|
        //           |<---OLD'--->|<----NEW--->|<----REMAIN--->|
        //                        ^
        //                        i1
        rest.height = remain;
        span_merge(vec, vec->insert(i1+1, rest)-1);
      } else if (remain == 0) {
#if SPAN_INSERT_DEBUG
        printf("case 2b %d\n", remain);
#endif
        // the new span exactly covers the remainder
        //           |<---------OLD----------->|
        //           |<---OLD'--->|<----NEW--->|
        //                        ^
        //                        i1
        span_merge(vec, i1);
      } else {
#if SPAN_INSERT_DEBUG
        printf("case 2c %d\n", remain);
#endif
        // the new span covers the remainder and then some
        // we need to eat up the covered stuff
        //                                     |<--remain-->|
        //           |<---------OLD----------->|
        //           |<---OLD'--->|<----NEW---------------->|
        //                        ^
        //                        i1
        for (SpanVector::iterator j=i1+1; j!=vec->end(); ++j) {
#if SPAN_INSERT_DEBUG
          printf("case 2c... [%zu] remain=%d height=%u\n",
                 j - vec->begin(),
                 remain,
                 j->height);
#endif
          remain += j->height;
          if (remain > 0) {
#if SPAN_INSERT_DEBUG
            // we have run j up far enough to pass the remainder
            // (note that i1 refers to post-OLD
            //                                       A    B    C
            //           |<---------OLD----------->|<->|<->|<----->|
            //
            //           |<---OLD'--->|<----NEW--------------->|
            //                        ^            |<->|<->|<--:-->|
            //                        i1             A    B    :C  :
            //                                     ^       ^   :   :
            //                                   i1+1      j   :   :
            //                                                 |<->|
            //                                                 remain
            printf("case 2c/1 ; removing %zu spans and adjusting [%zu] to height %d\n", 
                   (j - (i1+1)),
                   j - vec->begin(),
                   remain);
#endif
            // why, oh why, would this assertion be here??? assert(bottom > 0);
            j->height = remain;
            span_merge(vec, vec->erase(i1+1, j)-1);
            goto done;
          } else if (remain == 0) {
#if SPAN_INSERT_DEBUG
            printf("case 2c/2 %d\n", remain);
#endif
            span_merge(vec, vec->erase(i1+1, j+1)-1);
            goto done;
          } else {
#if SPAN_INSERT_DEBUG
            printf("case 2c/3 %d\n", remain);
#endif
          }
        }
#if SPAN_INSERT_DEBUG
        printf("case 2c/4 %d\n", remain);
#endif
        span_merge(vec, vec->erase(i1+1, vec->end())-1);
      }
      goto done;
    }
    bottom -= i0->height;
  }
  // Case 1: we ran through all the spans and still have headroom;
  // that means we need to strictly extend the spans with
  // (a) a gap, and (b) the new span
  if (bottom > 0) {
    if ((span.type == 0) && (span.flags == 0)) {
      // although we're only adding space on top of space...
      // there's an easy optimization for that :-)
      goto done;
    }
    Span gap;
    gap.height = bottom;
    gap.type = 0;
    gap.flags = 0;
    vec->push_back(gap);
  } else {
    // there's a possibility we could merge the new span with the
    // last one
    if (!vec->empty()) {
      Span *last = &vec->back();
      if ((last->type == span.type) && (last->flags == span.flags)) {
        // yup, just make the last span bigger
        last->height += span.height;
        goto done;
      }
    }
  }
  vec->push_back(span);
 done:
  // double-check
  for (i0=vec->begin(); i0!=vec->end(); ++i0) {
    assert(i0->height > 0);
  }
  // if we left space on the top of the column, strip it off
  assert(!vec->empty());
  if (vec->back().type == 0) {
    vec->pop_back();
  }
  // note that there shouldn't be more than one if we did our merges right
  if (!vec->empty()) {
    assert(vec->back().type != 0);
  }
}

////////////////////////////////////////////////////////////////////////

#ifdef UNIT_TEST
void print_result(const char *f, int l, SpanVector& v)
{
  printf("%s:%d: ", f, l);
  unsigned z = 0;
  for (SpanVector::iterator i=v.begin(); i!=v.end(); ++i) {
    printf("%03d-%03d:%d ", z, z + i->height, i->type);
    z += i->height;
  }
  printf("\n");

  char line[72];
  for (unsigned i=0; i<72; i++) {
    line[i] = (i % 10) + '0';
  }
  line[sizeof(line)-1] = '\0';
  printf("%s:%d: %s\n", f, l, line);

  memset(&line[0], ' ', sizeof(line));
  line[sizeof(line)-1] = '\0';

  z = 0;
  for (SpanVector::iterator i=v.begin(); i!=v.end(); ++i) {
    char ch = ' ';
    switch (i->type) {
    case 0: ch = ' '; break;
    case 1: ch = '*'; break;
    case 2: ch = '+'; break;
    case 3: ch = '@'; break;
    case 4: ch = '#'; break;
    case 240: ch = '~'; break;
    default: ch = '-'; break;
    }
    memset(&line[z], ch, i->height);
    z += i->height;
  }
  printf("%s:%d: %s\n", f, l, line);
}

#define PRINT_RESULT(x) print_result(__FILE__,__LINE__,x)

void test_appending()
{
  SpanVector vec;
  Span A = {3, 1, 0};
  insert_span(&vec, 20, A);
  PRINT_RESULT(vec);

  Span C = {5, 0, 0};
  insert_span(&vec, 60, C);     // adding space on top of space; should do nothinig
  PRINT_RESULT(vec);

  insert_span(&vec, 25, A);
  PRINT_RESULT(vec);

  Span B = {5, 3, 0};
  insert_span(&vec, 55, B);
  PRINT_RESULT(vec);
  insert_span(&vec, 60, B);
  PRINT_RESULT(vec);
}

void test_fully_interior()
{
  SpanVector vec;
  Span A = {5, 1, 0};
  insert_span(&vec, 60, A);
  PRINT_RESULT(vec);

  insert_span(&vec, 10, A);
  PRINT_RESULT(vec);

  Span B = {3, 2, 0};
  insert_span(&vec, 12, B);
  PRINT_RESULT(vec);

  insert_span(&vec, 60, B);
  PRINT_RESULT(vec);
}

void test_overhang()
{
  SpanVector vec;
  Span A = {20, 1, 0};
  Span B = {5, 2, 0};
  insert_span(&vec, 10, A);
  PRINT_RESULT(vec);

  insert_span(&vec, 27, B);
  PRINT_RESULT(vec);

  insert_span(&vec, 45, A);
  PRINT_RESULT(vec);

  Span C = {4, 3, 0};
  insert_span(&vec, 30, C);
  PRINT_RESULT(vec);
}

void test_supermerge()
{
  SpanVector vec;
  Span A = {10, 1, 0};
  insert_span(&vec, 10, A);
  insert_span(&vec, 20, A);
  insert_span(&vec, 30, A);

  Span B = {4, 2, 0};
  insert_span(&vec, 18, B);
  Span C = {1, 0, 0};
  insert_span(&vec, 18+4, C);
  PRINT_RESULT(vec);

  insert_span(&vec, 17, A);
  PRINT_RESULT(vec);
}

void test_interior_insert()
{
  SpanVector vec;
  Span A = {10, 3, 0}; // height, type, flags
  vec.push_back(A);
  Span B = {8, 1, 0}; // height, type, flags
  vec.push_back(B);
  Span C = {2, 0, 0}; // height, type, flags
  vec.push_back(C);
  Span D = {2, 200, 3}; // height, type, flags
  vec.push_back(D);
  Span E = {2, 240, 3}; // height, type, flags
  vec.push_back(E);
  Span F = {3, 0, 3}; // height, type, flags
  vec.push_back(F);
  Span G = {3, 1, 1}; // height, type, flags
  vec.push_back(G);

  PRINT_RESULT(vec);
  for (int i=8; i<25; i++) {
    SpanVector v2(vec);
    Span X = {2, 3, 8};
    insert_span(&v2, i, X);
    PRINT_RESULT(v2);
  }
  for (int i=8; i<25; i++) {
    SpanVector v2(vec);
    Span X = {3, 3, 8};
    insert_span(&v2, i, X);
    PRINT_RESULT(v2);
  }
  for (int i=8; i<32; i++) {
    SpanVector v2(vec);
    Span X = {5, 3, 8};
    insert_span(&v2, i, X);
    PRINT_RESULT(v2);
  }
}

int main()
{
  /*  printf("---------------------------------\n");
  test_appending();
  printf("---------------------------------\n");
  test_fully_interior();
  printf("---------------------------------\n");
  test_overhang();
  printf("---------------------------------\n");
  test_supermerge();
  printf("---------------------------------\n");
  */
  test_interior_insert();
}
#endif /* UNIT_TEST */
