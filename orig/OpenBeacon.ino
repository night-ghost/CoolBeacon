//
// OpenLRS Beacon Project
// by Konstantin Sbitnev
// Version 0.1 (2014-01-31)
//
// Based on:
// openLRSngBeacon by Kari Hautio - kha @ AeroQuad/RCGroups/IRC(Freenode)/FPVLAB etc.
// Narcoleptic - A sleep library for Arduino * Copyright (C) 2010 Peter Knight (Cathedrow)
// OpenTinyRX by Baychi soft 2013
//
//************************************ 
// Поисковый маяк на базе приемника OrangeRx Open LRS 433MHz 9Ch Receiver
//************************************
 
// макрос для легкого описания частот из сеток PMR и LPD
#define EU_PMR_CH(x) (445993750L + 12500L * (x)) // valid for ch 1 - 8
#define US_FRS_CH(x) (462537500L + 25000L * (x)) // valid for ch 1 - 7


// Тип аппаратуры
// 0 - OrangeRx Open LRS 433MHz 9Ch Receiver
// 1 - Приемник Expert LRS
#define HARDWARE_TYPE 1

// Частота маяка
#define BEACON_FREQUENCY       435000000L
//#define BEACON_FREQUENCY       EU_PMR_CH(1)
// Корректировка частоты конкретного экземпляра. Подбирается вручную.
#define BEACON_FREQ_CORRECTION     +7000L
// Корректировка таймера. Если спешит или отстает, то ставим в ноль, DEADTIME минут на 10 и засекаем время до первой посылки. 
// Считаем разницу в процентах и выставляем корректировку ( 1000 / (процент погрешности) )
// Плюс, если спешит. Минус, если отстает.
#define TIMER_CORRECTION +54

// Первичная задержка перед включением маяка (сек).
#define BEACON_DEADTIME (24*60*60)

// Интервал между посылками в начале поискового периода (сек). По умолчанию: 10 секунд
#define BEACON_INTERVAL_START 10
// Интервал между посылками в конце поискового периода (сек). По умолчанию: 60 секунд
#define BEACON_INTERVAL_LIMIT 60
// Поисковый период в течение которого работает маяк и интервал растет от BEACON_INTERVAL_START до BEACON_INTERVAL_LIMIT (минут). По умолчанию: 4 часа
#define BEACON_SEARCH_TIME (4 * 60)
// Задержка до следующего периода активности после истечения BEACON_SEARCH_TIME (мин). По умолчанию: маяк снова включится через 24 часа после первого срабатывания.
#define BEACON_SLEEP ( 24 * 60 - BEACON_SEARCH_TIME)

// Включение функционала "маяк по вызову": помимо основного функционала, маяк будет просыпаться через равные периоды времени и слушать эфир в ожидании вызова. 
// При обнаружении вызова переходит в режим маяка с заданными параметрами. (вкл=1/выкл=0)
#define BEACON_LISTEN 1
// Частота, услышав которую маяк просыпается. (Гц) По умолчанию: стандартный Tone Burst (1750Гц)
#define BEACON_LISTEN_FREQ 1750
// Период с которым маяк слушает эфир в ожидании вызова (сек)
#define BEACON_LISTEN_INTERVAL 2
// Продолжительность прослушивания эфира (мс)
#define BEACON_LISTEN_DURATION 100

// Нужно ли слать сигналы маяка после пробуждения (вкл=1/выкл=0)
#define WAKE_BEACON 1
// Нужно ли слать координаты в формате DTMF после пробуждения (вкл=1/выкл=0). Имеет смысл только при включенной функции GPS_MON, иначе посылается "###"
#define WAKE_DTMF 1
// Нужно ли слать координаты голосом после пробуждения (вкл=1/выкл=0). Имеет смысл только при включенной функции GPS_MON, иначе посылается "ноль ноль ноль"
#define WAKE_VOICE 1
// Количество циклов посылок маяка после пробуждения. Посылки идут в таком порядке: маяк, DTMF, голос. 
#define WAKE_REPEAT 3
// Интервал посылок маяка после пробуждения (сек). Интервал используется как внутри одного цикла, так и между циклами. 
#define WAKE_INTERVAL 10
// Язык голосовых сообщений RU=русский, EN=английский // Пока только RU
#define WAKE_VOICE_LANG RU

// Включение функционала отслеживания GPS координат (вкл=1/выкл=0)
#define GPS_MON 1
// Скорость порта приемника GPS
#define GPS_BAUD_RATE 38400
// Тип сообщений GPS:
// 1 - RMC, по умолчанию
// 2 - GGA
#define GPS_NMEA_MSG 1
// Интервал отслеживания GPS координат (сек)
#define GPS_MON_INTERVAL 10
// Таймаут одной попытки прослушивания порта GPS (мсек)
#define GPS_MON_DURATION 1500
// Количество допустимых ошибок чтения GPS координат. После превышения лимита, работа с GPS прекращается для экономии батареи.
#define GPS_ERROR_THRESHOLD 100
// Формат координат GPS
// 1 ГГ ММ СС.С  // Погрешность 3 метра
// 2 ГГ ММ.МММ   // Погрешность 2 метра
// 3 ГГ.ГГГГГ    // Погрешность 1 метр
// 4 ГГ.ГГГГ     // Погрешность 10 метров
// 5 .ГГГГГ      // Погрешность 1 метр, градусы не передаются. Компромис между точностью и экономичностью. Для поиска необходимо ориентироваться в координатах своей местности.
// 6 .ГГГГ       // Погрешность 10 метров, передаются только доли градусов широты и долготы. Самый экономичный вариант. Для поиска необходимо ориентироваться в координатах своей местности.
#define GPS_COORD_FMT 5

