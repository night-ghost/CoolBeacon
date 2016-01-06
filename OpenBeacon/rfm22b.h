// Мощности "пиков".
#define RFM_MAX_POWER 7
#define RFM_MID_POWER 4
#define RFM_MIN_POWER 0


#define RF22B_PWRSTATE_POWERDOWN    0x00
#define RF22B_PWRSTATE_READY        0x01
#define RF22B_PACKET_SENT_INTERRUPT 0x04
#define RF22B_PWRSTATE_RX           0x05
#define RF22B_PWRSTATE_TX           0x09

void spiWriteBit(uint8_t b);
void spiSendCommand(uint8_t command);
void spiSendAddress(uint8_t i);
uint8_t spiReadData(void);
void spiWriteData(uint8_t i);
uint8_t spiReadRegister(uint8_t address);
void spiWriteRegister(uint8_t address, uint8_t data);
inline void eeprom_write(int n, byte b) { eeprom_write_byte((byte *)n,b); }
inline byte eeprom_read(int n) { return eeprom_read_byte((byte *)n); }

void narcoleptic_sleep(uint8_t wdt_period,uint8_t sleep_mode);

inline boolean getBit(byte Reg, byte whichBit) {
    return  Reg & (1 << whichBit);
}
#define NOP() __asm__ __volatile__("nop")



// если RFMка выключена то дерготня SDI ей не мешает, так что не тратим время на проверку
ISR(TIMER2_COMPA_vect) {
    SDI_off;

    if(voiceOnBuzzer) BUZZER_HIGH;
}

ISR(TIMER2_OVF_vect)  {
    SDI_on;

    if(voiceOnBuzzer) BUZZER_LOW;
}


// **** SPI bit banging functions

void spiWriteData(uint8_t i){
  for (uint8_t n = 8; n >0; n--) {

    SCK_off;
    NOP();
    if (i & 0x80) {
        SDI_on;
    } else {
        SDI_off;
    }
    NOP();
    SCK_on;
    NOP();

    i = i << 1;
  }

  SCK_off;

}

void spiSendCommand(uint8_t command)
{
  nSEL_on; // pulse on nSEL - start of transmission
  SCK_off;
  nSEL_off;

  spiWriteData(command);
}

inline void spiSendAddress(uint8_t i)
{
  spiSendCommand(i & 0x7f);
}

uint8_t spiReadData(void)
{
  uint8_t Result = 0;
  SCK_off;

  for (uint8_t i = 8; i > 0; i--) {   //read fifo data byte
    uint8_t r = 0;
    SCK_on;
    NOP();

    Result = (Result << 1) | SDO_1;

    SCK_off;
    NOP();
  }

  return(Result);
}

uint8_t spiReadRegister(uint8_t address)
{
  uint8_t result;
  spiSendAddress(address);
  result = spiReadData();
  nSEL_on;
  return(result);
}

void spiWriteRegister(uint8_t address, uint8_t data)
{
  address |= 0x80; //
  spiSendCommand(address);
  spiWriteData(data);
  nSEL_on;
}



void beacon_tone(uint16_t hz, uint16_t len)
{
  uint16_t d = 500000L / hz; // минимальная используемая частота - 160гц, то есть макс задержка 3125 прекрасно лезет в 16 бит

  for(uint32_t cycles = millis() + len; millis()<cycles; ) {
    SDI_on;
    delayMicroseconds(d);
    SDI_off;
    delayMicroseconds(d);
  }
}

void beacon_tone(uint16_t hz) {
    beacon_tone(hz, BEACON_BEEP_DURATION);
}

void narcoleptic_sleep_down(uint8_t wdt_period ) {
    narcoleptic_sleep(wdt_period,SLEEP_MODE_PWR_DOWN);
}

