
struct Params {
/* 0
Частота (Гц, 433075000)
Частота, на которой будет работать маяк.
Задается в герцах. Например, первый канал LPD (433.075МГц) записывается так: "433075000".
По умолчанию настроен первый канал сетки LPD: 433.075МГц
*/    uint32_t Frequency;

/* 1
Поправка
RAM:012E Коэффициент коррекции частоты конкретного приемомопередающего модуля.
RAM:012E Требует индивидуальной процедуры подбора, см. раздел Калибровка частоты.
*/    uint32_t FrequencyCorrection; /**/

/* 2
Первичная задержка (сек, 86400)
Первичная задержка перед включением таймерного маяка.
Задается в секундах. По умолчанию установлено 86400 = 24 * 60 * 60, т.е. 24 часа.
Это означает, что маяк принудительно начнет передачу через 24 часа после подачи питания.
*/    uint32_t DeadTimer; 

/* 3
 Интервал в начале (сек, 10)
 Интервал между посылками в начале поискового периода таймерного маяка.
 Задается в секундах. По умолчанию: 10 секунд.
*/    uint32_t IntStart;

/* 4
Интервал в конце (сек, 60)
 Интервал между посылками в конце поискового периода таймерного маяка.
 Задается в секундах. По умолчанию: 60 секунд.
*/    uint32_t IntLimit;

/* 5
Поисковый период (мин, 240)
 Поисковый период, в течение которого работает таймерный маяк.
 Задается в минутах. По умолчанию: 240 = 4*60, т.е. 4 часа.
 Маяк будет передавать посылки в течение этого времени, постепенно увеличивая интервал между посылками.
 В начале и в конце периода интервал определятся соответствующими параметрами.
*/    uint32_t SearchTime; 


/* 6
 Задержка до следующего поискового периода (мин, 1200)
 Задержка до следующего периода активности.
 Задается в минутах. По умолчанию 1200 = 20 * 60, т.е. 20 часов.
 После завершения поискового периода маяк засыпает на заданный период. Со значениями по умолчанию, 
 маяк проснется через 24 часа, отработает 4 часа и уснет на 20 часов, чтобы проснуться в то же время на следующий день.
*/    uint32_t Sleep;

/* 7
Предел ошибок GPS (раз, 100)
 Количество допустимых ошибок чтения GPS координат.
 По умолчанию 100. После превышения лимита ошибок, констатируется обрыв или обесточивание приемника GPS, 
 и дальнейшее чтение координат производится в 100 раз реже.
*/    uint32_t GPS_ErrorTresh;

/* 8
Интервал чтения GPS (сек, 10)
 Интервал отслеживания GPS координат (сек).
 Задается в секундах. По умолчанию 10 секунд.
 Определяет интервал, с которым маяк будет просыпаться и прослушивать поступающие данные от GPS. 
 Чем ниже значение, тем более актуальными данными будет располагать маяк при обрыве GPS и тем выше будет расход энергии.
*/    uint32_t GPS_MonitInterval;

/* 9
Период чтения GPS (мсек, 1500)
 Таймаут одной попытки прослушивания порта GPS.
 Задается в миллисекундах. По умолчанию 1500.
 За этот период маяк должен получить координаты от GPS-приемника. В случае, если координаты получены, маяк немедленно засыпает. 
 В противном случае дожидается окончания таймаута, потребляя при этом повышенный ток.
 Рекомендуется устанавливать значение чуть большее, чем 1сек/частота посылок GPS.
 Например, для приемника GPS 10Гц, данное значение можно установить в 150мсек. 
 Для одногерцового приемника хорошо работает значение по умолчанию.
*/    uint32_t GPS_MonDuration;


/* 10
 Формат координат (ГГ.ГГГГГ)
 По умолчанию ГГ.ГГГГГ
 Можно выбрать наиболее удобный формат координат. Например, тот, что понимает ваш GPS-навигатор.
 Google Maps и Yandex карты понимают форматы 3, 4, 5, 6. Для форматов 5 и 6 нужно дополнительно знать градусы долготы и широты вашей местности.
 При выборе формата нужно понимать, что чем больше цифр передается, тем больше тратится на это энергии.
 Погрешности различных форматов:
 ГГ ММ СС.С 3 метра
 ГГ ММ.МММ   2 метра
 ГГ.ГГГГГ    1 метр
 ГГ.ГГГГ     10 метров
 .ГГГГГ      1 метр
 .ГГГГ       10 метров
*/    uint32_t GPS_Format;


/* 11
Интервал прослушивания (сек, 3)
 Период, с которым маяк будет прослушивать эфир в ожидании вызова.
 Задается в секундах. По умолчанию 3.
 Не рекомендуется ставить низкие значения из-за повышенного расхода энергии. В случае настройки по умолчанию, 
 для гарантированного пробуждения маяка требуется выдать в эфир вызывной сигнал в течении 3-4 сек.
*/    uint32_t ListenInterval;

/* 12
Период прослушивания (мсек, 50)
 Продолжительность прослушивания эфира.
 Задается в миллисекундах. По умолчанию 50.
 Чем больше значение, тем надежнее распознавание вызова и выше расход аккумулятора.
*/    uint32_t ListenDuration; 


/* 13
Количество посылок после вызова (раз, 3)
 Количество циклов посылок маяка после вызова.
 По умолчанию 3.
 Посылки идут в таком порядке: маяк, голос, при условии, что данный вид посылки разрешен в дополнительных настройках.
*/    uint32_t WakeRepeat;

/* 14
Интервал посылок (сек, 5)
 Интервал посылок маяка после вызова.
 Задается в секундах. По умолчанию 5.
 Интервал используется как внутри одной пачки между маяком и голосом, так и между пачками.
*/    uint32_t WakeInterval;

/* 15
Корректировка таймера (мсек, 0)
 Корректировка таймера. По умолчанию 0.
 Если на вашем экземпляре маяка таймер спешит или отстает, есть возможность внести поправку.
 Значение определяет разницу с реальной секундой в мсек. Плюс, если спешит. Минус, если отстает.
 Например, если маяк сработал не через 10 минут (600 сек), а через 9:30 (570 сек), то поправку можно посчитать по формуле 1000 - (1000 * 570/600) = 50.
*/    uint32_t TimerCorrection;

/* 16
    периодичность сообщения координат в таймерном режиме
    1 - каждую посылку
    5 - каждую пятую посылку

  default 5
*/    uint32_t SearchGPS_time;

/* 17
Маяк после пробуждения (вкл)
 Определяет нужно ли слать сигналы маяка после пробуждения. По умолчанию включено.
  default 1
*/    uint32_t boolWakeBeacon;

/* 18
Коорд. после пробужден. (вкл)
 Определяет нужно ли слать координаты после пробуждения. По умолчанию включено.
  default 1
*/    uint32_t boolWakeGPSvoice;

/* 19
    старший байт - номер ноги пищалки
    младший - тон
 default 0x6B1
*/    uint32_t WakeBuzzerParams; 

/* 20
    номер ноги вспышки

 default 5
*/    uint32_t WakeFlashPin;

/* 21
 Экономия заряда (вкл)
 Включение экономии заряда на мощном "пике". По умолчанию включено.
 Шумоподавители некоторых раций могут "не слышать" такой сигнал.
  default 0
*/    uint32_t HighSavePower;

/* 22
фильтр по количеству спутников GPS. если количество видимых спутников упадет ниже заданного, 
маяк проигнорирует поступающие координаты и будет использовать предыдущие, "хорошие".
 default 3
*/    uint32_t GPS_SatTreshHold;

/* 23
 default 60
*/    uint32_t MinAuxRSSI;

/* 24
 default 100
*/    uint32_t SpeechRate;

/* 25
 default 1750
*/    uint32_t CallToneFreq;

/* 26
    предварительно кодированные буквы позывного, 0 - точка, 1 - тире, паузовка автоматическая каждые 6
 default 0
*/    uint32_t MorzeSign2;

/* 27
 default 0
*/    uint32_t MorzeSign1; 

/* 28
    частота записи точек в EEPROM 
    1 - каждая точка
    2 - каждая вторая
    етц
    0 - вообще не писАть

 default 1
*/    uint32_t EEPROM_SaveFreq;

/* 29
    два порога напряжения основной батарейки - маяк работает продвинутой пищалкой
 default 0
*/    uint32_t ExtVoltageWarn;

/* 30 */

/* 30 .. 4 */
    char phone1[16];

/* 34 .. 4 */
    char phone2[16];

/* 38 .. 4 */
    char phone3[16];

/* 42 .. 4 */
    char phone4[16];

/* 46 .. 6 */
//    char apn[24];

/* 52 */

} p = { // начальные значения
/* 0  */    446031250,  //Frequency
/* 1  */    200,	//FrequencyCorrection
/* 2  */    0,
/* 3  */    10,
/* 4  */    60,
/* 5  */    240,
/* 6  */    1200,
/* 7  */    100,
/* 8  */    10,
/* 9  */    1500,
/* 10 */    3,
/* 11 */    3,
/* 12 */    50,		//ListenDuration
/* 13 */    3,
/* 14 */    5,
/* 15 */    0,
/* 16 */    5,		// SearchGPS_time
/* 17 */    1,		// boolWakeBeacon
/* 18 */    1,		// boolWakeGPSvoice
/* 19 */    0x6B1, 	// WakeBuzzerParams  старший байт - номер ноги, младший - тон
/* 20 */    5,	  	// номер ноги вспышки
/* 21 */    0,		// HighSavePower
/* 22 */    3,		// GPS_SatTreshHold
/* 23 */    60,		// MinAuxRSSI
/* 24 */    100,	// SpeechRate
/* 25 */    1750,	// CallToneFreq
/* 26 */    0,		// morze 1
/* 27 */    0,		// morze 2
/* 28 */    1,		// EEPROM_SaveFreq
/* 29 */    0,		// ExtVoltageWarn
#define PARAMS_END 30
/* 30 */    PHONE,
#ifdef PHUNE1
/* 34 */    PHONE1,
#endif
#ifdef PHUNE2
/* 38 */    PHONE2,
#endif
#ifdef PHUNE3
/* 42 */    PHONE3,
#endif
/* 46 *///    APN,


};