// Включение экономии заряда на мощном "пике". Шумодавы некоторых раций могут "не слышать" такой сигнал (да=1/нет=0)
#define BEACON_HIGH_SAVEPOWER 1

// Длительность одного "пика" (мс). По умолчанию: 1 секунда (1000мс)
#define BEACON_BEEP_DURATION 1000

// Мощности "пиков".
#define RFM_MAX_POWER 7
#define RFM_MID_POWER 4
#define RFM_MIN_POWER 0


//####################
//### CODE SECTION ###
//####################
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/common.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <Arduino.h>
#include <EEPROM.h>

#define SERIAL_BAUD_RATE 38400
//#define DEBUG
//#define DEBUG_B

#ifdef DEBUG
  #define DBG_PRINTLN(x)     { Serial.println(#x); Serial.flush(); }
  #define DBG_PRINTVARLN(x)  { Serial.print(#x); Serial.print(": "); Serial.println(x); Serial.flush(); }
  #define DBG_PRINTVAR(x)    { Serial.print(#x); Serial.print(": "); Serial.print(x); Serial.print(" "); Serial.flush(); }
#else
  #define DBG_PRINTLN(x)     {}
  #define DBG_PRINTVAR(x)    {}
  #define DBG_PRINTVARLN(x)  {}
#endif
#ifdef DEBUG_B
  #define DBGB_PRINTLN(x)     { Serial.println(#x); Serial.flush(); }
  #define DBGB_PRINTVARLN(x)  { Serial.print(#x); Serial.print(": "); Serial.println(x); Serial.flush(); }
  #define DBGB_PRINTVAR(x)    { Serial.print(#x); Serial.print(": "); Serial.print(x); Serial.print(" "); Serial.flush(); }
#else
  #define DBGB_PRINTLN(x)     {}
  #define DBGB_PRINTVAR(x)    {}
  #define DBGB_PRINTVARLN(x)  {}
#endif

// Включение контроля напряжения питания (Пока не работает)
#define VCC_CHECK 0
// Период проверки напряжения питания (количество периодов BEACON_INTERVAL)
#define VCC_CHECK_INTERVAL 6
// Порог напряжения батарии, ниже которого батарея считается разряженной (мВ)
#define VCC_LOW_THRESHOLD 3250
// Интервал между посылками (сек) при разряженной батарее
#define VCC_LOW_BEACON_INTERVAL 120
// Длительность одного "пика" (мс) при разряженной батарее
#define VCC_LOW_BEACON_BEEP_LENGTH 500
// Включение "пропикивания" напряжения батареи после проверки
#define VCC_BEEP 1


#if (BEACON_INTERVAL_LIMIT == BEACON_INTERVAL_START)
  #define BEACON_INT_GROW (60L * BEACON_SEARCH_TIME)
#else
  #define BEACON_INT_GROW ((60L * BEACON_SEARCH_TIME) / (BEACON_INTERVAL_LIMIT - BEACON_INTERVAL_START))
#endif

#if (HARDWARE_TYPE == 0)
  #define Red_LED A3
  #define Green_LED 13
  #define Red_LED_ON  PORTC |= _BV(3);
  #define Red_LED_OFF  PORTC &= ~_BV(3);
  #define Green_LED_ON  PORTB |= _BV(5);
  #define Green_LED_OFF  PORTB &= ~_BV(5);

  #define  nSEL_on PORTD |= (1<<4) //D4
  #define  nSEL_off PORTD &= 0xEF //D4
  #define  SCK_on PORTC |= (1<<2) //A2
  #define  SCK_off PORTC &= 0xFB //A2
  #define  SDI_on PORTC |= (1<<1) //A1
  #define  SDI_off PORTC &= 0xFD //A1
  #define  SDO_1 (PINC & 0x01) == 0x01 //A0
  #define  SDO_0 (PINC & 0x01) == 0x00 //A0
  #define SDO_pin A0
  #define SDI_pin A1
  #define SCLK_pin A2
  #define IRQ_pin 2
  #define nSel_pin 4
  #define IRQ_interrupt 0
#elif HARDWARE_TYPE == 1

  #define Red_LED_ON spiWriteRegister(0x0e, 0x04);
  #define Red_LED_OFF spiWriteRegister(0x0e, 0x00);
  #define Green_LED_ON spiWriteRegister(0x0e, 0x04);
  #define Green_LED_OFF spiWriteRegister(0x0e, 0x00);

