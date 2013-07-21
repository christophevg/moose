// bits.h
// author: Christophe VG <contact@christophe.vg>

// taken from tutorial by Sparkfun

// two simple macros to turn on/off bits on a PORT/PIN

#define sbi(var, mask) ((var) |= (uint8_t)(1 << mask))
#define cbi(var, mask) ((var) &= (uint8_t)~(1 << mask))
