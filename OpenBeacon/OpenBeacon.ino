/*
   Ardupilot Beacon
  
   Based on:
   binary code analysys of tBeacon 0.54
   OpenLRS Beacon Project (tBeacon late)  by Konstantin Sbitnev Version 0.1
    wihch based on
   openLRSngBeacon by Kari Hautio - kha @ AeroQuad/RCGroups/IRC(Freenode)/FPVLAB etc.
   Narcoleptic - A sleep library for Arduino * Copyright (C) 2010 Peter Knight (Cathedrow)
   OpenTinyRX by Baychi soft 2013
  
  ************************************ 
   Поисковый маяк на базе приемника OrangeRx Open LRS 433MHz 9Ch Receiver
  ************************************

Отличия от tBeacon:

* GPL
* понимает только MAVlink и предназначен для непосредственного подключения к контроллеру APM/PixHawk
* пока подключен к борту не экономит энергию и получает через MAVlink не только координаты но и другую информацию
* координаты перед сохранением в EEPROM фильтруются по минимальному расстоянию - зачем писАть стояние на месте?
* умеет "разговаривать" не только по радио но и пищалкой, и все предполетные разговоры идут на нее
* голос формируется по таймеру а не задержкой - получается гораздо чище
* во всех режимах мощность радиопередачи автоматически регулируется по силе принимаемого сигнала
* радио до дизарма не включается вообще дабы не создавать помех
* при дизарме в радиусе менее 10 метров от точки взлета маяк не срабатывает (кроме вызывного)
* при зафиксированном краше таймерный маяк включается сразу же, без задержки
* умеет управлять GSM-модулем SIM800 и отправить SMS с координатами при аварийной посадке (TODO: и в воздухе при аварийном снижении)
* TODO: управление парашютной системой спасения
* TODO: при взлете запоминать координаты и не включать маяк при посадке в непосредственной близости
* TODO: передавать точки на сервер по мере движения, с прореживанием и контролем расстояния

удалено:
* нет поддержки прямого подключения к GPS
* нет автораспознавания формата - всегда MAVlink
* нет автораспознавания скорости - MAVlink всегда на 57600
* нет поддержки морзянки (хотя если кому надо то можно и сделать, причем с кодированием на борту)
* нет управления форматом координат из конфигуратора - задается при сборке
* (почти) нет управления ногой подключения пищалки из конфигуратора - задается при сборке




LICENSE

 * This code is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Arduino SdFat Library.  If not, see
 * <http://www.gnu.org/licenses/>.

*/


#define DEBUG
//#define DEBUG_B

#ifdef DEBUG
  #define DBG_PRINTLN(x)     { serial.print_P(PSTR(x)); serial.println(); /* serial.flush(); */ } 
  #define DBG_PRINTVARLN(x)  { serial.print(#x); serial.print(": "); serial.println(x);  }
  #define DBG_PRINTVAR(x)    { serial.print(#x); serial.print(": "); serial.print(x); serial.print(" ");  }
#else
  #define DBG_PRINTLN(x)     {}
  #define DBG_PRINTVAR(x)    {}
  #define DBG_PRINTVARLN(x)  {}
#endif

#ifdef DEBUG_B
  #define DBGB_PRINTLN(x)     { serial.println(#x); serial.flush(); }
  #define DBGB_PRINTVARLN(x)  { serial.print(#x); serial.print(": "); serial.println(x); serial.flush(); }
  #define DBGB_PRINTVAR(x)    { serial.print(#x); serial.print(": "); serial.print(x); serial.print(" "); serial.flush(); }
#else
  #define DBGB_PRINTLN(x)     {}
  #define DBGB_PRINTVAR(x)    {}
  #define DBGB_PRINTVARLN(x)  {}
#endif




//#include <FastSerial.h>
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
//#include <EEPROM.h>
#include "compat.h" //   some missing definitions
#include <AltSoftSerial.h>

// own includes
#include "bufstream.h"
#include "config.h"
#include "vars.h"

//#include <AP_Common.h>
// uses AP_Param
#include <GCS_MAVLink.h> 


//FastSerialPort0(serial);
SingleSerialPort_x(serial);
SingleSerial serial;

// program's parts

#include "rfm22b.h"
//#include "gsm.h"
#include "chute.h"


void beepOnBuzzer(unsigned int length);
void sayVoltage(byte v, byte beeps);

void delay_300();



#include "voice.h"

inline void initBuzzer(){
     BuzzerToneDly = (byte)p.WakeBuzzerParams;

#if BUZZER_PIN
	pinMode(BUZZER_PIN,OUTPUT);

 #if defined(BUZZER_PIN_PORT) && defined(BUZZER_PIN_BIT)
    // OK
 #else
	buzzerBit = digitalPinToBitMask(BUZZER_PIN); // для работы с пищалкой напрямую, а не через DigitalWrite
	uint8_t port = digitalPinToPort(BUZZER_PIN);
	buzzerPort = portOutputRegister(port);
 #endif

	BUZZER_LOW; //digitalWrite(BUZZER_PIN, 0);

#else
    BuzzerPin     = ((byte *)&p.WakeBuzzerParams)[1];
    if(BuzzerPin) {
	pinMode(BuzzerPin,OUTPUT);

	buzzerBit = digitalPinToBitMask(BuzzerPin); // для работы с пищалкой напрямую, а не через DigitalWrite
	uint8_t port = digitalPinToPort(BuzzerPin);
	buzzerPort = portOutputRegister(port);

	digitalWrite(BuzzerPin, 0);
    }
#endif


#if FLASH_PIN
     pinMode(FLASH_PIN, OUTPUT);
     digitalWrite(FLASH_PIN,0);
#else
     FlashPin=(byte)p.WakeFlashPin;
     pinMode(FlashPin, OUTPUT);
     digitalWrite(FlashPin,0);
#endif
}

/* inline void beaconHorn(void) { // используется только при старте
   RFM_SetPower(1,RF22B_PWRSTATE_TX, RFM_MIN_POWER );
  
    for(byte i=3;i>0;i--) {
        Red_LED_ON
        beacon_tone(200, 150);
        Red_LED_OFF
        beacon_tone(500, 150);
    }
    RFM_off();
}*/


SIGNAL(WDT_vect) {
  wdt_disable();
  wdt_reset();
#ifdef WDTCSR
  WDTCSR &= ~_BV(WDIE);
#else
  WDTCR &= ~_BV(WDIE);
#endif
}

uint16_t getExtVoltage(){
    adc_setup();

#define VEXT_AVG 64
    register unsigned long sum=0;
    for(byte i=VEXT_AVG;i>0; i--){
	sum +=analogRead(VBAT_PIN);
	delay_1();
    }


    ADCSRA &= ~(1<<ADEN); //Disable ADC

//    if(sum < 64) { // нету напряжения, совсем нету
//	return 0; // оно и само так получится
//    }

    sum *= 57;

    return  sum /VEXT_AVG / 160; // 57/160 - какая-то калибровка

}



