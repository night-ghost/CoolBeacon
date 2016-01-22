#ifndef MorseEnDecoder_H
#define MorseEnDecoder_H

#if (ARDUINO <  100)
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

#define MORSE_ACTIVE_LOW true
#define MORSE_ACTIVE_HIGH false

#define WPM 13		// Word-per-minute speed
#define DOT_TIME  (1200 / WPM)       // morse dot time length in ms
#define DASH_TIME (3 * 1200 / wpm)
#define WORD_SPACE (7 * 1200 / wpm)


class morseEncoder
{
  public:
    morseEncoder();
    static void encode();
//    static void setspeed(int value);
    static void write(char temp);
    static void write(char *cp);
    inline static boolean available() {
        return !sendingMorse;
    }
    static byte morseSignals;       // nr of morse signals to send in one morse character
    static char morseSignalString[7];// Morse signal for one character as temporary ASCII string of dots and dashes
    static char *strPtr;

//  private:
    static char encodeMorseChar;   // ASCII character to encode
    static boolean sendingMorse;

//    static int morseSignalPos;
    static byte sendingMorseSignalNr;
    static long sendMorseTimer;
    static long lastDebounceTime;
// protected:
//    virtual void setup_signal();
//    virtual void start_signal(bool startOfChar, char signalType);
//    virtual void stop_signal(bool endOfChar, char signalType);
};


#endif

