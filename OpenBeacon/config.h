
#define TELEMETRY_SPEED  57600  // How fast our MAVLink telemetry is coming to Serial port

// Тип аппаратуры
// 0 - OrangeRx Open LRS 433MHz 9Ch Receiver
// 1 - Приемник Expert LRS
#define HARDWARE_TYPE 0

// Язык голосовых сообщений RU=русский, EN=английский 
#define WAKE_VOICE_LANG RU

// Формат координат GPS
// 1 ГГ ММ СС.С  // Погрешность 3 метра
// 2 ГГ ММ.МММ   // Погрешность 2 метра
// 3 ГГ.ГГГГГ    // Погрешность 1 метр
// 4 ГГ.ГГГГ     // Погрешность 10 метров
// 5 .ГГГГГ      // Погрешность 1 метр, градусы не передаются. Компромис между точностью и экономичностью. Для поиска необходимо ориентироваться в координатах своей местности.
// 6 .ГГГГ       // Погрешность 10 метров, передаются только доли градусов широты и долготы. Самый экономичный вариант. Для поиска необходимо ориентироваться в координатах своей местности.
#define GPS_COORD_FMT 4

// Длительность одного "пика" (мс). По умолчанию: 1 секунда (1000мс)
#define BEACON_BEEP_DURATION 600

#define DISARM_DELAY_TIME 180 // время после дизарма до начала паники, секунды

#define LAST_POINT_DISTANCE 3 // минимальное расстояние в метрах между точками

#define MIN_HOME_DISTANCE 10 // ближе чем 10 метров от точки взлета - не активизируем GSM


// Корректировка таймера. Если спешит или отстает, то ставим в ноль, DEADTIME минут на 10 и засекаем время до первой посылки..
// Считаем разницу в процентах и выставляем корректировку ( 1000 / (процент погрешности) )
// Плюс, если спешит. Минус, если отстает.
#define TIMER_CORRECTION 18

// Порог напряжения батарии, ниже которого батарея считается разряженной (мВ)
#define VCC_LOW_THRESHOLD 3250


// ---------- Hardware section

// номера ног вспышки и пищалки. Можно и без них (через конфигуратор), но при прямом указании лучше :)
//#define FLASH_PIN 5  // 
#define FLASH_PIN 12 // PB3
#define BUZZER_PIN 11 // PB4
//#define BUZZER_PIN2 12 // PB3

// нога фотовспышки. в отличие от FLASH_PIN вспыхивает не синхронно с пищалкой а один раз на цикл писка
//#define STROBE_PIN 12 // 

#define BUZZER_PIN_PORT (PORTB) // порт и битовая маска ноги пищалки, значительно улучшает обработку прерывания (хотя можно и без них)
#define BUZZER_PIN_BIT (_BV(PB4))

// эти ноги выведены на разъем приемника
#define VBAT_PIN 18 // A4 (PC5)  - Vext_bat (1К на землю, 20К на батарею)
#define VCC_PIN 18 // A4(PC5) - Vcc_bat - via magick mode
#


// --------- GSM section

#define USE_GSM // используем или нет

#define GSM_DTR 6
#define GSM_RING 7
#define GSM_TX 9
#define GSM_RX 8
#define GSM_EN 10
#define GSM_INT PD3

#define GSM_SPEED 38400// 9600 // 19200 //

//#define GSM_SMS_URL "maps.google.ru/maps?q="     // форматировать координвты под ссылку на карты гугля
#define GSM_SMS_URL "ykoctpa.ru/map?q="          // а тут короче и есть Яндекс


// -------  Morze section

#define USE_MORZE true

// other settings in morseEnDecoder.h

//-------- Chute section

#define CHUTE_ON_COUNT 25 // сработка при количестве ошибок: 5 пакетов в секунду, 5 секунд
#define CHUTE_MIN_ALT 700 // минимальная высота срабатывания парашюта, см
#define CHUTE_FALL_SPEED 1000 // скорость снижения, считающаяся падением см/с - 10м/с