//  #define Red_LED_ON
//  #define Red_LED_OFF
//  #define Green_LED_ON
//  #define Green_LED_OFF

  #define  nSEL_on  PORTC |=  (1<<0) //A0 /PC0
  #define  nSEL_off PORTC &= ~(1<<0) //A0  /PC0
  
  #define  SCK_on  PORTB |=  (1<<5) //D13 /PB5
  #define  SCK_off PORTB &= ~(1<<5) //D13 /PB5
  
  #define  SDI_on PORTB  |=  (1<<3) //D11 /PB3
  #define  SDI_off PORTB &= ~(1<<3) //D11 /PB3
  
  #define  SDO_1 (PINB & 0x10) == 0x10 //D12 PB4
  #define  SDO_0 (PINB & 0x10) == 0x00 //D12 PB4

  #define SDO_pin 12
  #define SDI_pin 11
  #define SCLK_pin 13
  #define IRQ_pin 2
  #define nSel_pin A0
  #define IRQ_interrupt 0
#endif


void spiWriteBit(uint8_t b);
void spiSendCommand(uint8_t command);
void spiSendAddress(uint8_t i);
uint8_t spiReadData(void);
void spiWriteData(uint8_t i);
uint8_t spiReadRegister(uint8_t address);
void spiWriteRegister(uint8_t address, uint8_t data);

#include "dtmf_voice.h"


uint32_t beaconBeepDuration=BEACON_BEEP_DURATION;
uint32_t watchdogTime_us = 16000;
uint32_t millisCounter = 0;
bool sendGPSBeaconFlag = false;

#define NOP() __asm__ __volatile__("nop")

#define RF22B_PWRSTATE_POWERDOWN    0x00
#define RF22B_PWRSTATE_READY        0x01
#define RF22B_PACKET_SENT_INTERRUPT 0x04
#define RF22B_PWRSTATE_RX           0x05
#define RF22B_PWRSTATE_TX           0x09

#define RF22B_Rx_packet_received_interrupt   0x02

uint8_t ItStatus1, ItStatus2;

// **** SPI bit banging functions

void spiWriteBit(uint8_t b)
{
  if (b) {
    SCK_off;
    NOP();
    SDI_on;
    NOP();
    SCK_on;
    NOP();
  } else {
    SCK_off;
    NOP();
    SDI_off;
    NOP();
    SCK_on;
    NOP();
  }
}

uint8_t spiReadBit(void)
{
  uint8_t r = 0;
  SCK_on;
  NOP();

  if (SDO_1) {
    r = 1;
  }

  SCK_off;
  NOP();
  return r;
}

void spiSendCommand(uint8_t command)
{
  nSEL_on;
  SCK_off;
  nSEL_off;

  for (uint8_t n = 0; n < 8 ; n++) {
    spiWriteBit(command & 0x80);
    command = command << 1;
  }

  SCK_off;
}

void spiSendAddress(uint8_t i)
{
  spiSendCommand(i & 0x7f);
}

void spiWriteData(uint8_t i)
{
  for (uint8_t n = 0; n < 8; n++) {
    spiWriteBit(i & 0x80);
    i = i << 1;
  }

  SCK_off;
}

