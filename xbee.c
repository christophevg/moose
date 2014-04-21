// xbee.c
// author: Christophe VG <contact@christophe.vg>

// functions to access an XBee S2/ZB from AVR

#include <stdio.h>

#include "xbee.h"
#include "clock.h"

#include <avr/interrupt.h>

// forward declarations of "private" functions to avoid puttin them on top ;-)
static void    _send_byte(uint8_t);
static void    _wait_until_tx_complete(void);
static uint8_t _receive_byte(void);
static uint8_t _peek_byte(void);
static bool    _data_available(void);
static void    _receive_rx(uint8_t);
static void    _send_at(uint8_t, uint8_t, xbee_at_handler_t);
static void    _receive_at(uint8_t);
static void    _check_ai(void);
static void    _get_my(void);
static void    _get_mp(void);
static void    _receive_ai_response(unsigned long);
static void    _receive_my_response(unsigned long);
static void    _receive_mp_response(unsigned long);
static bool    _ai_success(void);
static bool    _my_success(void);
static bool    _mp_success(void);
static void    _handle_ai_response(uint8_t, uint8_t*);
static void    _handle_my_response(uint8_t, uint8_t*);
static void    _handle_mp_response(uint8_t, uint8_t*);
static void    _start_tx_checksum(void);
static void    _start_rx_checksum(void);
static void    _send_checksum(void);
static bool    _rx_checksum_isvalid(void);
static void    _buffer_info(void);
static void    _receive_modem(uint8_t size);
static void    _receive_transmit_status(uint8_t size);

// metrics support
xbee_metrics_t metrics = { .bytes = 0, .frames = 0 };

void xbee_reset_counters(void) {
  metrics.bytes  = 0;
  metrics.frames = 0;
}

xbee_metrics_t xbee_get_counters(void) {
  return metrics;
}

// public interface

// initialization, uses generic register names that should be defined in the
// header file.
void xbee_init(void) {
  // make RX pin input pin by clearing it
  avr_clear_bit(XBEE_RX_PORT, XBEE_RX_PIN);
  
  // compute baud rate setting
  // thanks: http://www.appelsiini.net/2011/simple-usart-with-avr-libc
  #undef BAUD
  #define BAUD 9600
  #include <util/setbaud.h>
  
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
  UCSRxB |= (1 << RXCIEx);            // enable RX interrupt to accept bytes
  UCSRxB |= (1 << TXCIEx);            // enable TX interrupt to see EOT
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
    _check_ai();
    _receive_ai_response(100L); // typical delay is 40ms
  } while( ! _ai_success() );
  // once we're associated, we want to fetch our own nw address and our parent's
  do {
    _get_my();
    _receive_my_response(100L);
  } while( ! _my_success() );
  do {
    _get_mp();
    _receive_mp_response(100L);
  } while( ! _mp_success() );
}

uint8_t frame_id = 1;

