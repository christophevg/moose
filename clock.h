// clock.h
// author: Christophe VG <contact@christophe.vg>

// a clock that counts miliseconds

// this code sets up timer1 for a 1ms  @ 8Mhz Clock (Mode 12)

#ifndef __CLOCK_H
#define __CLOCK_H

void          clock_init(void);
unsigned long clock_get_millis(void);
void          clock_adjust(long);

#endif
