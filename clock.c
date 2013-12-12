// clock.c
// author: Christophe VG <contact@christophe.vg>

// a clock that counts miliseconds

// this code sets up timer1 for a 1ms  @ 8Mhz Clock (Mode 12)

// TODO: make this ATMEGA1284p-only code useable on ATMEGA8 & co

#include <avr/io.h>
#include <avr/interrupt.h>

#include "clock.h"

volatile static unsigned long ticks = 0;

void clock_init(void) { 
  // Clock Speed (8000000) / PreScaler(64) = 125000
  // * timeout (0.001sec)                  = 125
  // - 1                                   = 124
  ICR1 = 124;

  TCCR1B |= (1 << WGM13) | (1 << WGM12); // mode 12, CTC on ICR1
  TCCR1B |= (1 << CS10)  | (1 << CS11);  // prescaler of 64
    
  TIMSK1 |= (1 << OCIE1B);               // enable CTC interrupt 
} 

unsigned long clock_get_millis(void) {
  return ticks;
}

void clock_adjust(long diff) {
  ticks += diff;
}

ISR (TIMER1_COMPB_vect) {
  ticks++;
}
