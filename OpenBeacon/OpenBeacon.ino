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


GNU GPLv3 LICENSE

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



#include "compat.h" //   some missing definitions

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

#include "config.h"

#include <AltSoftSerial.h>

SingleSerialPort(serial);

// own includes
#include "version.h"
#include "bufstream.h"
#include "config-phones.h"
#include "vars.h"
#include "eeprom.h"



#if defined(USE_GSM)
#include "gsm.h"
GSM gsm;
#endif

#include "chute.h"

#if defined(USE_GSM)
#include "gsm_core.h"
#endif

#include "protocols.h"

#include "func.h"


#if defined(USE_MORZE)
#include "morseEnDecoder.h"
morseEncoder morze;
#endif




void beepOnBuzzer(unsigned int length);
void sayVoltage(byte v, byte beeps);

void delay_300();
void redBlink();
byte powerByRSSI();




// program's parts

#include "sleep.h"

#include "rfm22b.h"
#include "samples.h"
#include "voice.h"

#ifdef USE_DTMF
#include "dtmf.h"
#endif


#if defined(USE_MAVLINK)
extern BetterStream *mavlink_comm_0_port;
#endif





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



inline void initBuzzer(){
     BuzzerToneDly = (byte)p.WakeBuzzerParams;

#if defined(BUZZER_PIN)
	pinMode(BUZZER_PIN,OUTPUT);

 #if defined(BUZZER_PIN_PORT) && defined(BUZZER_PIN_BIT)
    // OK
 #else
	buzzerBit = digitalPinToBitMask(BUZZER_PIN); // для работы с пищалкой напрямую, а не через DigitalWrite
	uint8_t port = digitalPinToPort(BUZZER_PIN);
	buzzerPort = portOutputRegister(port);
 #endif

	BUZZER_LOW; 

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
//     pinMode(BUZZER_PIN2, INPUT_PULLUP);
#else
     FlashPin=(byte)p.WakeFlashPin;
     pinMode(FlashPin, OUTPUT);
     digitalWrite(FlashPin,0);
#endif
}


SIGNAL(WDT_vect) {
  wdt_disable();
  wdt_reset();
#ifdef WDTCSR
  WDTCSR &= ~_BV(WDIE);
#else
  WDTCR &= ~_BV(WDIE);
#endif
}

// get external voltage in mv
// на входе делитель 1k на землю 22k на батарею
uint16_t getExtVoltage(){
    adc_setup();

#define VEXT_AVG 100
    register unsigned long sum=0;
    for(byte i=VEXT_AVG;i>0; i--){
	sum +=analogRead(VBAT_PIN);
	delay_1();
    }

    ADCSRA &= ~(1<<ADEN); //Disable ADC

//    if(sum < 64) { // нету напряжения, совсем нету
//	return 0; // оно и само так получится
//    }

//    DBG_PRINTVARLN(sum); //17204 = 13.43v 1281 1/v

    sum *= 1000 / VEXT_AVG;

    return  sum / 1281; // калибровка
}


// vcc in mv
// 10k на землю, 15к на +
uint16_t readVCC() {

    adc_setup();

//    pinMode(12, 1) ; // 12 (PB3) MOSI - зачем его на вывод - ???
//    digitalWrite(12,0);

#define VCC_AVGA 64

    uint32_t sum=0;
    for(byte i=0; i<VCC_AVGA; i++){
        sum += analogRead(VCC_PIN);
        delay_1();
    }
//    pinMode(12, 0); // PB3 на ввod
//    DBG_PRINTVARLN(sum);
//delay(1000);

    sum *= 416;
     sum /= VCC_AVGA;
//DBG_PRINTVARLN(sum);
//delay(1000);
    sum /= 51;

//DBG_PRINTVARLN(sum);//33032 = 4200
//delay(1000);
//    if(sum < 26) return 0;
    if(sum > 3200) return sum; // что-то померялось
//*/

    ADMUX = 0x4e; //AVCC with external capacitor at AREF pin, 1.1v as meashured
    delay_1();
    sum=0;
    
#define VCC_AVGB 100

    for(byte i=VCC_AVGB; i>0; i--){
        delay_1();
    
	ADCSRA |= 1 << ADSC; // start conversion
    
	while (bit_is_set(ADCSRA, ADSC));
    
	byte low  = ADCL;
	byte high = ADCH;
    
	uint16_t v = (high << 8) | low;
    // return 11253 / v;
	sum+=  v;
    }

    return (118645531UL /* * VCC_AVGB / 100 */ ) / sum; // in mv, calibrated
}


byte powerByRSSI() {
    byte rssi = Got_RSSI;
    if(rssi)   		// хоть что-то принимаем
	lastRSSI = rssi;	// запоминаем последнее значение
    else		// сигнала нет
	if(lastRSSI > 50) 
	    lastRSSI -= 1; // убавляем по чуть-чуть
	else
	    lastRSSI = 0;
	
    // услышав посылку маяк убавляет силу сигнала, затем потихоньку прибавляет
    return lastRSSI < 50 ? RFM_MAX_POWER : ((RFM_MAX_POWER + 5)*10 - lastRSSI) / 10; // 120
}


void buzz_or_rfm(){
  if(!voiceOnBuzzer) {
#if USE_MORZE
      morze.flush(); // дождаться окончания передачи
#endif

      RFM_set_TX();

      delay_300(); // clear tone to open silencer
  }

}

void sendVOICE(char *string, byte beeps){

    buzz_or_rfm();

    _sendVOICE(string, beeps);

    RFM_off();
}


BS bs; // BuffStream


#define MULTIPLIER 10000000 // ArduPilot uses coords in LONG  multiplied by this value


//no use p.GPS_Format

#if GPS_COORD_FMT == 1 
// 1 ГГ ММ СС.С

void format_one(long n){
    if(n<0) {
	bs.print('-');
	n=-n;
    }

    unsigned int DD = n/MULTIPLIER; // grad 
    unsigned int MM =   (n - DD*MULTIPLIER) * 60 /MULTIPLIER;
    unsigned long SS = ((n - DD*MULTIPLIER) * 60 - MM) * 60 /MULTIPLIER;
    bs.printf_P(PSTR("%i %02i %02i.%1i"), DD, MM, (int)SS, (int)((SS - (unsigned long)SS)*10/MULTIPLIER));
}

#elif GPS_COORD_FMT == 2
 // 2 ГГ ММ.МММ

void format_one(long n){
    if(n<0) {
	bs.print('-');
	n=-n;
    }

    unsigned int DD = n/MULTIPLIER; //grad
    unsigned long  MM = (n - DD*MULTIPLIER) * 60 /MULTIPLIER;

    bs.printf_P(PSTR( "%i %02i.%03i"), DD, (int)MM, (int)((MM - (unsigned long)MM)*1000/MULTIPLIER));
}
#elif GPS_COORD_FMT == 3
      // 3 ГГ.ГГГГГ

