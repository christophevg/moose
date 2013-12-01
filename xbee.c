// xbee.c
// author: Christophe VG <contact@christophe.vg>

// functions to access an XBee S2/ZB from AVR

#include "xbee.h"

#include <stdio.h>
#include <string.h>
#include <avr/interrupt.h>

// one RX packet handler
static xbee_rx_handler_t rx_handler;

// QnD solution, pre-allocating 256 handlers :-(
static xbee_at_handler_t at_handlers[0xFF];
static uint8_t           at_handler_id = 0;

// global AI response
static uint8_t ai_response;

// forward declarations of "private" functions
static void    xbee_send_byte(uint8_t);
static void    xbee_wait_until_tx_complete(void);
static uint8_t xbee_receive_byte(void);
static bool    xbee_data_available(void);
static void    xbee_receive_rx(uint8_t);
static void    xbee_send_at(uint8_t, uint8_t, xbee_at_handler_t);
static void    xbee_receive_at(uint8_t);
static void    xbee_check_ai(void);
static bool    xbee_ai_success(void);
static void    xbee_handle_ai_response(uint8_t, uint8_t);

// TODO: move to serial
// internal buffering

static uint8_t buffer[256];
static uint8_t head = 0;
static uint8_t tail = 0;

static void xbee_accept_byte(uint8_t byte) {
  buffer[tail++] = byte;
}

// interrupt vector for handling reception of a single byte
ISR (USART0_RX_vect) {
  xbee_accept_byte(UDR0);
}

void xbee_buffer_info(void) {
  printf("buffer: head=%i, tail=%i\n", head, tail);
}

// public interface

void xbee_init(void) {
  // make RX pin input pin by clearing it
  avr_clear_bit(XBEE_RX_PORT, XBEE_RX_PIN);

  // Thanks: http://www.appelsiini.net/2011/simple-usart-with-avr-libc
  UBRRxH = UBRRH_VALUE;
  UBRRxL = UBRRL_VALUE;

  #if USE_2X
    UCSRxA |= _BV(U2Xx);
  #else
    UCSRxA &= ~(_BV(U2Xx));
  #endif

  UCSRxC = _BV(UCSZx1) | _BV(UCSZx0); // 8-bit data
  UCSRxB = _BV(RXENx)  | _BV(TXENx);  // Enable RX and TX

  // https://sites.google.com/site/qeewiki/books/avr-guide/usart  
  UCSRxB |= (1 << RXCIE0);            // enable interrupt
}

void xbee_sleep(void) {
  // power down XBee by setting its sleep pin high
  avr_set_bit(XBEE_SLEEP_PORT, XBEE_SLEEP_PIN);
}

void xbee_wakeup(void) {
  // power up XBee by setting its sleep pin low
  avr_clear_bit(XBEE_SLEEP_PORT, XBEE_SLEEP_PIN);
  xbee_wait_for_association();
}

void xbee_wait_for_association(void) {
  do {
    _delay_ms(10);    // really needed :-(
    xbee_check_ai();
    xbee_receive();
  } while( ! xbee_ai_success() );
}

static long checksum = 0;

static void xbee_start_checksum(void) {
  checksum = 0;
}

static uint8_t xbee_get_checksum(void) {
  return 0xFF - (checksum & 0xFF);
}

void xbee_send(xbee_tx_t *frame) {
  xbee_send_byte(XB_FRAME_START);

  // split out size + 14 bytes of protocol overhead into MSByte en LSByte
  xbee_send_byte((frame->size + 14) >> 8);
  xbee_send_byte(frame->size + 14);

  xbee_start_checksum();
  xbee_send_byte(XB_TX_PACKET);       // frame type = transmit
  
  xbee_send_byte(frame->id);

  // 64-bit address (MSB -> LSB)
  for(uint8_t i=56;i>0;i-=8) {
    xbee_send_byte(frame->address >> i);
  }
  xbee_send_byte(frame->address);

  // 16-bit network address
  xbee_send_byte(frame->nw_address >> 8);
  xbee_send_byte(frame->nw_address     );

  xbee_send_byte(frame->radius);   // broadcast radius
  xbee_send_byte(frame->options);  // options

  // data
  for(int8_t i=0;i<frame->size;i++) {
    xbee_send_byte(frame->data[i]);
  }

  xbee_send_byte( xbee_get_checksum() );
  
  xbee_wait_until_tx_complete();
}

