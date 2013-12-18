// clock.h
// author: Christophe VG <contact@christophe.vg>

// a clock that counts miliseconds

// this code sets up timer1 for a 1ms  @ 8Mhz Clock (Mode 12)

#ifndef __CLOCK_H
#define __CLOCK_H

#define time_t unsigned long

void   clock_init(void);
time_t clock_get_millis(void);
void   clock_adjust(long);

#endif
