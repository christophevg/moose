// NMEA parser - only support for GPGGA and its coordinates, for now ;-)
// constructed as a state machine

// example:
// $GPGGA,143211.000,5301.8555,N,01318.2236,E,...
// $.....,hhmmss.sss,ddmm.mmmm,[NS],ddmm.mmmm,[EW],...

#include <math.h>
#include <stdlib.h>

#include "nmea.h"

// FIXME: for some reason I can't remove this (unused) declared variable
nmea_position_handler position_handler = NULL;

// local variable to store the results of parsing
static nmea_position position;

nmea_position nmea_get_position() { return position; }

static void _reset_position(void) {
  position.time.hour     = 0;
  position.time.min      = 0;
  position.time.sec      = 0;
  position.lattitude.deg = 0;
  position.lattitude.min = 0;
  position.lattitude.ns  = '_';
  position.longitude.deg = 0;
  position.longitude.min = 0;
  position.longitude.ew  = '_';
}

// states use parser functions to do parts of the parsing
// they return a result code indicating if they failed to parse, want to 
// continue parsing or are done for their part
// the codes are used as their int value 0, 1, 2 in the transitions table
// to choose the next state.
enum result_code { FAIL, REPEAT, OK };

// detects the start of a command
int find_start (uint8_t b) { return b == '$' ? OK : FAIL; }

// detects the GPGGA command
int parse_gpgga(uint8_t b) {
  static uint8_t pos = 0;
  const uint8_t string[] = "GPGGA";

  // is it the expected next char?
  if(string[pos] == b) {
    if(pos == 4) {
      pos = 0;
      _reset_position();
      return OK;
    } else {
      pos++;
      return REPEAT;
    }
  }
  pos = 0;
  return FAIL;
}

// parses the UTC timestamp
int parse_time (uint8_t b) {
  static uint8_t pos = 0;
  switch(pos) {
    case 0: // Hhmmss.sss
    case 1: // hHmmss.sss
      if(b < '0' && b > '9') { pos = 0; return FAIL; }
      position.time.hour += (b - '0') * pow(10, (1-pos));
      break;
    case 2: // hhMmss.sss
    case 3: // hhmMss.sss
      if(b < '0' && b > '9') { pos = 0; return FAIL; }
      position.time.min += (b - '0') * pow(10, (3-pos));
      break;
    case 4: // hhmmSs.sss
    case 5: // hhmmsS.sss
      if(b < '0' && b > '9') { pos = 0; return FAIL; }
      position.time.sec += (b - '0') * pow(10, (5-pos));
      break;
    case 6: // skip .
      break;
    case 7: // hhmmss.Sss
    case 8: // hhmmss.sSs
    case 9: // hhmmss.ssS
      if(b < '0' && b > '9') { pos = 0; return FAIL; }
      position.time.sec += (b - '0') / pow(10, (pos - 6));
      break;
  }
  if(pos == 9) {
    pos = 0;
    return OK;
  } else {
    pos++;
    return REPEAT;
  }
}

// parses the lattitude information
int parse_lattitude(uint8_t b) {
  static uint8_t pos = 0;
  switch(pos) {
    case 0: // Ddmm.mmmm
    case 1: // dDmm.mmmm
      if(b < '0' && b > '9') { pos = 0; return FAIL; }
      position.lattitude.deg += (b - '0') * pow(10, (1-pos));
      break;
    case 2: // Ddmm.mmmm
    case 3: // dDmm.mmmm
      if(b < '0' && b > '9') { pos = 0; return FAIL; }
      position.lattitude.min += (b - '0') * pow(10, (3-pos));
      break;
    case 4: break; // skip .
    case 5:
    case 6:
    case 7:
    case 8:
      if(b < '0' && b > '9') { pos = 0; return FAIL; }
      position.lattitude.min += (b - '0') / pow(10, (pos - 4));
      break;
  }
  if(pos == 8) {
    pos = 0;
    return OK;
  } else {
    pos++;
    return REPEAT;
  }
}

// parses the lattitude orientation
int parse_ns_ind(uint8_t b) {
  if(b != 'N' && b != 'S') { return FAIL; }
  position.lattitude.ns = b;
  return OK;
}

// parses the liongitude information
int parse_longitude(uint8_t b) {
  static uint8_t pos = 0;
  switch(pos) {
    case 0: // Dddmm.mmmm
      position.longitude.deg = 0;
      position.longitude.min = 0;
    case 1: // dDdmm.mmmm
    case 2: // ddDmm.mmmm
      if(b < '0' && b > '9') { pos = 0; return FAIL; }
      position.longitude.deg += (b - '0') * pow(10, (2-pos));
      break;
    case 3: // Ddmm.mmmm
    case 4: // dDmm.mmmm
      if(b < '0' && b > '9') { pos = 0; return FAIL; }
      position.longitude.min += (b - '0') * pow(10, (4-pos));
      break;
    case 5: break; // skip .
    case 6:
    case 7:
    case 8:
    case 9:
      if(b < '0' && b > '9') { pos = 0; return FAIL; }
      position.longitude.min += (b - '0') / pow(10, (pos - 5));
      break;
  }
  if(pos == 9) {
    pos = 0;
    return OK;
  } else {
    pos++;
    return REPEAT;
  }
}

// parses the longitude orientation
int parse_ew_ind(uint8_t b) {
  if(b != 'E' && b != 'W') { return FAIL; }
  position.longitude.ew = b;

  gps_position_handler(position);

  return OK;
}

// function pointer for state_handlers
typedef int (* state_handler)(uint8_t);

enum state {
  start_state,
  gpgga_state,
  time_state,
  lattitude_state,
  ns_ind_state,
  longitude_state,
  ew_ind_state
};

struct transition {
  state_handler handler;
  int result[3];
};

static struct transition transitions[] = {
  /* in state              what to look for   FAIL         REPEAT            OK               */
  /* -------------------   ----------------   -----------  ----------------  ---------------  */
  /* start_state */      { find_start,      { start_state, start_state,      gpgga_state     } },
  /* gpgga_state */      { parse_gpgga,     { start_state, gpgga_state,      time_state      } },
  /* time_state */       { parse_time,      { start_state, time_state,       lattitude_state } },
  /* lattitude_state */  { parse_lattitude, { start_state, lattitude_state,  ns_ind_state    } },
  /* ns_ind_state */     { parse_ns_ind,    { start_state, start_state,      longitude_state } },
  /* longitude_state */  { parse_longitude, { start_state, longitude_state,  ew_ind_state    } },
  /* ew_ind_state */     { parse_ew_ind,    { start_state, start_state,      start_state     } }
};

static int current_state = start_state;
  
void nmea_parse(uint8_t b) {
  if(b == ',' || b == '\n' || b == '\r') {   // IGNORES
    // printf("skip\n");
    return;
  }
  int outcome   = transitions[current_state].handler(b);
  current_state = transitions[current_state].result[outcome];
  // printf(" > outcome = %d -> %d\n", outcome, current_state);
}
