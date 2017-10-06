// Мощности "пиков".
#define RFM_MAX_POWER 7
#define RFM_MID_POWER 4
#define RFM_MIN_POWER 0


#define RF22B_PWRSTATE_POWERDOWN    0x00
#define RF22B_PWRSTATE_READY        0x01
#define RF22B_PACKET_SENT_INTERRUPT 0x04
#define RF22B_PWRSTATE_RX           0x05
#define RF22B_PWRSTATE_TX           0x09

enum RFM22B_GPIO_Function {
    POWER_ON_RESET                              = 0x00,         // This depends on the GPIO!
    WAKE_UP_TIMER_1                             = 0x01,
    LOW_BATTERY_DETECT_1                        = 0x02,
    DIRECT_DIGITAL_INPUT                        = 0x03,
    EXTERNAL_INTERRUPT_FALLING                  = 0x04,
    EXTERNAL_INTERRUPT_RISING                   = 0x05,
    EXTERNAL_INTERRUPT_CHANGE                   = 0x06,
    ADC_ANALOGUE_INPUT                          = 0x07,
//      RESERVED                                = 0x08, 
//      RESERVED                                = 0x09,
    DIRECT_DIGITAL_OUTPUT                       = 0x0A,
//      RESERVED                                = 0x0B, 
//      RESERVED                                = 0x0C,
//      RESERVED                                = 0x0D,
    REFERENCE_VOLTAGE                           = 0x0E,
    DATA_CLOCK_OUTPUT                           = 0x0F,
    DATA_INPUT                                  = 0x10,
    EXTERNAL_RETRANSMISSION_REQUEST             = 0x11,
    TX_STATE                                    = 0x12,
    TX_FIFO_ALMOST_FULL                         = 0x13,
    RX_DATA                                     = 0x14,
    RX_STATE                                    = 0x15,
    RX_FIFO_ALMOST_FULL                         = 0x16,
    ANTENNA_1_SWITCH                            = 0x17,
    ANTENNA_2_SWITCH                            = 0x18,
    VALID_PREAMBLE_DETECTED                     = 0x19,
    INVALID_PREAMBLE_DETECTED                   = 0x1A,
    SYNC_WORD_DETECTED                          = 0x1B,
    CLEAR_CHANNEL_ASSESSMENT                    = 0x1C,
    VDD                                         = 0x1D,
    GND                                         = 0x1E
};

// Interrupt Enable spans 2 registers, but for the purpose of this enum they are treated as one (16 bit) register
enum RFM22B_Interrupt {
    POWER_ON_RESET_INT                          = (1 << 0),
    CHIP_READY                                  = (1 << 1),
    LOW_BATTERY_DETECT                          = (1 << 2),
    WAKE_UP_TIMER                               = (1 << 3),
    RSSI                                        = (1 << 4),
    INVALID_PREAMBLE                            = (1 << 5),
    VALID_PREAMBLE                              = (1 << 6),
    SYNC_WORD                                   = (1 << 7),
    CRC_ERROR                                   = (1 << 8),
    VALID_PACKET_RECEIVED                       = (1 << 9),
    PACKET_SENT                                 = (1 << 10),
    EXTERNAL_INT                                = (1 << 11),
    RX_FIFO_ALMOST_FULL_INT                     = (1 << 12),
    TX_FIFO_ALMOST_EMPTY_INT                    = (1 << 13),
    TX_FIFO_ALMOST_FULL_INT                     = (1 << 14),
    FIFO_UNDERFLOW_OVERFLOW                     = (1 << 15)
};

void spiSendCommand(uint8_t command);
uint8_t spiReadData(void);
void spiWriteData(uint8_t i);
uint8_t spiReadRegister(uint8_t address);
void spiWriteRegister(uint8_t address, uint8_t data);

void narcoleptic_sleep(uint8_t wdt_period,uint8_t sleep_mode);

static inline boolean getBit(byte Reg, byte whichBit) {
    return  Reg & (1 << whichBit);
}
#define NOP() __asm__ __volatile__("nop")



// если RFMка выключена то дерготня SDI ей не мешает, так что не тратим время на проверку
ISR(TIMER2_COMPB_vect) {

#ifndef HARD_VOICE
    SDI_off;
#endif
    if(voiceOnBuzzer) BUZZER_HIGH;
}

ISR(TIMER2_OVF_vect)  {
#ifndef HARD_VOICE
    SDI_on;
#endif
    if(voiceOnBuzzer) BUZZER_LOW;

#if defined(USE_DTMF) && !defined(DTMF_TIM0)
    extern void do_DTMF();
    extern volatile byte DTMF_enable;
    if(DTMF_enable) do_DTMF(); // calculate next value
#endif


}

