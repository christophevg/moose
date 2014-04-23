// random.c
// author: Christophe VG

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "../random.h"

int main(void) {
  int avg = 0;
  for(int i=0; i<100; i++) {
    unsigned int value = rnd(100);
    assert(value < 100);
    if( value < 50 ) { avg++; }
  }
  assert(avg == 50);
  exit(EXIT_SUCCESS);
}