uint16_t readVCC() {

    adc_setup();

/*  это вообще убрать надо, оставив чтение питания только "магическим" методом
//    pinMode(12, 1) ; // 12 (PB3) MOSI - зачем его на вывод - ???
//    digitalWrite(12,0);

    long sum=0;
    for(byte i=0; i<50; i++){
        sum += analogRead(VCC_PIN);
        delay_1();
    }
//    pinMode(12, 0); // PB3 на ввod
 
    sum /= 50;
    sum *= 33;
    sum /= 255;
    
    if(sum < 26) return 0;
    if(sum > 34) return sum;
*/
    ADMUX = 0x4e; //AVCC with external capacitor at AREF pin, 1.1v as meashured
    delay_1();
    delay_1();
    
    ADCSRA |= 1 << ADSC; // start conversion
    
    while (bit_is_set(ADCSRA, ADSC));
    
    byte low  = ADCL;
    byte high = ADCH;
    
    uint16_t v = (high << 8) | low;
    return 11253 / v;
    
}


byte powerByRSSI() {
    byte rssi = Got_RSSI;
    if(rssi)   		// хоть что-то принимаем
	lastRSSI = rssi;	// запоминаем последнее значение
    else		// сигнала нет
	if(lastRSSI > 50) 
	    lastRSSI -= 2; // убавляем по чуть-чуть
	else
	    lastRSSI = 0;
	
    // услышав посылку маяк убавляет силу сигнала, затем потихоньку прибавляет
    return lastRSSI < 50 ? RFM_MAX_POWER : (120-lastRSSI) / 10;
}


void sendVOICE(char *string, byte beeps)
{
  if(!voiceOnBuzzer) {
    RFM_SetPower(1, RF22B_PWRSTATE_TX, powerByRSSI()); // fInit, mode, power

    delay_100();
  }
  _sendVOICE(string, beeps);

  RFM_off();
}


void RFM22B_Int()
{
  byte reg=spiReadRegister(0x03);
  reg=spiReadRegister(0x04);

  if(reg & 64) {
    preambleDetected = true;
    preambleRSSI =spiReadRegister(0x26);
  }
}

// BuffStream
BS bs;


#if GPS_COORD_FMT == 1 
void format_one(PGM_P f, long n){
    if(n<0) {
	bs.print('-');
	n=-n;
    }

    unsigned int DD = n/10000000; // grad 
    unsigned int MM =   (n - DD*10000000) * 60 /10000000;
    unsigned long SS = ((n - DD*10000000) * 60 - MM) * 60 /10000000;
    bs.printf_P(f, DD, MM, (int)SS, (int)((SS - (unsigned long)SS)*10/10000000));
}

#elif GPS_COORD_FMT == 2
void format_one(PGM_P f, long n){
    if(n<0) {
	bs.print('-');
	n=-n;
    }

    unsigned int DD = n/10000000; //grad
    unsigned long  MM = (n - DD*10000000) * 60 /10000000;

    bs.printf_P(f, DD, (int)MM, (int)((MM - (unsigned long)MM)*1000/10000000));
}
#elif GPS_COORD_FMT == 3 || GPS_COORD_FMT == 4
void format_one(PGM_P f, long n, long d){

    if(n<0) {
	bs.print('-');
	n=-n;
    }
    unsigned int in=n/10000000;
    unsigned long tail = n - in*10000000;
    bs.printf_P(f, in, (unsigned long)(tail*d/10000000));
}
#elif GPS_COORD_FMT == 5 || GPS_COORD_FMT == 6
void format_one(PGM_P f, long n){

    if(n<0) {
	bs.print('-');
	n=-n;
    }
    unsigned int in=n/10000000;
    unsigned long tail = n - in*10000000;
    bs.printf_P(f, (unsigned long)(tail*d/10000000));
}
#endif

