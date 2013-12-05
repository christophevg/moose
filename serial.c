// serial.c
// author: Christophe VG <contact@christophe.vg>

// functions to operate the UART on AVR/ATMega

#include <stdio.h>
#include <avr/io.h>

#include "avr.h"
#include "serial.h"

// trick to make printf "print" to serial<->UART
static FILE mystdout = FDEV_SETUP_STREAM(serial_putchar, NULL, _FDEV_SETUP_WRITE);

void serial_init(void) {
  // set pin 4 on port B to 0 = input = MISO
  avr_clear_bit(DDRB, PB4);
  // set pin 0 on port D to 0 = input = RX
  avr_clear_bit(DDRD, PD0);

  // USART Baud rate: 9600
  UBRR0H = MYUBRR >> 8;
  UBRR0L = MYUBRR;
  UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    
  stdout = &mystdout; // required for printf init
}

static int serial_putchar(char c, FILE *stream) {
  if (c == '\n') serial_putchar('\r', stream); // add a CR before the LF

  loop_until_bit_is_set(UCSR0A, UDRE0);
  UDR0 = c;

  return 0;
}

uint8_t serial_getchar(void) {
  while( !(UCSR0A & (1<<RXC0)) );
  return(UDR0);
}
