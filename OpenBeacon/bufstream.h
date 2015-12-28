
#ifndef bufstream_h
#define bufstream_h


/*
    простенький класс для вывода в буферную память, в основном чтобы использовать printf_P

*/

class BS: public BetterStream
{
  public:
    BS(void);
    static void begin(char *p);
    virtual byte     available(void);
    virtual byte     read(void);
    virtual byte     peek(void);
    virtual void     flush(void);
    virtual size_t  write(uint8_t c);
    using BetterStream::write;

//  private:
    static byte * bufpos;
};

#endif

