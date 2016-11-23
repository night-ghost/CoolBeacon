#pragma once

typedef char prog_char;

// ADC speed
#define  PS_16  (1 << ADPS2)
#define  PS_32  (1 << ADPS2) | (1 << ADPS0)
#define  PS_64  (1 << ADPS2) | (1 << ADPS1)
#define  PS_128  (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0)


#define E2SIZE (E2END + 1)

#define ToDeg(x) (x*57.2957795131)	// *180/pi

#define NOINLINE __attribute__ ((noinline))

#define BYTE_OF(v,n) (((byte *)&(v))[n])

#define INLINE __attribute__ ((always_inline)) inline
#define WEAK __attribute__((weak))

#define TO_STRING2(x) #x

#define TO_STRING(x) TO_STRING2(x)

inline bool timeToScreen() { return false; } // compatibility with OSD code

