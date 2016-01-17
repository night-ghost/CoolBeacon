/*

    Нативная библиотека GSM чудовищна и огромна, а  нам много не надо

    к тому же нога прерывания, нужная для работы SoftwareSerial, занята


Отправка данных на сервер (GSM/GPRS shield)
Подключаемся к Arduino к контактам 7 и 8. 


http://cxem.net/arduino/arduino170.php
http://arduino.ua/ru/guide/ArduinoGSMShield

*/

// printf without float
#define SKIP_FLOAT

#define RX_BUFFER_SIZE 64 // AltSoftSerial buffer
//#define SERIAL_TX_BUFFER_SIZE 4 //SingleSerial buffer

#include "compat.h"
#include <SingleSerial.h>
#include <AltSoftSerial.h>
#include <avr/power.h>

extern SingleSerial serial;




#include "Arduino.h"// Get the common arduino functions

#include "config.h"
#include "gsm.h"

extern GSM gsm;
extern void delay_300();
extern void delay_100();
extern void delay_50();
extern void delay_10();
extern void delay_1();

char GSM::response[RESPONCE_LENGTH];

static const char PROGMEM s_cpin_q[]="+CPIN?";
static const char PROGMEM s_creg_q[]="+CREG?";
static const char PROGMEM s_cpin_a[]="CPIN: READY";
static const char PROGMEM s_creg_a[]="CREG: 0,1";

static const char PROGMEM s_ok[]="OK";

GSM::GSM(){

}


byte GSM::_available(){
    return gsm.available();
}
byte GSM::_read(void){
    return gsm.read();
}
byte GSM::_write(uint8_t c){
    return gsm.write(c);
}

void delay_1000(){
    delay(1000);
}

bool  GSM::begin(){
    power_timer1_enable();
    AltSoftSerial::begin(GSM_SPEED);

    digitalWrite(GSM_EN,LOW);
    delay_300();
    digitalWrite(GSM_EN,HIGH);
    delay_1000();

    for(byte i=15;i!=0; i--)
	if(GSM::command(PSTR(""), 1000) ) break;

            GSM::command(PSTR("E0")); // ECHO off
//            GSM::command(PSTR("+CNETLIGHT=0")); // LED off
            /*GSM::command(PSTR("+GSMBUSY=1")) && */ // not receive VOICE calls
    return  GSM::command(s_cpin_q,s_cpin_a) && // SIM ok?
	    GSM::command(s_creg_q,s_creg_a) &&   // NET registered?
	    GSM::command(PSTR("+CCALR?"),PSTR("CCALR: 1"));   // CALL enabled?
}


// закончили работу с GSM - выключим
void GSM::end() {
    GSM::command(PSTR("+CPOWD=1"),PSTR("DOWN")); //power down

    AltSoftSerial::end();

    power_timer1_disable();
}



void GSM::initGSM(void){
    pinMode(GSM_EN,OUTPUT);
    digitalWrite(GSM_EN,HIGH);

    pinMode(GSM_TX, OUTPUT);
    digitalWrite(GSM_TX,HIGH);

    pinMode(GSM_DTR, OUTPUT);
    digitalWrite(GSM_DTR,HIGH);
    
    pinMode(GSM_RX, INPUT);
    pinMode(GSM_INT, INPUT_PULLUP);
}


bool GSM::set_sleep(byte mode){
    if(mode)
	return GSM::command(PSTR("+CFUN=0")) // minimum func
	    /*&& GSM::command(PSTR("+CSCLK=1"))*/; //  sleep
    else {
	digitalWrite(GSM_DTR,LOW); // wake up
	delay_10();
	digitalWrite(GSM_DTR,HIGH);

	return GSM::command(PSTR("+CFUN=1")) && // work
	       GSM::command(s_cpin_q,s_cpin_a) && // SIM ok?
	       GSM::command(s_creg_q,s_creg_a);   // NET registered?
    }

}



void GSM::readOut() {
    char c;
    if( gsm._available()) {
//serial.print_P(PSTR("< "));
	while( gsm._available()) {
	    c=gsm._read();    // Clean the input buffer from last answer and unsolicit answers
//serial.print(c);
	    delay_10();
	}
    }

}


