// wifi.h
// author: Christophe VG <contact@christophe.vg>

// functions to access a WIFI module via serial connection

#ifndef __WIFI_H
#define __WIFI_H

#include <string.h>

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

#include "bool.h"
#include "avr.h"

// WIFI is controled via USART, some AVR devices have multiple USARTs
// the identifying 0 or 1 is replace by an 'x'

#define WIFI_ON_USART1_NAME STR(WIFI_ON_USART1)

#ifdef WIFI_ON_USART1     // USART1 (e.g. on the ATMEGA1284p)
// interrupts
#define USARTw_RX_vect USART1_RX_vect
#define USARTw_TX_vect USART1_TX_vect
// port + pin
#define WIFI_RX_PORT RX1_PORT
#define WIFI_RX_PIN  RX1_PIN
// registers
#define UBRRwH UBRR1H
#define UBRRwL UBRR1L  
#define UCSRwA UCSR1A
#define U2Xw   U2X1
#define UCSRwC UCSR1C
#define UCSRwB UCSR1B
#define UCSZw1 UCSZ11
#define UCSZw0 UCSZ10
#define USBSw  USBS1 
#define RXENw  RXEN1
#define TXENw  TXEN1
#define UCSRwA UCSR1A
#define UDREw  UDRE1
#define RXCw   RXC1
#define UDRw   UDR1
#define TXCw   TXC1
#define RXCIEw RXCIE1
#define TXCIEw TXCIE1
#else                     // USART0 (default)
// interruptss
#define USARTw_RX_vect USART0_RX_vect
#define USARTw_TX_vect USART0_TX_vect
// port + pin
#define WIFI_RX_PORT RX0_PORT
#define WIFI_RX_PIN  RX0_PIN
// registers
#define UBRRwH UBRR0H
#define UBRRwL UBRR0L  
#define UCSRwA UCSR0A
#define U2Xw   U2X0
#define UCSRwC UCSR0C
#define UCSRwB UCSR0B
#define UCSZw1 UCSZ01
#define UCSZw0 UCSZ00
#define USBSw  USBS0
#define RXENw  RXEN0
#define TXENw  TXEN0
#define UCSRwA UCSR0A
#define UDREw  UDRE0
#define RXCw   RXC0
#define UDRw   UDR0
#define TXCw   TXC0
#define RXCIEw RXCIE0
#define TXCIEw TXCIE0
#endif

// functions
void    wifi_init(void);
void    wifi_send_cmd(const char*, int);
uint8_t wifi_receive_byte(void);

#endif