uint8_t spiReadData(void)
{
  uint8_t Result = 0;
  SCK_off;

  for (uint8_t i = 0; i < 8; i++) {   //read fifo data byte
    Result = (Result << 1) + spiReadBit();
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

void rfmSetCarrierFrequency(uint32_t f)
{
  uint16_t fb, fc, hbsel;
  if (f < 480000000) {
    hbsel = 0;
    fb = f / 10000000 - 24;
    fc = (f - (fb + 24) * 10000000) * 4 / 625;
  } else {
    hbsel = 1;
    fb = f / 20000000 - 24;
    fc = (f - (fb + 24) * 20000000) * 2 / 625;
  }
  spiWriteRegister(0x75, 0x40 + (hbsel?0x20:0) + (fb & 0x1f));
  spiWriteRegister(0x76, (fc >> 8));
  spiWriteRegister(0x77, (fc & 0xff));
}

void beacon_tone(int16_t hz, int16_t len)
{
  int16_t d = 500 / hz; // somewhat limited resolution ;)

  if (d < 1) {
    d = 1;
  }

  uint32_t cycles = millis() + len;

  while(millis()<=cycles) {
    SDI_on;
    delay(d);
    SDI_off;
    delay(d);
  }
}

#if(WAKE_DTMF == 1 || WAKE_VOICE == 1)
  ISR(TIMER1_COMPA_vect)
  {
    SDI_off;
  }

  ISR(TIMER1_OVF_vect)
  { 
    SDI_on;
    #if( WAKE_DTMF == 1)
      if(DTMF_enable) {
        i_CurSinValA += dtmfFa;
        i_CurSinValB += dtmfFb;
        // normalize Temp-Pointer
        i_TmpSinValA  =  (char)(((i_CurSinValA+4) >> 3)&(0x007F));
        i_TmpSinValB  =  (char)(((i_CurSinValB+4) >> 3)&(0x007F));
        // calculate PWM value: high frequency value + 3/4 low frequency value
        OCR1A = (pgm_read_byte(&auc_SinParam[i_TmpSinValA]) + (pgm_read_byte(&auc_SinParam[i_TmpSinValB])-(pgm_read_byte(&auc_SinParam[i_TmpSinValB])>>2))) << 1;
      };
    #endif
  }
#endif

SIGNAL(WDT_vect) {
  wdt_disable();
  wdt_reset();
#ifdef WDTCSR
  WDTCSR &= ~_BV(WDIE);
#else
  WDTCR &= ~_BV(WDIE);
#endif
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
  uint8_t EECRcopy = EECR; EECR &= ~_BV(EERIE);
#endif
#ifdef EIMSK
  uint8_t EIMSKcopy = EIMSK; EIMSK = 0;
#endif
#ifdef PCMSK0
  uint8_t PCMSK0copy = PCMSK0; PCMSK0 = 0;
#endif
#ifdef PCMSK1
  uint8_t PCMSK1copy = PCMSK1; PCMSK1 = 0;
#endif
#ifdef PCMSK2
  uint8_t PCMSK2copy = PCMSK2; PCMSK2 = 0;
#endif
#ifdef TIMSK0
  uint8_t TIMSK0copy = TIMSK0; TIMSK0 = 0;
#endif
#ifdef TIMSK1
  uint8_t TIMSK1copy = TIMSK1; TIMSK1 = 0;
#endif
#ifdef TIMSK2
  uint8_t TIMSK2copy = TIMSK2; TIMSK2 = 0;
#endif
#ifdef SPCR
  uint8_t SPCRcopy = SPCR; SPCR &= ~_BV(SPIE);
#endif
#ifdef UCSR0B
  uint8_t UCSR0Bcopy = UCSR0B; UCSR0B &= ~(_BV(RXCIE0) | _BV(TXCIE0) | _BV(UDRIE0));
#endif
#ifdef TWCR
  uint8_t TWCRcopy = TWCR; TWCR &= ~_BV(TWIE);
#endif
#ifdef ACSR
  uint8_t ACSRcopy = ACSR; ACSR &= ~_BV(ACIE);
#endif
#ifdef ADCSRA
  uint8_t ADCSRAcopy = ADCSRA; ADCSRA &= ~_BV(ADIE);
#endif
#ifdef SPMCSR
  uint8_t SPMCSRcopy = SPMCSR; SPMCSR &= ~_BV(SPMIE);
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

void narcoleptic_calibrate() {
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
  narcoleptic_sleep(WDTO_15MS,SLEEP_MODE_IDLE);
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

void deepSleep(int milliseconds) {

  millisCounter += milliseconds;
  uint32_t microseconds = milliseconds * 1000L;
  
  narcoleptic_calibrate();
  microseconds -= watchdogTime_us;
  
  if (microseconds > 0) {
    uint16_t sleep_periods = microseconds / watchdogTime_us;
    while (sleep_periods >= 512) {
      narcoleptic_sleep(WDTO_8S,SLEEP_MODE_PWR_DOWN);
      sleep_periods -= 512;
    }
    if (sleep_periods & 256) narcoleptic_sleep(WDTO_4S,SLEEP_MODE_PWR_DOWN);
    if (sleep_periods & 128) narcoleptic_sleep(WDTO_2S,SLEEP_MODE_PWR_DOWN);
    if (sleep_periods & 64) narcoleptic_sleep(WDTO_1S,SLEEP_MODE_PWR_DOWN);
    if (sleep_periods & 32) narcoleptic_sleep(WDTO_500MS,SLEEP_MODE_PWR_DOWN);
    if (sleep_periods & 16) narcoleptic_sleep(WDTO_250MS,SLEEP_MODE_PWR_DOWN);
    if (sleep_periods & 8) narcoleptic_sleep(WDTO_120MS,SLEEP_MODE_PWR_DOWN);
    if (sleep_periods & 4) narcoleptic_sleep(WDTO_60MS,SLEEP_MODE_PWR_DOWN);
    if (sleep_periods & 2) narcoleptic_sleep(WDTO_30MS,SLEEP_MODE_PWR_DOWN);
    if (sleep_periods & 1) narcoleptic_sleep(WDTO_15MS,SLEEP_MODE_PWR_DOWN);
  }
}

void initRFM(void)
{
  ItStatus1 = spiReadRegister(0x03);   // read status, clear interrupt
  ItStatus2 = spiReadRegister(0x04);
  spiWriteRegister(0x05, 0x00);
  spiWriteRegister(0x06, 0x40);    // interrupt only on preamble detect
  spiWriteRegister(0x07, RF22B_PWRSTATE_READY);      // disable lbd, wakeup timer, use internal 32768,xton = 1; in ready mode
  spiWriteRegister(0x09, 0x7f);  // (default) c = 12.5p
  spiWriteRegister(0x0a, 0x05);
  spiWriteRegister(0x0b, 0x12);    // gpio0 TX State
  spiWriteRegister(0x0c, 0x15);    // gpio1 RX State
  #if HARDWARE_TYPE == 0
    spiWriteRegister(0x0d, 0xfd);    // gpio 2 micro-controller clk output
  #elif HARDWARE_TYPE == 1
    spiWriteRegister(0x0d, 0x0a);    // gpio2 - диод на Expert RX
  #endif
  spiWriteRegister(0x0e, 0x00);    // gpio    0, 1,2 NO OTHER FUNCTION.

  spiWriteRegister(0x30, 0x00);    //disable packet handling

  spiWriteRegister(0x79, 0);    // start channel

  spiWriteRegister(0x7a, 0x05);   // 50khz step size (10khz x value) // no hopping

  spiWriteRegister(0x70, 0x24);    // disable manchester

  spiWriteRegister(0x71, 0x12);   // trclk=[00] no clock, dtmod=[01] direct using SPI, fd8=0 eninv=0 modtyp=[10] FSK
  spiWriteRegister(0x72, 0x08);   // fd (frequency deviation) 8*625Hz == 5.0kHz
//  spiWriteRegister(0x72, 0x02);   // fd (frequency deviation) 2*625Hz == 1.25kHz 

  spiWriteRegister(0x73, 0x00);
  spiWriteRegister(0x74, 0x00);    // no offset

//  spiWriteRegister(0x35, 0x2a);    // preath = 5 (20bits), rssioff = 2
  spiWriteRegister(0x35, 0x7a);    // preath = 15 (60bits), rssioff = 2

  spiWriteRegister(0x1c, 0xc1);    // RFM calculator
  spiWriteRegister(0x1d, 0x3C);    // RFM calculator
  spiWriteRegister(0x20, 0xd6);    // RFM calculator
  spiWriteRegister(0x21, 0x00);    // RFM calculator
  spiWriteRegister(0x22, 0x98);    // RFM calculator
  spiWriteRegister(0x23, 0xeb);    // RFM calculator
  spiWriteRegister(0x24, 0x00);    // RFM calculator
  spiWriteRegister(0x25, 0x9b);    // RFM calculator
  
  unsigned int DR = ( ( (long) BEACON_LISTEN_FREQ ) << 16 ) / 15625;
  
  spiWriteRegister(0x6e, ( DR & 0xFF ));
  spiWriteRegister(0x6f, ( DR >> 8 ));

  rfmSetCarrierFrequency(BEACON_FREQUENCY + BEACON_FREQ_CORRECTION);
}

void powerOnRFM(void)
{
  initRFM();
  // TODO: SDN mangling in the future
}

void powerOffRFM(void)
{
  // TODO: SDN mangling in the future
  spiWriteRegister(0x07, RF22B_PWRSTATE_POWERDOWN);
}

void sendBeacon(void)
{
  Red_LED_ON
  powerOnRFM();
  
  spiWriteRegister(0x6d, RFM_MAX_POWER);

  Red_LED_OFF
  delay(10);
  Red_LED_ON
  spiWriteRegister(0x07, RF22B_PWRSTATE_TX);    // to tx mode
  delay(10);
  Red_LED_OFF

  #if(BEACON_HIGH_SAVEPOWER == 1) 
    for(unsigned int i=0;i<30*beaconBeepDuration/1000;i++) {
      spiWriteRegister(0x6d, RFM_MAX_POWER);
      beacon_tone(500, 18);
      spiWriteRegister(0x6d, 0x0);
      delay(15);
    }
  #else
      spiWriteRegister(0x6d, RFM_MAX_POWER);
      beacon_tone(500, beaconBeepDuration);
  #endif
  
  Red_LED_ON
  spiWriteRegister(0x6d, RFM_MID_POWER);   // 4 set mid power 15mW
  delay(10);
  Red_LED_OFF
  beacon_tone(250, beaconBeepDuration);

  Red_LED_ON
  spiWriteRegister(0x6d, RFM_MIN_POWER);   // 0 set min power 1mW
  delay(10);
  Red_LED_OFF
  beacon_tone(160, beaconBeepDuration);

  Red_LED_ON
  powerOffRFM();
  Red_LED_OFF
}

void beaconHorn(void) {
  powerOnRFM();
  spiWriteRegister(0x6d, RFM_MAX_POWER);   // 7 set max power 100mW
  delay(10);
  spiWriteRegister(0x07, RF22B_PWRSTATE_TX);    // to tx mode
  delay(10);
  for(int i=0;i<3;i++) {
    Red_LED_ON
    beacon_tone(200, 150);
    Red_LED_OFF
    beacon_tone(500, 150);
  }
  powerOffRFM();
}

#if (VCC_CHECK==1)
void beaconNumber(int hz, int num) {
  powerOnRFM();
  spiWriteRegister(0x6d, RFM_MAX_POWER);   // 7 set max power 100mW
  delay(10);
  spiWriteRegister(0x07, RF22B_PWRSTATE_TX);    // to tx mode  
  for(int i=0;i<num;i++) {
    delay(300);
    beacon_tone(hz, 300);
  }
  powerOffRFM();
}
#endif

uint32_t uptime;
uint32_t nextListenTime;
uint32_t nextSearchTime;
uint32_t nextNmeaTime;
uint32_t searchBeginTime;
uint32_t searchStopTime;
uint32_t beaconInterval;
uint32_t timeToGrow;
uint32_t nextBeepTime;
char nmeaString[128]; // Enough length?
char sLAT[10];
char sLON[10];
volatile bool preambleDetected;

#if (WAKE_VOICE == 1)
void sendVOICE(char *string)
{
  power_timer1_enable();
  powerOnRFM();
  spiWriteRegister(0x6d, RFM_MAX_POWER);   // 7 set max power 100mW
  delay(10);
  spiWriteRegister(0x07, RF22B_PWRSTATE_TX);    // to tx mode
  delay(500);
  _sendVOICE(string);
  powerOffRFM();
  power_timer1_disable();
}
#endif

#if (WAKE_DTMF == 1)
void sendDTMF(char *s, int digitLength, int interDigitPause)
{
  power_timer1_enable();
  powerOnRFM();
  spiWriteRegister(0x6d, RFM_MAX_POWER);   // 7 set max power 100mW
  delay(10);
  spiWriteRegister(0x07, RF22B_PWRSTATE_TX);    // to tx mode
  delay(500);
  _sendDTMF(s, digitLength, interDigitPause);
  powerOffRFM();
  power_timer1_disable();
}
#endif

void RFM22B_Int()
{
  char reg=spiReadRegister(0x03);
  reg=spiReadRegister(0x04);

  if(reg & 64) {
   preambleDetected = true;
  };
}

void setup(void) {
  
  //RF module pins
  pinMode(SDO_pin, INPUT);   //SDO
  pinMode(SDI_pin, OUTPUT);   //SDI
  pinMode(SCLK_pin, OUTPUT);   //SCLK
  pinMode(IRQ_pin, INPUT);   //IRQ
  pinMode(nSel_pin, OUTPUT);   //nSEL

  //LED and other interfaces
  #ifdef Red_LED
    pinMode(Red_LED, OUTPUT);   //RED LED
  #endif
  #ifdef Green_LED
    pinMode(Green_LED, OUTPUT);   //GREEN LED
  #endif
  
  attachInterrupt(IRQ_interrupt, RFM22B_Int, FALLING);
  sei();

  #if defined(DEBUG) || defined(DEBUG_B)
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println("OpenBeacon");
  #endif  
  
  #if(VCC_CHECK == 1)
    long vcc=readVcc();
    if(vcc < VCC_LOW_THRESHOLD) {
      lowBattery=1;
      beaconInterval=VCC_LOW_BEACON_INTERVAL;
      beaconBeepLength=VCC_LOW_BEACON_BEEP_LENGTH;
    }
    #ifdef DEBUG
      Serial.println(vcc);
    #endif
  #endif

  uptime = 1;
  nextSearchTime = uptime + BEACON_DEADTIME;
  nextListenTime = uptime + BEACON_LISTEN_INTERVAL;
  nextNmeaTime = uptime + GPS_MON_INTERVAL;
  searchBeginTime = 0;

  initRFM();

  beaconHorn();

  ADCSRA &= ~(1<<ADEN); //Disable ADC
  ACSR = (1<<ACD); //Disable the analog comparator
//  DIDR0 = 0x3F; //Disable digital input buffers on all ADC0-ADC5 pins
  DIDR1 = (1<<AIN1D)|(1<<AIN0D); //Disable digital input buffer on AIN1/0

  #if (!defined(DEBUG)) &&  (!defined(DEBUG_B))
    power_usart0_disable();
  #endif
  power_twi_disable();
  power_spi_disable();
  power_timer1_disable();
  power_timer2_disable();
}

#if (GPS_MON == 1)
  char COORDS[25];
  char LON[12];
  int idx, i, commaCNT;
  char GPScha [128];
  char tLAT[12];
  char tLON[12];
  #if GPS_NMEA_MSG == 1
    char GPSchk [8] = "$G*RMC,";  
    #define FIXPOS 2
    #define FIXCHAR 'A'
    #define LATPOS 3
    #define LONPOS 5
  #elif GPS_NMEA_MSG == 2
    char GPSchk [8] = "$G*GGA,";  
    #define FIXPOS 6
    #define FIXCHAR '1'
    #define LATPOS 2
    #define LONPOS 4
  #endif
  double fLAT, fLON; 
  int tmpI;
  char tmpS[10];
  unsigned int GpsErrorCount=0;
#endif

#if (BEACON_LISTEN == 1)
  void sendGPSBeacon() 
  {
    #if (WAKE_BEACON == 1)
      sendBeacon();
      deepSleep(WAKE_INTERVAL*1000);
    #endif

    #if (GPS_MON == 1)
      #if (WAKE_DTMF == 1)
        if(strlen(COORDS)>0) {
          sendDTMF(COORDS,100,100);
          deepSleep(WAKE_INTERVAL*1000);
        } else {
          sendDTMF("###", 100, 100);
          deepSleep(WAKE_INTERVAL*1000);
        }
      #endif

      #if (WAKE_VOICE == 1)
        if(strlen(COORDS)>0) {
          sendVOICE(COORDS);
          deepSleep(WAKE_INTERVAL*1000);
        } else {
          sendVOICE("000");
          deepSleep(WAKE_INTERVAL*1000);
        }
      #endif
    #endif
  }
#endif


void loop(void)
{
  unsigned long timeStart=(millis() + millisCounter);

  Green_LED_ON
  delay(30);
  Green_LED_OFF

  DBGB_PRINTVAR(nextSearchTime);
  DBGB_PRINTVAR(nextListenTime);
  DBGB_PRINTVAR(beaconInterval);
  DBGB_PRINTVAR(searchBeginTime);
  DBGB_PRINTVAR(searchStopTime);
  DBGB_PRINTVAR(nextNmeaTime);
  DBGB_PRINTVARLN(uptime);

  if( ( searchBeginTime != 0 ) || ( uptime >= nextSearchTime ) ) {
    // Time to beep and count intervals
    if( searchBeginTime == 0 ) {
      // Начало поискового периода
      DBGB_PRINTLN(Search time begin);
      searchBeginTime = uptime;
      nextBeepTime = uptime;
      beaconInterval = BEACON_INTERVAL_START;
      timeToGrow = uptime + BEACON_INT_GROW;
      searchStopTime  = uptime + BEACON_SEARCH_TIME * 60;
      nextSearchTime = searchStopTime + BEACON_SLEEP * 60;
    };
    
    if( uptime >= nextBeepTime ) {
      // Время очередной посылки
      if(!sendGPSBeaconFlag) {
        // Обычный маяк
        DBGB_PRINTLN(BEACON_B);
        sendBeacon();
        DBGB_PRINTLN(BEACON_E);
      } else {
        // На предыдущем цикле был обнаружен вызов, посылаем GPS, вместо маяка
        #if (BEACON_LISTEN == 1)
          sendGPSBeacon();
        #endif
        sendGPSBeaconFlag = false;
      }
      nextBeepTime = uptime + BEACON_BEEP_DURATION * 3 / 1000 + beaconInterval;
      #if ( (BEACON_LISTEN == 1) && (GPS_MON == 1) )
      // В конце посылки послушаем эфир
        preambleDetected=false;
        powerOnRFM();
        spiWriteRegister(0x07, RF22B_PWRSTATE_RX);
        delay( BEACON_LISTEN_DURATION );
        powerOffRFM();
        if(preambleDetected) {
          // Есть вызов
          DBGB_PRINTLN(Call is detected)
          // Следующая послыка будет с координатами
          sendGPSBeaconFlag = true;
        }
      #endif
    }
    
    if( uptime >= timeToGrow ) {
      // Увеличение интервала между посылками
      DBGB_PRINTLN(Interval grow);
      timeToGrow = uptime - (timeToGrow - uptime) + BEACON_INT_GROW;
      beaconInterval ++;
    }
    
    if( uptime >= searchStopTime ) {
      // Окончание периода работы маяка
      DBGB_PRINTLN(Search time end);
      searchBeginTime = 0;
    }
  } else {
    #if( GPS_MON )
      if( (uptime >= nextNmeaTime) && (GpsErrorCount < GPS_ERROR_THRESHOLD) ) {
        // Time to read GPS position
        DBG_PRINTLN(Read GPS)

        #if (!defined(DEBUG)) &&  (!defined(DEBUG_B))
          power_usart0_enable();
          Serial.begin(GPS_BAUD_RATE);
        #endif
        Serial.flush();
        
        long startTime=millis();
        idx = 0;
        GPScha[0]=0;
        while( millis() - startTime < GPS_MON_DURATION ) {
          if(Serial.available() > 0) {
            GPScha[idx] = Serial.read();
            if( (idx > 7) && (GPScha[idx] == '\r') ) break;
            if( (idx >= 7) || (idx == 2) || (GPScha[idx] == GPSchk[idx]) ) {
              idx++;
            } else {
              idx = 0;
            }
//            delay(GPS_MON_DURATION / 10);
          }
        }
        if(idx>0) 
          GPScha[idx+1]=0;
        else
          GPScha[0]=0;
        
        #if (!defined(DEBUG)) &&  (!defined(DEBUG_B))
          Serial.end();
          power_usart0_disable();
        #endif
        
/*
$GPRMC,154655,A,4428.1874,N,00440.5185,W,000.7,000.0,050407,,,A*6C
$GPGGA,182215.700,5510.6283,N,06121.3981,E,1,7,3.48,250.8,M,-11.4,M,,*71
*/
//        #ifdef DEBUG
//        strcpy(GPScha,"$GPRMC,154655,A,4428.1874,N,00404.0185,W,000.7,000.0,050407,,,A*6C\r");
//        idx=66;
//        #endif
        
        DBG_PRINTVAR(GPScha); DBG_PRINTLN();

        if( (GPScha[0] == '$') && (GPScha[idx] == '\r') ) {
          DBG_PRINTLN(Success reading GPS string); DBG_PRINTVAR(GPScha); DBG_PRINTLN();
          idx=0;
          commaCNT=0;
          
          while( GPScha[idx]!='\r' ) {
            if(GPScha[idx] == ',') {
              commaCNT++;
            } else {
              idx++;
              continue;
            }
            idx++;
            i=0;
            if(commaCNT == FIXPOS)
              if(GPScha[idx] != FIXCHAR) {
                tLAT[0]=0;
                tLON[0]=0;
                break;  // Invalid fix
              }
            if(commaCNT == LATPOS) {
              while( (GPScha[idx]!=',') && (i<10) ) tLAT[i++] = GPScha[idx++];
              tLAT[i]=0;
            }
            if(commaCNT == LONPOS) {
              while( (GPScha[idx]!=',') && (i<10) ) tLON[i++] = GPScha[idx++];
              tLON[i]=0;
            }
          }

          if( (strlen(tLAT)==0) && (strlen(tLON) == 0) ) {
            DBG_PRINTLN(No FIX);
          } else {
            // LAT
            strncpy(tmpS,tLAT+2,10);
            fLAT=atof(tmpS)/60;
            strncpy(tmpS,tLAT,2); tmpS[2]=0;
            fLAT+=atoi(tmpS);
            // LON
            strncpy(tmpS,tLON+3,10);
            fLON=atof(tmpS)/60;
            strncpy(tmpS,tLON,3);  tmpS[3]=0;
            fLON+=atoi(tmpS);

            #if GPS_COORD_FMT == 1
              // 1 ГГ ММ СС.С
              int latDD = (int)fLAT;
              int latMM = (int)((fLAT - latDD) * 60);
              double latSS = ((fLAT - latDD) * 60 - latMM) * 60;
              int lonDD = (int)fLON;
              int lonMM = (int)((fLON - lonDD) * 60);
              double lonSS = ((fLON - lonDD) * 60 - lonMM) * 60;
              snprintf(COORDS, 22, "%02i*%02i*%02i*%1i#%02i*%02i*%02i*%1i",
                latDD, latMM, (int)latSS, (int)((latSS - (unsigned long)latSS)*10),
                lonDD, lonMM, (int)lonSS, (int)((lonSS - (unsigned long)lonSS)*10));
            #elif GPS_COORD_FMT == 2
              // 2 ГГ ММ.МММ
              int latDD = (int)fLAT;
              double latMM = ((fLAT - latDD) * 60);
              int lonDD = (int)fLON;
              double lonMM = ((fLON - lonDD) * 60);
              snprintf(COORDS, 20, "%02i*%02i*%03i#%02i*%02i*%03i",
                latDD, (int)latMM, (int)((latMM - (unsigned long)latMM)*1000),
                lonDD, (int)lonMM, (int)((lonMM - (unsigned long)lonMM)*1000));
            #elif GPS_COORD_FMT == 3
              // 3 ГГ.ГГГГГ
              snprintf(COORDS, 20, "%02i*%05lu#%02i*%05lu", (int)fLAT, (unsigned long)((fLAT - (unsigned long)fLAT)*100000L),
                                                            (int)fLON, (unsigned long)((fLON - (unsigned long)fLON)*100000L));
            #elif GPS_COORD_FMT == 4
              // 4 ГГ.ГГГГ 
              snprintf(COORDS, 20, "%02i*%04lu#%01i*%04lu", (int)fLAT, (unsigned long)((fLAT - (unsigned long)fLAT)*10000L),
                                                            (int)fLON, (unsigned long)((fLON - (unsigned long)fLON)*10000L));
            #elif GPS_COORD_FMT == 5
              // 5 .ГГГГГ
              snprintf(COORDS, 20, "%05lu#%05lu", (unsigned long)((fLAT - (unsigned long)fLAT)*100000L),
                                                  (unsigned long)((fLON - (unsigned long)fLON)*100000L));
            #elif GPS_COORD_FMT == 6
              // 6 .ГГГГ
              snprintf(COORDS, 20, "%04lu#%04lu", (unsigned long)((fLAT - (unsigned long)fLAT)*10000L),
                                                  (unsigned long)((fLON - (unsigned long)fLON)*10000L));
            #endif

            DBG_PRINTVARLN(COORDS);
          }
          GpsErrorCount=0; // TODO: really needed?
        } else {
          GpsErrorCount++;
        }
        nextNmeaTime = uptime + GPS_MON_INTERVAL + GPS_MON_DURATION / 1000;
      }
    #endif
    #if(BEACON_LISTEN == 1)
      if( uptime >= nextListenTime ) {
        // Time to listen for a call
        DBGB_PRINTLN(Listen for tone burst)
        Red_LED_ON
        delay(30);
        Red_LED_OFF

        preambleDetected=false;
        powerOnRFM();
        spiWriteRegister(0x07, RF22B_PWRSTATE_RX);
        delay( BEACON_LISTEN_DURATION );
        powerOffRFM();
        if(preambleDetected) {
          // Call is detected
          DBGB_PRINTLN(Call is detected)
          
          for(int i=0;i<5;i++) {
            Red_LED_ON; Green_LED_ON;
            delay(30);
            Red_LED_OFF; Green_LED_OFF;
            delay(30);
          }
          
          deepSleep(BEACON_LISTEN_INTERVAL*1000);
          
          for(int i=0;i<WAKE_REPEAT;i++) {
            sendGPSBeacon();
          }
        }
        nextListenTime = uptime + BEACON_LISTEN_INTERVAL + BEACON_LISTEN_DURATION / 1000;
        
        Red_LED_ON
        delay(30);
        Red_LED_OFF

      }
    #endif

    #if(VCC_CHECK == 1)
    /*
    if( uptime >= nextVccCheckTime) {
      // Time to check our feed
      // REWORK!!!
        if(!lowBattery) {
          if(powerCheckDelay == 0) {
            long vcc=readVcc();
            #if(VCC_BEEP==1)
              beaconNumber(150, int(vcc/1000));
              delay(300);
              beaconNumber(200, int((vcc%1000)/100));
              delay(300);
            #endif
            if(vcc < VCC_LOW_THRESHOLD) {
              lowBattery=1;
              beaconInterval=VCC_LOW_BEACON_INTERVAL;
              beaconBeepLength=VCC_LOW_BEACON_BEEP_LENGTH;
            }
            powerCheckDelay=VCC_CHECK_INTERVAL;
          };
          powerCheckDelay--;
        }
      }
    */
    #endif
  }
  
// Sleep till the end of a second
  unsigned  long timeNow=(millis() + millisCounter);
  deepSleep(1000L - ( timeNow -  timeStart ) % 1000 + TIMER_CORRECTION);
  if(timeNow -  timeStart < 1000) {
    uptime++;
  } else {
    uptime += 1 + ( timeNow -  timeStart ) / 1000;
  }
}


