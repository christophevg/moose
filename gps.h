// gps.h
// author: Christophe VG <contact@christophe.vg>

// functions to access a GPS module via serial connection

#ifndef __GPS_H
#define __GPS_H

#include <string.h>

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

#include "bool.h"
#include "avr.h"

// GPS is controled via USART, some AVR devices have multiple USARTs
// the identifying 0 or 1 is replace by an 'x'

#define GPS_ON_USART1_NAME STR(GPS_ON_USART1)

#ifdef GPS_ON_USART1     // USART1 (e.g. on the ATMEGA1284p)
// interrupts
#define USARTg_RX_vect USART1_RX_vect
#define USARTg_TX_vect USART1_TX_vect
// port + pin
#define GPS_RX_PORT RX1_PORT
#define GPS_RX_PIN  RX1_PIN
// registers
#define UBRRgH UBRR1H
#define UBRRgL UBRR1L  
#define UCSRgA UCSR1A
#define U2Xg   U2X1
#define UCSRgC UCSR1C
#define UCSRgB UCSR1B
#define UCSZg1 UCSZ11
#define UCSZg0 UCSZ10
#define RXENg  RXEN1
#define TXENg  TXEN1
#define UCSRgA UCSR1A
#define UDREg  UDRE1
#define RXCg   RXC1
#define UDRg   UDR1
#define TXCg   TXC1
#define RXCIEg RXCIE1
#define TXCIEg TXCIE1
#else                     // USART0 (default)
// interruptss
#define USARTg_RX_vect USART0_RX_vect
#define USARTg_TX_vect USART0_TX_vect
// port + pin
#define GPS_RX_PORT RX0_PORT
#define GPS_RX_PIN  RX0_PIN
// registers
#define UBRRgH UBRR0H
#define UBRRgL UBRR0L  
#define UCSRgA UCSR0A
#define U2Xg   U2X0
#define UCSRgC UCSR0C
#define UCSRgB UCSR0B
#define UCSZg1 UCSZ01
#define UCSZg0 UCSZ00
#define RXENg  RXEN0
#define TXENg  TXEN0
#define UCSRgA UCSR0A
#define UDREg  UDRE0
#define RXCg   RXC0
#define UDRg   UDR0
#define TXCg   TXC0
#define RXCIEg RXCIE0
#define TXCIEg TXCIE0
#endif

// RX handler type
typedef struct {
  uint8_t deg;
  uint8_t min;
  float   sec;
} gps_coordinate;

typedef struct {
  gps_coordinate ns;
  gps_coordinate ew;
} gps_position;
  
// functions
void gps_init(void);
void gps_receive(void);

#endif