void formatGPS_coords(){ // печатает координаты в буфер для рассказа голосом
    lflags.hasGPSdata=true; // раз позвали печатать значит есть что сказать

    BS::begin(messageBuff);

//no use p.GPS_Format

#if GPS_COORD_FMT == 1
// 1 ГГ ММ СС.С
        format_one(PSTR( "%02i*%02i*%02i*%1i", coord.lat);
        format_one(PSTR("#%02i*%02i*%02i*%1i", coord.lon);
#elif GPS_COORD_FMT == 2
 // 2 ГГ ММ.МММ
        format_one(PSTR( "%02i*%02i*%03i", coord.lat);
        format_one(PSTR("#%02i*%02i*%03i", coord.lon);
#elif GPS_COORD_FMT == 3
      // 3 ГГ.ГГГГГ
	format_one(PSTR( "%i*%05lu"),coord.lat,100000L);
        format_one(PSTR("#%i*%05lu"),coord.lon,100000L);
#elif GPS_COORD_FMT == 4
      // 4 ГГ.ГГГГ 

	format_one(PSTR( "%i*%04lu"),coord.lat,10000L);
        format_one(PSTR("#%i*%04lu"),coord.lon,10000L);
#elif GPS_COORD_FMT == 5
      // 5 .ГГГГГ

	format_one(PSTR( "%05lu"),coord.lat,100000L);
        format_one(PSTR("#%05lu"),coord.lon,100000L);

#elif GPS_COORD_FMT == 6
      // 6 .ГГГГ
	format_one(PSTR( "%04lu"),coord.lat,10000L);
        format_one(PSTR("#%04lu"),coord.lon,10000L);
#endif
}

// чтение и запись мелких объектов
void eeprom_read_len(byte *p, uint16_t e, byte l){
    for(;l>0; l--, e++) {
	*p++ = (byte)eeprom_read_byte( (byte *)e );
//	DBG_PRINTVARLN(l);
    }
}

void eeprom_write_len(byte *p, uint16_t e, byte l){
    for(;  l>0; l--, e++)
	eeprom_write_byte( (byte *)e, *p++ );

}

bool is_eeprom_valid(){// родная реализация кривая, у нас есть CRC из MAVLINKa, заюзаем его

    register uint8_t *p, *ee;
    uint16_t i;

    eeprom_read_len((byte *)&i, EEPROM_VERS, sizeof(i));

    if(i != CURRENT_VERSION) return false;

    crc_init(&crc);

    for(i=sizeof(Params), p=(byte *)&p,  ee=(byte *)(EEPROM_PARAMS);  i>0; i--, ee++) // байта для адреса мало
	crc_accumulate(eeprom_read_byte((byte *) ee ), &crc);

    eeprom_read_len((byte *)&i, EEPROM_CRC, sizeof(crc));
    
    return i == crc;
}


void Read_EEPROM_Config(){ // TODO:

#if 0 // MAX_PARAMS * 4 < 255
    eeprom_read_len((byte *)&p, EEPROM_PARAMS, sizeof(p));
#else
    register uint8_t *pp;
    register uint16_t i, ee;

// eeprom_read_len недостаточно
    for(i=sizeof(p), pp=(byte *)&p,  ee=EEPROM_PARAMS;  i>0; i--,ee++) { // байта для адреса мало
	*pp++ = (byte)eeprom_read_byte( (byte *)ee );
//	DBG_PRINTVARLN(i);
    }
#endif
}

void write_Params_ram_to_EEPROM() { // записать зону параметров в EEPROM
// TODO:    

    register uint8_t *pp, *ee;
    register uint16_t i;
    byte v;

    crc_init(&crc);

    for(i=sizeof(p), pp=(byte *)&p,  ee=(byte *)(EEPROM_PARAMS);  i>0; i--,ee++) { // байта для адреса мало
	v = *pp++;
	eeprom_write_byte((byte *) ee, v );
	crc_accumulate( v, &crc);
    }

    eeprom_write_len((byte *)&crc, EEPROM_CRC, sizeof(crc));

    crc=CURRENT_VERSION;
    eeprom_write_len((byte *)&crc, EEPROM_VERS, sizeof(crc));

}

void printCoord(long l){ // signed long!
    if(l<0) {
	serial.print('-');
	l=-l;
    }
    unsigned int in = l / 10000000;
    serial.print(in);
    serial.print('.');
    serial.print( l - in * 10000000);
}

inline bool badCoord(long l){
    return l==BAD_COORD;
}

void uploadTrackPoints(){
// TODO:
    byte cnt=0;
    
    for(unsigned int i=EEPROM_TRACK + (eeprom_points_count +1)*sizeof(Coord) ; cnt <= MAX_TRACK_POINTS; cnt++) { // начинаем с точки после маркера конца
	eeprom_read_len((byte *)&p1,i,sizeof(Coord)); // вторая точка
	if(!badCoord(p1.lat) && !badCoord(p1.lon)) {
	    printCoord(p1.lat);
	    serial.print(' ');
	    printCoord(p1.lon);
	    serial.println();
	}
	i+=sizeof(Coord);
	if(i>=EEPROM_TRACK_SIZE)    i=EEPROM_TRACK; // кольцо
    }
} 

byte getLastTrackPoint(){// TODO:
    
    // последняя - та что не- NAN а за ней NAN
    eeprom_points_count=0; // с самого начала
    
    byte cnt=0;
    eeprom_read_len((byte *)&p1, EEPROM_TRACK, sizeof(Coord)); // первая точка
    
    for(unsigned int i=EEPROM_TRACK; cnt <= MAX_TRACK_POINTS; cnt++) { // намеренно на 1 точку больше
	i+=sizeof(Coord);
	if(i>=EEPROM_TRACK_SIZE)    i=EEPROM_TRACK; // кольцо
	
	eeprom_read_len((byte *)&p2,i,sizeof(Coord)); // вторая точка
	
	if(!badCoord(p1.lat) && badCoord(p2.lat) ) {
	    coord=p1;
	    eeprom_points_count=cnt+1;
	    break;
	}
	p1=p2; // переход к следующей точке
    }

    p1 = p2 = bad_coord; // затереть дабы отличать принятые от исторических
    
    // not found
    return eeprom_points_count;
}

//static Coord bad_coord ={BAD_COORD,BAD_COORD};

void SaveGPSPoint(){//TODO:

    eeprom_write_len((byte *)&coord, EEPROM_TRACK + eeprom_points_count*sizeof(Coord), sizeof(Coord)); // первая точка
    
    eeprom_points_count++; // переход к следующей
    if(eeprom_points_count >= MAX_TRACK_POINTS) eeprom_points_count=0; // кольцо

    eeprom_write_len((byte *)& bad_coord,   EEPROM_TRACK + eeprom_points_count*sizeof(Coord), sizeof(Coord)); // вторая точка - NAN

    lflags.pointDirty=false; // сохранено
}




void inline sendMorzeSign(); //not used

void Delay_listen(){
    delay( p.ListenDuration);
}

byte calibrate(){
     initRFM();
     
     RFM_SetPower(1, RF22B_PWRSTATE_TX, RFM_MIN_POWER );
      
     uint16_t min=10000; // min
      
     byte pos=0;
     byte deviation=0;

     for(byte deviation=0; deviation<128 ; deviation++){
         spiWriteRegister(0x09, deviation);
         delay_10();
         preambleDetected=0;
         Green_LED_ON;
         
         spiWriteRegister(0x07, RF22B_PWRSTATE_RX);// xton | rxon
         Delay_listen();

         Green_LED_OFF;
         
         if(preambleDetected){
         
//            ищем максимум и запоминаем соответствующую частоту. Родная сделана криво, надо чистить самому
//   	    диапазон 0-127
         
            int sum=0; // really signed
            for(int j=0;j<10;j++){
        	 spiWriteRegister(0x07, RF22B_PWRSTATE_RX); // xton | rxon
        	 delay_50();
        	 sum += spiReadRegister(0x2b); //  AFC Correction Read
            }
            if(sum<0) sum=-sum;
            if(sum < min){
        	min=sum;
        	pos=deviation;
            }
        }
    }

//    p.FrequencyCorrection=pos; понравится - сохраним вручную
    
    RFM_off();
    return pos;
}



// TODO: ограничить время ожидания
void getSerialLine(byte *cp){	// получение строки
    byte cnt=0; // строка не длиннее 256 байт
    unsigned long t=millis()+60000; //  макс 60 секунд ожидания - мы ж не руками в терминале а WDT выключен

    while(t>millis()){
	if(!serial.available()){
	    delay_1(); // ничего хорошего мы тут сделать не можем
	    continue;
	}
	
	byte c=serial.read();
	if(c==0) continue; // пусто
	
	if(c==0x0d || c==0x0a){
//	    serial.println();
	    cp[cnt]=0;
	    return;
	}
/*	if(c==0x08){
	    if(cnt) cnt--;
	    continue;
	}
*/
	cp[cnt]=c;
	if(cnt<SERIAL_BUFSIZE) cnt++;
    
    }

}



void println_SNS(const char *s1,int n, const char *s2) {
    serial.print_P(s1);
    serial.print(n);
    serial.print_P(s2);
    serial.println();
}

void printAllParams(){
    for(byte i=0; i<sizeof(Params)/sizeof(long);i++){
	println_SNS(PSTR(" R"),i,PSTR("= "));
	println_SNS(PSTR(" "),((long *)&p)[i], PSTR(" "));
    }

    serial.print_P(PSTR("> "));
}



void deepSleep_450(){
    deepSleep(450);
}


void Beacon_and_Voice() {
     sendBeacon();
     
    if( cycles_count % (byte)p.SearchGPS_time) {
//	SendMorzeSign();
	if(lflags.hasGPSdata){ // есть данные той или иной свежести
	    deepSleep_450();
	    sendVOICE(messageBuff,0);
	}
    }
    cycles_count +=1;

}



void sayCoords(){

//    if(messageBuff){
	sendVOICE(messageBuff, GPS_data_fresh?0:3);
//    } else {
//	sendVOICE("000", 0);
//    }
    
}

void  beepOnBuzzer_183(){
    beepOnBuzzer(183);
}

void beepOnBuzzer_333(){
    beepOnBuzzer(0x333);
}


void yield(){
//    wdt_reset();
}


void listen_quant(){
    preambleDetected=0;
    RFM_SetPower(1,RF22B_PWRSTATE_RX,0);
    Delay_listen();//delay( p.ListenDuration );
    RFM_off();
}


byte one_listen(){

    listen_quant();
    
    if(preambleDetected) {
	byte RSSI= preambleRSSI;
	
	for(byte i=64;i>=0 && preambleDetected;i--){ // дождаться окончания вызова но не более 64 раз
/*
	    preambleDetected=0;
	    RFM_SetPower(1,RF22B_PWRSTATE_RX,0);
	    Delay_listen();//delay( p.ListenDuration );
	    RFM_off();
*/          listen_quant();
	    Delay_listen();//delay( p.ListenDuration );
	    wdt_reset();
	}
	
	if(RSSI > 33) {
	    RSSI = (RSSI - 33) / 2;
	    if(RSSI>=100) RSSI=99;
	
	    return RSSI;
	} else {
	    return 1;
	}
    
    }
    return 0;

}

void  beepOnBuzzer(unsigned int length){
#if BUZZER_PIN
	for(;length>0;length--){
#if defined(BUZZER_PIN_PORT) && defined(BUZZER_PIN_BIT)
	    BUZZER_HIGH;
	    delayMicroseconds(BuzzerToneDly);
	    BUZZER_LOW;
#else
	    digitalWrite(BUZZER_PIN, 1);
	    delayMicroseconds(BuzzerToneDly);
	    digitalWrite(BUZZER_PIN, 0);
#endif
	    delayMicroseconds(BuzzerToneDly);
	}    
#else
    if(BuzzerPin){
	for(;length>0;length--){
	    digitalWrite(BuzzerPin, 1);
	    delayMicroseconds(BuzzerToneDly);
	    digitalWrite(BuzzerPin, 0);
	    delayMicroseconds(BuzzerToneDly);
	}    
    } else delay_50();
#endif

#if FLASH_PIN
	digitalWrite(FLASH_PIN, 1);
	delay_50();
	digitalWrite(FLASH_PIN, 0);
#else
    if(FlashPin) {
	digitalWrite(FlashPin, 1);
	delay_50();
	digitalWrite(FlashPin, 0);
    } else
	delay_50();
#endif
}

void sayVoltage(byte v, byte beeps){
//    char buf[4]; //используем имеющийся буфер разбора команд, экономим 20 байт флеша
    
    buf[0] = v / 10 + '0';
    buf[1] = v % 10 + '0';
    buf[2] = 0;
    
    sendVOICE((char *)buf,  beeps);
}


void waitForCall(byte t){
    for (byte i=t;i>0; i--){
	if((Got_RSSI = one_listen())) break;
	if(t) deepSleep_450();
    }
}



byte buzzer_SOS(){ 
    for(char i=-3; i!=6; i++){
	if((byte)i>=3)
	    beepOnBuzzer_333();
	else
	    beepOnBuzzer(0x111);
	waitForCall(0);
	if(Got_RSSI) break; // если во время писка услышали вызов - сразу выходим
    }
}

void proximity(byte rssi){
    byte wi = (byte)p.WakeInterval *2;
//    char buf[4]; // используем буфер разбора команд, экономим 22 байта флеша
    byte fAnswer=1; // r17
    byte needListen=1;		// needBeacon
//    long lRSSI=rssi;
    

begin:
DBG_PRINTLN("proximity begin");

    for(;;needListen=1) {
	for(; rssi; ){ //  при получении вызова сказать голосом силу сигнала
	    wdt_reset();
	
            buf[2]=0;
            buf[1] = rssi % 10 + '0';
            buf[0] = rssi / 10 + '0';
    
            sendVOICE((char *)buf, 0); // регулируем мощность согласно силе принимаемого сигнала
    
	    waitForCall( fAnswer? 2 : 6 );
	    rssi = Got_RSSI;
	    if(rssi) fAnswer=0; 		// сбросим если было несколько частых вызовов
DBG_PRINTVARLN(rssi);
	} 

DBG_PRINTLN("proximity loop end");
DBG_PRINTVARLN(fAnswer);

	if(!fAnswer) break; //если был только один вызов - сообщим координаты и бипы
	

DBG_PRINTLN("proximity signals");
	
	fAnswer=0; // одна серия сигналов - и больше не надо
	
	// ответы маяка после вызова
	for(byte i=(byte)p.WakeRepeat; i!=0; i--){
// похоже тут баг - во время цикла мы движемся, а сравнивается начальное значение RSSI
		if(rssi > (byte)p.MinAuxRSSI){		// если сигнал сильный то будем мигать и пищать голосом

DBG_PRINTLN("proximity SOS");

/*		    for(char j=-3; i!=6; i++){
			if((byte)j>=3)
			    beepOnBuzzer(0x333);
			else
			    beepOnBuzzer(0x111);
			rssi = waitForCall(0);
			if(rssi) goto begin; // если во время писка услышали вызов - идем на начало и сообщаем силу сигнала
		    }
*/
		    if(buzzer_SOS()) {
			fAnswer = 1;  // был вызов во время сигнала - снова как первый раз
			goto begin; // если во время писка услышали вызов - идем на начало и сообщаем силу сигнала
		    }
		    
		    
		}
		
		if((byte)p.boolWakeBeacon) {
DBG_PRINTLN("proximity beacon");

		    sendBeacon();
		    waitForCall(wi); // при получении вызова - все сначала
		    needListen=0; 		// если был маяк то сбросим флаг в любом случае
		    if((rssi=Got_RSSI)) {
			fAnswer = 1;  // был вызов во время сигнала - снова как первый раз
			goto begin;
		    }
		}
		
		if((byte)p.boolWakeGPSvoice){
DBG_PRINTLN("proximity coords");
		    sayCoords();
		    goto xWait;
		} else {
		    if(needListen){
xWait:			if(i!=1) { // кроме самого последнего цикла
DBG_PRINTLN("proximity listen");
			    waitForCall(wi);
			    needListen=1;	// флаг установим в любом случае - было сообщение о координатах или был флаг ранее
			    if((rssi = Got_RSSI)) {
				fAnswer = 1;  // был вызов во время сигнала - снова как первый раз
				goto begin;
			    }
			}
		    }
		}
		
	}
	    
//      if((uptime / 10 + preambleRSSI) & 1 ) sendMorzeSign();
    }
}


byte CalcNCells_SayVoltage(){
//    int v=getExtVoltage(); уже считано
    uint16_t v=vExt;
    byte n;
    
    
    if(v < 11) {
	return 0;
    }
    if(v<44)
	//return 0;
	n=1;
    //if(v<88)
    else if(v<88)
	n=2;
    else if(v<131)
	n=3;
    else if(v<175)
	n=4;
    else if(v<219)
	n=5;
    else 
	n=6;

    if(n<NumberOfCells) return 0;
    NumberOfCells = n;

    sayVoltage(v / n, 0);

    for(;n--;){
	beepOnBuzzer_183();
	delay(183);
    }
    return 1;
}

/*

// from http://www.stm32duino.com/viewtopic.php?t=56
unsigned int sqrt32(unsigned long n)
{
    unsigned int c = 0x8000;
    unsigned int g = 0x8000;

    for(;;) {
	if(g*g > n)
	    g ^= c;
	c >>= 1;
	if(c == 0)
	    return g;
	g |= c;
    }
}

*/


// see https://github.com/Traumflug/Teacup_Firmware/blob/master/dda_maths.c#L74
uint32_t approx_distance(uint32_t dx, uint32_t dy) {
  uint32_t min, max, approx;

  // If either axis is zero, return the other one.
  if (dx == 0 || dy == 0) return dx + dy;

  if ( dx < dy ) {
    min = dx;
    max = dy;
  } else {
    min = dy;
    max = dx;
  }

  approx = ( max * 1007 ) + ( min * 441 );
  if ( max < ( min << 4 ))
    approx -= ( max * 40 );

  // add 512 for proper rounding
  return (( approx + 512 ) >> 10 );
}

// расстояние между двумя точками по координатам
unsigned long distance(Coord p1, Coord p2){
//    float scaleLongDown = cos(abs(osd_home_lat) * 0.0174532925); нам не нужна супер-точность

    unsigned long dstlat = labs(p1.lat - p2.lat) / 100;
    unsigned long dstlon = labs(p1.lon - p2.lon) / 100 /* * scaleLongDown */;
//    return sqrt(sq(dstlat) + sq(dstlon)) * 111319.5 / 10000000.0; нам не нужно точное расстояние 
    return approx_distance(dstlat, dstlon);
}


void doOnDisconnect() {// отработать потерю связи
//	например передать SMS 
// и поставить таймер отключения телефонного модуля

    if(!badCoord(home_coord.lat) && !badCoord(home_coord.lon))
	if(distance(home_coord, coord) < MIN_HOME_DISTANCE) return; // слишком близко от дома


    nextSearchTime = uptime;	// сразу же включим таймерный маяк

}


void setup(void) {
  
//  DDRD=0x60; //0110 0000 -  The Port D Data Direction Register, D6 D5 as output -  умолчальные выводы пищалки и строба
//  PORTD=0;     все это давно не используется

    wdt_disable();
 
    // wiring настраивает таймер в режим 3 (FastPWM), в котором регистры компаратора буферизованы. Выключим, пусть будет NORMAL
    TCCR0A &= ~( (1<<WGM01) | (1<<WGM00) );
 
    //RF module pins
    pinMode(SDI_pin, OUTPUT);   //SDI
    pinMode(SCLK_pin, OUTPUT);   //SCLK
    pinMode(SDO_pin, INPUT);   //SDO
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
    serial.begin(TELEMETRY_SPEED);
#endif  
  
    RFM_off();

    initBuzzer();
    beepOnBuzzer_183();

    {//    consoleCommands(); // оно и EEPROM считает
//void consoleCommands(){
        struct Params loc_p;
    
        loc_p=p;

        Green_LED_ON;
        Red_LED_ON;
    
#if (!defined(DEBUG)) &&  (!defined(DEBUG_B))
        serial.begin(38400);
#endif

        static const char PROGMEM patt[] = "tBeacon";
        //const char *patt = PSTR("tBeacon");
    
        serial.print_P(patt);
        serial.println();
    
        byte try_count=0;

//    DBG_PRINTLN("console");

        while(1) {
            byte cnt=0;
            for(unsigned long t=millis()+3000; millis() < t;){
	        byte c=serial.read();

//if(strncasecmp_P( mav_txtbuf, pat, sizeof(pat) )==0){

	        if(cnt>=6) break;
	
	        if(c != pgm_read_byte(&patt[cnt]) ) {
	            cnt=0;
	            continue;
	        }
	        cnt++;
	    }

//		DBG_PRINTLN("3s done");
    
	    if(cnt == 6){
DBG_PRINTLN("console OK");
	        if(!is_eeprom_valid()) {
	            serial.print_P("CRC!\n");
	            write_Params_ram_to_EEPROM();
	        }

	        Read_EEPROM_Config();

	        while(true){
		    serial.print_P("[CONFIG]\n");
		    printAllParams();
		
		    getSerialLine(buf);
		
		    if(buf[0] && !buf[1]) { // one char command
		        switch(buf[0]){
			case 'd':
			    p=loc_p; // восстановить параметры

			    for(int i=4; i>=0; i--)    // испортить CRC
//			    for(unsigned int i=E2END; i>=0; i--)// стереть весь EEPROM
				 eeprom_write(i,0xFF); 
    
			// no break!
			case 'w':
			    write_Params_ram_to_EEPROM();
			    break;
			    
			case 'b':
//			    initBuzzer();
			    beepOnBuzzer(1000);
			    break;
			     
			case 'c':
			    println_SNS(PSTR(" "), calibrate(), PSTR(" "));
			    break;
			    
			case 't':
			    uploadTrackPoints();
			    break;
			    
			case 'q':
			    return;
			}
		    } else { // print param value
		        byte n=atol((char *)buf); // зачем еще и atoi тaщить
		    
		        if(n > sizeof(struct Params)/sizeof(long)) break;
		    
		        println_SNS(PSTR(" R"),n, PSTR("= "));
		    
		        getSerialLine(buf); // new data
		        if(!*buf) break;
		    
		        ((long *)&p )[n] = atol((char *)buf);
		    
		    }
		}
	
            } else {
	
	        if(is_eeprom_valid()){
		    Read_EEPROM_Config();

//		    DBG_PRINTLN("load EEPROM OK");

//		    delay(4000); // DEBUG
		    break;
	        }
		if(try_count>=20) {
//	            DBG_PRINTLN("Init EEPROM done");

		    write_Params_ram_to_EEPROM();
		    break;
		}
//	        DBG_PRINTLN("Wait command EEPROM bad");
		for(byte i=3; i>0; i--){
		    Red_LED_OFF;
		    delay_300();
		    Red_LED_ON;
		    Green_LED_OFF;
		    delay_300();
		    Green_LED_ON;
		}
	    
	    }
	    try_count++;
	}
	Green_LED_OFF;
	Red_LED_OFF;
#if !defined(DEBUG) && !defined(DEBUG_B)
	serial.end();
#endif

//}
    }
//    DBG_PRINTLN("console done");


    deepSleep(50); // пусть откалибруется

    uint16_t vcc=readVCC();
    if(vcc < VCC_LOW_THRESHOLD) {
	while((vcc=readVCC())<33){
//	      DBG_PRINTLN("low VCC");
	    Red_LED_ON;
	    deepSleep(50);
	    Red_LED_OFF;
	    deepSleep_450();
	}
    }


#define INITIAL_UPTIME 1

    uptime = INITIAL_UPTIME;
  
    if(p.DeadTimer==0){
        nextSearchTime-=1; // навсегда 
    } else
	nextSearchTime = p.DeadTimer+INITIAL_UPTIME; // uptume == 1 !
  
    NextListenTime =  (byte)p.ListenInterval + INITIAL_UPTIME;
  
    nextNmeaTime   =  (byte)p.GPS_MonitInterval +INITIAL_UPTIME;
    
    if((uint16_t)p.IntLimit == (uint16_t)p.IntStart){
	growTimeInSeconds =  p.SearchTime * 60;
    }else{
	growTimeInSeconds = (p.SearchTime * 60 ) / ((uint16_t)p.IntLimit - (uint16_t)p.IntStart);
    }
  
    // initRFM(); есть внутри всех функций, работающих с RFM

//  if( p.MorzeSign1 |  p.MorzeSign2)   не нада нам морзе
//	sendMorze();
//    else


//    DBG_PRINTLN("beacon horn");

    RFM_SetPower(1,RF22B_PWRSTATE_TX, RFM_MIN_POWER ); //    beaconHorn(); 
    for(byte i=3;i>0;i--) {
        Red_LED_ON
        beacon_tone(200, 150);
        Red_LED_OFF
        beacon_tone(500, 150);
    }
    RFM_off();


//    serial.print( (long)uptime / 1000);

//voiceOnBuzzer = true; 
//sendVOICE("0123456789:#.",  0);
//voiceOnBuzzer = false;

    if(vcc){
//	if(vcc>42) vcc=42; будем честными
//	DBG_PRINTVARLN(vcc);
	voiceOnBuzzer = true; // зачем при включении говорить по радио?
	sayVoltage(vcc,0);
	voiceOnBuzzer = false;
    }

//    if((byte)p.EEPROM_SaveFreq){ // ну не должна базовая функция зависеть от настройки, тем более что 
		    //		 сохранение точек в виде трека осуществляет wear leveling для EEPROM
		    //           100 000 перезаписей одной ячейки раз в секунду = 27 часов непрерывной работы, а тут нагрузка по записи
		    //           размазывается на ~ 90 точек, ~ 100 часов непрерывной работы маяка. Пусть полет это 20 минут - ресурс маяка получается
		    //           300 полетов, или около года работы и полетов каждый день.
		    //           А потом можно и Атмегу перепаять :)
	if(getLastTrackPoint()) {
	    DBG_PRINTLN("found coords");
	    formatGPS_coords();
	    DBG_PRINTVARLN(messageBuff);

	}
//    }

#if defined(USE_GSM)
    pinMode(GSM_TX, OUTPUT);
    pinMode(GSM_EN, OUTPUT);
    pinMode(GSM_RX, INPUT);
    pinMode(GSM_INT, INPUT_PULLUP);

/*

//    TODO: init & check GSM

//    включить и попробовать зарегиться в сети - а вдруг СИМ-карта дохлая?
    gsm.begin(19200);


    gsm.end();
    digitalWrite(GSM_EN, HIGH);

*/

#endif // USE_GSM

    ADCSRA &= ~(1<<ADEN); 	//Disable ADC
    ACSR = (1<<ACD); 		//Disable the analog comparator
//  DIDR0 = 0x3F; 		//Disable digital input buffers on all ADC0-ADC5 pins
    DIDR1 = (1<<AIN1D)|(1<<AIN0D); //Disable digital input buffer on AIN1/0


    wdt_reset();
//    DBG_PRINTLN("setup done");


#if !defined(DEBUG) &&  !defined(DEBUG_B)
    power_usart0_disable();
#endif
    power_twi_disable();
    power_spi_disable();
#if ! defined(USE_GSM)	 /* отключим после окончания работы с GSM */
    power_timer1_disable();
#endif
    power_timer2_disable();
    // power_adc_disable() ADC нам нужен

    wdt_enable(WDTO_8S);
}


void loop(void) {

    unsigned long timeStart=millis() + millisCounter; // время бодрствования плюс время сна

    wdt_enable(WDTO_8S);

    uptime = (timeStart - timeStart / 1000 * p.TimerCorrection) / 1000;
   
//   if(uptime < 60 * 60 * 8) {// 8 часов
    if(uptime < 60 * 10) {// 10 минут, только в начале полета
	Green_LED_ON;
	delay_1();
	Green_LED_OFF;
    }
    
    if(uptime % 1800 == 0) { // каждые полчаса
	beepOnBuzzer_183();
	beepOnBuzzer_183();
    }

// благодаря deepSleep в конце loop исполняется в батарейном режиме не чаще раза в секунду, если не слушаем MAVlink

    vExt = getExtVoltage(); // читаем всегда дабы знать о наличии питания
    
    // TODO - через MAVlink идет напряжение замеренное контроллером, использовать для (само)проверки?
    
    if(vExt<=100) { // питания нет
	lflags.hasPower = false;


	if(lflags.lastPowerState) { // только что было
	    DBG_PRINTLN("no vExt");
	    if( lflags.wasPower 		// маяк могут включить раньше коптера - не паникуем раньше времени
	     && (uptime - powerTime) > 60   // питание было подано больше минуты
	     && lflags.motor_armed		// и были заармлены моторы
	     && (uptime - armTime) > 30) { 	// причем более полминуты
	 
				     // случилось страшное
	        lflags.connected = false;     // соединения нет
	        DBG_PRINTLN("VCC gone");
	        
	        if(lflags.pointDirty){
	            SaveGPSPoint(); // сохранять последнюю найденную точку по обрыву питания
	        }
	
	        disconnectTime = uptime; // пошло время от отсоединения
	    } else { // нестрашное пропадание питания - сбросим флаг
	        lflags.wasPower = false;
	        DBG_PRINTLN("VCC ignored lost");
	    }
	}
    } else { // питалово есть
	DBG_PRINTVARLN(vExt);

	lflags.hasPower = true;
	if(!lflags.wasPower) {
	    powerTime=uptime;		// запомнили время включения питания
	    lflags.wasPower = true;
	}
	if(!lflags.lastPowerState) { // включение
	    disconnectTime=0;  // если выключили и включили питание - дисконнекта нет
	    DBG_PRINTLN("vExt=");
	    DBG_PRINTVARLN(vExt);

	}
    }

    lflags.lastPowerState = lflags.hasPower;

// вобщем-то секунда тут конфликтует со временем прослушивания, логичнее было б иметь критерий
// типа "два периода подряд ничего не было"
// НО
// в режиме связи с коптером у нас есть питание и слушаем всегда, а пропадание одного питания достаточно
// чтобы бить тревогу, МАВлинк контролируется на случай зависания автопилота

    if(lflags.mavlink_active && (millis() - lastMAVBeat) > 500) { // был но пропал Мавлинк и 0.5сек нету
	if(lflags.motor_armed) { 	// на ходу пропал - 
	    lflags.connected = false;     // соединения нет
	    disconnectTime = uptime; // пошло время от отсоединения

	    if(lflags.pointDirty){
		DBG_PRINTLN("MAVLINK gone");

	        SaveGPSPoint(); 	// сохранять последнюю найденную точку по обрыву MAVlink
	    }
	    
	    if(lflags.hasPower) { // питание есть а связь пропала без дизарма - завис автопилот! Сработка парашюта
		chute_start();
	    }
	}
    }

    if(lflags.motor_armed){
//        if(!lflags.motor_was_armed) { // если только что заармили
//        }
        if(!lflags.last_armed_status) { // заармили независимо от времени
	    disarmTime = 0; // снова заармили
	    disconnectTime = 0;

            armTime=uptime;		// запомнить время
	    if(lflags.hasPower)		// заармили и есть питание
		lflags.connected = true;     // соединение ОК
	    lflags.motor_was_armed=1;

	    if(GPS_data_fresh)
		home_coord = coord;	// если данные не из памяти то сохранить в роли координат дома

	    DBG_PRINTLN("Armed");

        }

	if(mav_severity){	// поступило новое сообщение
	    //"Crash: Disarming"
	    static const char PROGMEM pat[]="crash:";
	    if(strncasecmp_P( mav_txtbuf, pat, sizeof(pat) )==0){
		lflags.connected = false;     // соединения нет
		lflags.crash=true;
		doOnDisconnect();// отработать окончательную потерю связи
	    }
	}
    } else { // 		дизарм
	if(lflags.motor_was_armed){	// моторы были заармлены 
	    if( (uptime - armTime) < 60) // если меньше минуты то не считается
		lflags.motor_was_armed = false;
	    else {	// дизарм после минуты арма
		// если все хорошо, то ничего передавать не надо - маяк просто выключат через несколько минут или заармят снова
	        disarmTime=uptime; // запомним время
		DBG_PRINTLN("disarm lock");

	    }

	}
    }
    
    lflags.last_armed_status = lflags.motor_armed;

//  был дизарм
    if(disarmTime && (uptime - disarmTime) > DISARM_DELAY_TIME  ) { // дизарм, и за 2 минуты ничего не произошло
	    lflags.connected = false;     // будем считать что соединения нет
	    disconnectTime = uptime; // пошло время от отсоединения
	    disarmTime=0;

	    DBG_PRINTLN("disarm disconnect");
    }


// предусмотрим передергивание питания коптера без отключения питания маяка
    if(disconnectTime && (uptime - disconnectTime) > 60 ) { // ничего не произошло минуту после потери связи
	    doOnDisconnect();// отработать окончательную потерю связи

	    DBG_PRINTLN("disconnect lock");
	    disconnectTime=0;
    }

// теперь у нас есть флаг, показывающий стОит маяку шевелиться или нет


// контроль  батарей

    if((byte)p.ExtVoltageWarn) {
	if(NextVoltageTreshhold == 25) { // при старте там 25, так что меряем и говорим
	    if(powerCheckDelay-- == 0) {
		if(CalcNCells_SayVoltage()) {
		    NextVoltageTreshhold = 42; // успешно определили, батарейка полная
		    powerCheckDelay = 0;
		} else {
		    powerCheckDelay =15;
		}
	    }
	} else {
	    if(NumberOfCells){ // батарейка определилась
		unsigned int v = vExt / NumberOfCells;
		unsigned int v10 = v * 10; // чуть увеличим точность

		if( cell_voltage ) {
		    cell_voltage = (v10 +  cell_voltage*4) / 5; // комплиментарный фильтр
		} else
		    cell_voltage = v10;
		    
		if(v < 26) {
		    beepOnBuzzer_183();
		    beepOnBuzzer_183();

		    for(byte i=3; i>0; i--){
			sayVoltage(0,1); // сказать "0" с бипом
			deepSleep(200);
		    }
		    NextVoltageTreshhold = 25;
		} else {
		    if(v <  (byte)p.ExtVoltageWarn){
			NextVoltageTreshhold = 33;
			powerCheckDelay = 0;
			beepOnBuzzer_333();
		
			sayVoltage(v,3);
			
			beepOnBuzzer_333();
		    }
		}
		
		byte chk_v2 = NextVoltageTreshhold / 256; // старший байт - второй порог
		
		if(cell_voltage/10 < chk_v2  || NextVoltageTreshhold ==35){
		    if(NextVoltageTreshhold != 35)
			powerCheckDelay = 0;
			
		    if(!powerCheckDelay){
			if(v < chk_v2)
			    sayVoltage(cell_voltage/10, 1);
			else 
			    powerCheckDelay=14;
		    } else 
			powerCheckDelay-=1;
		
		}
	    }
	}
  } // p.ExtVoltageWarn


// -- GSM маяк ----------------------

/*

	нужен аналогичный период включений для прослушки и получения SMS, например 4 раза каждые полчаса
    но только при наличии свежих координат

при получении SMS - отправить координаты либо исполнить другую команду

*/


//---------- таймерный маяк  -------------------------------------------------------------
    if( searchBeginTime == 0 ) {
        if(  uptime >= nextSearchTime ) { // проверяем условия запуска

	DBG_PRINTLN("timed beacon init");

          // Начало поискового периода
          searchBeginTime = uptime; // запомнить время начала
          nextBeepTime = uptime;
      
          beaconInterval =  p.IntStart;
          timeToGrow = uptime + growTimeInSeconds;
          searchStopTime  = uptime + p.SearchTime * 60;
          nextSearchTime = searchStopTime + p.Sleep * 60;
        }
    } else { // searchBeginTime != 0 - мы в активном поиске
    
    
        if( uptime >= nextBeepTime ) {

	    DBG_PRINTLN("nextBeepTime");

	    Beacon_and_Voice();
	    
            nextBeepTime = uptime + BEACON_BEEP_DURATION * 3 / 1000 /* 3 */ + beaconInterval;
          
            // В конце посылки послушаем эфир
            Got_RSSI = one_listen();
           
            if(Got_RSSI) { // TODO:  помигать диодом
                // Есть вызов
                DBGB_PRINTLN("Call is detected")
              
                if( Got_RSSI > p.MinAuxRSSI) {
            	    buzzer_SOS();
                } 
        // тут маяк неадекватно реагирует на вызов в разных режимах - а надо чтобы одинаково, это более ожидаемо
        	sayCoords();
            }
        }
    
        if( uptime >= timeToGrow ) {
          // Увеличение интервала между посылками
          DBGB_PRINTLN("Interval grow");
          timeToGrow = uptime - (timeToGrow - uptime) + growTimeInSeconds;
          beaconInterval ++;
        }
    
        if( uptime >= searchStopTime ) {
          // Окончание периода работы маяка
          DBGB_PRINTLN("Search time end");
          searchBeginTime = 0;
        }
    } // searchBeginTime == 0
 


// -----------  прием координат -----------------------------------------------------------------------------------
    if(lflags.listenGPS) { // если слушаем
    
	if(read_mavlink()) { // по получению координат

	    DBG_PRINTLN("MAVLINK coords");

	    if(badCoord(coord.lat) || badCoord(coord.lon) ) { // not found
		GpsErrorCount +=1;
	    } else {

		if(mav_satellites_visible >  (byte)p.GPS_SatTreshHold && mav_fix_type>=2) { // 2d or 3d fix
		    GpsErrorCount = 0;

		    if(lastPointTime) { // есть предыдущая точка
			if( (millis() - lastPointTime)>=1000) {      // регистрируем точки не чаще одного раза в секунду
			    // посчитать расстояние между двумя точками и отбросить если изменилось меньше 3 метров
			    // переменные p1 p2 используются только при старте - в поиске последней актуальной точки, и 
			    // больше не нужны
			    if(distance(coord,p2)>LAST_POINT_DISTANCE) { // сдвинулись достаточно от последней точки
		                gps_points_count++; 
			        lastPointTime = millis(); // время последней обработанной точки
				p2=coord; // сама предыдущая точка

				//при 0 вообше не сохраняем, засада :(
		                if((byte)p.EEPROM_SaveFreq && lflags.motor_was_armed){ // какой смысл портить еепром координатами стоянки?
			            if(gps_points_count % (byte)p.EEPROM_SaveFreq == 0) {
	    DBG_PRINTLN("save coord");
			                SaveGPSPoint(); 
		                    }
		                }

			/* если в настройках указано передавать трек - передать точку через GSM
				p1 - последняя точка ушедшая по GSM
			*/
			

			    } else {
				// отбросили точку по расстоянию
			    }
			}
		    } else { // первая точка. Обычно она на земле, поэтому особо не интересна
	    DBG_PRINTLN("coord first point");
			lastPointTime = millis(); // время последней обработанной точки
			p2=coord; // сама точка, от нее будем считать сдвиг
		    }

		    if( GPS_data_fresh  && ! GPS_data_OK){ // сообщаем координаты в эфир по первому фиксу
//		        if(!lflags.motor_armed) { // не включать передачу если успели взлететь
			    voiceOnBuzzer=true; // скажем на пищалку а не в эфир
			    sayCoords();
			    voiceOnBuzzer=false;
//			}
		        GPS_data_OK = 1;
		    }

		}
	    }
	}

	// при включенном питании мы не обновляем gpsOffTime, поэтому при выключении питания
	// оно будет сильно просроченным и прием выключится сразу же
	if(gpsOffTime<millis() && !lflags.hasPower){ // пора выключить
	    lflags.listenGPS = false;
	    nextNmeaTime = uptime + p.GPS_MonitInterval;

	    DBG_PRINTLN("listen time over");

#if !defined(DEBUG) &&  defined(DEBUG_B)
	    serial.end();
	    power_usart0_disable();
#endif
	}
    } else { // не слушаем
  
	if( lflags.hasPower || uptime >= nextNmeaTime ) {  // а не пора ли включить?
	
	    if( lflags.hasPower || GpsErrorCount <  (uint16_t)p.GPS_ErrorTresh )  {

		DBG_PRINTLN("time to listen");

#if !defined(DEBUG) &&  defined(DEBUG_B)
		power_usart0_enable();    
		serial.begin(TELEMETRY_SPEED);
#endif
		lflags.listenGPS = true; // включим прослушивание;
		gpsOffTime = millis() + p.GPS_MonDuration; // время выключения

		coord = bad_coord; //coord.lat=coord.lon=NAN;		// сбросим принятые данные
		mav_satellites_visible=0;

	    
	    } else {					// по превышению числа ошибок
		nextNmeaTime = uptime + p.GPS_MonitInterval * 100; // в 100 раз реже
		GpsErrorCount -=1;
	    }
	}
    } // serial listen

  
    // voice listen - независимо от состояния подключения, а то вдруг ошибочка вышла
    if(uptime>NextListenTime) {
	Red_LED_ON;
	delay_1();
	Red_LED_OFF;
	Got_RSSI =  one_listen();

	DBG_PRINTLN("time to receive");


	if(Got_RSSI) {
	    for(byte i=5; i>0; i--) {
	        Red_LED_ON;
		Green_LED_ON;
	        delay_1();
	        Red_LED_OFF;
	        Green_LED_OFF;
	        delay_10();
	    }
	    DBG_PRINTLN("RSSI ok");

	    proximity(Got_RSSI);
	}
	NextListenTime = uptime + p.ListenInterval;
    }

  
    if(!lflags.hasPower && !lflags.listenGPS) { // если не слушаем GPS и на батарее то спим до конца секунды
delay_50(); // чтобы порт успел передать до засыпания все что накопилось
	// Sleep till the end of a second
        unsigned  long timeNow=(millis() + millisCounter);
        deepSleep(1000L - ( timeNow -  timeStart ) % 1000 + TIMER_CORRECTION);
    }

/* наф эти вычисления - при следующем входе в LOOP uptime пересчитается по факту
  if(timeNow -  timeStart < 1000) {
    uptime++;
  } else {
    uptime += 1 + ( timeNow -  timeStart ) / 1000;
  }
*/

}


