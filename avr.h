// avr.h
// author: Christophe VG <contact@christophe.vg>

// elementary functions for handling the AVR/ATMEGA MCU

void     avr_init(void);
void     avr_adc_init(void);
uint16_t avr_adc_read(uint8_t ch);
