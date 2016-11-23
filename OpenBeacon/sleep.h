//void narcoleptic_sleep(uint8_t wdt_period,uint8_t sleep_mode);

// more details in http://www.gammon.com.au/forum/?id=11497

void narcoleptic_sleep(uint8_t wdt_period,uint8_t sleep_mode) {
  
#ifdef BODSE
  // Turn off BOD in sleep (picopower devices only)
  MCUCR |= _BV(BODSE);
  MCUCR |= _BV(BODS);
#endif

  MCUSR = 0;
  WDTCSR &= ~_BV(WDE);
  WDTCSR = _BV(WDIF) | _BV(WDIE) | _BV(WDCE);
  
  wdt_enable(wdt_period);
  wdt_reset();
#ifdef WDTCSR
  WDTCSR |= _BV(WDIE);
#else
  WDTCR |= _BV(WDIE);
#endif
  set_sleep_mode(sleep_mode);

  // Disable all interrupts
  uint8_t SREGcopy = SREG; cli();

#ifdef EECR
   uint8_t EECRcopy = EECR; EECR &= ~_BV(EERIE);
#endif
#ifdef EIMSK
    uint8_t EIMSKcopy = EIMSK; EIMSK = 0;
#endif
#ifdef PCMSK0
    uint8_t PCMSK0copy = PCMSK0; PCMSK0 = 0;
#endif
#ifdef PCMSK1
    uint8_t PCMSK1copy = PCMSK1; PCMSK1 = 0;
#endif
#ifdef PCMSK2
    uint8_t PCMSK2copy = PCMSK2; PCMSK2 = 0;
#endif
#ifdef TIMSK0
    uint8_t TIMSK0copy = TIMSK0; TIMSK0 = 0;
#endif
#ifdef TIMSK1
    uint8_t TIMSK1copy = TIMSK1; TIMSK1 = 0;
#endif
#ifdef TIMSK2
    uint8_t TIMSK2copy = TIMSK2; TIMSK2 = 0;
#endif
#ifdef SPCR
    uint8_t SPCRcopy = SPCR; SPCR &= ~_BV(SPIE);
#endif
#ifdef UCSR0B
    uint8_t UCSR0Bcopy = UCSR0B; UCSR0B &= ~(_BV(RXCIE0) | _BV(TXCIE0) | _BV(UDRIE0));
#endif
#ifdef TWCR
    uint8_t TWCRcopy = TWCR; TWCR &= ~_BV(TWIE);
#endif
#ifdef ACSR
    uint8_t ACSRcopy = ACSR; ACSR &= ~_BV(ACIE);
#endif
#ifdef ADCSRA
    uint8_t ADCSRAcopy = ADCSRA; ADCSRA &= ~_BV(ADIE);
#endif
#ifdef SPMCSR
    uint8_t SPMCSRcopy = SPMCSR; SPMCSR &= ~_BV(SPMIE);
#endif
  
    sei();
    sleep_mode();
    wdt_disable();

  // Reenable all interrupts
#ifdef SPMCSR
    SPMCSR = SPMCSRcopy;
#endif
#ifdef ADCSRA
    ADCSRA = ADCSRAcopy;
#endif
#ifdef ACSR
  ACSR = ACSRcopy;
#endif
#ifdef TWCR
  TWCR = TWCRcopy;
#endif
#ifdef UCSR0B
  UCSR0B = UCSR0Bcopy;
#endif
#ifdef SPCR
  SPCR = SPCRcopy;
#endif
#ifdef TIMSK2
  TIMSK2 = TIMSK2copy;
#endif
#ifdef TIMSK1
  TIMSK1 = TIMSK1copy;
#endif
#ifdef TIMSK0
  TIMSK0 = TIMSK0copy;
#endif
#ifdef PCMSK2
  PCMSK2 = PCMSK2copy;
#endif
#ifdef PCMSK1
    PCMSK1 = PCMSK1copy;
#endif
#ifdef PCMSK0
    PCMSK0 = PCMSK0copy;
#endif
#ifdef EIMSK
    EIMSK = EIMSKcopy;
#endif
#ifdef EECR
    EECR = EECRcopy;
#endif

    SREG = SREGcopy;
  
#ifdef WDTCSR
    WDTCSR &= ~_BV(WDIE);
#else
    WDTCR &= ~_BV(WDIE);
#endif
}

void narcoleptic_sleep(uint8_t wdt_period ) {
    narcoleptic_sleep(wdt_period,SLEEP_MODE_PWR_DOWN);
}


