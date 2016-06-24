/*

    управление парашютной системой 

*/

extern void delay_10();
extern void delay_1();

void sendCoordsSms();
void sendCoordsSms(bool fChute);
unsigned int sqrt32(unsigned long n);


#if defined(USE_GSM)
extern GSM gsm;
#endif


void chute_release(){
    uint32_t dt=millis() + 2000; // 2 секунды серва будет тянуть

    uint8_t bit = digitalPinToBitMask(CHUTE_PIN); // move calculations from critical section
    uint8_t port = digitalPinToPort(CHUTE_PIN);
    volatile uint8_t *out = portOutputRegister(port);

//#define OUT_PORT(val) if (val == LOW) { *out &= ~bit; } else { *out |= bit; }
#define SET_LOW()   *out &= ~bit
#define SET_HIGH()  *out |= bit

    while(dt>millis()){
//	noInterrupts();		// pulse widh disabled interrups for accuracy
	SET_HIGH(); 		//digitalWrite(PWM_out_pin,1);
	delayMicroseconds(2000);
	SET_LOW();		//digitalWrite(PWM_out_pin,0);
	delay_10();
//	interrupts();
    }
}


void chute_start() { // сработка парашюта
    DBG_PRINTLN("chute on");

    lflags.chute=true;        // флаг сработки

    if( (mav_baro_alt - baro_alt_start) > CHUTE_MIN_ALT){ // достаточная высота
	// собственно команда на раскрытие
	chute_release();
    }


// перашЮт раскрыт, шлем СМС пока висим высоко
#if defined(USE_GSM)
    sendCoordsSms(true);
//    gsm.end(); // GSM сделал свое дело - закончим работу и выключим питание - пошлем еще одно при дизарме
    lflags.smsSent = false; // пошлем еще раз по приземлении
    gsm.set_sleep(true);
#endif

}


void chute_init(){
    pinMode(CHUTE_PIN, OUTPUT);
    digitalWrite(CHUTE_PIN,LOW);
    
/*
    PWM range - 1000..2000 uS
    timer step - 4uS so 250..500  oops...

*/

//    sbi(TCCR0A, COM0B1);	// connect pwm to pin on timer 0, channel B
//    OCR0B = val; 		// set pwm duty
}

/* данные с борта

uint8_t      mav_throttle;              // 0..100
long         mav_alt_rel;               // altitude - float from MAVlink! * 100
long         mav_climb;                 //< Current climb rate in meters/second * 100

int16_t      mav_pitch;                 // pitch from DCM
int16_t      mav_roll;                  // roll from DCM

byte         mav_mode;

long         mav_alt_error;             // float from MAVlink!  ///< Current altitude error in meters * 100

long         mav_baro_alt;              // altitude calculated from pressure * 100

int          mav_xacc;                  //< X acceleration (m/s^2) * 1000
int          mav_yacc;                  //< Y acceleration (m/s^2) * 1000
int          mav_zacc;                  //< Z acceleration (m/s^2) * 1000

*/

//void chute_check(){ // периодическая проверка состояния борта на тему "а не пора ли?" 
// лучше проверять по мере получения соответствующих пакетов с контроллера
//}

struct ModeFlags {
    bool useChute:1; // в этом режиме можно использовать парашют
    bool altHold:1;  // режим с автоподдержанием высоты
};

static const ModeFlags PROGMEM modeFlags[] = {
    /* mode 0  - stabilize           */ { 1, 0 },
    /* mode 1  - Acrobatic           */ { 0, 0 },
    /* mode 2  - Alt Hold            */ { 1, 1 },
    /* mode 3  - Auto                */ { 1, 1 },
    /* mode 4  - Guided              */ { 1, 1 },
    /* mode 5  - loiter              */ { 1, 1 },
    /* mode 6  - RTL                 */ { 1, 1 },
    /* mode 7  - Circle              */ { 1, 1 },
    /* mode 8  - Position hold (old) */ { 1, 0 },
    /* mode 9  - Land                */ { 1, 1 },
    /* mode 10 - OF_Loiter           */ { 1, 1 },
    /* mode 11 - Drift               */ { 1, 1 },
    /* mode 12 -                     */ { 0, 0 },
    /* mode 13 - sport               */ { 0, 0 },
    /* mode 14 - flip                */ { 0, 0 },
    /* mode 15 - tune                */ { 0, 0 },
    /* mode 16 - Position hold (new) */ { 1, 1 },
};

byte control_loss_count;   // number of iterations we have been out of control



void chute_lost() {
    if(!lflags.motor_armed) {
	control_loss_count=0;
	return;
    }

    if(lflags.chute) return; // уже сработал

    if(last_baro_alt < mav_baro_alt){	// если поднимаемся то все ОК
	control_loss_count=0;
	return;
    }
    
    control_loss_count++;

    if(control_loss_count > CHUTE_ON_COUNT)
	chute_start(); // сработка парашюта
}


static inline ModeFlags getModeFlags() {
    return modeFlags[mav_mode];
}

static inline void chute_got_climb(){ // получена скороподъемность, проверяем на тему "а не пора ли?" 
    ModeFlags f = getModeFlags();
    
    if(!f.useChute) return;
    if( f.altHold) return;	// только в режимах БЕЗ автоудержания высоты

/*
mav_throttle // 0-100
mav_alt_rel  //< Current altitude (MSL), in meters * 100
mav_climb    //< Current climb rate in meters/second * 100

    если скорость снижения превышает определенную - падаем. 
    Если моторы встали а высота уменьшается - падаем. 

*/

    if(mav_climb < -CHUTE_FALL_SPEED ||  mav_throttle == 0) {
	chute_lost();
    }
}

static inline void chute_got_imu(){ // получена показание приборов, проверяем на тему "а не пора ли?" 
    ModeFlags f = getModeFlags();

    if(!f.useChute) return;
    if( f.altHold) return;	// только в режимах БЕЗ автоудержания высоты


/*
mav_baro_alt // altitude in meters * 100
mav_xacc //< X acceleration (m/s^2) * 1000
mav_yacc //< Y acceleration (m/s^2) * 1000
mav_zacc //< Z acceleration (m/s^2) * 1000


    Если ускорение вниз почти нулевое
    винтом книзу с изменением высоты вниз более секунды
*/
    uint16_t acc = sqrt32(mav_xacc*mav_xacc + mav_yacc*mav_yacc + mav_zacc*mav_zacc);
    
    if(acc > 8000) {// общий вектор ускорения приближается к 1g
	chute_lost();
    }
}

static inline void chute_got_atti(){ // получена показание положения, проверяем на тему "а не пора ли?" 
    ModeFlags f = getModeFlags();

    if(!f.useChute) return;
//    if( f.altHold) return;	// только в режимах БЕЗ автоудержания высоты

/*
mav_pitch  в градусах
mav_roll
                
    винтом не туда с изменением высоты вниз более секунды
*/   
    if(labs(mav_pitch) > 80 || labs(mav_roll) > 80){
	chute_lost();
    }
}


static inline void chute_got_alterr(){ // получена ошибка высоты, проверяем на тему "а не пора ли?" 
    ModeFlags f = getModeFlags();

    if(!f.useChute) return;
    if(!f.altHold) return;	// только в режимах с автоудержанием высоты

/*
mav_alt_error // in meters * 100

    тут вроде как все просто: ошибка превысила порог и растет - контроллер не справился, падаем
*/
    if(mav_alt_error > 200) {// промах более 2-х метров
	chute_lost();
    }
}


