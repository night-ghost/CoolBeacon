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


#define GSM_DEBUG 1
//#define USSD_DEBUG 1

extern GSM gsm;
extern void delay_300();
extern void delay_100();
extern void delay_50();
extern void delay_10();
extern void delay_1();

char GSM::response[RESPONCE_LENGTH];
byte GSM::lastError=0;
byte GSM::isTransparent=0;
byte GSM::isActive=0;

static const char PROGMEM s_cpin_q[]="+CPIN?";
static const char PROGMEM s_creg_q[]="+CREG?";
static const char PROGMEM s_cpin_a[]="CPIN: READY";
static const char PROGMEM s_creg_a[]="CREG: 0,1";

static const char PROGMEM s_ok[]="OK";
char * result_ptr, * result2_ptr;


GSM::GSM(){

}

byte GSM::_write(uint8_t c){
    return gsm.write_S(c);
}

void delay_1000(){
    delay(1000);
}


bool GSM::cregWait(){
    static const PROGMEM char patt_creg[]="CREG: 0,";

    byte nTry=60; // seconds

    while(nTry-- >0){
         byte f= GSM::command(s_creg_q, s_ok, patt_creg);   // NET registered?
	// handle CREG: 0,2

#ifdef GSM_DEBUG
//serial.printf_P(PSTR("#+CREG? result2=%c\n"),*(result2_ptr+sizeof(patt_creg)-1));
#endif	
	if(!f) return false; // command failed

        switch(*(result2_ptr+sizeof(patt_creg)-1)){
    	case '1':	// registered or roaming
    	case '5':
    	     return true; 
        
        case '3':	//denied
            return false; 
            
        case '0':	//  not registered but searching
        case '2':
        default:
    	    delay_1000();// will wait
    	    continue;
        }
        break;// all loop done by continue;
    }
    return false;
}


bool GSM::syncSpeed() {
    for(byte i=25;i!=0; i--)			// speed negotiation
        if(GSM::command(PSTR(""), 200) ) return true;

    return false;
}


void GSM::pulseDTR(){
    digitalWrite(GSM_DTR,LOW); // wake up
    delay_300();
    digitalWrite(GSM_DTR,HIGH);
    delay_100();
}

void GSM::pulseReset(){
    digitalWrite(GSM_EN,LOW);	// this is RESET
    delay_1000();
    digitalWrite(GSM_EN,HIGH);
    delay_1000();

}

bool  GSM::begin(){
    return begin(GSM_SPEED);
}

bool  GSM::begin(long speed){
    if(isActive) return true;

    power_timer1_enable();
    AltSoftSerial::begin(speed);

    byte try_count=10;
    byte f=0;

    pulseReset();

    do {

	if(syncSpeed()){

            command(PSTR("E0 V1 X1 S0=0")); // ECHO off, set error response and do not pickup on ring
            command(PSTR("+CFUN=1")); // work  - +CFUN=1,1 to reset
//	                GSM::command(PSTR("+CNETLIGHT=0")); // LED off
            /*GSM::command(PSTR("+GSMBUSY=1")) && */ // not receive VOICE calls
    
            command(PSTR("+CSCB=1")); // запрет широковещательных уведомлений
                
            f=      command(s_cpin_q,s_cpin_a); // SIM ok?

    	// http://we.easyelectronics.ru/blog/part/369.html
	// AT+CMGF
	// AT+CSCB=1
	// AT+GSMBUSY=1
	// AT+CLIP=1 

	    if(f) f=cregWait();
            if(f) f=command(PSTR("+CCALR?"),PSTR("CCALR: 1"));   // CALL enabled?
        
            if(f) break; // all OK
	} else {
	    pulseDTR();	    	
	    pulseReset();
	}
        command(PSTR("+CFUN=1,1")); // reset
        for(byte i=5;i>0;i--)
            delay_1000();	// delay 5 seconds
        
    } while(try_count-- >0); /// максимальное время ожидания при отсутствии сети - 10 раз по (10 секунд плюс время ожидания команды), около 3 минут

    if(f) {
	isActive=true;
    } else {    	//	так и не удалось разбудить
	AltSoftSerial::end();
        power_timer1_disable();
    }

    return f;
}

