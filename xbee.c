// xbee.c
// author: Christophe VG <contact@christophe.vg>

// functions to access an XBee S2/ZB from AVR

#include "xbee.h"

#include <stdio.h>
#include <avr/interrupt.h>

// forward declarations of "private" functions to avoid puttin them on top ;-)
static void    _send_byte(uint8_t);
static void    _wait_until_tx_complete(void);
static uint8_t _receive_byte(void);
static bool    _data_available(void);
static void    _receive_rx(uint8_t);
static void    _send_at(uint8_t, uint8_t, xbee_at_handler_t);
static void    _receive_at(uint8_t);
static void    _check_ai(void);
static bool    _ai_success(void);
static void    _handle_ai_response(uint8_t, uint8_t);
static void    _start_tx_checksum(void);
static void    _start_rx_checksum(void);
static void    _send_checksum(void);
static bool    _rx_checksum_isvalid(void);
static void    _buffer_info(void);

// public interface

// initialization, uses generic register names that should be defined in the
// header file.
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

// power down XBee by setting its sleep pin high
void xbee_sleep(void) {
  avr_set_bit(XBEE_SLEEP_PORT, XBEE_SLEEP_PIN);
}


// power up XBee by setting its sleep pin low
void xbee_wakeup(void) {
  avr_clear_bit(XBEE_SLEEP_PORT, XBEE_SLEEP_PIN);
  xbee_wait_for_association();
}

// wrapper around the AI AT command check
void xbee_wait_for_association(void) {
  do {
    _delay_ms(10);    // really needed and it _must_ be here :-(
    _check_ai();
    xbee_receive();
  } while( ! _ai_success() );
}

// sends a frame
void xbee_send(xbee_tx_t *frame) {
  _send_byte(XB_FRAME_START);

  // split out size + 14 bytes of protocol overhead into MSByte en LSByte
  _send_byte((frame->size + 14) >> 8);
  _send_byte(frame->size + 14);

  _start_tx_checksum();
  {
    _send_byte(XB_TX_PACKET);       // frame type = transmit
  
    _send_byte(frame->id);

    // 64-bit address (MSB -> LSB)
    for(uint8_t i=56;i>0;i-=8) {
      _send_byte(frame->address >> i);
    }
    _send_byte(frame->address);

    // 16-bit network address
    _send_byte(frame->nw_address >> 8);
    _send_byte(frame->nw_address     );

    _send_byte(frame->radius);   // broadcast radius
    _send_byte(frame->options);  // options

    // data
    for(int8_t i=0;i<frame->size;i++) {
      _send_byte(frame->data[i]);
    }
  }
  _send_checksum();
  
  _wait_until_tx_complete();
}

// generic function to handle incoming packets.
// deals with common bytes and dispatches to helper functions to handle
// different known packet types
void xbee_receive(void) {
  // keep processing incoming data
  while( _data_available() ) {
    // _buffer_info();
    while( _receive_byte() != XB_FRAME_START ) {} // wait for start of frame

    // receive common to all packets: size and type
    uint8_t size = (_receive_byte() << 8) | (_receive_byte() & 0xFF);
    uint8_t type = _receive_byte();

    switch( type ) {
      case XB_RX_PACKET: _receive_rx(size); break;
      case XB_RX_AT    : _receive_at(size); break;
      // TODO remove this printf by DEBUG/WARN/ERROR support (to come)
      default: printf("WARNING: received unsupported packet type: %i\n", type);
    }
  }
}

// RX packet support

// one RX packet handler
static xbee_rx_handler_t rx_handler;

// function to register callback for the RX packet handler
void xbee_on_receive(xbee_rx_handler_t handler) {
  rx_handler = handler;
}

// handling of received (data) packets, dispatched by xbee_receive
static void _receive_rx(uint8_t size) {
  xbee_rx_t frame;

  uint64_t  address;
  uint16_t  nw_address;
  uint8_t   options;
  uint8_t  *data = (uint8_t*)malloc(size-12); // 12 bytes are protocol overhead

  _start_rx_checksum();
  {
    // 64-bit address (MSB -> LSB)
    for(uint8_t i=56; i>=0; i-=8) {
      address |= _receive_byte() << i;
    }

    // 16bit network address
    nw_address = (_receive_byte() << 8) | (_receive_byte());

    options = _receive_byte();
  
    // data
    for(int8_t i=0; i<size-12; i++) {
      data[i] = _receive_byte();
    }
  }  
  if( ! _rx_checksum_isvalid() ) {
    free(data);
    return;
  }

  // create a frame and have it taken care of
  frame.size       = size;
  frame.address    = address;
  frame.nw_address = nw_address;
  frame.options    = options;
  frame.data       = data;

  rx_handler(&frame);
}

