/*

    управление парашютной системой 

*/

void sendCoordsSms();

#if defined(USE_GSM)
extern GSM gsm;
#endif

void chute_start() { // сработка парашюта
    DBG_PRINTLN("chute on");



// перашЮт раскрыт, шлем СМС пока висим высоко
#if defined(USE_GSM)
    sendCoordsSms();
    gsm.end(); // GSM сделал свое дело - закончим работу и выключим питание
#endif
}


/*
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
//
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

inline ModeFlags getModeFlags() {
    return modeFlags[mav_mode];
}

inline void chute_got_climb(){ // получена скороподъемность, проверяем на тему "а не пора ли?" 
    if(!lflags.motor_armed) return;
    
    ModeFlags f = getModeFlags();
    
    if(!f.useChute) return;
    if( f.altHold) return;	// только в режимах БЕЗ автоудержаниея высоты
}

inline void chute_got_alterr(){ // получена ошибка высоты, проверяем на тему "а не пора ли?" 
    if(!lflags.motor_armed) return;

    ModeFlags f = getModeFlags();

    if(!f.useChute) return;
    if(!f.altHold) return;	// только в режимах с автоудержанием высоты
}

inline void chute_got_imu(){ // получена показание приборов, проверяем на тему "а не пора ли?" 
    if(!lflags.motor_armed) return;

    ModeFlags f = getModeFlags();

    if(!f.useChute) return;
}