void GSM::turnOff(){
    power_timer1_enable();
    AltSoftSerial::begin(GSM_SPEED);
    //digitalWrite(GSM_EN,LOW);
    syncSpeed();
    command(PSTR("E0")); // ECHO off
    end();
}


// закончили работу с GSM - выключим
void GSM::end() {
    command(PSTR("+CNETLIGHT=0"),200); // LED off
    command(PSTR("+CPOWD=1"),PSTR("DOWN"),200); //power down

    AltSoftSerial::end();
    power_timer1_disable();
    isActive=false;

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
    if(mode) {
               command(PSTR("+CNETLIGHT=0")); // LED off
	return command(PSTR("+CFUN=0")) // minimum func
	    /*&& GSM::command(PSTR("+CSCLK=1"))*/; //  sleep - we can't wake up
    } else {
        pulseDTR();

        byte try_count=10;
        do {	       
            bool f = false;
	    if(syncSpeed()){
                   command(PSTR("+CNETLIGHT=1")); // LED on
                f= command(PSTR("+CFUN=1")) && // work - +CFUN=1,1 to reset
                   command(s_cpin_q,s_cpin_a) && // SIM ok?
                   cregWait();   // wait for NET registered
		   
                if(f) return f;
                GSM::command(PSTR("+CFUN=1,1")); // reset
            } else {
                pulseDTR();	    	
		pulseReset();
	    }
            delay_1000();	// delay 1 seconds
	} while(try_count-- > 0);
    }

    return false;
}


#if defined(USE_GPRS)
void GSM::doOnDisconnect(){
    if(GSM::isTransparent) { // были в коннекте и что-то отвалилось
	// надо сбросить модуль и уйти на setup()
	pulseDTR();
	pulseReset();
	GSM::command(PSTR("+CFUN=1,1")); // reset
	__asm__ __volatile__ (    // Jump to RST vector
            "clr r30\n"
            "clr r31\n"
            "ijmp\n"
        );
    }
}
#endif

void GSM::readOut() { // Clean the input buffer from last answer and unsolicit answers
    char c;
    char * cp = response;

    while( gsm.available_S()) {
#ifdef GSM_DEBUG
serial.print_P(PSTR("< "));
#endif
	while( gsm.available()) {
	    c=gsm.read_S();    
#ifdef GSM_DEBUG
serial.print(c);	    
#endif
#if defined(USE_GPRS)
	    *cp++=c;

            if((result_ptr=strstr_P(response, PSTR("DEACT"))) != NULL)  { // окончательная ошибка
                doOnDisconnect();
	    }
	    if(c=='\n') cp = response;
#endif
	}
	delay_10(); // wait for next char
    }

}


// отправка AT-команд
// usage  command(PSTR("+SAPBR=3,1,\"CONTYPE\",\"GPRS\""), [PSTR("OK")], [10000]);
uint8_t GSM::command(const char* cmd, const char* answer, const char* answer2, uint16_t time){
    readOut();

#ifdef GSM_DEBUG
serial.printf_P(PSTR("# want: %S\n"),answer);
serial.print_P(PSTR("#> AT"));
serial.println_P(cmd);
#endif

    gsm.print_P(PSTR("AT"));    // Send the AT command 
    gsm.println_P(cmd);    // Send the command 

    return wait_answer(answer, answer2, time)==1; // 10 sec
}

uint8_t GSM::command(const char* cmd, const char* answer, const char* answer2){
    return GSM::command(cmd, answer, answer2,  10000); // 10s default wait time
}


uint8_t GSM::command(const char* cmd, const char* answer, uint16_t time){
    return GSM::command(cmd, answer, NULL, time);
}

uint8_t GSM::command(const char* cmd, const char* answer){
    return GSM::command(cmd, answer, 10000); // 10s default wait time
}

uint8_t GSM::command(const char* cmd){
    return GSM::command(cmd, s_ok);
}