// отправка AT-команд
// usage  command(PSTR("+SAPBR=3,1,\"CONTYPE\",\"GPRS\""), [PSTR("OK")], [10000]);
uint8_t GSM::command(const char* cmd, const char* answer, uint16_t time){
    readOut();

//serial.print_P(PSTR("> "));
//serial.print_P(cmd);

    gsm.print_P(PSTR("AT"));    // Send the AT command 
    gsm.println_P(cmd);    // Send the command 

    return wait_answer(answer, time)==1; // 10 sec
}

uint8_t GSM::command(const char* cmd, const char* answer){
    return GSM::command(cmd, answer, 10000);
}

uint8_t GSM::command(const char* cmd){
    return GSM::command(cmd, s_ok);
}

uint8_t GSM::command(const char* cmd, uint16_t time){
    return GSM::command(cmd, s_ok, time);
}

char * result_ptr, * result2_ptr;

uint8_t GSM::wait_answer(const char* answer, const char* answer2, unsigned int timeout){
    char * cp = response;
    unsigned long deadtime = millis() + timeout;
    char has_answer=0;

    delay_100();

    // this loop waits for the answer
    do{
        if(gsm._available()) {    // if there are data in the UART input buffer, reads it and checks for the asnwer
            char c;
            *cp++ = c = gsm._read();
            *cp=0;
//serial.print(c);

	    if(!has_answer) { // пока нет ответа - проверяем
                // check if the desired answer  is in the response of the module
                if((result_ptr=strstr_P(response, answer)) != NULL)  { // окончательный ответ
                    has_answer = 1;
                } else
                if((result_ptr=strstr_P(response, PSTR("ERROR"))) != NULL)  { // окончательная ошибка
                    has_answer = 3;
                }
                if(has_answer){ // ответ только что получен
//serial.println_P(PSTR("="));
		    do {
			while(gsm._available()) {    // if there are data in the UART input buffer, reads it and checks for the asnwer
        		    *cp++ = c = gsm._read();
        		    *cp=0;
//serial.print(c);
			}
			delay_10();
		    } while(gsm._available());
		}
            }
            // TODO: контролировать разбиение на строки
        } else if(has_answer) // если данные кончились и ответ получен - готово
    	    break;
    } while( millis() < deadtime ); // Waits for the asnwer with time out

    if( answer2 !=NULL) {
	result2_ptr=strstr_P(response, answer2); // промежуточный ответ
//serial.println(result2_ptr);
    }
    
//serial.println(" done");

    return has_answer;
}

uint8_t GSM::wait_answer(const char* answer, unsigned int timeout){
    return wait_answer(answer, NULL, timeout);
}


bool GSM::sendSMS(const char * phone, const char * text) {
  gsm.command(PSTR("+CMGF=1")); // text mode
  gsm.command(PSTR("+CMGD=1")); // delete all messages
//  gsm.command(PSTR("+CSMS=1")); // mode 1

  gsm.print_P(PSTR("AT+CMGS=\""));
  gsm.print_P(phone);
  readOut();
  gsm.println_P(PSTR("\""));

  if(!wait_answer(PSTR(">"),3000)) return false;

  gsm.print(text);
  gsm.println('\x1a');

  bool ok = wait_answer(s_ok,PSTR("+CMGS:"),30000) == 1;

  return ok && result2_ptr!=NULL;
}

static const char PROGMEM patt_minus[] = "\x1c\x38\x3d\x43\x41\x3a"; //1c 38 3d 43 41 3a - Минус в UTF без старшего байта


