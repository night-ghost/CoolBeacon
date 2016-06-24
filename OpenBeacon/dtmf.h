
#define DIGIT_LENGTH 200
#define INTER_DIGIT_PAUSE 50


// TOP was 1ff but now is only FF so we need to add internal counter. But OCR2A's value can't be more than 254 in any case

#define DO_DTMF_ON \
    if(DTMF_enable) {	\
      i_CurSinValA += dtmfFa; \
      i_CurSinValB += dtmfFb; \
      /* normalize Temp-Pointer */ \
      byte i_TmpSinValA  =  (char)(((i_CurSinValA+4) >> 3)&(0x007F)); \
      byte i_TmpSinValB  =  (char)(((i_CurSinValB+4) >> 3)&(0x007F)); \
      byte val_a = pgm_read_byte(&auc_SinParam[i_TmpSinValA]); \
      byte val_b = pgm_read_byte(&auc_SinParam[i_TmpSinValB]); \
      /* calculate PWM value: high frequency value + 3/4 low frequency value */ \
      OCR2A = (val_a + (val_b-(val_b>>2))) << 1; \
      if(DTMF_flag) SDI_on; /* only on 2nd interrupt*/ \
      DTMF_flag = ! DTMF_flag; \
    } else  \
	SDI_on;

#define DO_DTMF_OFF \
    if(DTMF_enable) {	\
	if(!DTMF_flag) SDI_off; /* only on 1st interrupt*/ \
    } else \
	SDI_off;


extern void redBlink();
extern void RFM_set_TX();
extern void RFM_off();
extern void delay_100();

volatile bool DTMF_enable = false;
volatile bool DTMF_flag=false;

volatile uint8_t dtmfFa = 0x00;               // step width of high frequency
volatile uint8_t dtmfFb = 0x00;               // step width of low frequency
volatile uint16_t i_CurSinValA = 0;           // position freq. A in LUT (extended format)
volatile uint16_t i_CurSinValB = 0;           // position freq. B in LUT (extended format)

const PROGMEM byte auc_SinParam[128] = {
  64,67,70,73,76,79,82,85,88,91,94,96,99,102,104,106,109,111,113,115,117,118,120,121,123,124,125,126,126,127,127,127,127,127,127,127,126,
  126,125,124,123,121,120,118,117,115,113,111,109,106,104,102,99,96,94,91,88,85,82,79,76,73,70,67,64,60,57,54,51,48,45,42,39,36,33,31,28,
  25,23,21,18,16,14,12,10,9,7,6,4,3,2,1,1,0,0,0,0,0,0,0,1,1,2,3,4,6,7,9,10,12,14,16,18,21,23,25,28,31,33,36,39,42,45,48,51,54,57,60
};

const unsigned char b[] = {44,  39,  44,  48, 39, 44, 48, 39, 44, 48, 48, 39};
const unsigned char a[] = {31,  23,  23,  23, 25, 25, 25, 28, 28, 28, 31, 31};
/*
1633:   53.30112
1477:   48.20928
1336:   43.60704
1209:   39.46176
941:    30.71424
852:    27.80928
770:    25.1328
697:    22.75008
*/

void _sendDTMF(char *s){

  power_timer2_enable();

    TIMSK2 = (1<<OCIE2A) | (1<<TOIE2);  // Int T2 Overflow + Compare enabled
    TCCR2A = (1<<WGM21) | (1<<WGM20);         // Fast PWM.
    TCCR2B = (1<<CS20);                     // CLK/1 и режим Fast PWM
    OCR2A=0x0; // начальное значение компаратора

  DTMF_enable = true;
  for(byte i=0; i<strlen(s); i++) {
    redBlink();

    byte digit = s[i]-'0';
    
    if(s[i]=='#') digit=10;
    if(s[i]=='*') digit=11;
    dtmfFa = a[digit];
    dtmfFb = b[digit];
    delay(DIGIT_LENGTH);
    dtmfFa = 0;
    dtmfFb = 0;
    delay(INTER_DIGIT_PAUSE);
  }
  DTMF_enable = false;

  TIMSK2 = 0;
  TCCR2A = 0;
  TCCR2B = 0;

  power_timer2_disable();

}

void sendDTMF(char *s)
{

#if USE_MORZE
    morze.flush(); // дождаться окончания передачи
#endif

    RFM_set_TX();

    delay_100();
    _sendDTMF(s);
    RFM_off();
}

