/*          MORSE ENDECODER
 
 - Morse encoder / decoder classes for the Arduino.

 Copyright (C) 2010-2012 raron

 GNU GPLv3 license:

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 

 Contact: raronzen@gmail.com
 Details: http://raronoff.wordpress.com/2010/12/16/morse-endecoder/

 TODO: (a bit messy but will remove in time as it (maybe) gets done)
 - Use micros() for faster timings
 - use different defines for different morse code tables, up to including 9-signal SOS etc
   - how to deal with different Morse language settings? Define's don't work with libraries in Arduino...
   - possibly combine binary tree with table for the last few signals, to keep size down.
 - UTF-8 and ASCII encoding
   - configurable setting or both simultaneous?
 - Speed auto sense? (would be nice).
   - e.g. average x nr. of shortest and also longest signals for a time?
   - All the time or just when asked to?
   - debounceDelay might interfere
 - Serial command parser example sketch (to change speed and settings on the fly etc) 
 

 History:
 2012.11.25 - raron: Implemented another type of binary tree and algorithms.
                morseSignalString is now backwards.
 2012.11.24 - AdH: wrapped enocer digitalWrite calls in virtual start_signal
                 and stop_signal functions to make alternate output methods 
                 easy via subclassing.
 2012.11.22 - Debugged the _underscore_ problem, it got "uppercased" to a
                question mark. Also, included ampersand (&)
 2012.11.20 - Finally moved table to PROGMEM! Cleaned up header comments a bit.
 2012.11.10 - Fixed minor bug: pinMode for the Morse output pin (thanks Rezoss!)
 2012.01.31 - Tiny update for Arduino 1.0. Fixed header comments.
 2010.12.06 - Cleaned up code a bit.
                Added the "MN digraph" ---. for alternate exclamation mark(!).
                Still encoded as the "KW digraph" -.-.-- though.
 2010.12.04 - Program changed to use (Decode and Encode) classes instead.
 2010.12.02 - Changed back to signed timers to avoid overflow.
 2010.11.30 - Morse punctuation added (except $ - the dollar sign).
 2010.11.29 - Added echo on/off command.
 2010.11.28 - Added simple Morse audio clipping filter + Command parser.
 2010.11.27 - Added Morse encoding via reverse-dichotomic path tracing.
                Thus using the same Morse tree for encoding and decoding.
 2010.11.11 - Complete Rewrite for the Arduino.
 1992.01.06 - My old rather unknown "Morse decoder 3.5" for Amiga 600.
                A 68000 Assembler version using a binary tree for Morse
                decoding only, of which this is based on.
*/ 

#include "MorseEnDecoder.h"


// Morse code binary tree table (dichotomic search table)

// ITU with most punctuation (but without non-english characters - for now)

#define morseTreeLevels  6 // Minus top level, also the max nr. of morse signals
const int morseTableLength = pow(2,morseTreeLevels+1);
char morseTable[] PROGMEM = 
  " ETIANMSURWDKGOHVF*L*PJBXCYZQ!*54*3***2&*+****16=/***(*7***8*90*"
  "***********?_****\"**.****@***'**-********;!*)*****,****:*******\0";




static byte morseEncoder::morseSignals;       // nr of morse signals to send in one morse character
static char morseEncoder::morseSignalString[7];// Morse signal for one character as temporary ASCII string of dots and dashes
char *morseEncoder::strPtr;

//  private:
static char morseEncoder::encodeMorseChar;   // ASCII character to encode
static boolean morseEncoder::sendingMorse;

//    static int morseSignalPos;
byte morseEncoder::sendingMorseSignalNr;
long morseEncoder::sendMorseTimer;
long morseEncoder::lastDebounceTime;


morseEncoder::morseEncoder()
{
  // some initial values
  sendingMorse = false;
  encodeMorseChar = '\0';
}



void morseEncoder::write(char temp) {
  if (!sendingMorse && temp != '*') encodeMorseChar = temp;
}

void morseEncoder::write(char *cp) {
    if (!sendingMorse){
	char c;
	while(c=*cp++) {
	    if(c=='*') continue;

	    encodeMorseChar = c;
	    strPtr=cp;
	    return;
	}
    }
}

extern volatile byte fInt, voicePWM; // флаг прерывания и новое значение ШИМ
extern volatile byte inc;            // инкремент таймера 0 для обеспечения нужной задержки
extern volatile bool fTone,fHalf;    // тон а не речь - меняем значение ШИМ самостоятельно, отсчитывая полупериоды

volatile uint32_t periods;


// near 1ms
ISR(TIMER0_COMPB_vect) {
    periods++;

    doSignals();
}


