// xbee.h
// author: Christophe VG <contact@christophe.vg>

// functions to access an XBee S2/ZB from AVR

#define XBEE_SLEEP 2

void     xbee_send_char(char c);
uint8_t  xbee_get_char(void);

void     xbee_wait_for_association(void);

void     xbee_at(uint8_t ch1, uint8_t ch2, uint8_t id);
uint8_t  xbee_get_response(uint8_t type, uint8_t id, uint8_t ch1, uint8_t ch2);

void     xbee_transmit(uint8_t* bytes, uint8_t length);
void     xbee_transmit_string(char* string);

void     xbee_sleep(void);
void     xbee_wakeup(void);
