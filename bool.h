// bool.h
// author: Christophe VG

// introduces the bool type and constants

#ifndef bool

#ifndef NESC          // temp guard for bool definition in nesC
#include <stdint.h>
typedef uint8_t bool;
#define TRUE  1
#define FALSE 0
#endif

#endif