static inline void narcoleptic_calibrate() {
  // Calibration needs Timer 1. Ensure it is powered up.
    uint8_t PRRcopy = PRR;
    PRR &= ~_BV(PRTIM1);
  
    uint8_t TCCR1Bcopy = TCCR1B;
    TCCR1B &= ~(_BV(CS12) | _BV(CS11) | _BV(CS10)); // Stop clock immediately

  // Capture Timer 1 state
    uint8_t TCCR1Acopy = TCCR1A;
    uint16_t TCNT1copy = TCNT1;
    uint16_t OCR1Acopy = OCR1A;
    uint16_t OCR1Bcopy = OCR1B;
    uint16_t ICR1copy = ICR1;
    uint8_t TIMSK1copy = TIMSK1;
//    uint8_t TIFR1copy = TIFR1;

    // Configure as simple count-up timer
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;
    TIMSK1 = 0;
//    TIFR1 = 0;
    // Set clock to /64 (should take 15625 cycles at 16MHz clock)
    TCCR1B = _BV(CS11) | _BV(CS10);
    narcoleptic_sleep(WDTO_15MS,SLEEP_MODE_IDLE);   // SLEEEEEEEEEEEEEP
    uint16_t watchdogDuration = TCNT1;
    TCCR1B = 0; // Stop clock immediately

    // Restore Timer 1
//    TIFR1 = TIFR1copy; hardware reset-only register
    TIMSK1 = TIMSK1copy;
    ICR1 = ICR1copy;
    OCR1B = OCR1Bcopy;
    OCR1A = OCR1Acopy;
    TCNT1 = TCNT1copy;
    TCCR1A = TCCR1Acopy;
    TCCR1B = TCCR1Bcopy;

    // Restore power reduction state
    PRR = PRRcopy;
  
    watchdogTime_us = watchdogDuration * (64 * 1000000 / F_CPU);
}

void doSleep(uint16_t sleep_periods){
    
    while (sleep_periods >= 512) {
      narcoleptic_sleep(WDTO_8S);
      sleep_periods -= 512;
    }

    if (sleep_periods & 256) narcoleptic_sleep(WDTO_4S);
    if (sleep_periods & 128) narcoleptic_sleep(WDTO_2S);
    if (sleep_periods & 64)  narcoleptic_sleep(WDTO_1S);
    if (sleep_periods & 32)  narcoleptic_sleep(WDTO_500MS);
    if (sleep_periods & 16)  narcoleptic_sleep(WDTO_250MS);
    if (sleep_periods & 8)   narcoleptic_sleep(WDTO_120MS);
    if (sleep_periods & 4)   narcoleptic_sleep(WDTO_60MS);
    if (sleep_periods & 2)   narcoleptic_sleep(WDTO_30MS);
    if (sleep_periods & 1)   narcoleptic_sleep(WDTO_15MS);
}

void deepSleep(uint16_t milliseconds) {

    sleepTimeCounter += milliseconds; // во время сна таймер системного времени стоит - нужно накапливать общее время сна
    uint32_t microseconds = milliseconds * 1000L;

#if defined(USE_GSM)
//    if(!watchdogTime_us) { // если первый запуск - то калибруем время сна
    if(!gsm.enabled()){	// если таймер не используется
#else
    {
#endif
	narcoleptic_calibrate(); // калибровка использует таймер 1 и мешает работе с GSM
	if(microseconds < watchdogTime_us ) return;
	
	microseconds -= watchdogTime_us;
    }

    doSleep( microseconds / watchdogTime_us);

/* GCC bug https://github.com/arduino/Arduino/issues/4339

    uint16_t sleep_periods = (microseconds - watchdogTime_us) / watchdogTime_us;

    while (sleep_periods >= 512) {
      narcoleptic_sleep(WDTO_8S);
      sleep_periods -= 512;
    }

    if (sleep_periods & 256) narcoleptic_sleep(WDTO_4S);
    if (sleep_periods & 128) narcoleptic_sleep(WDTO_2S);
    if (sleep_periods & 64)  narcoleptic_sleep(WDTO_1S);
    if (sleep_periods & 32)  narcoleptic_sleep(WDTO_500MS);
    if (sleep_periods & 16)  narcoleptic_sleep(WDTO_250MS);
    if (sleep_periods & 8)   narcoleptic_sleep(WDTO_120MS);
    if (sleep_periods & 4)   narcoleptic_sleep(WDTO_60MS);
    if (sleep_periods & 2)   narcoleptic_sleep(WDTO_30MS);
    if (sleep_periods & 1)   narcoleptic_sleep(WDTO_15MS);
*/
}