struct StrParam {
    char *ptr;
    byte length;
};

const StrParam strParam[] = {
    { p.phone1, 16 },
    { p.phone2, 16 },
    { p.phone3, 16 },
    { p.phone4, 16 },
    { (char *)&p.MorzeSign2, 8 },
//    { p.apn, 24 },
};

/*
const byte PROGMEM  ParamLength[] = {
    sizeof(p.Frequency),
    sizeof(p.FrequencyCorrection),
    sizeof(p.DeadTimer),
    sizeof(p.IntStart),
    sizeof(p.IntLimit),
    sizeof(p.SearchTime),
    sizeof(p.Sleep),
    sizeof(p.GPS_ErrorTresh),
    sizeof(p.GPS_MonitInterval),
    sizeof(p.GPS_MonDuration),
    sizeof(p.GPS_Format),
    sizeof(p.ListenInterval),
    sizeof(p.ListenDuration), 
    sizeof(p.WakeRepeat),
    sizeof(p.WakeInterval),
    sizeof(p.TimerCorrection),
    sizeof(p.SearchGPS_time),
    sizeof(p.boolWakeBeacon),
    sizeof(p.boolWakeGPSvoice),
    sizeof(p.WakeBuzzerParams),
    sizeof(p.WakeFlashPin),
    sizeof(p.HighSavePower),
    sizeof(p.GPS_SatTreshHold),
    sizeof(p.MinAuxRSSI),
    sizeof(p.SpeechRate),
    sizeof(p.CallToneFreq),
    sizeof(p.MorzeSign2),
    sizeof(p.MorzeSign1), 
    sizeof(p.EEPROM_SaveFreq),
    sizeof(p.ExtVoltageWarn),
};
*/

