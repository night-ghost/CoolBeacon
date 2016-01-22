#ifndef MorseEnDecoder_H
#define MorseEnDecoder_H

#if (ARDUINO <  100)
#include <WProgram.h>
#else
#include <Arduino.h>
#endif


#define WPM 15		// Word-per-minute speed

#define DOT_TIME  (1200 / WPM)       // morse dot time length in ms
#define DASH_TIME (3 * 1200 / WPM)
#define WORD_SPACE (7 * 1200 / WPM)


class morseEncoder
{
  public:
    morseEncoder();
    static void encode();

    static void write(char temp);
    static void write(char *cp);

    inline static boolean available() {
        return !sendingMorse;
    }
    static volatile char morseSignalString[7];// Morse signal for one character as temporary ASCII string of dots and dashes
    static char *strPtr;

//  private:
    static byte encodeMorseChar;   // ASCII character to encode
    static boolean sendingMorse;

    static volatile byte sendingMorseSignalNr;
    static uint32_t sendMorseTimer;
};


#include "morseEnDecoder.cpp"

#endif


