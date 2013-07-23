// xbee.c
// author: Christophe VG <contact@christophe.vg>

// functions to access an XBee S2/ZB from AVR

#include <string.h>

#include <avr/io.h>
#include <util/delay.h>

#include "avr.h"
#include "xbee.h"

void xbee_send_char(char c) {
  loop_until_bit_is_set(UCSR0A, UDRE0);
  UDR0 = c;
}

uint8_t xbee_get_char(void) {
  while( !(UCSR0A & (1<<RXC0)) );
  return(UDR0);
}

void xbee_wait_for_association(void) {
  uint8_t value      = 0xFF;
  uint8_t id         = 0x00;
  
  while(value != 0) {
    _delay_ms(10); /* breath */
    id = id + 1;
    xbee_at('A', 'I', id);
    value = xbee_get_response(0x88, id, 'A', 'I');
  }
}

void xbee_sleep(void) {
  // power down XBee by setting its sleep pin high
  avr_set_bit(PORTD, XBEE_SLEEP);
}

void xbee_wakeup(void) {
  // power up XBee by setting its sleep pin high
  avr_clear_bit(PORTD, XBEE_SLEEP);
  xbee_wait_for_association();
}

void xbee_at(uint8_t ch1, uint8_t ch2, uint8_t id) {
  xbee_send_char(0x7E);   // start frame
  
  xbee_send_char(0x00);   // length MSB
  xbee_send_char(0x04);   // length LSB

  xbee_send_char(0x08);   // Frame type = AT
  xbee_send_char(id);     // Frame ID
  xbee_send_char(ch1);    // AT command char 1
  xbee_send_char(ch2);    // AT command char 2

  long sum = 0x08 + id + ch1 + ch2;
  xbee_send_char(0xFF - (sum & 0xFF) ); // checksum
}

uint8_t xbee_get_response(uint8_t type, uint8_t id, uint8_t ch1, uint8_t ch2) {
  // TODO: make really generic
  while( xbee_get_char() != 0x7E ) {} // wait for start of frame
  xbee_get_char(); // skip MSB length, we know length
  xbee_get_char(); // skip LSB length, we know length
  if(xbee_get_char() == type) {
    if( xbee_get_char() == id ) {
      if( xbee_get_char() == ch1 ) {
        if( xbee_get_char() == ch2 ) {
          if( xbee_get_char() == 0x00 ) {      // command statue == OK
            return xbee_get_char();            // return command data
          }
        }
      }
    }
  }
  return 0xFF; // TODO: do this differently ?
}

void xbee_transmit(uint8_t* ch, uint8_t length) {
  xbee_send_char(0x7E);   // start frame
  
  xbee_send_char(0x00);   // length MSB

  xbee_send_char(length+14);   // length LSB = payload length + 14 "overhead"

  xbee_send_char(0x10);  // Frame type = transmit
  xbee_send_char(0x00);  // no response (FOR NOW == TODO)

  // 64-bit address
  // coordinator == 0x0000000000000000
  xbee_send_char(0x00);    xbee_send_char(0x00);
  xbee_send_char(0x00);    xbee_send_char(0x00);
  xbee_send_char(0x00);    xbee_send_char(0x00);
  xbee_send_char(0x00);    xbee_send_char(0x00);

  // 16-bit address
  xbee_send_char(0xFF);    xbee_send_char(0xFE);
  
  xbee_send_char(0x00);  // broadcast radius
  
  xbee_send_char(0x00);  // options
  
  // data + checksum calc
  long sum = 0x10 + 0xFF + 0xFE;
  int8_t i;
  for(i=0;i<length;i++) {
    xbee_send_char(ch[i]);
    sum = sum + ch[i];
  }
  
  xbee_send_char(0xFF - (sum & 0xFF) ); // checksum
}

void xbee_transmit_string(char* string) {
  // TODO: convert char* to uint8* to remove warnings
  xbee_transmit(string, strlen(string));
}
