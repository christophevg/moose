// test for NMEA parser
// author: Christophe VG

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "nmea.h"

// forward declarations of "private" functions ;-)
int parse_gpgga(uint8_t);
int parse_time(uint8_t);
int parse_lattitude(uint8_t);
int parse_longitude(uint8_t);
nmea_position nmea_get_position(void);

// to stay in sync with internals ;-)
enum result_code { FAIL, REPEAT, OK };

// storage and handler to receive position information
nmea_position pos;
void gps_position_handler(nmea_position position) {
  pos = position;
  printf(
    "UTC=%2d:%2d:%2.3f | latt: %2d %2.4f\" %c | long: %3d %2.4f\" %c\n",
    pos.time.hour, pos.time.min, pos.time.sec,
    pos.lattitude.deg, pos.lattitude.min, pos.lattitude.ns,
    pos.longitude.deg, pos.longitude.min, pos.longitude.ew    
  );
}

void test_gpgga() {
  assert(parse_gpgga('G') == REPEAT);
  assert(parse_gpgga('P') == REPEAT);
  assert(parse_gpgga('G') == REPEAT);
  assert(parse_gpgga('G') == REPEAT);
  assert(parse_gpgga('A') == OK);
  
  assert(parse_gpgga('G') == REPEAT);
  assert(parse_gpgga('G') == FAIL);
}

void test_time() {
  // 233211.123
  parse_time('2');
  parse_time('3');
  parse_time('3');
  parse_time('2');
  parse_time('1');
  parse_time('1');
  parse_time('.');
  parse_time('1');
  parse_time('2');
  parse_time('3');
  
  pos = nmea_get_position();

  assert(pos.time.hour   == 23);
  assert(pos.time.min == 32);
  assert(pos.time.sec  - 11.123 < 0.00001);
}

void test_lattitude() {
  // 5301.8555
  parse_lattitude('5');
  parse_lattitude('3');
  parse_lattitude('0');
  parse_lattitude('1');
  parse_lattitude('.');
  parse_lattitude('8');
  parse_lattitude('5');
  parse_lattitude('5');
  parse_lattitude('5');
  
  pos = nmea_get_position();

  assert(pos.lattitude.deg == 53);
  assert(pos.lattitude.min - 1.8555 < 0.000001);
}

void test_longitude() {
  // 01318.2236
  parse_longitude('0');
  parse_longitude('1');
  parse_longitude('3');
  parse_longitude('1');
  parse_longitude('8');
  parse_longitude('.');
  parse_longitude('2');
  parse_longitude('2');
  parse_longitude('3');
  parse_longitude('6');
  
  pos = nmea_get_position();
  
  assert(pos.longitude.deg == 13);
  assert(pos.longitude.min - 18.2236 < 0.000001);
}

