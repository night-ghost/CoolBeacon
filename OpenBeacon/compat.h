
typedef char prog_char;

// ADC speed
#define  PS_16  (1 << ADPS2)
#define  PS_32  (1 << ADPS2) | (1 << ADPS0)
#define  PS_64  (1 << ADPS2) | (1 << ADPS1)
#define  PS_128  (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0)


#define E2SIZE (E2END + 1)

enum ap_var_type {
    AP_PARAM_NONE    = 0,
    AP_PARAM_INT8,
    AP_PARAM_INT16,
    AP_PARAM_INT32,
    AP_PARAM_FLOAT,
    AP_PARAM_VECTOR3F,
    AP_PARAM_VECTOR6F,
    AP_PARAM_MATRIX3F,
    AP_PARAM_GROUP
};

#define ToDeg(x) (x*57.2957795131)	// *180/pi

#define NOINLINE __attribute__ ((noinline))