void format_one(long n){

    if(n<0) {
	bs.print('-');
	n=-n;
    }
    unsigned int in=n/MULTIPLIER;
    unsigned long tail = n - in*MULTIPLIER;
    bs.printf_P(PSTR( "%i.%05lu"), in, (unsigned long)(tail*100000/MULTIPLIER));
}
#elif GPS_COORD_FMT == 4
      // 4 ГГ.ГГГГ 
void format_one(long n){

    if(n<0) {
	bs.print('-');
	n=-n;
    }
    unsigned int in=n/MULTIPLIER;
    unsigned long tail = n - in*MULTIPLIER;
    bs.printf_P(PSTR( "%i.%04lu"), in, (unsigned long)(tail*10000/MULTIPLIER));
}
#elif GPS_COORD_FMT == 5
      // 5 .ГГГГГ
void format_one(long n){

    if(n<0) {
	bs.print('-');
	n=-n;
    }
    unsigned int in=n/MULTIPLIER;
    unsigned long tail = n - in*MULTIPLIER;
    bs.printf_P(PSTR( "%05lu"), (unsigned long)(tail*100000/MULTIPLIER));
}
#elif GPS_COORD_FMT == 6
      // 6 .ГГГГ
void format_one(long n){

    if(n<0) {
	bs.print('-');
	n=-n;
    }
    unsigned int in=n/MULTIPLIER;
    unsigned long tail = n - in*MULTIPLIER;
    bs.printf_P(PSTR( "%04lu"), (unsigned long)(tail*10000/MULTIPLIER));
}
#endif

void formatGPS_coords(){ // печатает координаты в буфер для рассказа голосом
    lflags.hasGPSdata=true; // раз позвали печатать значит есть что сказать

    BS::begin(messageBuff);

    format_one(coord.lat);
    bs.print('#');	// beep-beep
    format_one(coord.lon);
    
    if(lflags.wasCrash)     bs.print('*'); // если краш а не посадка то на конце будет длинный бип
}

bool is_eeprom_valid(){// родная реализация кривая, у нас есть CRC из MAVLINKa, заюзаем его

    register uint8_t *p, *ee;
    uint16_t i;

    eeprom_read_len((byte *)&i, EEPROM_VERS, sizeof(i));
    if(i != CURRENT_VERSION) return false;		// разная версия EEPROM

    crc_init(&crc);

    for(i=sizeof(Params), p=(byte *)&p,  ee=(byte *)(EEPROM_PARAMS);  i>0; i--, ee++) 
	crc_accumulate(eeprom_read_byte((byte *) ee ), &crc);

    eeprom_read_len((byte *)&i, EEPROM_CRC, sizeof(crc));
    
    return i == crc;
}


void Read_EEPROM_Config(){ 

#if 1 // MAX_PARAMS * 4 < 255
    eeprom_read_len((byte *)&p, EEPROM_PARAMS, sizeof(p));
#else
    uint8_t *pp;
    uint16_t i, ee;

// eeprom_read_len недостаточно
    for(i=sizeof(p), pp=(byte *)&p,  ee=EEPROM_PARAMS;  i>0; i--,ee++) { // байта для адреса мало
	*pp++ = (byte)eeprom_read_byte( (byte *)ee );
    }
#endif
}

void write_Params_ram_to_EEPROM() { // записать зону параметров в EEPROM
    uint8_t *pp, *ee;
    uint16_t i;
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

DBG_PRINTLN("EEPROM write done!");
}

void inline printCoord(long& l){ // signed long!
//    if(l<0) {
//	serial.write_S('-');
//	l=-l;
//    }
/*
    unsigned int in = l / MULTIPLIER;
    serial.print(in);
    serial.print('.');
    serial.print( l - in * MULTIPLIER);
*/
    serial.print( l ); // пусть конфигуратор разбирается с множителем
}

bool badCoord(Coord  *p){
    return p->lat==BAD_COORD || p->lon == BAD_COORD;
}


// вообще ценность трека из маяка сильно сомнительна - раз маяк на руках значит мы его нашли :)
void uploadTrackPoints(){// TODO:
    byte cnt=0;
    
    for(unsigned int i=EEPROM_TRACK + (eeprom_points_count +1)*sizeof(Coord) ; cnt <= MAX_TRACK_POINTS; cnt++) { // начинаем с точки после маркера конца
	eeprom_read_len((byte *)&point1,i,sizeof(Coord)); // вторая точка
	if(!badCoord(&point1)) {
	    printCoord(point1.lat);
	    serial.write_S(' ');
	    printCoord(point1.lon);
	    serial.println();
	}
	i+=sizeof(Coord);
	if(i>=EEPROM_TRACK_SIZE)    i=EEPROM_TRACK; // кольцо
    }
} 