uint8_t GSM::command(const char* cmd, uint16_t time){
    return GSM::command(cmd, s_ok, time);
}

//"DEACT"


uint8_t GSM::wait_answer(const char* answer, const char* answer2, unsigned int timeout){
    char * cp = response;
    unsigned long deadtime = millis() + timeout;
    char has_answer=0;
    result2_ptr=0;

    byte hasAnswer2 = (answer2 == 0);

    Red_LED_ON; 	// при работе с GSM красный мыргает на каждую команду и зеленый при получении данных
    delay_100(); // модуль не особо шустрый
    Red_LED_OFF; 

#ifdef GSM_DEBUG
serial.print_P(PSTR("#"));
#endif
    do { // this loop waits for the answer
        delay_10(); // чуть подождать чтобы что-то пришло


        if(gsm.available_S()) {	// за время ожидания что-то пришло: 38400 бод - 4800 байт/с, за 10мс придет 48 байт
    	    Green_LED_ON;
    	    do {
        	char c;
        	*cp++ = c = gsm.read_S();
        	*cp=0;
#ifdef GSM_DEBUG
serial.print(c); if(c=='\n') serial.print('#'); 
#endif
    	    } while(gsm.available_S()); // вычитать все что есть, а потом проверять будем
    	    Green_LED_OFF;

	    // данные закончились, можно и проверить, если еще ответ не получен
	    if(!has_answer) { 
                // check if the desired answer  is in the response of the module
                if((result_ptr=strstr_P(response, answer)) != NULL)  { // окончательный ответ
                    has_answer = 1;
#ifdef GSM_DEBUG
//serial.print_P(PSTR("="));
#endif
                } else
                if((result_ptr=strstr_P(response, PSTR("ERROR"))) != NULL)  { // окончательная ошибка
                    has_answer = 3;
#ifdef GSM_DEBUG
//serial.print_P(PSTR(" ERR"));
#endif
                }
#if defined(USE_GPRS)
                if((result_ptr=strstr_P(response, PSTR("DEACT"))) != NULL)  { // окончательная ошибка
                    doOnDisconnect();
		}
#endif
/*
                if(has_answer){ // ответ только что получен
		    do {
			while(gsm.available()) {
        		    *cp++ = c = gsm.read();
        		    *cp=0;
//serial.print(c);
			}
			delay_10();
		    } while(gsm.available());
		}
*/
            }

            if( answer2 !=NULL) {
                result2_ptr=strstr_P(response, answer2); // промежуточный ответ
                hasAnswer2 = (result2_ptr!=0);
#ifdef GSM_DEBUG
//serial.printf_P(PSTR("#2< "),result2_ptr);
#endif
            }

            // TODO: контролировать разбиение на строки
        } else if(has_answer && hasAnswer2) // за время ожидания ничего не пришло - данные кончились, если ответ получен - готово
    	    break;
    } while( millis() < deadtime ); // Waits for the asnwer with time out

    
#ifdef GSM_DEBUG
//serial.println("\n# done");
#endif

    return lastError=has_answer;
}

uint8_t GSM::wait_answer(const char* answer, unsigned int timeout){
    return wait_answer(answer, NULL, timeout);
}


byte GSM::sendSMS(const char * phone, const char * text) {
    if(!strlen(phone)) return true;	// если номер не задан то считаем что все ОК
    
    gsm.command(PSTR("+CMGF=1")); // text mode
    gsm.command(PSTR("+CMGD=1")); // delete all messages
//  gsm.command(PSTR("+CSMS=1")); // mode 1

    gsm.print_P(PSTR("AT+CMGS=\""));
    gsm.print(phone);
    readOut();
    gsm.println_P(PSTR("\""));

    if(1 != wait_answer(PSTR(">"),3000)) return false;

    gsm.print(text);
    gsm.println('\x1a');

    bool ok = wait_answer(s_ok,PSTR("+CMGS:"),30000) == 1;

    return ok && result2_ptr!=NULL;
}

