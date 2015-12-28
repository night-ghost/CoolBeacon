
typedef char prog_char;

// ADC speed
#define  PS_16  (1 << ADPS2)
#define  PS_32  (1 << ADPS2) | (1 << ADPS0)
#define  PS_64  (1 << ADPS2) | (1 << ADPS1)
#define  PS_128  (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0)


#define E2SIZE (E2END + 1)
