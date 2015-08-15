// gps.c
// author: Christophe VG <contact@christophe.vg>

// functions to access a GPS module via serial connection

#include <stdio.h>

#include <avr/interrupt.h>

#include "gps.h"
#include "nmea.h"

// cyclic IO buffers
typedef struct {
  volatile uint8_t head;
  volatile uint8_t tail;
  volatile uint8_t buffer[0xff];
} cyclic_buffer_t;

volatile cyclic_buffer_t incoming = {0, 0, {0}};

// interrupt vector for handling reception of a single byte
ISR (USARTg_RX_vect) {
  incoming.buffer[incoming.tail++] = UDRg;
}

static bool _data_available(void) {
  return incoming.head != incoming.tail;
}

// blocking !
static uint8_t _receive_byte(void) {
  while( ! _data_available() );
  return incoming.buffer[incoming.head++];
}

// TODO: unused for now, maybe future use
// // blocking !
// static uint8_t _peek_byte(void) {
//   while( ! _data_available() );
//   return incoming.buffer[incoming.head];
// }
//
// static void _buffer_info(void) {
//   debug_printf("buffer: head = %i : ", incoming.head);
//   for(uint8_t i=incoming.head;i!=incoming.tail;i++) {
//     debug_printf("%c", incoming.buffer[i]);
//   }
//   debug_printf(" / tail = %i\n", incoming.tail);
// }

// public interface

// initialization, uses generic register names that should be defined in the
// header file.
void gps_init(void) {
  // make RX pin input pin by clearing it
  avr_clear_bit(GPS_RX_PORT, GPS_RX_PIN);

  // compute baud rate setting
  // thanks: http://www.appelsiini.net/2011/simple-usart-with-avr-libc
  #undef BAUD
  #define BAUD 4800
  #include <util/setbaud.h>
  
  UBRRgH = UBRRH_VALUE;
  UBRRgL = UBRRL_VALUE;

  #if USE_2X
    UCSRgA |= _BV(U2Xg);
  #else
    UCSRgA &= ~(_BV(U2Xg));
  #endif

  UCSRgC = _BV(UCSZg1) | _BV(UCSZg0); // 8-bit data
  UCSRgB = _BV(RXENg)  | _BV(TXENg);  // Enable RX and TX

  // https://sites.google.com/site/qeewiki/books/avr-guide/usart  
  UCSRgB |= (1 << RXCIEg);            // enable RX interrupt to accept bytes
  UCSRgB |= (1 << TXCIEg);            // enable TX interrupt to see EOT
  
  // enable interrupts
  sei();
}

void gps_receive(void) {
  while( _data_available() ) {
    nmea_parse(_receive_byte());
  }
}
