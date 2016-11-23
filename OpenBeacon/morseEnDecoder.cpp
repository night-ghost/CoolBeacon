/*         

    morse encoder for CoolBeacon

    rewritten to work as background thread by interrupts
    
    based on:
 
 - Morse encoder / decoder classes for the Arduino.

 Copyright (C) 2010-2012 raron

 Contact: raronzen@gmail.com
 Details: http://raronoff.wordpress.com/2010/12/16/morse-endecoder/

 

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

#if defined(USE_MORZE)

#include "morseEnDecoder.h"
#include <avr/power.h>


// Morse code binary tree table (dichotomic search table)

// ITU with most punctuation (but without non-english characters - for now)

#define morseTreeLevels  6 // Minus top level, also the max nr. of morse signals
#define morseTableLength  (1<<(morseTreeLevels+1))

static const char morseTable[] PROGMEM = 
  " ETIANMSURWDKGOHVF*L*PJBXCYZQ!*54*3***2&*+****16=/***(*7***8*90*"
  "***********?_****\"**.****@***'**-********;!*)*****,****:*******\0";




volatile char morseEncoder::morseSignalString[7];// Morse signal for one character as temporary ASCII string of dots and dashes
char *morseEncoder::strPtr;

//  private:
byte morseEncoder::encodeMorseChar;   // ASCII character to encode
boolean morseEncoder::sendingMorse;

//    static int morseSignalPos;
volatile byte morseEncoder::sendingMorseSignalNr;
uint32_t morseEncoder::sendMorseTimer;
bool morseEncoder::wasCall;


extern void doSignals();
extern void RFM_off(void);
extern void RFM_set_TX();
extern void waitForCall(byte t);
extern void redBlink();
extern void delay_50();

morseEncoder::morseEncoder()
{
  // some initial values
  sendingMorse = false;
  encodeMorseChar = '\0';
}



void morseEncoder::write(char temp) {
  if (!sendingMorse && temp != '*') encodeMorseChar = temp;
  encode();
}

void morseEncoder::write(char *cp) {
  strPtr=cp;
  encode();
}

extern volatile byte inc;      // инкремент таймера 0 для обеспечения нужной задержки
extern volatile byte fTone;    // тон а не речь - меняем значение ШИМ самостоятельно, отсчитывая полупериоды

volatile uint32_t periods;     // вместо счета миллисекунд считаем свои прерывания

volatile bool fListen; // флаг "передача кончилась, можно слушать"

// near 1ms - 1024 uS
ISR(TIMER0_COMPB_vect) { 
    periods++;

    morseEncoder::doSignals();
}


void morseEncoder::doSignals() {
    uint32_t currentTime = periods;

    if (sendingMorseSignalNr == 0 ) {
    //	вобщем-то можно прямтут и подготовить новый символ, разрешив прерывания
	sei();
        encode();
	return; // character done
    }

    uint16_t diff = currentTime - sendMorseTimer; // надолго промахнуться не должны

//DBG_PRINTVARLN(diff);


    volatile char& currSignalType = morseSignalString[sendingMorseSignalNr-1];

    bool endOfChar = sendingMorseSignalNr <= 1;

//DBG_PRINTVARLN(currSignalType);
    
    switch (currSignalType)  {
      case '.': // Send a dot (actually, stop sending a signal after a "dot time")
        if (diff >= DOT_TIME) {
    	    goto off;
        }
        break;
        
      case '-': // Send a dash (same here, stop sending after a dash worth of time)
        if (diff >= DASH_TIME)    {
off:      fTone=false; 
	  // fListen=true; Шумодав станции проглатывает половину если отключать передачу
          currSignalType = 'x'; // Mark the signal as sent
          goto res; //morseEncoder::sendMorseTimer = currentTime;
        }
        break;

      case 'x': // To make sure there is a pause between signals
        if (!endOfChar)    {   // Pause between signals in the same letter
          if (diff >= DOT_TIME)   {
            fTone=true; //signal(true); // Start sending the next signal //@@@
	    fListen=false;
            goto next;
          }
        } else {     // Pause between letters
          if (diff >= DASH_TIME)    {
next:       sendingMorseSignalNr--;
	    if(sendingMorseSignalNr <= 1) fListen=true;

res:        sendMorseTimer = currentTime;       // reset the timer
          }
        }
        break;

      case ' ': // Pause between words (minus pause between letters - already sent)
      default:  // Just in case its something else
        if (diff > WORD_SPACE - DASH_TIME)
	    sendingMorseSignalNr--;
    }
    
}

void morseEncoder::flush() {
    while(!available()) {
	// encode(); in interrupt
	redBlink();
	delay_50();
    }
}


void morseEncoder::encode() {

    if(sendingMorse){
	if(fListen) {	// один раз  после каждой посылки
	    fListen=false;
/* Шумодав станции проглатывает половину если отключать передачу */
/*	    waitForCall(0); // оно принудительно переключит на прием
	    RFM_set_TX(); // вернем режим передачи
	    if(Got_RSSI) {
DBG_PRINTLN("morze call!");
		wasCall=true;
		sendingMorseSignalNr=0;
		encodeMorseChar = '\0';
		goto stop;
	    }
//*/
	}
    
	if (sendingMorseSignalNr == 0 ) { // character done
	    encodeMorseChar = '\0';

	    if(strPtr) {
	        goto prepare; // есть продолжение
	    } else  { // nothing to send
stop:
                sendingMorse = false; // all finished
		RFM_off();

//DBG_PRINTLN("morze stop");

                //HW deinit
                TIMSK0 &= ~( (1 << OCIE0B) | (1 << OCIE0A) ); // запретим compare interrupt
	        TIMSK2 = 0;
	        //TCCR2A = 0; его можно не сбрасывать, таймер остановлен
	        TCCR2B = 0;
	        power_timer2_disable();
	    }
	} 
    } else { // not sending

prepare:
	if(!encodeMorseChar && strPtr){ // we sending string
	    char c;
//DBG_PRINTLN("morze prepare");

	    while(strPtr) {
		c=*strPtr++;
		if(!*strPtr){ // это последний символ
		    strPtr=NULL;
		}
		if(!c){ // перестраховка
		    strPtr=NULL;
		    break;
		}
		if(c=='*') continue; // звезды пропускаем
	        encodeMorseChar = c;
	        break;
	    }
        }
	if(encodeMorseChar) {
//DBG_PRINTVARLN(encodeMorseChar);

	    if (encodeMorseChar > 96) encodeMorseChar -= 32; // change to capital letter if not
  
	    // Scan for the character to send in the Morse table
	    byte p;
	    for (p=0; p<morseTableLength+1; p++) 
		if (pgm_read_byte_near(morseTable + p) == encodeMorseChar) 
		    break;


	    if (p >= morseTableLength) p = 0; // not found, but send a space instead

	    // Reverse binary tree path tracing
	    byte pNode; // parent node
	    
	    volatile char *cp = morseSignalString; //byte morseSignals = 0;

	    // Travel the reverse path from position p to the top of the morse table
	    if (p > 0)  {
	        // build the morse signal (backwards morse signal string from last signal to first)
	        pNode = p;
	        while (pNode > 0)   {
	            //morseSignalString[morseSignals++] = pNode & 0x0001? '.' : '-';
	            *cp++ = pNode & 0x0001? '.' : '-';
	            // Find parent node
	            pNode = int((pNode-1)/2);
	        }
	    } else { // Top of Morse tree - Add the top space character
	        // cheating a little; a wordspace for a "morse signal"
	        //morseSignalString[morseSignals++] = ' ';
	        *cp++ = ' ';
	    }

	    *cp = '\0'; // close string

//DBG_PRINTVARLN((char *)morseSignalString);
	    
	    uint8_t SREGcopy = SREG; cli(); // запрещаем прерывания чтобы не спугнуть
	    sendMorseTimer = periods; // millis();
	    SREG = SREGcopy;

	    if (morseSignalString[0] != ' ') // start tone
		fTone=true; // signal(true); 

	    // start sending the the character
	    sendingMorseSignalNr = cp - morseSignalString; // Sending signal string backwards

	    if(!sendingMorse){ // если передача выключена
		sendingMorse = true;	// надо включить и инициализировать железо
	        // init HW

//DBG_PRINTLN("morze start");
		RFM_set_TX();
//		RFM_tx_min();

	        //inc = 250/4; // 2000 Hz
	        inc = BEEP_TONE(2500); // 2500Hz
	        power_timer2_enable();

// ШИМ частотой 16MHz / 256 = 62.5KHz (период 16мкс)
	        TIMSK2 = (1 << OCIE2A) | (1 << TOIE2);  // Int T2 Overflow + Compare enabled
	        TCCR2A = (1<<WGM21) | (1<<WGM20);       // Fast PWM.
	        TCCR2B = (1<<CS20);                     // CLK/1 и режим Fast PWM
// зачем мешать естественному ходу вещей?    OCR0B = TCNT0;   // отложим прерывание на нужное время
	        TIFR0   = (1<<OCF0B)  | (1<<OCF0A);     // clear flags
	        TIMSK0 |= (1<<OCIE0B) | (1<<OCIE0A);    // разрешим compare  interrupt
	        OCR2A=0x0; // начальное значение компаратора

	    }
	} // new char
    } // sendingMorse


//doSignals();
}



#endif // USE_MORZE