#define RX_SIZE 128 // 2 пакета MAVlink
#define TX_SIZE 16
uint8_t rxBuf[RX_SIZE], txBuf[TX_SIZE];


// булевые флаги кучей
struct loc_flags {
    bool mavlink_got; 		// флаг получения пакета
    bool mavlink_active; 	// флаг активности (навсегда)
    bool mavbeat;		// MAVLink session control
    bool callActive;	// недавно принят вызов
    
    bool hasGPSdata;  // есть координаты для сообщения
    bool pointDirty;  // последняя принятая точка не сохранена в EEPROM
    bool listenGPS;   // состояние - слушаем или нет GPS
    bool gsm_ok;	// инициализация удалась


    bool connected;  // в багдаде все спокойно, ничего не оторвалось и мы на внешнем питании - можно не скромничать
    bool crash;		// c мавлинка пришло сообщение о краше

    bool hasPower;   // текущее состояние питания
    bool lastPowerState; // предыдущее состояние для отслеживания изменений
    bool wasPower;   // ранее было подано питание с коптера
    
    bool motor_armed;	// текущее состояние
    bool last_armed_status; // предыдущее состояние для отслеживания изменений
    bool motor_was_armed;// флаг "моторы были включены" навсегда
    bool smsSent;	// флаг отправки SMS
};

struct loc_flags lflags = {0,0,0,0,0,0,0}; // все булевые флаги кучей

#define BAD_COORD 0xffffffff

// из mavlink координаты идут в LONG,  домноженные на 10000000
//                      максимальное беззнаковое  - 4294967296
// и  нет никакого смысла связываться с float!
struct Coord {
    long lat;
    long lon;
} coord, home_coord={BAD_COORD,BAD_COORD}; // домашних координат нет


Coord bad_coord ={BAD_COORD,BAD_COORD};

byte gps_points_count; // количество принятых точек, для фильтрации

unsigned int vExt; // всегда читаем, пусть будет в одном месте

uint32_t  powerTime; //время включения питания
uint32_t  armTime;   // время арма моторов
uint32_t  disarmTime;   // время дизарма моторов - для таймера запуска
uint32_t  disconnectTime; // время обнаружения потери связи с коптером.