void narcoleptic_sleep(uint8_t wdt_period,uint8_t sleep_mode) {
  
#ifdef BODSE
  // Turn off BOD in sleep (picopower devices only)
  MCUCR |= _BV(BODSE);
  MCUCR |= _BV(BODS);
#endif

  MCUSR = 0;
  WDTCSR &= ~_BV(WDE);
  WDTCSR = _BV(WDIF) | _BV(WDIE) | _BV(WDCE);
  
  wdt_enable(wdt_period);
  wdt_reset();
#ifdef WDTCSR
  WDTCSR |= _BV(WDIE);
#else
  WDTCR |= _BV(WDIE);
#endif
  set_sleep_mode(sleep_mode);

  // Disable all interrupts
  uint8_t SREGcopy = SREG; cli();

#ifdef EECR
  register uint8_t EECRcopy = EECR; EECR &= ~_BV(EERIE);
#endif
#ifdef EIMSK
   register uint8_t EIMSKcopy = EIMSK; EIMSK = 0;
#endif
#ifdef PCMSK0
   register uint8_t PCMSK0copy = PCMSK0; PCMSK0 = 0;
#endif
#ifdef PCMSK1
   register uint8_t PCMSK1copy = PCMSK1; PCMSK1 = 0;
#endif
#ifdef PCMSK2
   register uint8_t PCMSK2copy = PCMSK2; PCMSK2 = 0;
#endif
#ifdef TIMSK0
   register uint8_t TIMSK0copy = TIMSK0; TIMSK0 = 0;
#endif
#ifdef TIMSK1
   register uint8_t TIMSK1copy = TIMSK1; TIMSK1 = 0;
#endif
#ifdef TIMSK2
   register uint8_t TIMSK2copy = TIMSK2; TIMSK2 = 0;
#endif
#ifdef SPCR
   register uint8_t SPCRcopy = SPCR; SPCR &= ~_BV(SPIE);
#endif
#ifdef UCSR0B
   register uint8_t UCSR0Bcopy = UCSR0B; UCSR0B &= ~(_BV(RXCIE0) | _BV(TXCIE0) | _BV(UDRIE0));
#endif
#ifdef TWCR
   register uint8_t TWCRcopy = TWCR; TWCR &= ~_BV(TWIE);
#endif
#ifdef ACSR
   register uint8_t ACSRcopy = ACSR; ACSR &= ~_BV(ACIE);
#endif
#ifdef ADCSRA
   register uint8_t ADCSRAcopy = ADCSRA; ADCSRA &= ~_BV(ADIE);
#endif
#ifdef SPMCSR
   register uint8_t SPMCSRcopy = SPMCSR; SPMCSR &= ~_BV(SPMIE);
#endif
  
  sei();
  sleep_mode();
  wdt_disable();

  // Reenable all interrupts
#ifdef SPMCSR
  SPMCSR = SPMCSRcopy;
#endif
#ifdef ADCSRA
  ADCSRA = ADCSRAcopy;
#endif
#ifdef ACSR
  ACSR = ACSRcopy;
#endif
#ifdef TWCR
  TWCR = TWCRcopy;
#endif
#ifdef UCSR0B
  UCSR0B = UCSR0Bcopy;
#endif
#ifdef SPCR
  SPCR = SPCRcopy;
#endif
#ifdef TIMSK2
  TIMSK2 = TIMSK2copy;
#endif
#ifdef TIMSK1
  TIMSK1 = TIMSK1copy;
#endif
#ifdef TIMSK0
  TIMSK0 = TIMSK0copy;
#endif
#ifdef PCMSK2
  PCMSK2 = PCMSK2copy;
#endif
#ifdef PCMSK1
  PCMSK1 = PCMSK1copy;
#endif
#ifdef PCMSK0
  PCMSK0 = PCMSK0copy;
#endif
#ifdef EIMSK
  EIMSK = EIMSKcopy;
#endif
#ifdef EECR
  EECR = EECRcopy;
#endif

  SREG = SREGcopy;
  
#ifdef WDTCSR
  WDTCSR &= ~_BV(WDIE);
#else
  WDTCR &= ~_BV(WDIE);
#endif
}