/* ------------------------------

 про дополнительный проводочек
 
 инструкция отсюда - http://forum.rcdesign.ru/blogs/127344/blog18303.html

но у RFM нога 15 это SDN - описание
--
Shutdown input pin. 0–VCC 
V digital input. SDN should be = 0 in all modes except Shutdown mode. When SDN =1 the chip will be 
completely shutdown and the contents of the registers will be lost.
--

более того, нога 13 АТмеги это PB1 - OC1A, действительно выход аппаратного ШИМ, но таймер инициализируется так
TCCR1A = (1<<WGM10);
то есть он не используется!!!

нога внешней модуляции - GPIO2 имеет номер 8, так что похоже автор просто лоханулся... Что подтверждают обработчики прерываний от таймера :)

*/



#if HARDWARE_TYPE == 0 // Orange/HawkEye LRS receiver
  #define Red_LED A3
  #define Green_LED 13

  #define Red_LED_ON  PORTC |= _BV(3);
  #define Red_LED_OFF  PORTC &= ~_BV(3);
  #define Green_LED_ON  PORTB |= _BV(5);
  #define Green_LED_OFF  PORTB &= ~_BV(5);

  #define  nSEL_on PORTD |= (1<<4) //D4
  #define  nSEL_off PORTD &= 0xEF //D4
  #define  SCK_on PORTC |= (1<<2) //A2
  #define  SCK_off PORTC &= 0xFB //A2
  #define  SDI_on PORTC |= (1<<1) //A1
  #define  SDI_off PORTC &= 0xFD //A1
  #define  SDO_1 (PINC & 0x01) != 0 //A0
  #define  SDO_0 (PINC & 0x01) == 0 //A0
  #define SDO_pin A0
  #define SDI_pin A1
  #define SCLK_pin A2
  #define IRQ_pin 2 // INT0
  #define nSel_pin 4 // PD4
  #define IRQ_interrupt 0 // Int0

#elif HARDWARE_TYPE == 1 //Приемник Expert LRS

  #define Red_LED_ON spiWriteRegister(0x0e, 0x04);
  #define Red_LED_OFF spiWriteRegister(0x0e, 0x00);
  #define Green_LED_ON spiWriteRegister(0x0e, 0x04);
  #define Green_LED_OFF spiWriteRegister(0x0e, 0x00);

  #define  nSEL_on  PORTC |=  (1<<0) //A0 /PC0
  #define  nSEL_off PORTC &= ~(1<<0) //A0  /PC0
  
  #define  SCK_on  PORTB |=  (1<<5) //D13 /PB5
  #define  SCK_off PORTB &= ~(1<<5) //D13 /PB5
  
  #define  SDI_on PORTB  |=  (1<<3) //D11 /PB3
  #define  SDI_off PORTB &= ~(1<<3) //D11 /PB3
  
  #define  SDO_1 (PINB & 0x10) == 0x10 //D12 PB4
  #define  SDO_0 (PINB & 0x10) == 0x00 //D12 PB4

  #define SDO_pin 12
  #define SDI_pin 11
  #define SCLK_pin 13
  #define IRQ_pin 2
  #define nSel_pin A0
  #define IRQ_interrupt 0
#endif


#define EN 1
#define RU 2

#if BUZZER_PIN
 #if defined(BUZZER_PIN_PORT) && defined(BUZZER_PIN_BIT)
  #define BUZZER_HIGH (BUZZER_PIN_PORT |=  BUZZER_PIN_BIT)
  #define BUZZER_LOW  (BUZZER_PIN_PORT &= ~BUZZER_PIN_BIT)
 #else
  #define BUZZER_HIGH (*buzzerPort |=  buzzerBit)
  #define BUZZER_LOW  (*buzzerPort &= ~buzzerBit)
 #endif
#endif
