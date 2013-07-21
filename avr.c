// avr.c
// author: Christophe VG <contact@christophe.vg>

// elementary functions for handling the AVR/ATMEGA MCU

#include <avr/io.h>

void avr_init(void) {
  // 1 = output, 0 = input
  // default = all output
  DDRB = 0b11111111;
  DDRC = 0b11111111;  // PORTC0-5 = ADC0->5
  DDRD = 0b11111111;
}