inline void narcoleptic_calibrate() {
  // Calibration needs Timer 1. Ensure it is powered up.
  uint8_t PRRcopy = PRR;
  PRR &= ~_BV(PRTIM1);
  
  uint8_t TCCR1Bcopy = TCCR1B;
  TCCR1B &= ~(_BV(CS12) | _BV(CS11) | _BV(CS10)); // Stop clock immediately

  // Capture Timer 1 state
   uint8_t TCCR1Acopy = TCCR1A;
   uint16_t TCNT1copy = TCNT1;
   uint16_t OCR1Acopy = OCR1A;
   uint16_t OCR1Bcopy = OCR1B;
   uint16_t ICR1copy = ICR1;
   uint8_t TIMSK1copy = TIMSK1;
   uint8_t TIFR1copy = TIFR1;

  // Configure as simple count-up timer
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  TIMSK1 = 0;
  TIFR1 = 0;
  // Set clock to /64 (should take 15625 cycles at 16MHz clock)
  TCCR1B = _BV(CS11) | _BV(CS10);
  narcoleptic_sleep(WDTO_15MS,SLEEP_MODE_IDLE);   // SLEEEEEEEEEEEEEP
  uint16_t watchdogDuration = TCNT1;
  TCCR1B = 0; // Stop clock immediately

  // Restore Timer 1
  TIFR1 = TIFR1copy;
  TIMSK1 = TIMSK1copy;
  ICR1 = ICR1copy;
  OCR1B = OCR1Bcopy;
  OCR1A = OCR1Acopy;
  TCNT1 = TCNT1copy;
  TCCR1A = TCCR1Acopy;
  TCCR1B = TCCR1Bcopy;

  // Restore power reduction state
  PRR = PRRcopy;
  
  watchdogTime_us = watchdogDuration * (64 * 1000000 / F_CPU);
}

void doSleep(uint16_t sleep_periods){
    while (sleep_periods >= 512) {
      narcoleptic_sleep_down(WDTO_8S);
      sleep_periods -= 512;
    }

    if (sleep_periods & 256) narcoleptic_sleep_down(WDTO_4S);
    if (sleep_periods & 128) narcoleptic_sleep_down(WDTO_2S);
    if (sleep_periods & 64)  narcoleptic_sleep_down(WDTO_1S);
    if (sleep_periods & 32)  narcoleptic_sleep_down(WDTO_500MS);
    if (sleep_periods & 16)  narcoleptic_sleep_down(WDTO_250MS);
    if (sleep_periods & 8)   narcoleptic_sleep_down(WDTO_120MS);
    if (sleep_periods & 4)   narcoleptic_sleep_down(WDTO_60MS);
    if (sleep_periods & 2)   narcoleptic_sleep_down(WDTO_30MS);
    if (sleep_periods & 1)   narcoleptic_sleep_down(WDTO_15MS);

}

void deepSleep(uint16_t milliseconds) {

    millisCounter += milliseconds; // во время сна таймер системного времени стоит - нужно накапливать общее время сна
    uint32_t microseconds = milliseconds * 1000L;

    if(!watchdogTime_us) { // если первый запуск - то калибруем время сна
	narcoleptic_calibrate(); // калибровка использует таймер 1 и мешает работе с GSM
	if(microseconds < watchdogTime_us ) return;
	
	microseconds -= watchdogTime_us;
    }

    doSleep(microseconds / watchdogTime_us);
/* GCC bug

    uint16_t sleep_periods = (microseconds - watchdogTime_us) / watchdogTime_us;

    while (sleep_periods >= 512) {
      narcoleptic_sleep_down(WDTO_8S);
      sleep_periods -= 512;
    }

    if (sleep_periods & 256) narcoleptic_sleep_down(WDTO_4S);
    if (sleep_periods & 128) narcoleptic_sleep_down(WDTO_2S);
    if (sleep_periods & 64)  narcoleptic_sleep_down(WDTO_1S);
    if (sleep_periods & 32)  narcoleptic_sleep_down(WDTO_500MS);
    if (sleep_periods & 16)  narcoleptic_sleep_down(WDTO_250MS);
    if (sleep_periods & 8)   narcoleptic_sleep_down(WDTO_120MS);
    if (sleep_periods & 4)   narcoleptic_sleep_down(WDTO_60MS);
    if (sleep_periods & 2)   narcoleptic_sleep_down(WDTO_30MS);
    if (sleep_periods & 1)   narcoleptic_sleep_down(WDTO_15MS);
*/
}



inline void rfmSetCarrierFrequency(uint32_t f)
{
  uint16_t fb, fc;
  byte hbsel;
  if (f < 480000000) {
    hbsel = 0;
    fb = f / 10000000 - 24;
    fc = (f - (fb + 24) * 10000000) * 4 / 625;
  } else {
    hbsel = 1;
    fb = f / 20000000 - 24;
    fc = (f - (fb + 24) * 20000000) * 2 / 625;
  }
  spiWriteRegister(0x75, 0x40 | (hbsel?0x20:0) | (fb & 0x1f));
  spiWriteRegister(0x76, (fc >> 8));
  spiWriteRegister(0x77, (fc & 0xff));
}

