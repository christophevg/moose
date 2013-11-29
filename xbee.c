// xbee.c
// author: Christophe VG <contact@christophe.vg>

// functions to access an XBee S2/ZB from AVR

#include "xbee.h"

// one RX packet handler
static xbee_rx_handler_t rx_handler;

// QnD solution, pre-allocating 256 handlers :-(
static xbee_at_handler_t at_handlers[0xFF];
static uint8_t           at_handler_id = 0;

// global AI response
static uint8_t ai_response;

static void    xbee_send_byte(uint8_t c);
static void    xbee_wait_until_tx_complete();
static uint8_t xbee_receive_byte(void);
static bool    xbee_data_available(void);
static void    xbee_receive_rx(uint8_t size);
static void    xbee_send_at(uint8_t ch1, uint8_t ch2, xbee_at_handler_t *hndlr);
static void    xbee_receive_at(uint8_t size);
static void    xbee_check_ai();
static bool    xbee_ai_success();
static void    xbee_handle_ai(uint8_t status, uint8_t response);

// public interface

void xbee_sleep(void) {
  // power down XBee by setting its sleep pin high
  avr_set_bit(PORTD, XBEE_SLEEP);
}

void xbee_wakeup(void) {
  // power up XBee by setting its sleep pin low
  avr_clear_bit(PORTD, XBEE_SLEEP);
  xbee_wait_for_association();
}

void xbee_wait_for_association(void) {
  do { xbee_check_ai(); } while( ! xbee_ai_success() )
}

void xbee_send(xbee_tx_t *frame) {
  xbee_send_byte(XB_FRAME_START);

  // split out size + 14 bytes of protocol overhead into MSByte en LSByte
  xbee_send_byte((frame->size + 14) >> 8);
  xbee_send_byte(frame->size + 14);

  xbee_send_byte(XB_TX_PACKET);       // Frame type = transmit
  long sum = XB_TX_PACKET;
  
  xbee_send_byte(frame->id);
  sum += frame->id;

  // 64-bit address (MSB -> LSB)
  for(uint8_t i=56;i>=0;i-=8) {
    xbee_send_byte(frame->address >> i);
    sum += (frame->address >> i) & 0x00000000000000FF;
  }

  // 16-bit address
  xbee_send_byte(frame->nw_address >> 8);
  sum += frame->nw_address >> 8;
  xbee_send_byte(frame->nw_address     );
  sum += frame->address & 0x00FF;

  xbee_send_byte(frame->radius);  // broadcast radius
  sum += frame->radius;

  xbee_send_byte(frame->options);  // options
  sum += frame->options;

  // data
  for(int8_t i=0;i<frame->size;i++) {
    xbee_send_byte(frame->data[i]);
    sum += frame->data[i];
  }

  xbee_send_byte(0xFF - (sum & 0xFF) ); // checksum
}

void xbee_receive(void) {
  while( xbee_data_available() ) {
    while( xbee_get_byte() != XB_FRAME_START ) {} // wait for start of frame
    uint8_t size = (xbee_get_byte() << 8) | (xbee_get_byte() & 0xFF); // get size

    // switch on frame type
    switch( xbee_get_byte() ) {
      case XB_RX_PACKET: xbee_receive_rx(size); break;
      case XB_RX_AT    : xbee_receive_at(size); break;
      // whoops, unsupported :-)
    }
}

void xbee_on_receive(xbee_rx_handler_t handler) {
  rx_handler = handler;
}

// internals

static void xbee_send_byte(uint8_t c) {
  loop_until_bit_is_set(UCSRxA, UDREx); // wait until USART Data Reg Empty
  UDR0 = c;
}

static void xbee_wait_until_tx_complete() {
  loop_until_bit_is_set(UCSRxA, TXCx);  // wait until TX Complete
}

static uint8_t xbee_receive_byte(void) {
  while( ! xbee_data_available() );
  return(UDRx);
}

static bool xbee_data_available(void) {
  return UCSRxA & (1<<RXCx);
}

static void xbee_receive_rx(uint8_t size) {
  xbee_rx_t frame;

  uint64_t  address;
  uint16_t  nw_address;
  uint8_t   options;
  uint8_t  *data = (uint8_t*)malloc(size-12); // 12 bytes are protocol overhead
  
  // 64-bit address (MSB -> LSB)
  for(uint8_t i=56; i>=0; i-=8) {
    address |= xbee_get_byte() << i;
  }

  // 16bit network address
  nw_address = (xbee_get_byte() << 8) | (xbee_get_byte());

  options = xbee_get_byte();
  
  // data
  for(int8_t i=0; i<size-12; i++) {
    data[i] = xbee_get_byte();
  }
  
  // create a frame and have it taken care of
  frame.size       = size;
  frame.address    = address;
  frame.nw_address = nw_address;
  frame.options    = options;
  frame.data       = data;

  rx_handler(&frame);
}

static void xbee_send_at(uint8_t ch1, uint8_t ch2, xbee_at_handler_t *handler) {
  // install response handler, id maps to entry in table
  at_handlers[at_handler_id] = handler;

  // send frame
  xbee_send_byte(XB_START_FRAME);
  
  xbee_send_byte(0x00);          // MSB
  xbee_send_byte(0x04);          // LSB (fixed length)

  xbee_send_byte(XB_TX_AT);
  xbee_send_byte(at_handler_id); // frame ID
  xbee_send_byte(ch1);           // AT command char 1
  xbee_send_byte(ch2);           // AT command char 2

  long sum = 0x08 + id + ch1 + ch2;
  xbee_send_byte(0xFF - (sum & 0xFF) ); // checksum
  
  at_handler_id++;
}

static void xbee_receive_at(uint8_t size) {
  uint8_t id  = xbee_get_byte();
  uint8_t ch1 = xbee_get_byte();
  uint8_t ch2 = xbee_get_byte();

  xbee_at_handler_t handler = at_handlers[id];

  uint8_t status = xbee_get_byte();

  uint8_t data = 0;
  if(size-5 > 0) {  // command data available ?
    data = xbee_get_byte();
  }

  handler(status, data);
}

static void xbee_check_ai() {
  ai_response = XB_AT_AI_SCANNING; // seems most logical non-ok default value
  xbee_send_at('A', 'I', xbee_handle_ai_response);
  _delay_ms(10);  // TODO: tune this
}

static bool xbee_ai_success() {
  return ai_response == XB_AT_AI_SUCCESS;
}
  
static void xbee_handle_ai(uint8_t status, uint8_t response) {
  if(status == XB_AT_OK) {
    ai_response = response;
  }
}
