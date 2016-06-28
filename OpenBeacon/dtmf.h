/**
 idea from
 
 * dtmf generator.
 * AVR AppNote314
 * 
 * @date	2007-03-15
 * @author	Anders Runeson arune at sf dot net
 *   
 */
 
 
#define DIGIT_LENGTH 200
#define INTER_DIGIT_PAUSE 50


// TOP was 1ff but now is only FF so we need to add internal counter. But OCR2A's value can't be more than 254 in any case

#define DO_DTMF \
    if(DTMF_enable) {	\
      i_CurSinPtr_A += dtmfFa; \
      i_CurSinPtr_B += dtmfFb; \
      /* normalize Temp-Pointer by 8 */ \
      byte val_a = getSinValue(i_CurSinPtr_A); \
      byte val_b = getSinValue(i_CurSinPtr_B); \
      /* calculate PWM value: high frequency value + 3/4 low frequency value */ \
      OCR2A = val_a + (val_b-(val_b>>2)); /* <<1 */ \
    } 


//Table of x_SW (excess 8): x_SW = ROUND(FP*N_SAMPLES*f*256/Fck)

extern void redBlink();
extern void RFM_set_TX();
extern void RFM_off();
extern void delay_100();
extern void beep();

volatile bool DTMF_enable = false;
//volatile bool DTMF_flag=false;

volatile uint8_t dtmfFa = 0x00;               // step width of high frequency
volatile uint8_t dtmfFb = 0x00;               // step width of low frequency
volatile uint16_t i_CurSinPtr_A = 0;          // position freq. A in LUT (extended format)
volatile uint16_t i_CurSinPtr_B = 0;          // position freq. B in LUT (extended format)


#define DTMF_NORM 16

#define PRESCALER     1 
#define FCK           F_CPU/PRESCALER
#define DTMF_DIV      (62500 / 128 / 2 / DTMF_NORM)
#define DTMF_FREQ(hz) ((hz+DTMF_DIV/2) / DTMF_DIV)
#define FP			256


#if 1

#define N_SAMPLES     128
const PROGMEM byte sinusTable[N_SAMPLES] = { // 128*128
  64,67,70,73,76,79,82,85,88,91,94,96,99,102,104,106,109,111,113,115,117,118,120,121,123,124,125,126,126,127,127,127,127,127,127,127,126,
  126,125,124,123,121,120,118,117,115,113,111,109,106,104,102,99,96,94,91,88,85,82,79,76,73,70,67,64,60,57,54,51,48,45,42,39,36,33,31,28,
  25,23,21,18,16,14,12,10,9,7,6,4,3,2,1,1,0,0,0,0,0,0,0,1,1,2,3,4,6,7,9,10,12,14,16,18,21,23,25,28,31,33,36,39,42,45,48,51,54,57,60
};

byte getSinValue(uint16_t ipos){
    byte pos =  ((ipos+DTMF_NORM/2) / DTMF_NORM) & (N_SAMPLES-1); 

    return sinusTable[ pos ];
}

#else

#define N_SAMPLES 256
const PROGMEM byte  sinusTable[64] = {
63, 63, 63, 63, 63, 63, 62, 62, 62, 61, 61, 61, 60, 60, 59, 59, 58, 58, 57, 
56, 56, 55, 54, 53, 52, 52, 51, 50, 49, 48, 47, 46, 45, 43, 42, 41, 40, 39, 
38, 36, 35, 34, 32, 31, 30, 28, 27, 26, 24, 23, 21, 20, 18, 17, 15, 14, 12, 
11,  9,  8,  6,  5,  3,  2 };

byte getSinValue(uint16_t ipos){
    byte pos =  ((ipos+DTMF_NORM/2) / DTMF_NORM) & (N_SAMPLES-1); 

    byte val;
    if (pos<N_SAMPLES/4) {
	val=sinusTable[ pos ];
    } else if (pos<N_SAMPLES/2) {
	val= -sinusTable[ (N_SAMPLES/2-1)-pos ];
    } else if (pos<N_SAMPLES*3/4) {
	val= -sinusTable[ pos-(N_SAMPLES/2) ];
    } else {
	val=sinusTable[ (N_SAMPLES-1)-pos ];
    }
    return val+64;
}

#endif


//                          0  1   2   3   4   5   6   7   8   9   #   *
const PROGMEM byte b[] = {44, 39, 44, 48, 39, 44, 48, 39, 44, 48, 48, 39};
const PROGMEM byte a[] = {31, 23, 23, 23, 25, 25, 25, 28, 28, 28, 31, 31};
/*

1 	2 	3 	A 	697 Гц
4 	5 	6 	B 	770 Гц
7 	8 	9 	C 	852 Гц
* 	0 	# 	D 	941 Гц
1209 Гц 1336 Гц 1477 Гц 1633 Гц

PWM 62.5KHz (период 16мкс)

if K==1 we will generate tone 62.5KHz / 128 (samples) /2 ( period)/ 8 (normalization) = 30,517578125 

DTMF_NORM*N_SAMPLES*f*510/FCK



1633:   53.30112 - 30,637254902
1477:   48.20928
1336:   43.60704
1209:   39.46176

941:    30.71424
852:    27.80928
770:    25.1328
697:    22.75008 - 30,637254902
*/


void _sendDTMF(char *s, byte beeps){

  power_timer2_enable();

    TIMSK2 = (1<<OCIE2A) | (1<<TOIE2);  // Int T2 Overflow + Compare enabled
    TCCR2A = (1<<WGM21)  | (1<<WGM20);         // Fast PWM.
    TCCR2B = (1<<CS20);                     // CLK/1 и режим Fast PWM
    OCR2A=0x0; // начальное значение компаратора

    for(; beeps>0; beeps--){
        beep();
        delay_100();
    }

  DTMF_enable = true;
  for(byte i=0; i<strlen(s); i++) {
    redBlink();

    byte digit;
    
    switch(s[i]) {
    case '#': 
	digit=10; 
	break;
    case '*': 
    case '.': 
	digit=11; 
	break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
	digit = s[i]-'0';
	break;
    default:
	goto play_silent;
    }
#if 0
    dtmfFa = a[digit];
    dtmfFb = b[digit];
#else /* test */
    dtmfFa = DTMF_FREQ(1750); // 117
    dtmfFb = 0;
#endif

play_silent:
    delay(DIGIT_LENGTH);
    dtmfFa = 0;
    dtmfFb = 0;
    i_CurSinPtr_A=0;
    i_CurSinPtr_B=0;
    delay(INTER_DIGIT_PAUSE);
  }
  DTMF_enable = false;

  TIMSK2 = 0;
  TCCR2A = 0;
  TCCR2B = 0;

  power_timer2_disable();

}

void sendDTMF(char *s, byte beeps=0){

#if USE_MORZE
    extern morseEncoder morze;
    morze.flush(); // дождаться окончания передачи
#endif

    RFM_set_TX();

    delay_100(); 	//  открыть шумодав
    _sendDTMF(s, beeps);
    RFM_off();
}

