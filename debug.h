// debug.h
// author: Christophe VG

// compile-time driven debug facilities
// based on :
// http://stackoverflow.com/questions/1644868/c-define-macro-for-debug-printing

#define DEBUG_name = STR(DEBUG)

#ifdef DEBUG
#include <stdio.h>
#define DEBUG_TEST 1
#define debug_printf(...) \
            do { if (DEBUG_TEST) fprintf(stdout, ##__VA_ARGS__); } while (0)
#else
#define debug_printf(...)
#endif