void initRFM(void)
{
  ItStatus1 = spiReadRegister(0x03);   // read status, clear interrupt
  ItStatus1 = spiReadRegister(0x04);

// Interrupt Enable 1 enfferr entxffafull entxffaem enrxffafull enext enpksent enpkvalid encrcerror 00h
// Interrupt Enable 2 enswdet enpreaval enpreainval enrssi enwut enlbd enchiprdy enpor 03h
   spiWriteRegister(0x05, 0x00);
  spiWriteRegister(0x06, 0x40);    // interrupt only on preamble detect

  spiWriteRegister(0x07, RF22B_PWRSTATE_READY);      // disable lbd, wakeup timer, use internal 32768,xton = 1; in ready mode
  spiWriteRegister(0x09, (byte)p.FrequencyCorrection);  // (default) c = 12.5p
  spiWriteRegister(0x0a, 0x05);//  Microcontroller Output Clock  Reserved Reserved clkt[1] clkt[0] enlfc mclk[2] mclk[1] mclk[0] 06h
  spiWriteRegister(0x0b, 0x12);    // gpio0 TX State
  spiWriteRegister(0x0c, 0x15);    // gpio1 RX State
//  #if HARDWARE_TYPE == 0
    spiWriteRegister(0x0d, 0xfd);    // gpio 2 micro-controller clk output
//  #elif HARDWARE_TYPE == 1
//    spiWriteRegister(0x0d, 0x0a);    // gpio2 - диод на Expert RX
//  #endif
  spiWriteRegister(0x0e, 0x00);    // gpio    0, 1,2 NO OTHER FUNCTION.

  spiWriteRegister(0x30, 0x00);    //disable packet handling

// *
  spiWriteRegister(0x79, 0);    // start channel
  spiWriteRegister(0x7a, 0x05);   // 50khz step size (10khz x value) // no hopping
  spiWriteRegister(0x70, 0x24);    // disable manchester
  spiWriteRegister(0x71, 0x12);   // trclk=[00] no clock, dtmod=[01] direct using SPI, fd8=0 eninv=0 modtyp=[10] FSK  - 
				// 
// fd[7] fd[6] fd[5] fd[4] fd[3] fd[2] fd[1] fd[0] 
  spiWriteRegister(0x72, 0x08);    // fd (frequency deviation) 8*625Hz == 5.0kHz
//  spiWriteRegister(0x72, 0x02);   // fd (frequency deviation) 2*625Hz == 1.25kHz 

  spiWriteRegister(0x73, 0x00);
  spiWriteRegister(0x74, 0x00);    // no frequency offset

// Preamble Detection Control preath[4] preath[3] preath[2] preath[1] preath[0] rssi_off[2] rssi_off[1] rssi_off[0] 
//  spiWriteRegister(0x35, 0x2a);    // preath = 5 (20bits), rssioff = 2
  spiWriteRegister(0x35, 0x7a);    // preath = 15 (60bits), rssioff = 2

// AFC Loop Gearshift Override afcbd enafc afcgearh[2] afcgearh[1] afcgearh[0] 1p5 bypass matap ph0size 40h
    spiWriteRegister(0x1d, 0x40);
// AFC Limiter Afclim[7] Afclim[6] Afclim[5] Afclim[4] Afclim[3] Afclim[2] Afclim[1] Afclim[0] 00h
    spiWriteRegister(0x2a, 0x14);
//*
    spiWriteRegister(0x36, 0x55); // sync words
    spiWriteRegister(0x37, 0x55);
    spiWriteRegister(0x38, 0x55);
    spiWriteRegister(0x39, 0x55);

    byte n;
    if(p.CallToneFreq>=1000) {
	spiWriteRegister(0x1c, 0xc1); //  IF Filter Bandwidth:  dwn3_bypass=1 ndec=4 filset=1
	n=4;
    }else{
	spiWriteRegister(0x1c, 0xd1); //                        dwn3_bypass=1 ndec=5 filset=1
	n=5;
    }

    //rxosr  http://www.datasheetlib.com/datasheet/1158810/rfm22_hope-microelectronics.html?page=97#datasheet

    register unsigned long rxosr = 1500000 / (p.CallToneFreq << (n-2));
    register unsigned long ncoff = (p.CallToneFreq << (n+14)) / 11719;
    
    // rxosr[7] rxosr[6] rxosr[5] rxosr[4] rxosr[3] rxosr[2] rxosr[1] rxosr[0] 64h
    spiWriteRegister(0x20, (byte)rxosr);
    // rxosr[10] rxosr[9] rxosr[8] stallctrl ncoff[19] ncoff[18] ncoff[17] ncoff[16] 01h
    spiWriteRegister(0x21, (byte)( ((rxosr & 0x700) >>3) | ((ncoff >> 16) & 0x0f ) ) );
    // ncoff[15]  ncoff[14] ncoff[13] ncoff[12] ncoff[11] ncoff[10] ncoff[9] ncoff[8] 47h
    spiWriteRegister(0x22, (byte)(ncoff >> 8));
    // ncoff[7] ncoff[6] ncoff[5] ncoff[4] ncoff[3] ncoff[2] ncoff[1] ncoff[0] AEh
    spiWriteRegister(0x23, (byte)ncoff); //  clock recovery offset 0


    uint16_t k3=(0x8000 / rxosr) + 2;
    // Clock Recovery Timing Loop Gain -  Reserved Reserved Reserved rxncocomp crgain2x crgain[10] crgain[9] crgain[8] 1
    spiWriteRegister(0x24, (byte)((k3>>8) & 0x7) ); //  
    // crgain[7]  crgain[6] crgain[5] crgain[4] crgain[3] crgain[2] crgain[1] crgain[0]
    spiWriteRegister(0x25, (byte)k3); //  

    rfmSetCarrierFrequency(p.Frequency);
    
//  unsigned int DR = ( ( (long) BEACON_LISTEN_FREQ ) << 16 ) / 15625;
  
//  spiWriteRegister(0x6e, ( DR & 0xFF )); //TX Data Rate 1
 // spiWriteRegister(0x6f, ( DR >> 8 )); // TX Data Rate 0
}

