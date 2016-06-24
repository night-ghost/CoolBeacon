#ifndef gsm_h
#define gsm_h


/*
    простенький класс для работы с SIM800

*/

#define RESPONCE_LENGTH 200 // ответ на USSD бывает весьма длинным

/*

TODO: у нас никогда не бывает одновременной работы MAVlink и GSM - память можно объединить

*/

/*
для работы с GSM используется слегка модифицированная библиотека AltSoftSerial

изменения: наследование не от Stream а от BetterStream
добавлена возможность опроса состояния
некоторая оптимизация
статические read() write() available()

*/

extern byte buf[];

class GSM: public AltSoftSerial
{
  public:
    GSM(void);
    static bool begin();
    static void end();

    static byte _write(uint8_t c);

    static void initGSM(void);
    static void readOut(void);
    static bool set_sleep(byte mode);
    static void turnOff();
    static uint8_t wait_answer(const char* answer, const char* answer2, unsigned int timeout);
    static uint8_t wait_answer(const char* answer, unsigned int timeout);
    static uint8_t command(const char* cmd, const char* answer, const char* answer2, uint16_t time);
    static uint8_t command(const char* cmd, const char* answer, const char* answer2);
    static uint8_t command(const char* cmd, const char* answer, uint16_t time);
    static uint8_t command(const char* cmd, const char* answer);
    static uint8_t command(const char* cmd, uint16_t time);
    static uint8_t command(const char* cmd);
    static byte sendSMS(const char * phone, const char * text);
    static byte sendUSSD(uint16_t text);
    static char * getRSSI(void);
    static int balance(void);
//  private:
    static char response[RESPONCE_LENGTH];
    static byte lastError;
};

#endif