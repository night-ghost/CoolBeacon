#ifndef gsm_h
#define gsm_h


/*
    простенький класс для работы с SIM800

*/

class GSM: public AltSoftSerial
{
  public:
    GSM(void);
    static void  begin();
    static void  end();
    virtual byte     available(void);
    virtual byte     read(void);
    virtual byte     peek(void);
    virtual void     flush(void);
    virtual size_t  write(uint8_t c);
    using AltSoftSerial::write;
    using AltSoftSerial::read;
    using AltSoftSerial::available;
    using AltSoftSerial::flush;


    static uint8_t command(char* cmd, char* answer, unsigned int timeout);
//  private:

    static AltSoftSerial gsmSerial;
    
    static char response[150];
};

#endif