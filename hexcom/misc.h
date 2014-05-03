#ifndef _H_HEXCOM_MISC
#define _H_HEXCOM_MISC

#include <vector>
#include <stdint.h>
#include <string>

long real_time(void);    // real time in microseconds

static inline double real_ftime(void) {
  return real_time() * 1.0e-6;
}

/**
 *  Load a file from disk
 */

const char *load_file(const char *path, size_t *len);

/**
 *  Likewise, but build a std::vector instead
 */
std::vector<uint8_t> *load_file(const char *path);

#endif /* _H_HEXCOM_MISC */
