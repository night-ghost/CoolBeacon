# CoolBeacon
All-in-One beacon and tracker: GSM, Radio, voice, flashlamp
(english description late)

Based on:
   
   OpenLRS Beacon Project (tBeacon late)  by Konstantin Sbitnev Version 0.3,
       wihch based on
       
   openLRSngBeacon by Kari Hautio - kha @ AeroQuad/RCGroups/IRC(Freenode)/FPVLAB etc.
   
   Narcoleptic - A sleep library for Arduino * Copyright (C) 2010 Peter Knight (Cathedrow)
   
   OpenTinyRX by Baychi soft 2013 

   binary code analysys of tBeacon 0.54 (voice samples), disgrace to GPL violators!

  ************************************
  
   "Все-в одном" Поисковый маяк на базе приемника HawkEye / OrangeRx Open LRS 433MHz 9Ch Receiver
   
   Радио, GSM, голос, вспышка
   
  ************************************

Отличия от tBeacon:

* понимает *только* один протокол и предназначен для непосредственного подключения к полетному контроллеру APM/PixHawk/INAV
* пока подключен к борту не экономит энергию и получает через MAVlink не только координаты но и другую информацию
* координаты перед сохранением в EEPROM фильтруются по минимальному расстоянию - зачем писАть стояние на месте?
* умеет "разговаривать" не только по радио но и пищалкой, можно все предполетные разговоры пустить на нее
* голос формируется по таймеру а не задержкой - получается гораздо чище
* во всех режимах мощность радиопередачи автоматически регулируется по силе принимаемого сигнала
* умеет дожидаться окончания немодулированной несущей перед включением передачи
* после арма и до дизарма радио не включается вообще дабы не создавать помех
* при дизарме в радиусе менее 10 метров от точки взлета маяк не срабатывает (кроме вызывного коий работает всегда)
* при зафиксированном краше таймерный маяк включается сразу же, без задержки (если разрешен)
* умеет управлять GSM-модулем SIM800 и отправить SMS с координатами при аварийной посадке (и в воздухе при аварийном снижении или сработке парашюта)
* управление парашютной системой спасения (серво на CHUTE_PIN)
* умеет управлять газоразрядной фотовспышкой (STROBE_PIN), срабатывать будет один раз на вызов
* может передавать координаты морзянкой
* может передавать координаты DTMF
* TODO:? передавать точки на сервер по мере движения, с прореживанием и контролем расстояния 
    (нужность сомнительна, а GSM передатчик на борту не сильно полезен остальной электронике)
* GPLv3 :)

удалено:
* нет поддержки прямого подключения к GPS
* нет автораспознавания формата - всегда MAVlink/UAVtalk/MWII
* нет поддержки позывного морзянкой (хотя если кому надо то можно легко сделать, причем с кодированием на борту)
* нет управления форматом координат из конфигуратора - задается при сборке
* (почти) нет управления ногой подключения пищалки из конфигуратора - задается при сборке
* нет режима HighSavePower
* нет произнесения двоеточия, вместо этого двойной "beep"


ЛИЦЕНЗИОННОЕ СОГЛАШЕНИЕ (честно потыренное у Ubilling)


Данный программный продукт (далее ПП) распространяется под лицензией GNU GPLv3 с полностью открытым кодом и полностью бесплатно.
Так было всегда, так и останется. Без компромиссов. THIS IS SPARTA OPENSOURCE!!!
Обрекший себя на использование данного продукта принимает тот факт, что ему никто и ничего не должен.
Вообще никаких гарантий - ни явных ни подразумеваемых. Все работает так как работает.
Перед использованием ПП верующим рекомендуется поставить свечку в храме, атеистам - геморроидальную.
Мы не несем никакой ответственности за хаос, панику, разрушения и апокалипсис, возникшие в процессе эксплуатации.
Если у вас возникли вопросы - потрудитесь почитать документацию. Если документация не дала ответов о смысле жизни - попробуйте спросить на форуме.
По умолчанию все баги являются фичами, если вы считаете что нашли багу - попробуйте ее использовать в повседневной жизни ну или расскажите нам.
Если у вас есть какая-то хотелка нужная строго вам - вы можете реализовать ее либо самостоятельно и сделать PR на гитхабе,
либо заказать, либо продолжать хотеть.
Это намек на то, что бесплатно работать фуллтайм на вас никто не вписывался.

***** ENGLISH **********************************************************************************************

"All-in-one" Search beacon on the basis of  HawkEye / OrangeRx Open LRS 433MHz 9Ch Receiver (https://github.com/openLRSng/openLRSngWiki/wiki/Hardware-Guide)

   Radio, GSM, voice, flashlight

  ************************************

Differences from tBeacon:

* Understands *only* MAVlink/MSP/LTM and is designed for direct connection to the controller APM / PixHawk / INAV
* When connected to the battery it no saves energy and gets through MAVlink not only coordinates but other information
* Coordinates before storing the EEPROM are filtered by the minimum distance - why write standing still?
* Knows how to "talk" not only to the radio but to buzzer, one can move all preflight "talks" to it
* Voice formed by a timer rather than delay() - so is a much cleaner
* In all operating power radio is automatically adjusted according to the strength of the received signal
* Knows how to wait for the end of the unmodulated carrier before switching to transmission
* After arm and up to disarm radio does not turn on at all - so as not to interfere
* When disarming at a radius less than 10 meters from the point of arming the beacon does not turns on (except for the call mode which always works)
* After crash timed beacon activated immediately, without delay (if enabled)
* Beacon is able to operate GSM-module SIM800, and send an SMS with the coordinates of a crash landing (and in the air in case of emergency or decline triggered the parachute)
* Control of the parachute rescue system (servo on CHUTE_PIN)
* Can operate gas discharge flashlight (STROBE_PIN) which flashes once a call
* Can transmit the coordinates in Morse code (for radio men)
* Can transmit the coordinates in DTMF
* TODO:(?) transfer coordinate points to the server as it moves, with thinning and controlled distance.
    (questionable necessity because GSM transmitter on board is not very helpful for the rest of the electronics)
* GPLv3 :)

Removed:
* No support for a direct connection to the GPS
* No Auto protocol - always MAVlink/UAVtalk/MWII
* No support callsign Morse code (although if anyone should something can be done easily and with the encoding on board)
* There is no control the format of coordinates of the configurator - it is given in the compile time
* (Almost) do not have control of the pin connecting buzzer in ConfigTool - is given in compile time
* No HighSavePower mode
* There is no word "pinnacle" instead a double "beep"


v0.9
Config Tool is working!


to compile:

download SingleSerial library from Github
download GCS_MAVLink library from OSD project

then add 

 ##define SKIP_FLOAT

to SingleSerial/BetterSteam.h

then use build script build.sh 
