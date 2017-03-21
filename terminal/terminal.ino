/*
   simple terminal in CoolBeacon hardware to debug SIM800-related code
  

*/


#include <SingleSerial.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/common.h>
#include <avr/eeprom.h>
#include <inttypes.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <Arduino.h>

#include "compat.h" //   some missing definitions
#include <AltSoftSerial.h>

#include "bufstream.h"


#include "config.h"
#include "config-phones.h"

#include "vars.h"

#define voiceOnBuzzer false


uint32_t millisCounter;
byte ItStatus1;

byte powerByRSSI();


#include "rfm22b.h"

byte powerByRSSI() {
    return RFM_MAX_POWER;
}

SingleSerialPort(serial);

#include "gsm.h"
#include "gsm_core.h"
GSM gsm;

// BuffStream
BS bs;


void setup(void) {
  
//  DDRD=0x60; //0110 0000 -  The Port D Data Direction Register, D6 D5 as output -  умолчальные выводы пищалки и строба
//  PORTD=0;     все это давно не используется

    wdt_disable();
 
    //RF module pins
    pinMode(SDI_pin, OUTPUT);   //SDI
    pinMode(SCLK_pin, OUTPUT);   //SCLK
    pinMode(SDO_pin, INPUT);   //SDO
    pinMode(IRQ_pin, INPUT);   //IRQ
    pinMode(nSel_pin, OUTPUT);   //nSEL

    gsm.initGSM();
    
    watchdogTime_us = 16000;

  //LED and other interfaces
 #ifdef Red_LED
    pinMode(Red_LED, OUTPUT);   //RED LED
  #endif
  #ifdef Green_LED
    pinMode(Green_LED, OUTPUT);   //GREEN LED
  #endif


    sei();

    BUZZER_LOW;

    serial.begin(TELEMETRY_SPEED);
  
    RFM_off();


	gsm.begin();
	


    wdt_reset();


//    gsm.sendUSSD(100);

    int bal=gsm.balance();
    serial.print_P(PSTR("Balance="));
    serial.println(bal);

    serial.print_P(PSTR("sleep"));
    gsm.set_sleep(true);

    delay(20000);

    serial.print_P(PSTR("sleep wake"));
    gsm.set_sleep(false);

    bs.begin((char *)buf);
    
    char messageBuff[]="45.123,45.321";
    
//    bs.printf_P(PSTR("www.ykoctpa.ru/map?markers=%s"),messageBuff);
    bs.printf_P(PSTR("maps.google.ru/maps?q=%s"),messageBuff);

    if(gsm.sendSMS(PSTR(PHONE),(char *)buf))
	serial.print_P(PSTR("SMS sent!"));
}


void loop(void) {
    while(gsm.available()) {
	Red_LED_ON;
	serial.write(gsm.read());
	Red_LED_OFF;
    }

    if(serial.available()) {
	while(serial.available()) {
	    Green_LED_ON;
	    gsm.write(serial.read());
	    Green_LED_OFF;
	    delay(10);
	}
    }
}