int main(void) {
  // a few unittests
  test_gpgga();
  test_time();
  test_lattitude();

  // a complete parse
  char stream[] = "$GPGGA,143211.000,5301.8555,N,01318.2236,E,...";

  for(uint8_t t=0; t<strlen(stream); t++) {
    // printf("%c : ", stream[t]);
    nmea_parse(stream[t]);
  }
  
  assert(pos.time.hour == 14);
  assert(pos.time.min  == 32);
  assert(pos.time.sec - 11.000 < 0.0001);
  
  assert(pos.lattitude.deg == 53);
  assert(pos.lattitude.min - 1.8555 < 0.00001);
  
  assert(pos.lattitude.ns == 'N');

  assert(pos.longitude.deg == 13);
  assert(pos.longitude.min - 18.2236 < 0.00001);

  assert(pos.longitude.ew == 'E');
  
  char stream2[] = "$GPGGA,231322.010,0801.1565,S,15819.6636,W,...";

  for(uint8_t t=0; t<strlen(stream2); t++) {
    // printf("%c : ", stream[t]);
    nmea_parse(stream2[t]);
  }
  
  assert(pos.time.hour   == 23);
  assert(pos.time.min == 13);
  assert(pos.time.sec - 22.010 < 0.0001);
  
  assert(pos.lattitude.deg == 8);
  assert(pos.lattitude.min - 1.1565 < 0.00001);
  
  assert(pos.lattitude.ns == 'S');

  assert(pos.longitude.deg == 158);
  assert(pos.longitude.min - 19.6636 < 0.00001);

  assert(pos.longitude.ew == 'W');
  
  char stream3[] = "$GPGGA,220704.000,5301.8561,N,01318.2237,E,1,08,1.0,46.3,M,44.1,M,,0000*64\
,11,09,75,238,44,23,61,072,39,06,52,266,37,03,31,116,34*77\
$GPGSV,3,2,11,02,28,309,43,07,23,180,33,26,14,056,32,16,10,084,16*7C\
$GPGSV,3,3,11,51,00,295,,10,70,284,,31,00,029,*42\
$GPRMC,220704.000,A,5301.8561,N,01318.2237,E,0.00,18.20,140815,,,A*5D\
$GPGGA,220705.000,5301.8561,N,01318.2237,E,1,08,1.0,46.3,M,44.1,M,,0000*65\
$GPGSA,A,3,09,23,06,03,02,07,26,16,,,,,1.8,1.0,1.5*35\
$GPRMC,220705.000,A,5301.8561,N,01318.2237,E,0.00,18.20,140815,,,A*5C\
$GPGGA,220706.000,5301.8561,N,01318.2237,E,1,08,1.0,46.3,M,44.1,M,,0000*66\
$GPGSA,A,3,09,23,06,03,02,07,26,16,,,,,1.8,1.0,1.5*35\
$GPRMC,220706.000,A,5301.8561,N,01318.2237,E,0.00,18.20,140815,,,A*5F\
$GPGGA,220707.000,5301.8561,N,01318.2237,E,1,08,1.0,46.3,M,44.1,M,,0000*67\
$GPGSA,A,3,09,23,06,03,02,07,26,16,,,,,1.8,1.0,1.5*35\
$GPRMC,220707.000,A,5301.8561,N,01318.2237,E,0.00,18.20,140815,,,A*5E\
$GPGGA,220708.000,5301.8561,N,01318.2237,E,1,08,1.0,46.3,M,44.1,M,,0000*68\
$GPGSA,A,3,09,23,06,03,02,07,26,16,,,,,1.8,1.0,1.5*35\
$GPRMC,220708.000,A,5301.8561,N,01318.2237,E,0.00,18.20,140815,,,A*51\
$GPGGA,220709.000,5301.8561,N,01318.2237,E,1,08,1.0,46.3,M,44.1,M,,0000*69\
$GPGSA,A,3,09,23,06,03,02,07,26,16,,,,,1.8,1.0,1.5*35\
$GPGSV,3,1,11,09,75,238,41,23,61,072,39,06,52,266,26,03,31,116,20*77\
$GPGSV,3,2,11,02,28,309,40,07,23,180,32,26,14,056,31,16,10,084,18*73\
$GPGSV,3,3,11,46,00,288,,10,70,284,,31,00,029,*48\
$GPRMC,220709.000,A,5301.8561,N,01318.2237,E,0.00,18.20,140815,,,A*50\
$GPGGA,220710.000,5301.8561,N,01318.2237,E,1,08,1.0,46.3,M,44.1,M,,0000*61\
$GPGSA,A,3,09,23,06,03,02,07,26,16,,,,,1.8,1.0,1.5*35\
$GPRMC,220710.000,A,5301.8561,N,01318.2237,E,0.00,18.20,140815,,,A*58\
$GPGGA,220711.000,5301.8561,N,01318.2237,E,1,08,1.0,46.3,M,44.1,M,,0000*60\
$GPGSA,A,3,09,23,06,03,02,07,26,16,,,,,1.8,1.0,1.5*35\
$GPRMC,220711.000,A,5301.8561,N,01318.2237,E,0.00,18.20,140815,,,A*59";
  
  for(int t=0; t<strlen(stream3); t++) {
    // printf("%c : ", stream[t]);
    nmea_parse(stream3[t]);
  }

  return(EXIT_SUCCESS);
}
