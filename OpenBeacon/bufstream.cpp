#include "compat.h"

// printf without float
#define SKIP_FLOAT

#include <SingleSerial.h>

// Get the common arduino functions
#include "Arduino.h"

#include "bufstream.h"


byte * BS::bufpos;


BS::BS()
{
}

/*void BS::begin(char *p)
{
    bufpos = (byte *)p;
}*/

//------------------ write ---------------------------------------------------

size_t BS::write(uint8_t c){

    *bufpos++ = c;
    *bufpos = 0; // строка в буфере всегда закрыта
    return 1;
}

//------------------ pure virtual ones (just overriding) ---------------------

byte  BS::available(void){
	return 0;
}
byte  BS::read(void){
	return 0;
}
byte  BS::peek(void){
	return 0;
}
void BS::flush(void){
}