void doSignals() {
    uint32_t currentTime = periods;

    char& currSignalType = morseSignalString[sendingMorseSignalNr-1];

    bool endOfChar = sendingMorseSignalNr <= 1;
    switch (currSignalType)  {
      case '.': // Send a dot (actually, stop sending a signal after a "dot time")
        if (currentTime - sendMorseTimer >= dotTime) {
    	    goto off;
//          fTone=false; // signal(false);
//          sendMorseTimer = currentTime;
//          currSignalType = 'x'; // Mark the signal as sent
        }
        break;
        
      case '-': // Send a dash (same here, stop sending after a dash worth of time)
        if (currentTime - sendMorseTimer >= dashTime)    {
off:      fTone=false; //signal(false);		//@@@
          sendMorseTimer = currentTime;
          currSignalType = 'x'; // Mark the signal as sent
        }
        break;

      case 'x': // To make sure there is a pause between signals
        if (sendingMorseSignalNr > 1)    {
          // Pause between signals in the same letter
          if (currentTime - sendMorseTimer >= dotTime)   {
            signal(true); // Start sending the next signal //@@@
            goto next;
          }
        } else {
          // Pause between letters
          if (currentTime - sendMorseTimer >= dashTime)    {
next:       sendingMorseSignalNr--;
            sendMorseTimer = currentTime;       // reset the timer
          }
        }
        break;

      case ' ': // Pause between words (minus pause between letters - already sent)
      default:  // Just in case its something else
        if (currentTime - sendMorseTimer > wordSpace - dashTime) 
	    sendingMorseSignalNr--;
    }
    if (sendingMorseSignalNr == 0 ) {
        sendingMorse = false; // char finished
        // Ready to encode more letters
        encodeMorseChar = '\0';

	if(strPtr){ // we sending string
	    char c;
	    while(true) {
		c=*strPtr++;
		if(!c){
		    strPtr=NULL;
		    break;
		}
		if(c=='*') continue;
	        encodeMorseChar = c;
	        break;
	    }
        }

        //HW deinit
        TIMSK0 &= ~(1 << OCIE0B); // запретим compare interrupt
	TIMSK2 = 0;
	TCCR2A = 0;
	TCCR2B = 0;
	power_timer2_disable();
    }
}

void morseEncoder::encode() {

  if (!sendingMorse && encodeMorseChar) {
    // change to capital letter if not
    if (encodeMorseChar > 96) encodeMorseChar -= 32;
  
    // Scan for the character to send in the Morse table
    byte p;
    for (p=0; p<morseTableLength+1; p++) 
	if (pgm_read_byte_near(morseTable + p) == encodeMorseChar) 
	    break;

    if (p >= morseTableLength) p = 0; // not found, but send a space instead


    // Reverse binary tree path tracing
    int pNode; // parent node
    morseSignals = 0;

    // Travel the reverse path from position p to the top of the morse table
    if (p > 0)  {
      // build the morse signal (backwards morse signal string from last signal to first)
      pNode = p;
      while (pNode > 0)   {
        if ( (pNode & 0x0001) == 1) {
          // It is a dot
          morseSignalString[morseSignals++] = '.';
        } else {
          // It is a dash
          morseSignalString[morseSignals++] = '-';
        }
        // Find parent node
        pNode = int((pNode-1)/2);
      }
    } else { // Top of Morse tree - Add the top space character
      // cheating a little; a wordspace for a "morse signal"
      morseSignalString[morseSignals++] = ' ';
    }
    
    morseSignalString[morseSignals] = '\0';


    // start sending the the character
    sendingMorse = true;
    sendingMorseSignalNr = morseSignals; // Sending signal string backwards
    sendMorseTimer = periods; // millis();

    if (morseSignalString[0] != ' ') // start tone
	fTone=true; // signal(true); 

    // init HW
    power_timer2_enable();

    inc = 180/4; // 2777 Hz

// ШИМ частотой 16MHz / 256 = 62.5KHz (период 16мкс), значит одно значение ШИМ звучит 155/16 ~= 10 периодов
    TIMSK2 = (1 << OCIE2A) | (1 << TOIE2);  // Int T2 Overflow + Compare enabled
    TCCR2A = (1<<WGM21) | (1<<WGM20);         // Fast PWM.
    TCCR2B = (1<<CS20);                     // CLK/1 и режим Fast PWM
    OCR2A=0x0; // начальное значение компаратора

    OCR0B = TCNT0;   // отложим прерывание на нужное время
    TIFR0  |=  1<<OCF0B;   // clear flag
    TIMSK0 |= (1<<OCIE0B); // разрешим compare  interrupt
  } // new char


  // Send Morse signals to output
  if (sendingMorse) {
	doSignals();
  }
}
