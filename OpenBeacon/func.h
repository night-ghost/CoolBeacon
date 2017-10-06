


// чтение пакетов нужного протокола
bool getData(){
//LED_BLINK;
    byte sts;

    static uint8_t cnt;

    if(lflags.data_link_active || lflags.data_mode){
again:
#if defined(USE_MAVLINK)
	sts=read_mavlink();
#elif defined(USE_UAVTALK)
	sts=uavtalk_read();
#elif defined(USE_MWII)
	sts=mwii_read();
#else
#error no protocol defined
#endif

	lflags.data_mode=lflags.data_link_active; // если не пришло валидного пакета то сбросится
	return sts;
    } else {


#if defined(AUTOBAUD)
	    serial.end();
	    if(cnt==0){
    	        Red_LED_ON;
	        Green_LED_ON; // two leds together = no valid data
	    }
	    
	    static uint8_t last_pulse = 15;
	    uint32_t pt = millis() + 100; // не более 0.1 секунды
	
	    uint8_t pulse=255;
	    
	    for(byte i=250; i!=0; i--){
	        long t=pulseIn(PD0, 0, 2500); // 2500uS * 250 = 
	        if(t>255) continue;	     // too long - not single bit
	        uint8_t tb = t;       // it less than 255 so convert to byte
	        if(tb==0) continue;   // no pulse at all
	        if(tb<pulse) pulse=tb;// find minimal possible - it will be bit time
	        
	        if(millis()>pt) break; 
	    }
	    
	    long speed;
	    
	    if(pulse == 255)    pulse = last_pulse; // no input at all - use last
	    else                last_pulse = pulse; // remember last correct time
	
	// F_CPU   / BAUD for 115200 is 138
	// 1000000 / BAUD for 115200 is 8.68uS
	//  so I has no idea about pulse times - they are simply measured
	
	    if(     pulse < 11) 	speed = 115200;
	    else if(pulse < 19) 	speed =  57600;
	    else if(pulse < 29) 	speed =  38400;
	    else if(pulse < 40) 	speed =  28800;
	    else if(pulse < 60) 	speed =  19200;
	    else if(pulse < 150)	speed =   9600;
	    else                        speed =   4800;

	    if(cnt==0){
    	        Green_LED_OFF;
	        Red_LED_OFF;
	    }
	    cnt = (cnt+1)%4;

	    serial.begin(speed);
#endif    
	
	    lflags.data_mode=true; // пробуем почитать данные
	    if(lflags.hasPower) goto again; // при наличии питания - сразу же
    }
    return 0;
}

#if !defined(USE_MAVLINK) // we need  crc_init() and crc_accumulate()!
 #include "../GCS_MAVLink/include/mavlink/v1.0/checksum.h"
#endif