char * GSM::getRSSI(){
    static const PROGMEM char patt[]="+CSQ:";
    byte f= GSM::command(PSTR("+CSQ"), s_ok, patt);   // NET registered?
                
    if(!f) return 0; // command failed

    return result2_ptr+sizeof(patt)-1;
}

static const char PROGMEM patt_minus[] = "\x1c\x38\x3d\x43\x41\x3a"; //1c 38 3d 43 41 3a - Минус в UTF без старшего байта




byte GSM::sendUSSD(uint16_t text) {
  readOut();
  byte flg=0;
 
again:
  gsm.print_P(PSTR("AT+CUSD=1,\""));
  gsm.write(flg==0?'#':'*');
  gsm.print(text);
  gsm.println_P(PSTR("#\""));

#if defined(USSD_DEBUG) && 0
serial.print_P(PSTR("#USSD ["));
serial.print_P(PSTR("AT+CUSD=1,\""));
serial.write(flg==0?'#':'*');
serial.print(text);
serial.println_P(PSTR("#\""));
serial.println_P(PSTR("#]"));
#endif


  static const char PROGMEM patt[]="+CUSD:";

    if(1!=wait_answer(patt,15000)) {
#ifdef USSD_DEBUG
serial.print_P(PSTR("#No ans "));
#endif

	if(flg==0){
	    flg++;
	    goto again;
	}
#ifdef USSD_DEBUG
serial.print_P(PSTR("#ERROR flg=1 "));
#endif
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

#ifdef USSD_DEBUG
serial.printf_P(PSTR("#\n#ans: %s flg=%d "), bp, flg);
#endif

    for(char *cp = result_ptr + sizeof(patt)-1;;){
        char c=*cp++;
    
        if(!c){
#ifdef USSD_DEBUG
serial.print_P(PSTR("#\n# EOL "));
#endif
        
    	     break;	// end of line
    	}
        if(c=='"'){

#ifdef USSD_DEBUG
serial.print_P(PSTR("#\n# kaw "));
serial.println(cp);
#endif

	    if(fIn) {
#ifdef USSD_DEBUG
serial.print_P(PSTR("#\n# close kaw "));
serial.println(cp);
serial.println((char *)buf);
#endif
		break; // внутри строки - строка кончилась
	    } else fIn=true;
        }else{
	    if(!fIn) continue; // пропускаем все перед кавычкой

	    if(flg){	// parse unicode
#ifdef USSD_DEBUG
serial.printf_P(PSTR("#\n#UTF c='%c' "),c);
#endif

	        num <<= 4;
	        if(c>0x60) c-=0x20;
	        c-='0';
	        if(c>9) c+='0' - 'A' + 10;
	        num += c;
	
	        if(++cnt==4) {// конец
#ifdef USSD_DEBUG
//serial.print(num,16);
//serial.println();
//serial.println(cp);
// 1c 38 3d 43 41 3a - Минус в UTF без старшего байта
#endif

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
	    
	    } else  { //just write
#ifdef USSD_DEBUG
serial.printf_P(PSTR("#c=%c\n"),c);
#endif
	        *bp++=c;
	        *bp=0;
	    }
	}
    }
  
    if(!fIn) {
#ifdef USSD_DEBUG
serial.printf_P(PSTR("# not fIn!!! cp=%s\n"),(char *)buf);
#endif
	return 0;
    }

#ifdef USSD_DEBUG
serial.printf_P(PSTR("# !!!=%s\n"),(char *)buf);
#endif

    return 1;
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


// есть телеметрия, которая и так передает координаты, к тому же GSM на борту не сильно полезен остальным приборам
#if 0

void gprs_point() {  //Процедура начальной инициализации GSM модуля
 
    bs.begin(buf);
    bs.print_P(PSTR("ll="));
    
 //Установка настроек подключения
    gsm.command(PSTR("+SAPBR=1,1"));     //             Открыть несущую (Carrier)
    gsm.command(PSTR("+SAPBR=3,1,\"CONTYPE\",\"GPRS\"")); //тип подключения - GPRS  
    gsm.command(PSTR("+SAPBR=3,1,\"APN\",\"" APN "\""));
//     gsm.command(PSTR("+SAPBR=3,1,\"USER\",\"user\""));
//     gsm.command(PSTR("+SAPBR=3,1,\"PWD\",\"pass\""));
    gsm.command(PSTR("+SAPBR=1,1"));  // Устанавливаем GPRS соединение
    gsm.command(PSTR("+HTTPINIT"));  //  Инициализация http сервиса
    gsm.command(PSTR("+HTTPPARA=\"CID\",1"));  //Установка CID параметра для http сессии
    gsm.print_P(PSTR("+HTTPPARA=\"URL\",\""));
    gsm.print_P(PSTR(URL)); //     http:/????????.ru/gps/track.php?id=?N&ll=    //Собственно URL, после sprintf с координатами
    readOut();
    gsm.print(coord.lat);
    gsm.print(',');
    gsm.print(coord.lon);
    gsm.println_P(PSTR("\""));

    if(1 != wait_answer(patt,15000)) {
//serial.print_P(PSTR("No ans"));
	return false;
    }
 
    gsm.command(PSTR("+HTTPACTION=0"));    //Запросить данные методом GET
    gsm.command(PSTR("+HTTPREAD"));   //дождаться ответа
    gsm.command(PSTR("+HTTPTERM"));    //остановить HTTP
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


bool GSM::initGPRS(){
    return
	gsm.command(PSTR("+CREG=2")) && // Enable network registration messages in extended format
	gsm.command(PSTR("+CMEE=2")) && // enable CMEE error reporting in extended format
	gsm.command(PSTR("+CR=1"))   && // enable intermediate result code
	gsm.command(PSTR("+CRC=1"))  && // enable extended format of incoming call indicator
	gsm.command(PSTR("+CSNS=4")) && // Single Numbering Scheme mode=data
	gsm.command(PSTR("+CSMINS=1")) && // enable SIM status report
	gsm.command(PSTR("+CSCLK=0")) && // disable slow clock
	gsm.command(PSTR("+CIURC=1")) && // enable URC presentation
	gsm.command(PSTR("+CGEREP=2"))&& // GPRS error reporting: 0 disable, 1 enable
	gsm.command(PSTR("+CIPMUX=0"))&& // Single channel communication (ie only one socket can be opened)
	gsm.command(PSTR("+CIPMODE=1"))&& // Transparent bridge mode
	//                          NmRetry, WaitTm, SendSz, Esc. 
	gsm.command(PSTR("+CIPCCFG=8,10,400,0,0,460,50"));  // GPRS params
	//                 mode,subset,portspeed(4->57600),frame size, ack time,
//	gsm.command(PSTR("+CMUX=0,0,4,32768,10,3,30,10,2")); // GPRS/IP params
}

bool GSM::setAPN(char *apn) {
    return 
        gsm.command(PSTR("+CGATT?"),PSTR("OK"),PSTR("CGATT: 1"),60000) && // Make sure GPRS is Attached
        gsm.command(PSTR("+CSTT=\"internet\",\"\",\"\"")); // AT+CSTT="APN","username","password" - login to service provider/carrier
}


bool GSM::initUDP(uint16_t port){
    bool f=
        gsm.command(PSTR("+CIICR"),3000) && // Connect!
        gsm.command(PSTR("+CIFSR"),PSTR("."),PSTR("OK")); // Get IP address (for info only);
    
    if(!f) return f;
    
    readOut();

    gsm.print_P(PSTR("+CLPORT=\"UDP\",")); // Prep UDP Port 8888
    gsm.println(port);

    return 1==wait_answer(PSTR("OK"),1000);

}

bool GSM::connectUDP(char *url, uint16_t port){
    readOut();

    gsm.print_P(PSTR("+CIPSTART=\"UDP\",\""));
    gsm.print(url);
    gsm.print_P(PSTR("\","));
    gsm.println(port);
    // AT+CIPSTART="protocol","ip address or domain","port #"
    if(1==wait_answer(PSTR("OK"),PSTR("CONNECT"),3000)) {
	GSM::isTransparent=1;
	return true;
    }
    return false;
    
}

