#ifndef bufstream_h
#define bufstream_h

/*
    простенький класс для (форматированного) вывода в буферную память, в основном чтобы использовать printf_P

*/

class BS: public BetterStream
{
  public:
    BS(void);
    inline static  void    begin(char *p) {  bufpos = (byte *)p; };
    virtual byte    available(void);
    virtual byte    read(void);
    virtual byte    peek(void);
    virtual void    flush(void);
    virtual size_t  write(uint8_t c);
    using BetterStream::write;

//  private:
    static byte * bufpos;
};

#endif

