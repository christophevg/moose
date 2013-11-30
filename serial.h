// serial.h
// author: Christophe VG <contact@christophe.vg>

// functions to operate the UART on AVR/ATMega

#ifndef __SERIAL_H
#define __SERIAL_H

// compute baud rate setting
#ifndef BAUD
#define BAUD 9600
#endif

#include <util/setbaud.h>

#include <stdio.h>
#include <avr/io.h>

#include "avr.h"

// some AVR devices have multiple USARTs
// in the actual identifiers are the idenfying 0 or 1 is replaced by an 's'

#define SERIAL_ON_USART1_NAME STR(SERIAL_ON_USART1)

#ifdef SERIAL_ON_USART1   // USART1 (e.g. on the ATMEGA1284p)
// port + pin
#define SERIAL_RX_PORT RX1_PORT
#define SERIAL_RX_PIN  RX1_PIN
// registers
#define UBRRsH UBRR1H
#define UBRRsL UBRR1L  
#define UCSRsA UCSR1A
#define UCSRsB UCSR1B
#define UCSRsC UCSR1C
#define U2Xs   U2X1
#define UCSZs1 UCSZ11
#define UCSZs0 UCSZ10
#define RXENs  RXEN1
#define TXENs  TXEN1
#define UDREs  UDRE1
#define RXCs   RXC1
#define UDRs   UDR1
#else                     // USART0 (default)
// port + pin
#define SERIAL_RX_PORT RX0_PORT
#define SERIAL_RX_PIN  RX0_PIN
// registers
#define UBRRsH UBRR0H
#define UBRRsL UBRR0L  
#define UCSRsA UCSR0A
#define U2Xs   U2X0
#define UCSRsC UCSR0C
#define UCSRsB UCSR0B
#define UCSZs1 UCSZ01
#define UCSZs0 UCSZ00
#define RXENs  RXEN0
#define TXENs  TXEN0
#define UDREs  UDRE0
#define RXCs   RXC0
#define UDRs   UDR0
#endif

// public functions

void    serial_init(void);
int     serial_putchar(char c, FILE *stream);
uint8_t serial_getchar(void);

#endif
