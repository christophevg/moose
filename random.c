// random.c
// author: Christophe VG

#include "random.h"

// simple LCG. values based on those used by GCC
// from: http://en.wikipedia.org/wiki/Linear_congruential_generator
unsigned int rnd(unsigned int limit) {
  static long u = 123456;   // seed
  long a = 1103515245;      // multiplier
  long m = 2L << 24;        // modulus
  long c = 12345;           // increment
  u = (a * u + c) % m;
  return (u / (m/1.0)) * limit;
}