void delay_1(){
    delay(1);
}

void delay_10(){
    delay(10);
}

void delay_50(){
    delay(50);
}

void delay_100(){
    delay_50();
    delay_50();
}

void delay_300(){
    delay_100();   delay_100();   delay_100();
}

void adc_setup(){

    ADCSRA |= (1<<ADEN); //Enable ADC

// see http://www.microsmart.co.za/technical/2014/03/01/advanced-arduino-adc/

// set up the ADC
    ADCSRA &= ~PS_128;  // remove bits set by Arduino library

  // you can choose a prescaler from above.
  // PS_16, PS_32, PS_64 or PS_128
    ADCSRA |= PS_64;    // set our own prescaler to 64.

    delay_10();
}

void RFM_SetPower(byte fInit, uint8_t mode, uint8_t power) {
    if(fInit) {
	initRFM();
        delay_50();
    }
    spiWriteRegister(0x6d, power );
    spiWriteRegister(0x07, mode );
    delay_10();
}


void RFM_off(void)
{
  // TODO: SDN mangling in the future
  spiWriteRegister(0x07, RF22B_PWRSTATE_POWERDOWN);
}


void sendBeacon(void)
{
    RFM_SetPower(1, RF22B_PWRSTATE_TX, RFM_MAX_POWER);

/*  if((byte)p.HighSavePower) {
    for(unsigned int i=30*BEACON_BEEP_DURATION/1000; i>0;i--) {
      spiWriteRegister(0x7, RF22B_PWRSTATE_TX);		// TX on, XT on
      spiWriteRegister(0x6d, RFM_MAX_POWER);
      beacon_tone(500, 18);
      spiWriteRegister(0x7, RF22B_PWRSTATE_READY);		// XT on
      spiWriteRegister(0x6d, RFM_MIN_POWER);
      delay_10();
    }
  } else {
*/
      beacon_tone(500);
//  }

//  RFM_SetPower(0,RF22B_PWRSTATE_TX,RFM_MID_POWER);
  spiWriteRegister(0x6d, RFM_MID_POWER);   // 4 set mid power 15mW
  delay_10();
  beacon_tone(250);

//     RFM_SetPower(0,RF22B_PWRSTATE_TX, RFM_MIN_POWER);
  spiWriteRegister(0x6d, RFM_MIN_POWER);   // 0 set min power 1mW
  delay_10();
  beacon_tone(160);

  RFM_off();
}