// обработчик прерывания по получению вызова
//void RFM22B_Int()
//*
#if IRQ_interrupt == 0
ISR(INT0_vect)
#else
ISR(INT1_vect)
#endif
//*/
{
  volatile byte reg=spiReadRegister(0x03); // bye-bye
  reg=spiReadRegister(0x04);

  if(reg & 64) { 	// ipreaval
    preambleDetected = true;
    preambleRSSI = spiReadRegister(0x26);
  }
}


// **** SPI bit banging functions

void spiWriteData(uint8_t i){
  for (uint8_t n = 8; n >0; n--) {

    SCK_off;
    NOP(); // времени проверки достаточно
    if (i & 0x80) {
        SDI_on;
    } else {
        SDI_off;
    }
    NOP();
    SCK_on;
    NOP(); // время выполнения сдвига достаточно

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

static inline void spiSendAddress(uint8_t i)
{
  spiSendCommand(i & 0x7f);
}

uint8_t spiReadData(void)
{
  uint8_t Result = 0;
  SCK_off;

  for (uint8_t i = 8; i > 0; i--) {   //read fifo data byte
    SCK_on;
    NOP();

    Result = (Result << 1) | SDO_read;

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

  for(uint32_t to = millis() + len; millis()<to; ) {
    SDI_on;
    delayMicroseconds(d);
    SDI_off;
    delayMicroseconds(d);
  }
}

void beacon_tone(uint16_t hz) {
    beacon_tone(hz, BEACON_BEEP_DURATION);
}

void beacon_tone_150(uint16_t hz) {
    beacon_tone(hz, 150);
}


static inline void rfmSetCarrierFrequency(uint32_t f)
{
  uint16_t fb, fc;
  byte hbsel;
/*			52 bytes
  if (f < 480000000) {
    hbsel = 0;
    fb = f / 10000000 - 24;
    fc = (f - (fb + 24) * 10000000) * 4 / 625;
  } else {
    hbsel = 1;
    fb = f / 20000000 - 24;
    fc = (f - (fb + 24) * 20000000) * 2 / 625;
  }
*/
  long mul;
  byte n;
  if (f < 480000000) {
    hbsel = 0;
    mul= 10000000;
    n=4;
  } else {
    hbsel = 1;
    mul = 20000000;
    n=2;
  }
  fb = f / mul - 24;
  fc = (f - (fb + 24) * mul) * n / 625;

  spiWriteRegister(0x75, 0x40 | (hbsel?0x20:0) | (fb & 0x1f));
  spiWriteRegister(0x76, (fc >> 8));
  spiWriteRegister(0x77, (fc & 0xff));
}

void initRFM(void)
{
    byte ItStatus1;

    ItStatus1 = spiReadRegister(0x00);   // device type
DBG_PRINTVARLN(ItStatus1);
    ItStatus1 = spiReadRegister(0x01);   // device version
DBG_PRINTVARLN(ItStatus1);
    ItStatus1 = spiReadRegister(0x02);   // device state
DBG_PRINTVARLN(ItStatus1);

    ItStatus1 = spiReadRegister(0x1b); //batt voltage
DBG_PRINTVARLN(ItStatus1);

    ItStatus1 = spiReadRegister(0x03);   // read status, clear interrupt
DBG_PRINTVARLN(ItStatus1);
    ItStatus1 = spiReadRegister(0x04);
DBG_PRINTVARLN(ItStatus1);


#if 1  // 114 bytes of flash

    static const byte PROGMEM pairs[] = {
// Interrupt Enable 1 enfferr entxffafull entxffaem enrxffafull enext enpksent enpkvalid encrcerror 00h
// Interrupt Enable 2 enswdet enpreaval enpreainval enrssi enwut enlbd enchiprdy enpor 03h
	 0x05, 0x00 ,
	 0x06, 0x40 ,			// interrupt only on preamble detect
	 0x07, RF22B_PWRSTATE_READY,	     // disable lbd, wakeup timer, use internal 32768,xton = 1; in ready mode
	 0x0a, 0x05 ,			//  Microcontroller Output Clock  Reserved Reserved clkt[1] clkt[0] enlfc mclk[2] mclk[1] mclk[0] 06h
	 0x0b, 0x12 ,	// gpio0 TX State
	 0x0c, 0x15 ,	// gpio1 RX State
	 0x0d, 0xfd ,	// gpio 2 micro-controller clk output
	 0x0e, 0x00 ,	// gpio    0, 1,2 NO OTHER FUNCTION.
	 0x30, 0x00 ,	// disable packet handling
	 0x79, 0 ,	// start channel
	 0x7a, 0x05 ,   // 50khz step size (10khz x value) // no hopping
	 0x70, 0x24 ,	// disable manchester
#ifdef HARD_VOICE
	 0x0b, 0x12 ,	// gpio0 TX State
	 0x0c, 0x15 ,	// gpio1 RX State
	 0x0d, 0x10 ,	// gpio 2 data input
         0x71, 0b00000010, // trclk=[00] no clock, dtmod=[00] direct using GPIO pin, fd8=0 eninv=0 modtyp=[10] FSK  - 
#else
	 0x0b, 0x12 ,	// gpio0 TX State
	 0x0c, 0x15 ,	// gpio1 RX State
	 0x0d, 0xfd ,	// gpio 2 micro-controller clk output
	 0x71, 0x12 ,	   // trclk=[00] no clock, dtmod=[01] direct using SPI SDI, fd8=0 eninv=0 modtyp=[10] FSK  - 
#endif
// fd[7] fd[6] fd[5] fd[4] fd[3] fd[2] fd[1] fd[0] 
	 0x72, 0x08 ,	// fd (frequency deviation) 8*625Hz == 5.0kHz

	 0x73, 0x00 ,
	 0x74, 0x00 ,   // no frequency offset
	 0x35, 0x7a ,	// preath = 15 (60bits), rssioff = 2

// AFC Loop Gearshift Override afcbd enafc afcgearh[2] afcgearh[1] afcgearh[0] 1p5 bypass matap ph0size 40h
	 0x1d, 0x40 ,

// AFC Limiter Afclim[7] Afclim[6] Afclim[5] Afclim[4] Afclim[3] Afclim[2] Afclim[1] Afclim[0] 00h
	 0x2a, 0x14 ,
	 // 		 sync words
	 0x36, 0x55 ,
	 0x37, 0x55 ,
	 0x38, 0x55 ,
	 0x39, 0x55 ,
	 0
    };

    for(const byte *pp=pairs;;){
	byte addr=pgm_read_byte((uint16_t)pp++);
	if(addr==0) break;
	spiWriteRegister(addr, pgm_read_byte((uint16_t)pp++));
    }

    spiWriteRegister(0x09, (byte)p.FrequencyCorrection);  // (default) c = 12.5p

#else
  spiWriteRegister(0x05, 0x00);
  spiWriteRegister(0x06, 0x40);    

  spiWriteRegister(0x07, RF22B_PWRSTATE_READY); 
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
#endif

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
}

void delay_1(){
    delay(1);
}

void delay_10(){
    delay(10);
}

void delay_50(){
    delay(50);
    // delay_10(); delay_10(); delay_10(); delay_10(); delay_10(); better on 2 bytes only
}

void delay_100(){
    delay_50();
    delay_50();
}

void delay_300(){
    delay_100();   delay_100();   delay_100();
}


void RFM_SetPower(uint8_t mode, uint8_t power) {
/* это нельзя делать тут ибо надо периодически переключаться на прием
#if USE_MORZE
    morze.flush(); // дождаться окончания передачи
#endif
*/
    if(!lflags.rfm_on) {
	initRFM();
        lflags.rfm_on=true;
//DBG_PRINTLN("RFM on");
        delay_50();
    }
//DBG_PRINTVARLN(mode);
    spiWriteRegister(0x6d, power );
    spiWriteRegister(0x07, mode );
    delay_10();
}


void RFM_set_TX(){
    RFM_SetPower(RF22B_PWRSTATE_TX, powerByRSSI()); // fInit, mode, power
}

void RFM_tx_min(){
    RFM_SetPower(RF22B_PWRSTATE_TX, RFM_MIN_POWER );
}

void RFM_off(byte force)
{  // TODO: SDN mangling in the future
  if(lflags.rfm_on || force)
    spiWriteRegister(0x07, RF22B_PWRSTATE_POWERDOWN);
  lflags.rfm_on=false;
//DBG_PRINTLN("RFM off");
}

void RFM_off(){
    RFM_off(0);
}

void sendBeacon(void)
{
    RFM_SetPower(RF22B_PWRSTATE_TX, RFM_MAX_POWER);

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

  RFM_SetPower(RF22B_PWRSTATE_TX,RFM_MID_POWER);// 4 set mid power 15mW
  beacon_tone(250);

  RFM_SetPower(RF22B_PWRSTATE_TX, RFM_MIN_POWER); // 0 set min power 1mW
  beacon_tone(160);

  RFM_off();
}