uint32_t  lastPointTime; // время с последней точки от GPS

byte BuzzerToneDly, BuzzerPin, FlashPin;

uint8_t  ItStatus1;

//uint32_t watchdogTime_us = 0; // было 16000 но все равно калибровать будем
uint16_t watchdogTime_us = 0; // было 16000 но все равно калибровать будем, а таймер 16-битный, экономим 40 байт флеша

uint32_t sleepTimeCounter = 0;
uint32_t uptime;
uint32_t nextListenTime;
uint32_t nextSearchTime;
uint32_t nextNmeaTime;
uint32_t searchBeginTime;
uint32_t searchStopTime;
uint32_t beaconInterval;
uint32_t timeToGrow;
uint32_t nextBeepTime;
uint32_t growTimeInSeconds;


volatile byte preambleDetected, preambleRSSI;


byte lastRSSI = 99; // сначала полный ибо мы рядом, потом потрем
byte Got_RSSI;	// RSSI во время детекта тона
byte tmpRSSI; // для контроля прекращения передачи

uint16_t cycles_count;
byte GPS_data_fresh; // данные с канала а не из памяти
byte GPS_data_OK;    // про данные рассказали


byte NextVoltageTreshhold = 25;
byte powerCheckDelay = 0;
byte NumberOfCells;
uint16_t cell_voltage;

uint16_t GpsErrorCount;
uint32_t NextListenTime, Next_GPS_Time;
uint32_t gpsOffTime; // время выключения прослушивания GPS
uint32_t chuteTime;  // время след проверки парашюта
uint32_t callTime;  // время получения вызова (его окончания)

#define MESSAGE_SIZE 24
char messageBuff[MESSAGE_SIZE + 1] = "000"; // буфер вывода координат для сообщения голосом

#define SERIAL_BUFSIZE 64 /* 19  */
byte buf[SERIAL_BUFSIZE+1]; // буфер команд с интерфейса и прочих надобностей

byte eeprom_points_count; // номер текущей точки (указатель записи), кольцо

// MAVlink data
uint32_t     lastMAVBeat;               // время приема последнего пакета, ms
uint8_t      mav_satellites_visible;    // number of satelites
uint8_t      mav_fix_type;              // GPS lock 0-1=no fix, 2=2D, 3=3D

// эти параметры не нужны пищалке, но могут помочь определить падение и отправить СМС *до* удара
uint8_t      mav_throttle;              // 0..100
long         mav_alt_rel;               // altitude - float from MAVlink!
long         mav_climb;                 // Current climb rate in meters/second
int16_t      mav_pitch;                 // pitch from DCM
int16_t      mav_roll;                  // roll from DCM

byte         mav_mode;
long         mav_alt_error;             // float from MAVlink!  // Current altitude error in meters * 100 

long         mav_baro_alt;              // altitude calculated from pressure * 100
int          mav_xacc;                  // X acceleration (m/s^2) * 1000
int          mav_yacc;                  // Y acceleration (m/s^2) * 1000
int          mav_zacc;                  // Z acceleration (m/s^2) * 1000

// напряжение с полетного контроллера, мв - для проверки собственной измерялки
uint16_t     mav_vbat_A;

// эти вообще не нужны
//byte         apm_mav_system;
//byte         apm_mav_component;
//long       mav_alt_gps;


bool voiceOnBuzzer; // говорить только по радио или пищалкой тоже
#if defined(BUZZER_PIN_PORT) && defined(BUZZER_PIN_BIT)
    // ничего лишнего
#else
volatile byte * buzzerPort; // cохраним дабы работать напрямую а не через digitalWrite
byte buzzerBit;
#endif

// работа со статическими временными переменными намного проще чем со стеком
Coord p1, p2; // временные координаты для работы с треком
uint16_t crc; // статический буфер выгоднее 



//-----  EEPROM  --------------------------------------------------------------------

// стартовый адрес параметров в EEPROM
#define EEPROM_PARAMS sizeof(long) // от начала на 4 байта
#define MAX_PARAMS (64-1) // c запасом, перед параметрами - 32 бита CRC

#define EEPROM_CRC 0 // 4 байта с самого начала
#define EEPROM_VERS 2 // CRC занимает 2 байта

//стартовый адрес трека в EEPROM
#define EEPROM_TRACK ((MAX_PARAMS+1) * sizeof(long)) // зарезервируем место для MAX_PARAMS параметров, остальное - 768 байт - на трек
#define EEPROM_TRACK_SIZE (E2SIZE - EEPROM_TRACK)
#define MAX_TRACK_POINTS  (EEPROM_TRACK_SIZE / sizeof(Coord))

#define CURRENT_VERSION 55
