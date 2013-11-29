#ifndef __PINS_H
#define __PINS_H

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)
#define MCU_NAME STR(MCU)

// mapping of functions -> actual ports
#if MCU == atmega1284p

// MISO
#define MISO_PORT DDRB
#define MISO_PIN  PB6
// RX
#define RX_PORT   DDRD
#define RX_PIN    PD0

#else

// MISO
#define MISO_PORT DDRB
#define MISO_PIN  PB4
// RX
#define RX_PORT   DDRD
#define RX_PIN    PD0

#endif

#endif