// AT command support

// global AI response
static uint8_t ai_response;

// functional function to initiate an AI check
static void _check_ai(void) {
  ai_response = XB_AT_AI_SCANNING; // seems most logical non-ok default value
  _send_at('A', 'I', _handle_ai_response);
}

// callback for handling AI responses
static void _handle_ai_response(uint8_t status, uint8_t response) {
  if(status == XB_AT_OK) {
    ai_response = response;
  }
}

// function to check for successful association
static bool _ai_success(void) {
  return ai_response == XB_AT_AI_SUCCESS;
}

// cyclic buffer of handlers for at responses
static xbee_at_handler_t at_handlers[0xFF];
static uint8_t           at_handler_id = 0;

// generic function to send AT command, requires two command letters + handler
// callback function
static void _send_at(uint8_t ch1, uint8_t ch2, xbee_at_handler_t handler) {
  while(at_handler_id==0) { at_handler_id++; }

  // install response handler, id maps to entry in table
  at_handlers[at_handler_id] = handler;

  // send frame
  _send_byte(XB_FRAME_START);
  
  _send_byte(0x00);          // MSB
  _send_byte(0x04);          // LSB (fixed length)

  _start_tx_checksum();
  {
    _send_byte(XB_TX_AT);
    _send_byte(at_handler_id); // frame ID
    _send_byte(ch1);           // AT command char 1
    _send_byte(ch2);           // AT command char 2
  }
  _send_checksum();
  
  at_handler_id++;
}

// generic handling of AT responses, dispatched by xbee_receive
static void _receive_at(uint8_t size) {
  uint8_t data = 0;
  uint8_t status;
  uint8_t id;

  _start_rx_checksum();
  {
    id = _receive_byte();
    _receive_byte();
    _receive_byte();
    status = _receive_byte();

    if(size - 5 > 0) {  // command data available ?
      data = _receive_byte();
    }
  }
  if( _rx_checksum_isvalid() ) {
    (*(at_handlers[id]))(status, data);
  }
}

// checksumming support

static long tx_checksum = 0,
            rx_checksum = 0;

static void _start_tx_checksum(void) {
  tx_checksum = 0;
}

static void _start_rx_checksum(void) {
  rx_checksum = 0;
}

static void _send_checksum(void) {
  _send_byte(0xFF - (tx_checksum & 0xFF));
}

static bool _rx_checksum_isvalid(void) {
  _receive_byte();
  // after receiving the checksum byte, the checksum should be zero
  if( rx_checksum != 0 ) {
    printf("ERROR: invalid checksum");
    return FALSE;
  }
  return TRUE;
}

// technical (serial-oriented) functions to send one byte and wait until
// transmission has finished receiving of one byte is done through interrupts
// and an internal buffer see below

static void _send_byte(uint8_t c) {
  loop_until_bit_is_set(UCSRxA, UDREx); // wait until USART Data Reg Empty
  UDRx = c;
  tx_checksum += c;
}

static void _wait_until_tx_complete(void) {
  loop_until_bit_is_set(UCSRxA, TXCx);  // wait until TX Complete
}

// internal buffering of received bytes using cyclic buffer and interrupts
// TODO: overflow detection

static uint8_t buffer[0xFF];  // another 256 bytes :-(
static uint8_t head = 0;
static uint8_t tail = 0;

static void _buffer_info(void) {
  printf("buffer:\n  head = %i\n  tail = %i\n  content = ", head, tail);
  for(uint8_t i=head;i!=tail;i++) {
    printf("%i ", buffer[i]);
  }
  printf("\n");
}

// interrupt vector for handling reception of a single byte
ISR (USARTx_RX_vect) {
  buffer[tail++] = UDRx;
}

static uint8_t _receive_byte(void) {
  while( ! _data_available() );
  
  return buffer[head++];
}

static bool _data_available(void) {
  return head != tail;
}
