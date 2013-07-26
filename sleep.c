// avr.c
// author: Christophe VG <contact@christophe.vg>

// support for sleeping
// based on: http://www.gammon.com.au/forum/?id=11497

#include <avr/io.h>

#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#include <util/delay.h>

#include "sleep.h"

// watchdog interrupt
ISR (WDT_vect) {
  wdt_disable();  // disable watchdog
}

void sleep_init(void) {
  // turn on interrupts
  sei();
}

void sleep_ms(uint32_t ms)  {
  // TODO: dynamically changing the watchdog's time-out doesn't seem to work
  //       for now only 1 second resolution ;-)
  uint32_t step;
  
  // to avoid garbage (e.g. on serial)
  _delay_ms(10);
  
  // disable ADC
  ADCSRA = 0;  

  while(ms > 0) {
    // clear various "reset" flags
    MCUSR = 0;     
    // allow changes, disable reset
    WDTCSR = _BV (WDCE) | _BV (WDE);

    // set interrupt mode and an interval 
    // set WDIE and delay of ...
    /*
    if( ms > 8000 ) {
      WDTCSR = _BV (WDIE) | _BV (WDP3) | _BV (WDP0);              // 8 seconds
      step = 8000;
    } else if( ms > 4000 ) {
      WDTCSR = _BV (WDIE) | _BV (WDP3);                           // 4 seconds
      step = 4000;
    } else if( ms > 2000 ) {
      WDTCSR = _BV (WDIE) | _BV (WDP2) | _BV (WDP1) | _BV (WDP0); // 2 seconds
      step = 2000;
    } else if( ms > 1000 ) { */
      WDTCSR = _BV (WDIE) | _BV (WDP2) | _BV (WDP1);              // 1 second
      step = 1000;
    /*
    } else if( ms > 500 ) {
      WDTCSR = _BV (WDIE) | _BV (WDP2) | _BV (WDP0);              // 1/2 second
      step = 500;
    } else if( ms > 250 ) {
      WDTCSR = _BV (WDIE) | _BV (WDP2);                           // 1/4 second
      step = 250;
    } else if( ms > 125 ) {
      WDTCSR = _BV (WDIE) | _BV (WDP1) | _BV (WDP0);              // 1/8 second
      step = 125;
    } else if( ms > 64 ) {
      WDTCSR = _BV (WDIE) | _BV (WDP1);                           // 1/16 second
      step = 64;
    } else if( ms > 32 ) {
      WDTCSR = _BV (WDIE) | _BV (WDP0);                           // 1/32 second
      step = 32;
    } else  {
      WDTCSR = _BV (WDIE);                                        // 1/64 second
      step = 16;
    }
    */
    wdt_reset();  // pat the dog
  
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);  
    sleep_enable();
 
    // BODS seems unknown ...
    // TODO: investigate
    // turn off brown-out enable in software
    // MCUCR = _BV (BODS) | _BV (BODSE);
    // MCUCR = _BV (BODS); 

    sleep_cpu ();
  
    // cancel sleep as a precaution
    sleep_disable(); 
    
    ms = ms - step;
  }
}
