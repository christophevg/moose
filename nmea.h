// NMEA parser - only support for GPGGA and its coordinates, for now ;-)
// author: Christophe VG

#include <stdint.h>

#ifndef __NMEA_H
#define __NMEA_H

// parser generates a struct containing the position
typedef struct {
  struct {
    uint8_t hour;
    uint8_t min;
    float   sec;
  } time;
  struct {
    uint8_t deg;
    float   min;
    char    ns;
  } lattitude;
  struct {
    uint8_t deg;
    float   min;
    char    ew;
  } longitude;
} nmea_position;

// FIXME: for some reason I can't remove this (unused) type declaration
typedef void(*nmea_position_handler)(nmea_position);

// TODO: to be implemented by top-level using functionality
extern void gps_position_handler(nmea_position pos);

// feed the parser one byte at a time
void nmea_parse(uint8_t);

// function to explicitely get the current computed position
nmea_position nmea_get_position(void);

#endif
