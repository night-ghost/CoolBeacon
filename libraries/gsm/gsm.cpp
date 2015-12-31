/*

    Нативная библиотека GSM чудовищна и огромна, а  нам много не надо

    к тому же нога прерывания, нужная для работы SoftwareSerial, занята


Отправка данных на сервер (GSM/GPRS shield)
Подключаемся к Arduino к контактам 7 и 8. 

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

/*

http://cxem.net/arduino/arduino170.php
http://arduino.ua/ru/guide/ArduinoGSMShield

*/
#include "compat.h"
#include <SingleSerial.h>
#include <AltSoftSerial.h>



// printf without float
#define SKIP_FLOAT



#include "Arduino.h"// Get the common arduino functions

#include "config.h"
#include "gsm.h"


/*
    GSM(void);
    static void  begin();
    static void  end();
    virtual byte     available(void);
    virtual byte     read(void);
    virtual byte     peek(void);
    virtual void     flush(void);
    virtual size_t  write(uint8_t c);
    using BetterStream::write;
*/

AltSoftSerial GSM::gsmSerial;

GSM::GSM(){}

void  GSM::begin(
    GSM::gsmSerial.begin(GSM_SPEED);
    digitalWrite(GSM_EN,LOW);
)

void  GSM::end(){

}

byte GSM::available(void) {
    return gsmSerial.available();
}
byte GSM::read(void){
    return gsmSerial.available();
}
byte GSM::peek(void){
    return gsmSerial.peek();
}
void GSM::flush(void){
    gsmSerial.flush();
}
size_t GSM::write(uint8_t c){
    return gsmSerial.write(c);
}



// отправка AT-команд
// usage  command("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", "OK", 2000);
uint8_t GSM::command(char* cmd, char* answer, unsigned int timeout){
    uint8_t has_answer=0;

    char *cp;
    char response[150];
    unsigned long deadtime = millis() + timeout;
    char c;
 
    memset(response, '\0', 150);    // Initialize the string

    delay(100);
    while( available()) read();    // Clean the input buffer from last answer and unsolicit answers

    println(cmd);    // Send the AT command 
    cp = response;

    // this loop waits for the answer
    do{
        if(available()) {    // if there are data in the UART input buffer, reads it and checks for the asnwer
            *cp++ = c = read();
            // Serial.print(c);

            // check if the desired answer  is in the response of the module
            if (strstr(response, answer) != NULL)  {
                has_answer = 1;
                break;
            }
            // TODO: контролировать разбиение на строки
        }
    } while( millis() < deadtime ); // Waits for the asnwer with time out
    
//    Serial.println(response);

    return has_answer;
}

#if 0

void sms(char * text, char * phone) {
//  serial.println("SMS send started");
  gsm.print_P("AT+CMGS=\"")
  gsm.print(phone);
  gsm.print_P("\"\n\r");
  delay(1000);
  gsm.print(text);
  delay(300);
  gsm.print_P("\01a\n\r");
//  delay(300);
 // Serial.println("SMS send finish");
//  delay(3000);
}


void gprs_init() {  //Процедура начальной инициализации GSM модуля
 
 //Установка настроек подключения
     gsm.print_P("AT+SAPBR=1,1");     //             Открыть несущую (Carrier)
     gsm.print_P("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""); //тип подключения - GPRS  
     delay(500 * 6);
     gsm.printf_P ("AT+SAPBR=3,1,\"APN\",\"%S\"",APN);
     delay(500 * 1);
     gsm.print_P("AT+SAPBR=3,1,\"USER\",\"user\"\r\n");
     delay(500 * 1);
     gsm.print_P("AT+SAPBR=3,1,\"PWD\",\"pass\"\r\n");
     delay(500 * 1);
     gsm.print_P("AT+SAPBR=1,1\r\n");  // Устанавливаем GPRS соединение
     delay(500 * 3);
     gsm.print_P("AT+HTTPINIT\r\n");  //  Инициализация http сервиса
     delay(500 * 3);
     gsm.print_P("AT+HTTPPARA=\"CID\",1\r\n");  //Установка CID параметра для http сессии

     gsm.print_P("AT+HTTPPARA=\"URL\",\"http:/????????.ru/gps_tracker/gps_tracker1.php?id_avto=?N&lat=XXXXXlon=YYYYY\"");    //Собственно URL, после sprintf с координатами
     gsm.print_P("AT+HTTPACTION=0");    //Запросить данные методом GET
     gsm.print_P("AT+HTTPREAD");   //дождаться ответа
     gsm.print_P("AT+HTTPTERM");    //остановить HTTP

}

// надо бы ограничиться одной строкой
void ReadGSM(char *p) {  //функция чтения данных от GSM модуля
    byte c;
    while (gsm.available()) {
        c = gsm.read();
        *p++ = c;
        *p=0;
        delay(10);
    }
}

#endif