byte getLastTrackPoint(){    // последняя - та что не-NAN а за ней NAN

    eeprom_points_count=0; // с самого начала
    byte cnt=0;

    eeprom_read_len((byte *)&point1, EEPROM_TRACK, sizeof(Coord)); // первая точка
    
    for(unsigned int i=EEPROM_TRACK; cnt <= MAX_TRACK_POINTS; cnt++) { // намеренно на 1 точку больше
	i+=sizeof(Coord);
	if(i>=EEPROM_TRACK_SIZE)    i=EEPROM_TRACK; // кольцо
	
	eeprom_read_len((byte *)&point2,i,sizeof(Coord)); // вторая точка
	
	if(!badCoord(&point1) && badCoord(&point2) ) {
	    coord=point1;		// нашли нужную точку
	    eeprom_points_count=cnt+1;
	    break;
	}
	point1=point2; // переход к следующей точке
    }
// если еще не сохраняли точек то закончим цикл, eeprom_points_count будет 0 и запись начнется с самого начала

    point1 = point2 = bad_coord; // затереть дабы отличать принятые от исторических
    
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


void redBlink(){
    Red_LED_ON;
    delay_1();
    Red_LED_OFF;
}


static void inline sendMorzeSign(); //not used

void Delay_listen(){
    delay( p.ListenDuration);
}

byte calibrate(){
 
     RFM_tx_min(); // RFM_SetPower(RF22B_PWRSTATE_TX, RFM_MIN_POWER );

     int16_t min=10000; // min
      
     byte pos=0;

     byte good_count=0;

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
         
            good_count++;
            
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
    if(good_count>64)
        return pos;
    else
	return 0;
}



// TODO: ограничить время ожидания ?
void getSerialLine(byte *cp, void(cb)() ){	// получение строки
    byte cnt=0; // строка не длиннее 256 байт
//    unsigned long t=millis()+60000; //  макс 60 секунд ожидания - мы ж не руками в терминале а WDT выключен

//    while(t>millis()){
    while(true){
	if(!serial.available_S()){
	    if(cb) 
		cb();	// если есть callback то вызовем
	    else
		delay_1(); // ничего хорошего мы тут сделать не можем
	    continue;
	}
	
	byte c=serial.read_S();
// uses .available	if(c==0) continue; // пусто
	
	if(c==0x0d || (cnt && c==0x0a)){
//	    serial.println();
	    cp[cnt]=0;
	    return;
	}
	if(c==0x0a) continue; // skip unneeded LF
	
/*	if(c==0x08){   no manual editing
	    if(cnt) cnt--;
	    continue;
	}
*/
	cp[cnt]=c;
	if(cnt<SERIAL_BUFSIZE) cnt++;
    
    }

}

void getSerialLine(byte *cp) {
    getSerialLine(cp, NULL);
}


void print_SNS(const char *s1, unsigned long n, const char *s2) {
    serial.print_P(s1);
    serial.print(n);
    serial.print_P(s2);
}


void deepSleep_450(){
    deepSleep(450);
}


//таймерный маяк - посылка и координаты
inline void Beacon_and_Voice() {
#if USE_MORZE
    morze.flush(); // дождаться окончания передачи
#endif
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

void  beepOnBuzzer(unsigned int length){

#if FLASH_PIN
	digitalWrite(FLASH_PIN, 1); // первым делом включить вспышку
#else
    if(FlashPin) 
	digitalWrite(FlashPin, 1);
#endif


// затем попищать
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

// и не забыть выклюючить вспышку
#if FLASH_PIN
	digitalWrite(FLASH_PIN, 0);
#else
    if(FlashPin) 
	digitalWrite(FlashPin, 0);
#endif
}

void format_10(byte v){
    buf[0] = v / 10 + '0';
    buf[1] = v % 10 + '0';
    buf[2] = 0;
}

void sayVoltage(byte v, byte beeps){
//    char buf[4]; //используем имеющийся буфер разбора команд, экономим 20 байт флеша
    format_10(v);
    
    sendVOICE((char *)buf,  beeps);
}




void sayCoords(bool onlyVoice=false){

    if(onlyVoice || p.flags.f.wakeGPSvoice || p.flags.f.timeGPSvoice)
	sendVOICE(messageBuff, GPS_data_fresh?0:3);

    if(onlyVoice) return;

#if defined(USE_DTMF)
    if( p.flags.f.wakeGPS_DTMF || p.flags.f.timeGPS_DTMF)
	sendDTMF(messageBuff, GPS_data_fresh?0:3);
#endif
    
#if USE_MORZE
    if( p.flags.f.wakeGPS_MORZE || p.flags.f.timeGPS_MORZE)
        morze.write(messageBuff);
#endif
}

void  beepOnBuzzer_183(){
    beepOnBuzzer(183);
}

void beepOnBuzzer_333(){
    beepOnBuzzer(0x333);
}


//byte tmpRSSI;

void listen_quant(){
    preambleDetected=0;
    RFM_SetPower(RF22B_PWRSTATE_RX,0);
    Delay_listen();
    tmpRSSI = spiReadRegister(0x26); // считать RSSI независимо от наличия вызова
#if USE_MORZE
    if(!morze.busy())  RFM_off();
#else
    RFM_off();
#endif
    wdt_reset();
}


byte one_listen(){ // ожидание вызова 1 интервал, затем ожидание перехода абонента на прием
    byte RSSI = 0;

    listen_quant();

    if(preambleDetected) {  // если услышали вызов то дождаться его завершения
	RSSI= preambleRSSI;
	
	// дождаться окончания вызова но не более 64 раз
	// по умолчанию интервал прослушивания всего 50мс, так что время ожидания ограничено 6.4 секунды. не маловато?
	//for(byte i=64;i>0 && preambleDetected;i--){ 
	for(byte i=100;i>0 && (preambleDetected || tmpRSSI>70);i--){ // фоновой шум дома рядом с компом дает RSSI 52..68
            listen_quant();
	    Delay_listen();
//DBG_PRINTVARLN(tmpRSSI);
	}
	
	
	if(RSSI > 33) {
	    RSSI = (RSSI - 33) / 2;
	    if(RSSI>=100) RSSI=99;
	} else {
	    RSSI= 1;
	}
    
    }
    Got_RSSI = RSSI;
    return RSSI;

}

// ожидание вызова t периодов прослушивания
void waitForCall(byte t){
    while(true){
	if(one_listen()) {
	    lflags.callActive=true; // режим недавнего вызова
	    callTime = uptime;       // запомним время последнего вызова
	    break;
	}
	if(t==0) break;
	if(--t) deepSleep_450();
    }
}

void buzzer_SOS(){ 
#if defined(STROBE_PIN)
    digitalWrite(STROBE_PIN,HIGH);
#endif

    for(char i=-3; i!=6; i++){
	if((byte)i<3)
	    beepOnBuzzer_333();
	else
	    beepOnBuzzer(0x111);

	waitForCall(0);

	if(Got_RSSI) break; // если во время писка услышали вызов - сразу выходим
    }

#if defined(STROBE_PIN)
    digitalWrite(STROBE_PIN,LOW);
#endif
}



void yield(){
//    wdt_reset();
}



void proximity(byte rssi){
    byte wi = (byte)p.WakeInterval *2;
//    char buf[4]; // используем буфер разбора команд, экономим 22 байта флеша
    byte needListen=1;


begin_ans:    
    byte fAnswer=1; 
    byte lastRSSI=rssi;

begin:
//DBG_PRINTLN("proximity begin");

    for(;;needListen=1) {
	for(; rssi; ){ //  при получении вызова сказать голосом силу сигнала
	    wdt_reset();
	
	    format_10(rssi);
            sendVOICE((char *)buf, 0); // регулируем мощность согласно силе принимаемого сигнала
    
	    waitForCall( fAnswer? 2 : 6 );
	    
	    if((rssi = Got_RSSI)) {
		fAnswer=0; 	// сбросим если было несколько частых вызовов
		lastRSSI=rssi;
	    }
	} 

//DBG_PRINTLN("proximity loop end");
//DBG_PRINTVARLN(fAnswer);

	if(!fAnswer) break; //если был только один вызов - сообщим координаты и бипы
	

//DBG_PRINTLN("proximity signals");
	
	fAnswer=0; // одна серия сигналов - и больше не надо
	
	// ответы маяка после вызова
	for(byte i=(byte)p.WakeRepeat; i!=0; i--){
// похоже тут баг - во время цикла мы движемся, а сравнивается начальное значение RSSI

//DBG_PRINTVARLN(rssi);
//DBG_PRINTVARLN((byte)p.MinAuxRSSI);

		if(lastRSSI > (byte)p.MinAuxRSSI){		// если сигнал сильный то будем мигать и пищать голосом

		    buzzer_SOS();
		    if((rssi = Got_RSSI)){ // был вызов во время сигнала - снова как первый раз
			goto begin_ans; // если во время писка услышали вызов - идем на начало и сообщаем силу сигнала
		    }
		}
		
		if((byte)p.boolWakeBeacon) {
//DBG_PRINTLN("proximity beacon");

		    sendBeacon();
		    waitForCall(wi); // при получении вызова - все сначала
		    needListen=0; 		// если был маяк то сбросим флаг в любом случае
		    if((rssi=Got_RSSI)) {
			fAnswer = 1;  // был вызов во время сигнала - снова как первый раз
			lastRSSI=rssi;
			goto begin;
		    }
		}
		
		if(p.flags.f.wakeGPSvoice || p.flags.f.wakeGPS_DTMF || p.flags.f.wakeGPS_MORZE ){ // wake flags
//DBG_PRINTLN("proximity coords");
		    sayCoords();
		    goto xWait;
		} else {
		    if(needListen){
xWait:			if(i!=1) { // кроме самого последнего цикла
//DBG_PRINTLN("proximity listen");
			    waitForCall(wi);
			    needListen=1;	// флаг установим в любом случае - было сообщение о координатах или был флаг ранее
			    if((rssi = Got_RSSI)) {// был вызов во время сигнала - снова как первый раз
				goto begin_ans;
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

    while(n--){
	beepOnBuzzer_183();
	delay_100();
    }
    return 1;
}



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



// see https://github.com/Traumflug/Teacup_Firmware/blob/master/dda_maths.c#L74
uint32_t approx_distance(uint32_t dx, uint32_t dy) {
  uint32_t min, max, approx;

  // If either axis is zero, return the other one.
//  if (dx == 0 || dy == 0) return dx + dy;
  if (dx == 0) return dy;
  if (dy == 0) return dx;

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
unsigned long distance(Coord *p1, Coord *p2){
//    float scaleLongDown = cos(abs(osd_home_lat) * 0.0174532925); нам не нужна супер-точность

    unsigned long dstlat = labs(p1->lat - p2->lat) / 100;
    unsigned long dstlon = labs(p1->lon - p2->lon) / 100 /* * scaleLongDown */;
//    return sqrt(sq(dstlat) + sq(dstlon)) * 111319.5 / MULTIPLIER; нам не нужно точное расстояние 
//    return approx_distance(dstlat, dstlon);
    return sqrt32(dstlat*dstlat + dstlon*dstlon);
}


#if defined(USE_GSM)
void sendCoordsSms(bool fChute){
    if(lflags.gsm_ok && ! lflags.smsSent)  { // только 1 раз
	bs.begin((char *)buf);

	if(lflags.wasCrash) bs.println_P(PSTR("Crash!"));
	if(fChute)          bs.println_P(PSTR("Chute!"));

	bs.print(p.url);		    // форматировать координаты под ссылку на карты гугля
	bs.println(messageBuff);
    
	gsm.set_sleep(false); // leave sleep mode
	bool fSent=false;
	fSent = gsm.sendSMS(p.phone1, (char *)buf);
	fSent = gsm.sendSMS(p.phone2, (char *)buf) || fSent;
	fSent = gsm.sendSMS(p.phone3, (char *)buf) || fSent;
	fSent = gsm.sendSMS(p.phone4, (char *)buf) || fSent;

	if(fSent) {// куда-то отправили
	    lflags.smsSent = true;
	}
    }
}
void sendCoordsSms(){
    sendCoordsSms(false);
}

#endif

void doOnDisconnect() {// отработать потерю связи

    if(!badCoord(&home_coord)) {
	if(distance(&home_coord, &coord) < p.MIN_HOME_DISTANCE) {
	    goto exit; // слишком близко от дома
	}
    }


    if(p.DeadTimer)			// если таймерный маяк разрешен
	nextSearchTime = uptime;	// то сразу же включим таймерный маяк

#if defined(USE_GSM)
    sendCoordsSms();

exit:
    gsm.end(); // GSM сделал свое дело - закончим работу и выключим питание
#else
exit:
#endif
    power_timer1_disable(); // turn off timer 1
}


#if defined(USE_GSM)
void readGSM() {	// трансляция всего с GSM
    while(gsm.available_S()) serial.write_S(gsm.read_S());
}
#else
void readGSM() {}
#endif


inline void printAllParams(){

    serial.print_P(PSTR("[CONFIG]\n"));


    for(byte i=0; i< PARAMS_END ;i++){
	print_SNS(PSTR("R"),i,PSTR("="));
	serial.println(((long *)&p)[i]);
    }
    for(byte i=0; i<sizeof(strParam)/sizeof(StrParam);i++){
	print_SNS(PSTR("S"),i+PARAMS_END,PSTR("="));
	serial.println((char *)pgm_read_word(&(strParam[i].ptr)));
    }

    serial.print_P(PSTR(">\n"));
}

static const char PROGMEM beacon_patt[] = "cBeacon ";

void consoleCommands(){
        struct Params loc_p;
    
        loc_p=p;

        Green_LED_ON;
        Red_LED_ON;
    
#if !defined(DEBUG)
        serial.begin(TELEMETRY_SPEED);
#endif


        serial.print_P(beacon_patt);
        serial.print_P(PSTR(TO_STRING(RELEASE_NUM)));
        serial.println();

        byte try_count=0;

//DBG_PRINTLN("console");

        while(1) {
            byte cnt=0;
            for(unsigned long t=millis()+1000; millis() < t;){
        	if(serial.available_S()) {
		    byte c=serial.read_S();

//		if(strncasecmp_P( buf, pat, sizeof(pat) )==0){

	            if(cnt>=(sizeof(beacon_patt)-2)) break;
		    byte p = pgm_read_byte(&beacon_patt[cnt]);
	            if(c != p ) {
	                cnt=0;
//DBG_PRINTLN("skip");
//DBG_PRINTVARLN(c);
//DBG_PRINTVARLN(p);
	                continue;
	            }
	            cnt++;
	        }
	    }

//DBG_PRINTLN("3s done");
//DBG_PRINTVARLN(cnt);
    
	    if(cnt == sizeof(beacon_patt)-2){
//DBG_PRINTLN("console OK");
	        if(!is_eeprom_valid()) {
//	            serial.print_P(PSTR("CRC!\n"));
	            write_Params_ram_to_EEPROM();
	        }

	        Read_EEPROM_Config();

	        while(true){
		    printAllParams();

		    getSerialLine(buf);

		    if(buf[0] && !buf[1]) { // one char command

//DBG_PRINTLN("command");
//DBG_PRINTVARLN(buf[0]);
		        switch(buf[0]){
			case 'd':
			    p=loc_p; // восстановить параметры

//			    for(int i=4; i>=0; i--)    // испортить CRC
//			    for(unsigned int i=E2END; i>=0; i--)// стереть весь EEPROM
//				 eeprom_write(i,0xFF); 
			// no break!

			case 'w':
			    write_Params_ram_to_EEPROM();
			    break;
			    
			case 'b':
			    beepOnBuzzer(1000);
			    break;
			     
			case 'c':
			    print_SNS(PSTR(" "), calibrate(), PSTR("\r\n"));
			    break;
			    
			case 't':
			    uploadTrackPoints();
			    break;
			    
			case 'z':	// тест генератора речи на пищалку
			    voiceOnBuzzer = true; 
			// no break!!!
			case 's': // say digits
			    static const PROGMEM char v_patt[]="0123456789:#. *";
			    strcpy_P((char *)buf, v_patt);
			    sendVOICE((char *)buf,  0);
			    voiceOnBuzzer = false;
			    break;

#if defined(USE_DTMF)
			case 'l':
			    voiceOnBuzzer = true;
			case 'f':
			    //static const PROGMEM char d_patt[]="0123456789#*";
			    strcpy_P((char *)buf, v_patt);
			    sendDTMF((char *)buf, 2);
			    voiceOnBuzzer = false;
			    break;
#endif

#if defined(USE_MORZE)
			case 'v':
			    static const PROGMEM char m_patt[]=" v" TO_STRING(RELEASE_NUM) " ";
			    strcpy_P((char *)buf, beacon_patt);
			    strcat_P((char *)buf, m_patt); 
			    strcat_P((char *)buf, v_patt); 
			    morze.write((char *)buf);
			    morze.flush();
			    break;
#endif

//			case 'p':
//			    printAllParams();
//			    break;
			
			case 'q':
			    //return;
			    goto console_done;
		    
#if defined(USE_GSM)
			case 'm': // get balance
//DBG_PRINTLN("balance ");
			    byte n;
			    *buf=0;
			    if(gsm.begin()) {
	   			gsm.sendUSSD(100);
DBG_PRINTLN("result = ");
DBG_PRINTVARLN((char *)buf);
			
    			        // in buffer
    			        serial.print((char *)buf);
    			    } else 
    				print_SNS(PSTR("ERROR="),gsm.lastError,PSTR("\n"));
			    break;
			    
			case 'g': // прямая связь с модемом
			    gsm.begin();
//DBG_PRINTLN("terminal mode");   
			    do {
				getSerialLine(buf, readGSM); // считать строку, во время ожидания выводим все с GSM в линию
				if(*buf =='.' && buf[1]==0) break; // выход по точке
				gsm.println((char *)buf);
			    } while(*buf ); // выход по пустой строке
			    
//			    gsm.end();	// всяко это потребуется только дома у компа, а значит потом передернут питание
//DBG_PRINTLN("Exit terminal mode");
			    gsm.pulseDTR();
			    break;

 #if 1 // debug and testing only

			case 'e': 	// send SMS
			    if(gsm.begin()) {
				bs.begin((char *)buf);

				bs.println_P(PSTR("SMS test"));
				bs.println(gsm.getRSSI());

				//bs.print(p.url);		    // форматировать координаты под ссылку на карты гугля
				//bs.println(messageBuff);
    
				byte fSent = gsm.sendSMS(p.phone1, (char *)buf);
				if(fSent)
				    serial.println_P(PSTR("Sent OK"));
    			    } else 
    				print_SNS(PSTR("ERROR="),gsm.lastError,PSTR("\n"));
			    break;
 #endif
#endif

			}
		    } else { // new param value
			byte *bp=buf+1;

//DBG_PRINTLN("set val");
//DBG_PRINTVARLN((char *)bp);

		        byte n=atol((char *)bp); // зачем еще и atoi тaщить
//DBG_PRINTVARLN(n);
//		        if(n > sizeof(struct Params)/sizeof(long)) {
		        if(n > PARAMS_END + sizeof(strParam)/sizeof(StrParam)) {
			    break;
			}
		    
			PGM_P fmt;
			if(n<PARAMS_END)
			    fmt=PSTR("R");
			else
			    fmt=PSTR("S");
			
			while(*bp) {
			    if(*bp++ == '=') break;
			}
		
			if(n<PARAMS_END) {	// numeric prams
		    	    if(*bp) 
		                ((long *)&p )[n] = atol((char *)bp); // если не пустая строка то преобразовать и занести в численный параметр
//DBG_PRINTLN("new n val");
//DBG_PRINTVARLN(((long *)&p )[n]);

		        } else  {		// string params
		    	    n-=PARAMS_END;
		    	    if(n<sizeof(strParam)/sizeof(StrParam)) { 
		    		const StrParam *sp = &strParam[n];
				strncpy((char *)pgm_read_word(&(sp->ptr)), (char *)bp, pgm_read_byte(&(sp->length))-1);	// занести как есть в строковый параметр
//DBG_PRINTLN("new s val");
//DBG_PRINTVARLN((char *)buf);
//DBG_PRINTVARLN(s.length);
			    }
/* else {
    DBG_PRINTLN("skip s val");
    DBG_PRINTVARLN(n);
}*/

			}
		    }
		}
            } else {
	        if(is_eeprom_valid()){
		    Read_EEPROM_Config();

//    DBG_PRINTLN("load EEPROM OK");
		    break;
	        }
		if(try_count>=20) {
		    write_Params_ram_to_EEPROM();
		    break;
		}
//   DBG_PRINTLN("Wait command EEPROM bad");
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

console_done:
	Green_LED_OFF;
	Red_LED_OFF;
#if !defined(DEBUG) 
	serial.end();
#endif
}

void green_blink(){
    Green_LED_ON;
    delay_1();
    Green_LED_OFF;
}

void setup(void) {
    wdt_disable();
 
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

#if defined(STROBE_PIN)
    pinMode(STROBE_PIN, OUTPUT);   //nSEL
    digitalWrite(STROBE_PIN,LOW);
#endif

#if defined(USE_GSM)
    gsm.initGSM();  // настроить ноги к GSM модулю
#endif

    // wiring настраивает таймер в режим 3 (FastPWM), в котором регистры компаратора буферизованы. Выключим, пусть будет NORMAL
    TCCR0A &= ~( (1<<WGM01) | (1<<WGM00) );


//    attachInterrupt(IRQ_interrupt, RFM22B_Int, FALLING); // ISR(INT0_vect) - ~150 bytes
//*
#if IRQ_interrupt == 0
    EICRA = (EICRA & ~((1 << ISC00) | (1 << ISC01))) | (FALLING << ISC00);
    EIMSK |= (1 << INT0);
#else
    EICRA = (EICRA & ~((1 << ISC10) | (1 << ISC11))) | (FALLING << ISC10);
    EIMSK |= (1 << INT1);
#endif
//*/
    sei();

#if defined(DEBUG)
    serial.begin(TELEMETRY_SPEED);
#endif  

    RFM_off(1);

    initBuzzer();

    deepSleep(50); // пусть откалибруется

//    beepOnBuzzer_183();
    beepOnBuzzer_333();

    uint16_t vcc=readVCC();

// DBG_PRINTVARLN(vcc);

    if(vcc < VCC_LOW_THRESHOLD) {

//DBG_PRINTLN("low VCC");

	while((vcc=readVCC())<3300){
//	      DBG_PRINTLN("low VCC");
	    redBlink(); //delay_1();
	    deepSleep_450();
	}
    }

    consoleCommands(); // оно и EEPROM считает

    byte sts=0;

#if defined(USE_GSM) //init & check GSM
// GSM жрет как не в себя и должен быть проверен и усыплен как можно быстрее

    if(getExtVoltage() > 10) { // проверим наличие питания - GSM инициализируем только на внешнем
	if(gsm.begin()){ // удалось инициализировать и зарегиться в сети - а вдруг СИМ-карта дохлая?

DBG_PRINTLN("GSM init OK");
	    int bal=gsm.balance();

	    if( bal == 0 ) { // не удалось запросить
		sts=2; 
		lflags.gsm_ok=true; // но можно попытаться
DBG_PRINTLN("balance fail");
	    } 
	    else if( bal > 2) {
DBG_PRINTLN("GSM OK");
		lflags.gsm_ok=true; // денег хватит на СМС
	    } else sts=3;		//баланс отрицательный
	} else sts=1; // не удалось инициализировать GSM

	if(lflags.gsm_ok) gsm.set_sleep(true); // sleep mode
	else		  gsm.end();		// turn off
    } else { // питания нет - например был краш и маяк обесточивался, просто выключим модуль чтоб не жрал
	gsm.turnOff();
    }
#endif

//delay(30000);

#define INITIAL_UPTIME 1

    uptime = INITIAL_UPTIME;
  
    if(p.DeadTimer==0){
        nextSearchTime-=1; // навсегда 
    } else
	nextSearchTime = p.DeadTimer + INITIAL_UPTIME; // uptume == 1 !
  
    NextListenTime =  (byte)p.ListenInterval + INITIAL_UPTIME;
  
    nextNmeaTime   =  (byte)p.GPS_MonitInterval + INITIAL_UPTIME;
    
    if((uint16_t)p.IntLimit == (uint16_t)p.IntStart){
	growTimeInSeconds =  p.SearchTime * 60;
    }else{
	growTimeInSeconds = (p.SearchTime * 60 ) / ((uint16_t)p.IntLimit - (uint16_t)p.IntStart);
    }
  
//  if( p.MorzeSign1 |  p.MorzeSign2)   не нада нам морзе
//	sendMorze();
//    else

//{    beaconHorn(); 
    RFM_tx_min(); // RFM_SetPower(RF22B_PWRSTATE_TX, RFM_MIN_POWER ); 
    for(byte i=3;i>0;i--) {
        Red_LED_ON
        beacon_tone_150(400);
        Red_LED_OFF
        beacon_tone_150(800);
    }
//}
#if defined(USE_GSM) //report GSM error:
// 1 - init failed
// 2 - can't ask balance
// 3 - negative balance 

    delay_300();

    for(byte i=sts;i>0;i--) {
        beacon_tone_150(1750);
        delay_100();
    }

#endif
    RFM_off();

    if(vcc){
//	if(vcc>4200) vcc=4200; будем честными, можно и не от батареи работать

//	voiceOnBuzzer = true; // зачем при включении говорить по радио? - для проверки!
	sayVoltage(vcc/100,0);
//	voiceOnBuzzer = false;
    }


//    if((byte)p.EEPROM_SaveFreq){ // ну не должна базовая функция зависеть от настройки, тем более что 
		    //		 сохранение точек в виде трека осуществляет wear leveling для EEPROM
		    //           100 000 перезаписей одной ячейки раз в секунду = 27 часов непрерывной работы, а тут нагрузка по записи
		    //           размазывается на ~ 90 точек, ~ 100 часов непрерывной работы маяка. Пусть полет это 20 минут - ресурс маяка получается
		    //           300 полетов, или около года работы и полетов каждый день.
		    //           А потом можно и Атмегу перепаять :)
	if(getLastTrackPoint()) {
DBG_PRINTLN("found EE coords");
	    formatGPS_coords();
DBG_PRINTVARLN(messageBuff);
	}
//    }


    ADCSRA &= ~(1<<ADEN); 	//Disable ADC
    ACSR = (1<<ACD); 		//Disable the analog comparator
//  DIDR0 = 0x3F; 		//Disable digital input buffers on all ADC0-ADC5 pins
    DIDR1 = (1<<AIN1D)|(1<<AIN0D); //Disable digital input buffer on AIN1/0

    power_twi_disable();
    power_spi_disable();
#if ! defined(USE_GSM)	 /* отключим после окончания работы с GSM */
    power_timer1_disable();
#endif
    power_timer2_disable();
    // power_adc_disable() ADC нам нужен

    wdt_reset();
    wdt_enable(WDTO_8S);
}


// функции для снижения размера кода. 
// Работа с LONG дается процессору очень тяжело, а работать надо много.
// вызов функции с передачей адресов 2х переменных гораздо компактнее, а скорость нас не интересует

bool NOINLINE diff_more( uint32_t& l2, uint16_t d){
    return l2 && (uptime-l2) > d;
}

void NOINLINE nextTime(uint32_t& dst, uint16_t add){
    dst = uptime + add;
}

bool NOINLINE nowIsTime(uint32_t& t){
    return  uptime >= t;
}

void loop(void) {

    unsigned long timeStart=millis() + sleepTimeCounter; // время бодрствования плюс время сна - запомним время входа в цикл
//    unsigned int frac;

    wdt_enable(WDTO_8S);

    {
	uint32_t tc = timeStart - timeStart / 1000 * p.TimerCorrection;
	uptime = tc / 1000;
//	frac = tc % 1000;
    }
   
//   if(uptime < 60 * 60 * 8) {// 8 часов
    if(uptime < 60 * 10) {// 10 минут, только в начале полета
	green_blink();
    }
    

// благодаря deepSleep в конце loop исполняется в батарейном режиме не чаще раза в секунду, если не слушаем MAVlink

    vExt = getExtVoltage(); // читаем всегда дабы знать о наличии питания

//    DBG_PRINTVARLN(vExt);
    
    // TODO - через MAVlink идет напряжение замеренное контроллером, использовать для (само)проверки?
    if(vExt<=10) { // <1v питания нет
	lflags.hasPower = false;

	if(lflags.lastPowerState) { // только что было
//   DBG_PRINTLN("no vExt");
	    if( lflags.wasPower 		// маяк могут включить раньше коптера - не паникуем раньше времени
	     && diff_more( powerTime, 60)   // питание было подано больше минуты
	     && lflags.motor_armed		// и были заармлены моторы
	     && diff_more( armTime, 30) ) { 	// причем более полминуты
	 
				     // случилось страшное
	        lflags.connected = false;     // соединения нет
//     DBG_PRINTLN("VCC gone");
	        
	        if(lflags.pointDirty){
	            SaveGPSPoint(); // сохранять последнюю найденную точку по обрыву питания
	        }
	
	        disconnectTime = uptime; // пошло время от отсоединения
	    } else { // нестрашное пропадание питания - сбросим флаг
	        lflags.wasPower = false;
//     DBG_PRINTLN("VCC ignored lost");
	    }
	}

	// пищим раз в полчаса если на батарее
	// если не спим то пищит многовато - ну значит не будем пищать на внешнем питании
	if(uptime % 1800 == 0 /* && frac<500*/ ) { // каждые полчаса
	    beepOnBuzzer_183();
	    beepOnBuzzer_183();
	}

    } else { // питалово есть
	lflags.hasPower = true;

	if(!lflags.wasPower) {
	    powerTime=uptime;		// запомнили время включения питания
	    lflags.wasPower = true;
	}
	if(!lflags.lastPowerState) { // включение
	    disconnectTime=0;  // если выключили и включили питание - дисконнекта нет
// DBG_PRINTLN("VCC up");
//DBG_PRINTVARLN(vExt);
	}
    }

    lflags.lastPowerState = lflags.hasPower;


// вобщем-то секунда тут конфликтует со временем прослушивания, логичнее было б иметь критерий
// типа "два периода подряд ничего не было"
// НО
// в режиме связи с коптером у нас есть питание и слушаем всегда, а пропадание одного питания достаточно
// чтобы бить тревогу, МАВлинк контролируется на случай зависания автопилота

    if(lflags.data_link_active && (millis() - lastMAVBeat) > 500) { // был но пропал Мавлинк и 0.5сек нету
	if(lflags.motor_armed) { 	// на ходу пропал - 
	    lflags.connected = false;     // соединения нет
	    disconnectTime = uptime; // пошло время от отсоединения

DBG_PRINTLN("MAVLINK gone");

	    if(lflags.pointDirty){
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

	    baro_alt_start = mav_baro_alt; // запомнить давление при старте для контроля парашюта
	    control_loss_count=0;  // reset
DBG_PRINTLN("Armed");
        }

	if(lflags.crash){	// поступило сообщение "Crash: Disarming"
DBG_PRINTLN("Crash");
	    lflags.connected = false;     // соединения нет
	    lflags.wasCrash=true;	// запомним
	    lflags.crash=false;           // обработано
	    doOnDisconnect();             // отработать окончательную потерю связи
	}
/* проверяется после каждого пакета
	if(chuteTime<timeStart) { //проверка парашюта
	    chute_check();
	    chuteTime = timeStart + CHUTE_INTERVAL;
	}
*/
    } else { // 		дизарм
	if(lflags.motor_was_armed){	// моторы были заармлены 
	    if( !diff_more( armTime, 30) ) // если меньше полуминуты то не считается
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
//    if(disarmTime && (uptime - disarmTime) > DISARM_DELAY_TIME  ) { // дизарм, и за 2 минуты ничего не произошло
    if( diff_more( disarmTime,  p.DISARM_DELAY_TIME)  ) { // дизарм, и за 2 минуты ничего не произошло
	    lflags.connected = false;     		// будем считать что соединения нет
	    disconnectTime = uptime; // пошло время от отсоединения
	    disarmTime=0;

DBG_PRINTLN("disarm disconnect");
    }


// предусмотрим передергивание питания коптера без отключения питания маяка
//    if(disconnectTime && (uptime - disconnectTime) > 60 ) { // ничего не произошло минуту после потери связи
    if( diff_more( disconnectTime, 60) ) { // ничего не произошло минуту после потери связи
	    doOnDisconnect();// отработать окончательную потерю связи - CMC с координатами

DBG_PRINTLN("disconnect lock");
	    disconnectTime=0;
    }

// теперь у нас есть флаг, показывающий стОит маяку шевелиться или нет


// контроль  батареи
#if 0 /* а телеметрия на что? */
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
		unsigned int v10 = vExt * 10 / NumberOfCells; // чуть увеличим точность

		if( cell_voltage ) {
		    cell_voltage = (v10 +  cell_voltage*4) / 5; // комплиментарный фильтр
		} else
		    cell_voltage = v10;

		unsigned int v   = cell_voltage / 10; 
		    
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
		
			sayVoltage(v,3); // вообще-то включать передатчик на ходу не лучшая идея...
			
			beepOnBuzzer_333();
		    }
		}
		
		byte chk_v2 = NextVoltageTreshhold / 256; // старший байт - второй порог
		
		if(v < chk_v2  || NextVoltageTreshhold ==35){
		    if(NextVoltageTreshhold != 35)
			powerCheckDelay = 0;
			
		    if(!powerCheckDelay){
			if(v < chk_v2)
			    sayVoltage(v, 1);
			else 
			    powerCheckDelay=14;
		    } else 
			powerCheckDelay-=1;
		
		}
	    }
	}
    } // p.ExtVoltageWarn
#endif


// --------------- GSM маяк ----------------------

/* -- а надо ли?

    нужен аналогичный период включений для прослушки и получения SMS, например 4 раза каждые полчаса
    но только при наличии свежих координат

    при получении SMS - разобрать и отправить координаты либо исполнить другую команду

    но пока мы отправляем SMS ТОЛЬКО один раз при потере связи с контроллером
*/


//---------- таймерный маяк  -------------------------------------------------------------
    if( searchBeginTime == 0 ) {
        if( nowIsTime(nextSearchTime) /* uptime >= nextSearchTime */ ) { // проверяем условия запуска

DBG_PRINTLN("timed beacon init");

          // Начало поискового периода
          searchBeginTime = uptime; // запомнить время начала
          nextBeepTime = uptime; // прямо сразу же
      
          beaconInterval =  p.IntStart;
          nextTime(timeToGrow, growTimeInSeconds); //  timeToGrow = uptime + growTimeInSeconds;
          nextTime(searchStopTime, p.SearchTime * 60); //  searchStopTime  = uptime + p.SearchTime * 60;
          nextSearchTime = searchStopTime + p.Sleep * 60;
        }
    } else { // searchBeginTime != 0 - мы в активном поиске
    
    
        if( nowIsTime(nextBeepTime) /* uptime >= nextBeepTime */ ) {

//	    DBG_PRINTLN("nextBeepTime");

	    Beacon_and_Voice();
	    
            nextTime(nextBeepTime, BEACON_BEEP_DURATION * 3 / 1000 + beaconInterval);  //nextBeepTime = uptime + BEACON_BEEP_DURATION * 3 / 1000 + beaconInterval;
          
            // В конце посылки послушаем эфир
#if USE_MORZE
	    if(morze.gotCall() || morze.available() && one_listen() ) {   // Поймали вызов при передаче или не передаем и есть вызов
#else
            if(one_listen()) {   // Есть вызов
#endif
//   DBG_PRINTLN("Call is detected")
                redBlink();// помигать диодом
                
                if( Got_RSSI > p.MinAuxRSSI) {
            	    buzzer_SOS();
                } 
        // тут маяк неадекватно реагирует на вызов в разных режимах - а надо чтобы одинаково, это более ожидаемо
    		if(p.flags.f.timeGPSvoice || p.flags.f.timeGPS_DTMF || p.flags.f.timeGPS_MORZE ) // time flags
        	    sayCoords();
            }
        }
    
        if( nowIsTime(timeToGrow) /* uptime >= timeToGrow */ ) {
          // Увеличение интервала между посылками
// DBG_PRINTLN("Interval grow");
          nextTime(timeToGrow,growTimeInSeconds - (timeToGrow - uptime));//timeToGrow = uptime - (timeToGrow - uptime) + growTimeInSeconds;
          beaconInterval ++;
        }
    
        if( nowIsTime(searchStopTime) /* uptime >= searchStopTime*/ ) {
          // Окончание периода работы маяка
// DBG_PRINTLN("Search time end");
          searchBeginTime = 0;
        }
    } // searchBeginTime == 0
 


// -----------  прием координат -----------------------------------------------------------------------------------
    if(lflags.listenGPS) { // если слушаем
    
	if(getData()) { // по получению координат

DBG_PRINTLN("MAVLINK coords");

	    if(badCoord(&coord) ) { // not found
		GpsErrorCount +=1;
	    } else {

		if(mav_satellites_visible >  (byte)p.GPS_SatTreshHold && mav_fix_type>=2) { // 2d or 3d fix
		    GpsErrorCount = 0;

		    if(lastPointTime) { // есть предыдущая точка
			if( (millis() - lastPointTime)>=1000) {      // регистрируем точки не чаще одного раза в секунду
			    // посчитать расстояние между двумя точками и отбросить если изменилось меньше 3 метров
			    // переменные point1 point2 используются только при старте - в поиске последней актуальной точки, и 
			    // больше не нужны
			    if(distance(&coord,&point2)>p.LAST_POINT_DISTANCE) { // сдвинулись достаточно от последней точки
		                gps_points_count++; 
			        //lastPointTime = millis(); // время последней обработанной точки
			        millis_plus(&lastPointTime, 0);
				point2=coord; // сама предыдущая точка

				//при 0 вообше не сохраняем, засада :( - починено, приравняно к 1
				byte sf = (byte)p.EEPROM_SaveFreq;
		                if(lflags.motor_was_armed){ // какой смысл портить еепром координатами стоянки?
			            if(sf == 0 || gps_points_count % sf == 0) {
    DBG_PRINTLN("save coord");
			                SaveGPSPoint(); 
		                    }
		                }

			/* если в настройках указано передавать трек - передать точку через GSM/GPRS
				point1 - последняя точка ушедшая по GSM
				
				вот только надо ли? У нас есть телеметрия и на пульте и на радиомодемах, а мощный 
				GSM передатчик на борту не сильно полезен остальной электронике
			*/
			
			    } else {
				// отбросили точку по расстоянию
    DBG_PRINTLN("skip coord");
			    }
			}
		    } else { // первая точка. Обычно она на земле, поэтому особо не интересна, поэтому не сохраняем
    DBG_PRINTLN("coord first point");
			//lastPointTime = millis(); // время последней обработанной точки
			millis_plus(&lastPointTime, 0);
			point2=coord; // сама точка, от нее будем считать сдвиг
		    }

		    if( GPS_data_fresh  && ! GPS_data_OK){ // сообщаем координаты в эфир по первому фиксу
		        if(lflags.motor_armed)  // если успели взлететь
			    voiceOnBuzzer=true; // скажем на пищалку а не в эфир
			sayCoords(true);
			voiceOnBuzzer=false;
		        GPS_data_OK = 1;
		    }

		}
	    }
	}

	// при включенном питании мы не обновляем gpsOffTime, поэтому при выключении питания
	// оно будет сильно просроченным и прием выключится сразу же
	if(gpsOffTime<millis() && !lflags.hasPower){ // пора выключить
	    lflags.listenGPS = false;
	    nextTime(nextNmeaTime,(uint16_t)p.GPS_MonitInterval);   //nextNmeaTime = uptime + p.GPS_MonitInterval;

//	    DBG_PRINTLN("listen time over");

#if !defined(DEBUG)
	    serial.end();
	    power_usart0_disable();
#endif
	}
    } else { // не слушаем
  
	if( lflags.hasPower || nowIsTime(nextNmeaTime) /* uptime >= nextNmeaTime */ ) {  // а не пора ли включить?
	
	    if( lflags.hasPower || GpsErrorCount <  (uint16_t)p.GPS_ErrorTresh )  {

//		DBG_PRINTLN("time to listen");

#if !defined(DEBUG)
		power_usart0_enable();
		serial.begin(TELEMETRY_SPEED);
#endif
		lflags.listenGPS = true; // включим прослушивание;
		//gpsOffTime = millis() + p.GPS_MonDuration; // время выключения
		millis_plus(&gpsOffTime, p.GPS_MonDuration);

		coord = bad_coord; // сбросим ранее принятые данные
		mav_satellites_visible=0;

	    
	    } else {					// по превышению числа ошибок
		nextTime(nextNmeaTime,(uint16_t)p.GPS_MonitInterval * 100);//nextNmeaTime = uptime + p.GPS_MonitInterval * 100; // в 100 раз реже
		GpsErrorCount -=1;
	    }
	}
    } // serial listen


#if USE_MORZE
    if(morze.gotCall() ) {   // Поймали вызов при передаче 
	goto morzeGotCall;
    }
#endif

  
    // voice listen - независимо от состояния подключения, а то вдруг ошибочка вышла
    if( nowIsTime(NextListenTime) /* uptime>NextListenTime */ ) {
	redBlink(); 

//	DBG_PRINTLN("time to receive");

#if USE_MORZE
	if( morze.available() && one_listen() ) {   // не передаем и есть вызов
#else
//	if(one_listen() > 5) {
	if(one_listen()) { // приняли ЛЮБОЙ вызов
#endif
morzeGotCall:
	    for(byte i=5; i>0; i--) {
		Green_LED_ON;
	        redBlink(); 
	        Green_LED_OFF; // 5 мыргов синхронно обоими диодами
	        delay_10();
	    }
//	    DBG_PRINTLN("proximity");

	    proximity(Got_RSSI);
	}
	nextTime(NextListenTime, (uint16_t)p.ListenInterval);  //NextListenTime = uptime + p.ListenInterval;
    }

#if USE_MORZE
    if(!lflags.hasPower && !lflags.listenGPS && morze.available()) { // если на батарее, не слушаем GPS и не передаем морзе то спим до конца секунды
#else
    if(!lflags.hasPower && !lflags.listenGPS) { 		     // если на батарее n не слушаем GPS то спим до конца секунды
#endif

#ifdef DEBUG
	delay_50(); // чтобы порт успел передать до засыпания все что накопилось
#endif

	// время от входа в цикл
        unsigned  long loopTime = (millis() + sleepTimeCounter) - timeStart; // + TIMER_CORRECTION;
//        deepSleep(1000L - loopTime % 1000 ); // Sleep till the end of a second
	if(loopTime < 1000) deepSleep(1000L - loopTime );  // sleep to full second
    }

/* наф эти вычисления - при следующем входе в LOOP uptime пересчитается по факту
  if(timeNow -  timeStart < 1000) {
    uptime++;
  } else {
    uptime += 1 + ( timeNow -  timeStart ) / 1000;
  }
*/

}

