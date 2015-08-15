// wifi.c
// author: Christophe VG <contact@christophe.vg>

// functions to access a WIFI module via serial connection

#include <stdio.h>

#include <avr/interrupt.h>

#include "bool.h"
#include "wifi.h"
#include "nmea.h"

volatile static bool tx_in_progress;

// interrupt vector for handling completion of transmission
ISR (USARTw_TX_vect) {
  tx_in_progress = FALSE;
}

static void _wait_until_tx_complete(void) {
  do {} while(tx_in_progress);
}

static void _send_byte(uint8_t c) {
  loop_until_bit_is_set(UCSRwA, UDREw); // wait until USART Data Reg Empty
  tx_in_progress = TRUE;
  UDRw = c;
  _wait_until_tx_complete();
}

// cyclic IO buffers
typedef struct {
  volatile uint8_t head;
  volatile uint8_t tail;
  volatile uint8_t buffer[0xff];
} cyclic_buffer_t;

volatile cyclic_buffer_t incoming = {0, 0, {0}};

// interrupt vector for handling reception of a single byte
ISR (USARTw_RX_vect) {
  incoming.buffer[incoming.tail++] = UDRw;
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
void wifi_init(void) {
  // make RX pin input pin by clearing it
  avr_clear_bit(WIFI_RX_PORT, WIFI_RX_PIN);

  // compute baud rate setting
  // thanks: http://www.appelsiini.net/2011/simple-usart-with-avr-libc
  #undef BAUD
  #define BAUD 9600
  #include <util/setbaud.h>
  
  UBRRwH = UBRRH_VALUE;
  UBRRwL = UBRRL_VALUE;

  #if USE_2X
    UCSRwA |= _BV(U2Xw);
  #else
    UCSRwA &= ~(_BV(U2Xw));
  #endif

  UCSRwC = _BV(UCSZw1) | _BV(UCSZw0) | (0<<USBSw); // 8-bit data + 1 top bit
  UCSRwB = _BV(RXENw)  | _BV(TXENw);               // Enable RX and TX

  // https://sites.google.com/site/qeewiki/books/avr-guide/usart  
  UCSRwB |= (1 << RXCIEw);            // enable RX interrupt to accept bytes
  UCSRwB |= (1 << TXCIEw);            // enable TX interrupt to see EOT
  
  // enable interrupts
  sei();
}

void wifi_send_cmd(const char *cmd, int size) {
  for(int b=0; b<size; b++) {
    _send_byte(cmd[b]);
  }
  _send_byte('\r');
  _send_byte('\n');
}

uint8_t wifi_receive_byte(void) {
  return _receive_byte();
}
