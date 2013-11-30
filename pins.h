#ifndef __PINS_H
#define __PINS_H

#define MCU_NAME STR(MCU)

// mapping of functions -> actual ports
#if MCU == atmega1284p

// RX0
#define RX0_PORT  DDRD
#define RX0_PIN   PD0

// RX1
#define RX1_PORT   DDRD
#define RX1_PIN    PD2

#else

// RX
#define RX0_PORT   DDRD
#define RX0_PIN    PD0

#endif

#endif