void xbee_receive(void) {
  while( xbee_data_available() ) {
    while( xbee_receive_byte() != XB_FRAME_START ) {} // wait for start of frame

    // receive common to all packets: size and type
    uint8_t size = (xbee_receive_byte() << 8) | (xbee_receive_byte() & 0xFF);
    uint8_t type = xbee_receive_byte();

    switch( type ) {
      case XB_RX_PACKET: xbee_receive_rx(size); break;
      case XB_RX_AT    : xbee_receive_at(size); break;
      default: printf("WARNING: received unsupported packet type: %i\n", type);
    }
  }
}

void xbee_on_receive(xbee_rx_handler_t handler) {
  rx_handler = handler;
}

// internals

static void xbee_send_byte(uint8_t c) {
  loop_until_bit_is_set(UCSRxA, UDREx); // wait until USART Data Reg Empty
  UDRx = c;
  checksum += c;
}

static void xbee_wait_until_tx_complete(void) {
  loop_until_bit_is_set(UCSRxA, TXCx);  // wait until TX Complete
}

static uint8_t xbee_receive_byte(void) {
  while( ! xbee_data_available() );
  return buffer[head++];
}

static bool xbee_data_available(void) {
  return head != tail;
}

static void xbee_receive_rx(uint8_t size) {
  xbee_rx_t frame;

  uint64_t  address;
  uint16_t  nw_address;
  uint8_t   options;
  uint8_t  *data = (uint8_t*)malloc(size-12); // 12 bytes are protocol overhead
  
  // 64-bit address (MSB -> LSB)
  for(uint8_t i=56; i>=0; i-=8) {
    address |= xbee_receive_byte() << i;
  }

  // 16bit network address
  nw_address = (xbee_receive_byte() << 8) | (xbee_receive_byte());

  options = xbee_receive_byte();
  
  // data
  for(int8_t i=0; i<size-12; i++) {
    data[i] = xbee_receive_byte();
  }
  
  // create a frame and have it taken care of
  frame.size       = size;
  frame.address    = address;
  frame.nw_address = nw_address;
  frame.options    = options;
  frame.data       = data;

  rx_handler(&frame);
}

static void xbee_send_at(uint8_t ch1, uint8_t ch2, xbee_at_handler_t handler) {
  while(at_handler_id==0) { at_handler_id++; }

  // install response handler, id maps to entry in table
  at_handlers[at_handler_id] = handler;

  // send frame
  xbee_send_byte(XB_FRAME_START);
  
  xbee_send_byte(0x00);          // MSB
  xbee_send_byte(0x04);          // LSB (fixed length)

  xbee_send_byte(XB_TX_AT);
  xbee_send_byte(at_handler_id); // frame ID
  xbee_send_byte(ch1);           // AT command char 1
  xbee_send_byte(ch2);           // AT command char 2

  long sum = XB_TX_AT + at_handler_id + ch1 + ch2;
  xbee_send_byte(0xFF - (sum & 0xFF) ); // checksum
  
  at_handler_id++;
}

static void xbee_receive_at(uint8_t size) {
  uint8_t id     = xbee_receive_byte();
  xbee_receive_byte();
  xbee_receive_byte();
  uint8_t status = xbee_receive_byte();

  uint8_t data = 0;
  if(size - 5 > 0) {  // command data available ?
    data = xbee_receive_byte();
  }

  uint8_t checksum = xbee_receive_byte();
  // TODO validate checksum

  (*(at_handlers[id]))(status, data);
}

static void xbee_check_ai(void) {
  ai_response = XB_AT_AI_SCANNING; // seems most logical non-ok default value
  xbee_send_at('A', 'I', xbee_handle_ai_response);
}

static bool xbee_ai_success(void) {
  return ai_response == XB_AT_AI_SUCCESS;
}
  
static void xbee_handle_ai_response(uint8_t status, uint8_t response) {
  if(status == XB_AT_OK) {
    ai_response = response;
  }
}