// sends a frame
void xbee_send(xbee_tx_t *frame) {
  _send_byte(XB_FRAME_START);

  // split out size + 14 bytes of protocol overhead into MSByte en LSByte
  _send_byte((frame->size + 14) >> 8);
  _send_byte(frame->size + 14);

  _start_tx_checksum();
  {
    _send_byte(XB_TX_PACKET);       // frame type = transmit
  
    _send_byte(frame_id++);

    // 64-bit address (MSB -> LSB)
    for(int8_t i=56;i>0;i-=8) {
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
  
  metrics.frames++;
  metrics.bytes += frame->size + 14 + 2; // +2 = start delim and checksum

  _wait_until_tx_complete();
}

// generic function to handle incoming packets.
// deals with common bytes and dispatches to helper functions to handle
// different known packet types
void xbee_receive(void) {
  // keep processing incoming data
  while( _data_available() ) {
    _buffer_info();
    // skip to start of frame
    while( _data_available() && _peek_byte() != XB_FRAME_START ) {
      _receive_byte();
    }

    // actual next packet ? -> this happens when we skip trailing garbage
    if( ! _data_available() ) {
      debug_printf("WARNING: no next packet...\n");
      break;
    }

    _receive_byte();  // received peeked frame start

    // receive common to all packets: size and type
    uint8_t size = (_receive_byte() << 8) | (_receive_byte() & 0xFF);
    uint8_t type = _peek_byte();

    switch( type ) {
      case XB_RX_PACKET       : _receive_rx(size);              break;
      case XB_RX_AT           : _receive_at(size);              break;
      case XB_MODEM_STATUS    : _receive_modem(size);           break;
      case XB_TRANSMIT_STATUS : _receive_transmit_status(size); break;

      case XB_FRAME_START  : break; // this happens, let's not lose a packet

      default:
        printf("WARNING: received unsupported packet type: %i\n", type);
    }
  }
  // debug_printf("no more data...\n");
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

  uint64_t  address    = 0;
  uint16_t  nw_address = 0;
  uint8_t   options    = 0;
  uint8_t  *data = (uint8_t*)malloc(size-12); // 12 bytes are protocol overhead

  _start_rx_checksum();
  {
    _receive_byte();                  // frame type is part of the checksum

    address = ((uint64_t)(_receive_byte()) << 56)
            | ((uint64_t)(_receive_byte()) << 48)
            | ((uint64_t)(_receive_byte()) << 40)
            | ((uint64_t)(_receive_byte()) << 32)
            | ((uint64_t)(_receive_byte()) << 24)
            | ((uint64_t)(_receive_byte()) << 16)
            | (_receive_byte() <<  8)
            | (_receive_byte());

    // 16bit network address
    nw_address = (_receive_byte() << 8)
               | (_receive_byte());

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
  frame.size       = size-12; // return actual payload size (excluding headers)
  frame.address    = address;
  frame.nw_address = nw_address;
  frame.options    = options;
  frame.data       = data;

  rx_handler(&frame);
}

// AT command support

// global AI response
static uint8_t ai_response;
static bool    ai_response_received;

// functional function to initiate an AI check
static void _check_ai(void) {
  debug_printf("sending AI\n");
  ai_response = XB_AT_AI_SCANNING; // seems most logical non-ok default value
  ai_response_received = FALSE;
  _send_at('A', 'I', _handle_ai_response);
}

static void _receive_ai_response(unsigned long timeout) {
  unsigned long stop = clock_get_millis() + timeout;
  debug_printf("starting wait for ai response at %lu\n", stop - timeout);
  while(clock_get_millis() < stop) {
    xbee_receive();
    if( ai_response_received ) {
      debug_printf("got ai response at %lu\n", clock_get_millis());
      return;
    }
  }
  // timeout occured
  debug_printf("timeout waiting for AI response (%lu)\n", clock_get_millis());
}

// callback for handling AI responses
static void _handle_ai_response(uint8_t status, uint8_t* response) {
  debug_printf("AI response = %i - %i\n", status, response[0]);
  if(status == XB_AT_OK) {
    ai_response = response[0];
  }
  ai_response_received = TRUE;
}

// function to check for successful association
static bool _ai_success(void) {
  return ai_response == XB_AT_AI_SUCCESS;
}

// MY = own network address
static uint16_t nw_address = 0xFFFE;
static bool     my_response_received;

uint16_t xbee_get_nw_address(void) {
  return nw_address;
}

static void _get_my(void) {
  debug_printf("sending MY\n");
  my_response_received = FALSE;
  _send_at('M', 'Y', _handle_my_response);
}

static void _receive_my_response(unsigned long timeout) {
  unsigned long stop = clock_get_millis() + timeout;
  debug_printf("starting wait for MY response at %lu\n", stop - timeout);
  while(clock_get_millis() < stop) {
    xbee_receive();
    if( my_response_received ) {
      debug_printf("got MY response at %lu\n", clock_get_millis());
      return;
    }
  }
  // timeout occured
  debug_printf("timeout waiting for MY response (%lu)\n", clock_get_millis());
}

// callback for handling MY responses
static void _handle_my_response(uint8_t status, uint8_t* response) {
  debug_printf("MY response = %i - %02x%02x\n", status, response[0], response[1]);
  if(status == XB_AT_OK) {
    nw_address = response[1] | (response[0] << 8);
  }
  my_response_received = TRUE;
}

static bool _my_success(void) {
  return nw_address != 0xFFFE;
}

// MP = parent's network address
static uint16_t parent_address = 0xFFFE;
static bool     mp_response_received;

uint16_t xbee_get_parent_address(void) {
  return parent_address;
}

static void _get_mp(void) {
  debug_printf("sending MP\n");
  mp_response_received = FALSE;
  _send_at('M', 'P', _handle_mp_response);
}

static void _receive_mp_response(unsigned long timeout) {
  unsigned long stop = clock_get_millis() + timeout;
  debug_printf("starting wait for MP response at %lu\n", stop - timeout);
  while(clock_get_millis() < stop) {
    xbee_receive();
    if( mp_response_received ) {
      debug_printf("got MP response at %lu\n", clock_get_millis());
      return;
    }
  }
  // timeout occured
  debug_printf("timeout waiting for MP response (%lu)\n", clock_get_millis());
}

static void _handle_mp_response(uint8_t status, uint8_t* response) {
  debug_printf("MP response = %i - %02x%02x\n", status, response[0], response[1]);
  if(status == XB_AT_OK) {
    parent_address = response[1] | (response[0] << 8);
    mp_response_received = TRUE;
  }
}

static bool _mp_success(void) {
  return mp_response_received; // parent_address != 0xFFFE; <-- routers
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

  _wait_until_tx_complete();
  debug_printf("at tx complete\n");
}

// generic handling of AT responses, dispatched by xbee_receive
static void _receive_at(uint8_t size) {
  uint8_t id;
  uint8_t status;
  uint8_t* data = NULL;

  _start_rx_checksum();
  {
    _receive_byte();          // frame type is part of the checksum
    id = _receive_byte();
    _receive_byte();          // first command character
    _receive_byte();          // second
    status = _receive_byte();

    if(size - 5 > 0) {        // command data available ?
      data = malloc((size-5)*sizeof(uint8_t));
      for(uint8_t i=0; i<size-5; i++) {
        data[i] = _receive_byte();
      }
    }
  }
  if( _rx_checksum_isvalid() ) {
    (*(at_handlers[id]))(status, data);
  }
  if(data != NULL) { free(data); }
}

// modem status support

static void _receive_modem(uint8_t size) {
  // status is only used while debugging, so only include is if needed :-)
#ifdef DEBUG
  uint8_t status;
#endif
  _start_rx_checksum();
  {
    _receive_byte();        // frame type is part of the checksum
#ifdef DEBUG
    status =
#endif
             _receive_byte();
  }
  if( _rx_checksum_isvalid() ) {
    debug_printf("modem status: %i\n", status);
  }
}

// TX status support

static void _receive_transmit_status(uint8_t size) {

  uint8_t id, addr_h, addr_l, retries, delivery, discovery;

  _start_rx_checksum();
  {
    _receive_byte();        // frame type is part of the checksum

    id        = _receive_byte();
    addr_h    = _receive_byte();
    addr_l    = _receive_byte();
    retries   = _receive_byte();
    delivery  = _receive_byte();
    discovery = _receive_byte();
  }
  if( _rx_checksum_isvalid() && delivery != 0x00) {
    printf("transmission failed: %d (%02x %02x) %d : %02x  / %02x\n",
           id, addr_h, addr_l, retries, delivery, discovery);
  }
}


// checksumming support

static uint8_t tx_checksum = 0,
               rx_checksum = 0;

static void _start_tx_checksum(void) {
  tx_checksum = 0;
}

static void _start_rx_checksum(void) {
  rx_checksum = 0;
}

static void _send_checksum(void) {
  _send_byte(0xFF - tx_checksum);
}

static bool _rx_checksum_isvalid(void) {
  _receive_byte();
  // after receiving the checksum byte, the checksum should be zero
  if( rx_checksum != 0xFF ) {
    debug_printf("ERROR: invalid checksum = %i\n", rx_checksum);
    return FALSE;
  }
  return TRUE;
}

// technical (serial-oriented) functions to send one byte and wait until
// transmission has finished receiving of one byte is done through interrupts
// and an internal buffer see below

volatile static bool tx_in_progress;

// interrupt vector for handling completion of transmission
ISR (USARTx_TX_vect) {
  tx_in_progress = FALSE;
}

static void _send_byte(uint8_t c) {
  loop_until_bit_is_set(UCSRxA, UDREx); // wait until USART Data Reg Empty
  tx_in_progress = TRUE;
  UDRx = c;
  tx_checksum += c;
  _wait_until_tx_complete();
}

static void _wait_until_tx_complete(void) {
  do {} while(tx_in_progress);
}

// cyclic IO buffers
typedef struct {
  volatile uint8_t head;
  volatile uint8_t tail;
  volatile uint8_t buffer[0xff];
} cyclic_buffer_t;

cyclic_buffer_t incoming = {0, 0, {0}};

// interrupt vector for handling reception of a single byte
ISR (USARTx_RX_vect) {
  incoming.buffer[incoming.tail++] = UDRx;
}

// blocking !
static uint8_t _receive_byte(void) {
  while( ! _data_available() );
  rx_checksum += incoming.buffer[incoming.head];
  return incoming.buffer[incoming.head++];
}

// blocking !
static uint8_t _peek_byte(void) {
  while( ! _data_available() );

  return incoming.buffer[incoming.head];
}

static bool _data_available(void) {
  return incoming.head != incoming.tail;
}

static void _buffer_info(void) {
  debug_printf("buffer: head = %i : ", incoming.head);
  for(uint8_t i=incoming.head;i!=incoming.tail;i++) {
    debug_printf("%i ", incoming.buffer[i]);
  }
  debug_printf(" / tail = %i\n", incoming.tail);
}
