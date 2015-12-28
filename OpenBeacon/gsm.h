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





#if 0

void sms(char * text, char * phone) {
//  serial.println("SMS send started");
  gsm.print("AT+CMGS=\"")
  gsm.print(phone);
  gsm.print("\"\n\r");
  delay(1000);
  gsm.print(text);
  delay(300);
  gsm.print("\01a\n\r");
//  delay(300);
 // Serial.println("SMS send finish");
//  delay(3000);
}

const char gprs0[] PROGMEM = "AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"";  //Установка настроек подключения
const char gprs1[] PROGMEM = "AT+SAPBR=3,1,\"APN\",\"internet.tele2.ru\"";
const char gprs2[] PROGMEM = "AT+SAPBR=3,1,\"USER\",\"tele2\"";
const char gprs3[] PROGMEM = "AT+SAPBR=3,1,\"PWD\",\"tele2\"";
const char gprs4[] PROGMEM = "AT+SAPBR=1,1";  //Устанавливаем GPRS соединение
const char gprs5[] PROGMEM = "AT+HTTPINIT";  //Инициализация http сервиса
const char gprs6[] PROGMEM = "AT+HTTPPARA=\"CID\",1";  //Установка CID параметра для http сессии

const char * const ATs[] PROGMEM = {  //массит АТ команд
    gprs0, gprs1, gprs2, gprs3, gprs4, gprs5, gprs6
};

const byte ATsDelays[] PROGMEM = {6, 1, 1, 1, 3, 3, 1}; //массив задержек



void gprs_init() {  //Процедура начальной инициализации GSM модуля
  int d = 500;
  
 

  byte ATsDelays[] = {6, 1, 1, 1, 3, 3, 1}; //массив задержек
//  Serial.println("GPRG init start");
  for (byte i = 0; i <  sizeof(ATs) / sizeof(char *); i++) {
//    Serial.println(ATs[i]);  //посылаем в монитор порта
    gsm.println(ATs[i]);  //посылаем в GSM модуль
    delay(500 * ATsDelays[i]);
//    Serial.println(ReadGSM());  //показываем ответ от GSM модуля
//    delay(500);
  }
//  Serial.println("GPRG init complete");
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
