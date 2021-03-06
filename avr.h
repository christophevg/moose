// avr.h
// author: Christophe VG <contact@christophe.vg>

#ifndef __AVR_H
#define __AVR_H

// this is a little trick to be able to use externally defined variables
// to use it, add one more #define <name-of-var>_name = STR(<name-of-var>)
#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)

// always provide (complete compile-time enabled) debugging facilities
// checks DEBUG variable, which can come from outside
#include "debug.h"

#ifndef F_CPU     // should come in from the Makefile
#define F_CPU  18000000
#endif

#include "pins.h"

// two simple macros to turn on/off bits on a PORT/PIN
// taken from tutorial by Sparkfun
#define avr_set_bit   (port, pin) ((port) |= (uint8_t) (1 << pin))
#define avr_clear_bit (port, pin) ((port) &= (uint8_t)~(1 << pin))
#define avr_bit_is_set(port, pin) ((port) &  (uint8_t) (1 << pin))

// elementary functions for handling the AVR/ATMEGA MCU
void     avr_init(void);
void     avr_adc_init(void);
uint16_t avr_adc_read(uint8_t ch);
uint16_t avr_get_vcc(void);

#endif
