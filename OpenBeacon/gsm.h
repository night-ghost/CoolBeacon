#ifndef gsm_h
#define gsm_h


/*
    простенький класс для работы с SIM800

*/

#define RESPONCE_LENGTH 200 // ответ на USSD бывает весьма длинным

/*
для работы с GSM используется слегка модифицированная библиотека AltSoftSerial

изменения: наследование не от Stream а от BetterStream
добавлена возможность опроса состояния
некоторая оптимизация

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
    static void readOut();
    static bool set_sleep(byte mode);
    static uint8_t wait_answer(const char* answer, const char* answer2, unsigned int timeout);
    static uint8_t wait_answer(const char* answer, unsigned int timeout);
    static uint8_t command(const char* cmd, const char* answer, uint16_t time);
    static uint8_t command(const char* cmd, const char* answer);
    static uint8_t command(const char* cmd, uint16_t time);
    static uint8_t command(const char* cmd);
    static bool sendSMS(const char * phone, const char * text);
    static bool sendUSSD(uint16_t text);
    static int balance();
//  private:
    static char response[RESPONCE_LENGTH];
};

#endif