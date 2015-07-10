// clock.h
// author: Christophe VG <contact@christophe.vg>

// a clock that counts miliseconds

// this code sets up timer1 for a 1ms  @ 8Mhz Clock (Mode 12)

#ifndef __CLOCK_H
#define __CLOCK_H

#define time_t unsigned long

volatile time_t current_millis;

void   clock_init(void);

#define clock_get_millis() current_millis
#define clock_adjust(diff) (current_millis += diff)

#endif
