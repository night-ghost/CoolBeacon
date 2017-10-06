
static  uint8_t index; // глобальный дабы не передавать ссылку


// 4->16 bit IMA ADPCM tables of index changes and quantizer step size lookup
static const int8_t  PROGMEM adpcm_index_tab[] = {
    -1, -1, -1, -1, 2, 4, 6, 8,  // две строки полностью одинаковы, так что достаточно чуууть поменять маску
//    -1, -1, -1, -1, 2, 4, 6, 8
};

static const uint16_t PROGMEM adpcm_stepsize_tab[89] = {
      7,     8,     9,    10,    11,    12,    13,    14,    16,    17,
     19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
     50,    55,    60,    66,    73,    80,    88,    97,   107,   118,
    130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
    337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
    876,   963,  1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
   2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
   5894,  6484,  7132,  7845,  8630,  9493, 10442, 11487, 12635, 13899,
  15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

#define MAX_ADPCM_INDEX ((uint8_t)(sizeof(adpcm_stepsize_tab)/sizeof(uint16_t) - 1))

uint16_t adpcmDec(uint8_t sample_in, uint16_t sample_prev) {
    uint16_t diffq, step;

    sample_prev += 32768; // convert to unsigned

    // Find quantizer step size from lookup table using index 
    step = pgm_read_word(&adpcm_stepsize_tab[index]);

    // Inverse quantize the ADPCM code into a difference using the quantizer step size 
    if (sample_in & 4) diffq  = step; else diffq = 0; step >>= 1;
    if (sample_in & 2) diffq += step;                 step >>= 1;
    if (sample_in & 1) diffq += step;                 step >>= 1;
    diffq += step;

    // Add the difference to the predicted sample 
    if (sample_in & 8) { // sign
        if (sample_prev > diffq)            sample_prev -= diffq;
        else sample_prev = 0;
    } else {
        if ((0xffff - sample_prev) > diffq) sample_prev += diffq;
        else sample_prev = 0xffff;
    }
    sample_prev -= 32768; // restore sign

    
    /* Find new quantizer step size by adding the old index and a table lookup using the ADPCM code */
    int idx = index + pgm_read_byte(&adpcm_index_tab[sample_in & 0x7]);

    /* Check for overflow of the new quantizer step size index */
    if      (idx < 0)               idx = 0;
    else if (idx > MAX_ADPCM_INDEX) idx = MAX_ADPCM_INDEX;

    index=idx;
    return sample_prev;
}


// timer0 uses 16MHz / 64 = 250Hkz as clock so ticks each 4 microseconds
// dly  - 134 (0),  155uS (100),  165 (200)
// так что можно регулировать с шагом 4uS компаратором с 0 таймера, не так плавно зато точнее 

volatile byte fInt, voicePWM; // флаг прерывания и новое значение ШИМ
volatile byte inc;            // инкремент таймера 0 для обеспечения нужной задержки
volatile byte fTone;          // тон а не речь - меняем значение ШИМ самостоятельно, отсчитывая полупериоды
volatile byte fHalf;          // счет полупериодов


// частота тиков 250Hkz
// генерация задержек на системном таймере
ISR(TIMER0_COMPA_vect) {
    OCR0A = TCNT0 + inc; // отложим следующее прерывание на величину задержки

    if(fTone) {	//		если генерируем тон
	OCR2B = (++fHalf & 1)?0x30:0xd0; // то новое значение задаем самостоятельно
//	fHalf=!fHalf; 		// другой полупериод
/* там и так это значение, ничего страшного
#ifdef DO_DTMF
    } else if(DTMF_enable){
	// all done
#endif
*/
    } else {
	OCR2B = voicePWM;        // записать новое значение ШИМ
    }
    fInt = 1; 		// было прерывание

#if defined(DO_DTMF) && defined(DTMF_TIM0)
    extern void do_DTMF();
    extern volatile byte DTMF_enable;
    if(DTMF_enable) do_DTMF(); // calculate next value
#endif

}

// используется ТОЛЬКО изнутри формирования звука при включенном таймере

//#define BEEP_TONE(freq) ((((1000000 + freq/2)/ freq +1 ) / 2 /* 2 periods */+2) / 4 /* 4 us each tick */)

void beep(unsigned int t){ // used also in DTMF
#if 0
    byte a=0x30,c=0xd0;

    uint32_t dt=millis()+t;

    while(dt>millis()){
    //for(byte i=255; i!=0; i--){
        OCR2B=a;
        delayMicroseconds(180);
        OCR2B=c;
        delayMicroseconds(180);
    }
#else
    inc=BEEP_TONE(2500); // 2500Hz = 50 * 4uS
//    inc=180/4;
//DBG_PRINTVARLN(inc);
    fTone=true;
    delay(t); 
    fTone=false;
#endif
}

void beep(){
    beep(100); // 0.1s one Beep
}

static void NOINLINE wave(int v){
    voicePWM = (byte)(v / 380  - 96);
    fInt=0; while(!fInt);	// wait for interrupt
}


// RC выходной цепи - 75мкс, или при резисторе 1к емкость 75нф

void _sendVOICE(char *s, byte beeps){
    byte *wav;
    uint16_t len;
    uint16_t sample_prev=0;
    
    index=0; // c начала 

    power_timer2_enable();

// ШИМ частотой 16MHz / 256 = 62.5KHz (период 16мкс), значит одно значение ШИМ звучит 155/16 ~= 10 периодов
    TIMSK2 = (1<<OCIE2B) | (1<<TOIE2);  // Int T2 Overflow + Compare enabled
#if defined(HARD_VOICE) && HARD_VOICE==2
    TCCR2A = (1<<WGM21) | (1<<WGM20) | (1<<COM2B1);   // Fast PWM non-inverted output
#elif defined(HARD_VOICE) && HARD_VOICE==1
    TCCR2A = (1<<WGM21) | (1<<WGM20) | (1<<COM2A1);   // Fast PWM non-inverted output
#else
    TCCR2A = (1<<WGM21) | (1<<WGM20);   // Fast PWM
#endif
    TCCR2B = (1<<CS20);                 // CLK/1 и режим Fast PWM
    OCR2B=0x0; // начальное значение компаратора

    
    
    TIFR0   = (1<<OCF0A);   // clear flag
    TIMSK0 |= (1<<OCIE0A);  // разрешим compare  interrupt - все время формирования голоса пусть тикает

//beep(500);

//DBG_PRINTLN("beep off");
//delay(2500);
//DBG_PRINTLN("delay off");

    for(; beeps>0; beeps--){
        beep();
        delay_100();
    }

    for(;;){
        delay(p.SpeechRate); // задержка ДО проверки на конец строки дабы договорить успела и пауза после последней цифры была

        byte c= *s++; // byte from string
        if(c==0) break;

        redBlink(); 

        byte n=c - '0';
#if !defined(SKIP_STAR)
        if(n<=10){ // плюс еще сэмпл с кодом 3A - ":" - "pinnacle"
#else
        if(n<10){ 
#endif
	    wav=(byte *)pgm_read_word(&samples[n]);
	    len=        pgm_read_word(&sample_len[n]);
        } else {
	    if(c == '#' ) {		// разделяет широту и долготу
	        beep(); delay_50(); beep();
	    } else if(c=='.') {	// точка, разделяет целое и дробное
	        beep();
	    } else if(c=='*') {	// признак краша
	        beep(500);
	    } 
	    delay_300(); //delay(p.SpeechRate * 3); и так сойдет зато компактнее
	    continue;
        }

//    uint16_t dly = (( p.SpeechRate + 1400 ) * 155L ) / 1500; // 155uS default 
        uint16_t dly = (( p.SpeechRate + 1400 ) * 220L ) / 1500; // 220uS  - время расчета не входит в задержку
        inc = dly/4; // инкремент таймера 0 по 4 мкс - портится от beep
    
//	   voicePWM=0; - без него лучше
        // зачем мешать естественному порядку вещей?! // OCR0A = TCNT0 + inc;   // отложим прерывание на нужное время для определенности
        // таймер тикает все время так что просто дождемся прерывания
        fInt=0; while(!fInt);	// wait for interrupt
        for(;len>0;len--) {
            uint8_t s=pgm_read_byte(wav++);

            sample_prev = adpcmDec(s,    sample_prev);
            wave((int)sample_prev);

            sample_prev = adpcmDec(s>>4, sample_prev);
            wave((int)sample_prev);
	}
//    voicePWM=0; // убрать ШИМ на время паузы, а не оставлять последнее значение - так хуже
//    fInt=0;   while(!fInt);	 // догенерить остаток
    //delay_50(); // пауза между цифрами есть и в начале
    }

    TIMSK0 &= ~(1 << OCIE0A); // запретим compare interrupt

    TIMSK2 = 0; // выключить таймер
    TCCR2A = 0; // его можно не сбрасывать, таймер все равно остановлен - но надо отключить ногу в режиме HardVoice
    TCCR2B = 0;
 
    BUZZER_LOW; // могли говорить в пищальник и забыть
    power_timer2_disable();

}

