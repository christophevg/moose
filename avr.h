// avr.h
// author: Christophe VG <contact@christophe.vg>

#ifndef __AVR_H
#define __AVR_H

#ifndef F_CPU     // should come in from the Makefile
#define F_CPU  18000000
#endif

#include "pins.h"

// two simple macros to turn on/off bits on a PORT/PIN
// taken from tutorial by Sparkfun
#define avr_set_bit(var, mask) ((var) |= (uint8_t)(1 << mask))
#define avr_clear_bit(var, mask) ((var) &= (uint8_t)~(1 << mask))

// elementary functions for handling the AVR/ATMEGA MCU
void     avr_init(void);
void     avr_adc_init(void);
uint16_t avr_adc_read(uint8_t ch);
uint16_t avr_get_vcc(void);

#endif
