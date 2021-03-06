// serial.c
// author: Christophe VG <contact@christophe.vg>

// functions to operate the UART on AVR/ATMega

#include "serial.h"

// trick to make printf "print" to serial<->UART
static FILE mystdout = FDEV_SETUP_STREAM(serial_putchar, NULL, _FDEV_SETUP_WRITE);

void serial_init(void) {
  // make RX pin input pin by clearing it
  avr_clear_bit(SERIAL_RX_PORT, SERIAL_RX_PIN);

  // compute baud rate setting
  // thanks: http://www.appelsiini.net/2011/simple-usart-with-avr-libc
  #undef BAUD
  #define BAUD 9600
  #include <util/setbaud.h>

  UBRRsH = UBRRH_VALUE;
  UBRRsL = UBRRL_VALUE;

#if USE_2X
  UCSRsA |= _BV(U2Xs);
#else
  UCSRsA &= ~(_BV(U2Xs));
#endif

  UCSRsC = _BV(UCSZs1) | _BV(UCSZs0); // 8-bit data
  UCSRsB = _BV(RXENs)  | _BV(TXENs);  // Enable RX and TX

  stdout = &mystdout; // required for printf init
}

int serial_putchar(char c, FILE *stream) {
  if (c == '\n') serial_putchar('\r', stream); // add a CR before the LF

  loop_until_bit_is_set(UCSRsA, UDREs);
  UDRs = c;

  return 0;
}

uint8_t serial_getchar(void) {
  while( !(UCSRsA & (1<<RXCs)) );
  return(UDRs);
}