bool GSM::sendUSSD(uint16_t text) {
  readOut();
  gsm.print_P(PSTR("AT+CUSD=1,\"*"));

//serial.print_P(PSTR("USSD"));
  gsm.print(text);
  gsm.println_P(PSTR("#\""));

  static const char PROGMEM patt[]="+CUSD:";

    if(!wait_answer(patt,15000)) {
//serial.print_P(PSTR("No ans"));
	return false;
    }
  
  delay_300();
  
  byte cnt=0;
  bool fIn=false;
  uint16_t num=0;
  //  0, "002D0037002E003000320440002E003004310430043B002E041D04300431043504400438044204350020002A00310030003600230020041E0431043504490430043D043D044B04390020043F043B0430044204350436002E00200418043D0444043E00200030003000300036", 72
  char *bp;
  byte pp = 0;
  
  bp = (char *)buf;
  for(char *cp = result_ptr + sizeof(patt);;){
    char c=*cp++;
    
    if(!c) break;	// end of line
    if(c=='"'){
//serial.print_P(PSTR("kaw"));
//serial.println(cp);

	if(fIn) break; // внутри строки - строка кончилась
	else
	    fIn=true;
    }else{
	if(!fIn) continue; // пропускаем все перед кавычкой
	
	num <<= 4;
	if(c>0x60) c-=0x20;
	c-='0';
	if(c>9) c+='0' - 'A' + 10;
	num += c;
	
	if(++cnt==4) {// конец
//serial.print(num,16);
//serial.println();
//serial.println(cp);
// 1c 38 3d 43 41 3a - Минус в UTF без старшего байта

	    cnt=0;
	    c= (byte)num;
	    if(c == patt_minus[pp]) {
		pp++;
	    } else if(pp==sizeof(patt_minus)-1) { // при совпадении запишем минус
		*bp++='-';
		*bp=0;
	    } else 
		pp=0; // reset on difference
	
	    if(num<0x100){  // skip all unicode
		*bp++=c;
		*bp=0;
	    }
	    num=0;
	}
    }
  }
  
    if(!fIn) return false;
//serial.println((char *)buf);

    return true;
}


int GSM::balance(){
    if(!GSM::sendUSSD(100)) {
	return 0; //
    }
    
    // in Buf
    
    char data[14];
    char *cp=data;
    bool fNeg=false;
    char *ptr=(char *)buf;
    
/*    if(strncmp_P((char *)buf,patt,sizeof(patt)-1) == 0) {
//serial.print_P(PSTR("minus"));
	fNeg=true;
	ptr += sizeof(patt)-1;
    }
*/
    for(*cp=0;;){
	char c=*ptr++;
	if(!c) break;
	if(c=='-')   fNeg=true;
	else if(c>='0' && c <= '9') { // digits
	    *cp++ = c;
	    *cp=0;

//serial.println(ptr);
//serial.println(data);

	}
	else if(c=='.' || c==',') break;
	// skip others
    }
//serial.println(data);

    if(!strlen(data)) return 0;

    int v=(int) atol(data);
    if(fNeg) {
	if(v==0) v=-1;
	else
	    v=-v;
    }
    return v;
}

#if 0



void gprs_init() {  //Процедура начальной инициализации GSM модуля
 
 //Установка настроек подключения
     gsm.command("AT+SAPBR=1,1");     //             Открыть несущую (Carrier)
     gsm.command("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""); //тип подключения - GPRS  
     gsm.command("AT+SAPBR=3,1,\"APN\",\"%S\"",APN);
     gsm.command("AT+SAPBR=3,1,\"USER\",\"user\"\r\n");
     gsm.command("AT+SAPBR=3,1,\"PWD\",\"pass\"\r\n");
     gsm.command("AT+SAPBR=1,1\r\n");  // Устанавливаем GPRS соединение
     gsm.command("AT+HTTPINIT\r\n");  //  Инициализация http сервиса
     gsm.command("AT+HTTPPARA=\"CID\",1\r\n");  //Установка CID параметра для http сессии
     gsm.command("AT+HTTPPARA=\"URL\",\"http:/????????.ru/gps_tracker/gps_tracker1.php?id_avto=?N&lat=XXXXXlon=YYYYY\"");    //Собственно URL, после sprintf с координатами
     gsm.command("AT+HTTPACTION=0");    //Запросить данные методом GET
     gsm.command("AT+HTTPREAD");   //дождаться ответа
     gsm.command("AT+HTTPTERM");    //остановить HTTP

}
#endif

/*
Для получения страницы по определенному URL нужно послать следующие команды:
AT+SAPBR=1,1    //Открыть несущую (Carrier)
AT+SAPBR=3,1,"CONTYPE","GPRS"   //тип подключения - GPRS
AT+SAPBR=3,1,"APN","internet.beeline.ru" //APN, для Билайна - internet
AT+HTTPINIT    //Инициализировать HTTP 
AT+HTTPPARA="CID",1    //Carrier ID для использования.
AT+HTTPPARA="URL","http:/????????.ru/gps_tracker/gps_tracker1.php?id_avto=?N&lat=XXXXXlon=YYYYY"    //Собственно URL, после sprintf с координатами
AT+HTTPACTION=0    //Запросить данные методом GET
AT+HTTPREAD   //дождаться ответа
AT+HTTPTERM    //остановить HTTP
*/
