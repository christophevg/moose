// avr.c
// author: Christophe VG <contact@christophe.vg>

// elementary functions for handling the AVR/ATMEGA MCU

#include <avr/io.h>
#include <util/delay.h>

#include "avr.h"

void avr_init(void) {
  // 1 = output, 0 = input
  // default = all output
  DDRB = 0b11111111;
  DDRC = 0b11111111;  // PORTC0-5 = ADC0->5
  DDRD = 0b11111111;
}

void avr_adc_init(void) {
  // AREF = AVcc
  ADMUX = (1 << REFS0);

  // ADC Enable and prescaler of 128
  // 18000000/128 = 140625
  ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
}

uint16_t avr_adc_read(uint8_t ch) {
  // select the corresponding channel 0~7
  // ANDing with ’7′ will always keep the value
  // of ‘ch’ between 0 and 7
  // ch &= 0b00000111;            // AND operation with 7
  // this trick doesn't work with 0b00000101 ;-)
  if( ch < 0 ) { ch = 0; } else if( ch > 5 ) { ch = 5; }
  ADMUX = (ADMUX & 0xF8) | ch; // clears the bottom 3 bits before ORing

  // start single convertion
  // write ’1′ to ADSC
  avr_set_bit(ADCSRA, ADSC);

  // wait for conversion to complete
  // ADSC becomes ’0′ again
  // till then, run loop continuously
  while( ADCSRA & (1 << ADSC) );

  // clear ADIF by writing one to it
  avr_set_bit(ADCSRA, ADIF);

  return (ADC);
}

uint8_t avr_get_vcc(void) {
  uint8_t vcc;

  // set the Band Gap voltage as the ADC input
  ADMUX = 0xE; 

  _delay_ms(50);

  // TODO: figure this out ;-)
  ADCSRA = (1 << ADEN) | (1 << ADATE) | (1 << ADIE) | (1 << ADSC) | 5;

  // read 8 bit value 
  vcc = ADC >> 2;

  // reset to normal ADC references
  avr_adc_init();

  return vcc;
}