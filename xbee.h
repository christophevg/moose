// xbee.h
// author: Christophe VG <contact@christophe.vg>

// functions to access an XBee S2/ZB from AVR

#ifndef __XBEE_H
#define __XBEE_H

#include <string.h>

#include <avr/io.h>
#include <util/delay.h>

#include "avr.h"

// magic bytes

// internal
#define XB_FRAME_START      0x7E
#define XB_TX_PACKET        0x10

// externally useable
#define XB_RX_PACKET        0x90
#define XB_TX_AT            0x08
#define XB_RX_AT            0x88

#define XB_TX_NO_RESPONSE   0x00      // id to use when no response wanted
#define XB_BROADCAST        0xFFFE    // nw_address for broadcast
#define XB_NW_ADDR_UNKNOWN  0xFFFE    //             or unknown
#define XB_MAX_RADIUS       0x00
#define XB_OPT_NO_ACK       0x01
#define XB_OPT_APS_ENC      0x20
#define XB_OPT_EXT_TIMEOUT  0x40

// AT command status
#define XB_AT_OK            0x00
#define XB_AT_ERROR         0x01
#define XB_AT_INV_CMD       0x02
#define XB_AT_INV_PARAM     0x03
#define XB_AT_TX_FAIL       0x04

// AT AI responses
#define XB_AT_AI_SUCCESS    0x00
#define XB_AT_AI_SCANNING   0xFF

// XBEE is controled via USART, some AVR devices have multiple USARTs
// the identifying 0 or 1 is replace by an 'x'

#ifdef XBEE_ON_USART1     // USART1 (e.g. on the ATMEGA1284p)
#define UCSRxA UCSR1A
#define UDREx  UDRE1
#define RXCx   RXC1
#define UDRx   UDR1
#define TXCx   TXC1
#else                     // USART0 (default)
#define UCSRxA UCSR0A
#define UDREx  UDRE0
#define RXCx   RXC0
#define UDRx   UDR0
#define TXCx   TXC0
#endif

// pin mapping

#define XBEE_SLEEP 2

// TX struct
typedef struct {
  uint16_t size;
  uint8_t  id;
  uint64_t address;
  uint16_t nw_address;
  uint8_t  radius
  uint8_t  options;
  uint8_t *data;
} xbee_tx_t;

// RX struct
typedef struct {
  uint16_t size;
  uint64_t address;
  uint16_t nw_address;
  uint8_t  options;
  uint8_t *data;
} xbee_rx_t;

// RX handler type
typedef void (*xbee_rx_handler_t)(xbee_rx_t *frame);

// AT response handler type
typedef void (*xbee_at_handler_t)();

// functions

void xbee_sleep(void);
void xbee_wakeup(void);
void xbee_wait_for_association(void);

void xbee_send(xbee_tx_t *frame);
void xbee_receive();
void xbee_on_receive(xbee_rx_handler_t handler);

#endif
