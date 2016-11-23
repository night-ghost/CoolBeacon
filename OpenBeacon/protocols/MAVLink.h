#ifndef MAVLink_H
#define MAVLink_H

#define HardwareSerial_h

#include <GCS_MAVLink.h>

// gcs_mavlink.h not included, so
// severity levels used in STATUSTEXT messages
enum gcs_severity {
    SEVERITY_LOW=1,
    SEVERITY_MEDIUM,
    SEVERITY_HIGH,
    SEVERITY_CRITICAL,
    SEVERITY_USER_RESPONSE
};

// static int packet_drops = 0; unused
// static int parse_error = 0; unused

extern struct loc_flags lflags;  // все булевые флаги кучей

void NOINLINE millis_plus(uint32_t *dst, uint16_t inc) {
    *dst = millis() + inc;
}

bool read_mavlink(){
    mavlink_status_t status;

    uint8_t base_mode=0;
    bool got_coords=0; // to return flag

    lflags.mavlink_got=0;	// нет данных


    //grabing data 
    while(serial.available_S() > 0) { 
        uint8_t c = serial.read_S();
	byte mav_severity;
	byte apm_mav_type;

        //trying to grab msg  
        if(mavlink_parse_char(MAVLINK_COMM_0, c, NULL, &status)) {
            //lastMAVBeat = millis();
            millis_plus(&lastMAVBeat, 0);
            
            lflags.data_link_active = lflags.mavlink_got = true;

	    if( msg.m.msgid!=MAVLINK_MSG_ID_HEARTBEAT &&                // not heartbeat
		mav_system && mav_system != msg.m.sysid)        // another system
		    break; // skip packet

            //handle msg
            switch(msg.m.msgid) {
            case MAVLINK_MSG_ID_HEARTBEAT:
                apm_mav_type = mavlink_msg_heartbeat_get_type(&msg.m);  // quad hexa octo etc
                if(apm_mav_type == 6) break; // GCS
                
//                lflags.mavbeat = 1; not used
                mav_system    = msg.m.sysid;
//                   apm_mav_component = msg.m.compid;
                mav_mode = (uint8_t)mavlink_msg_heartbeat_get_custom_mode(&msg.m); // flight mode
            
                //Mode (arducoper armed/disarmed)
                base_mode = mavlink_msg_heartbeat_get_base_mode(&msg.m);
                lflags.motor_armed = (base_mode & MAV_MODE_FLAG_SAFETY_ARMED)!=0;
                break;
                
            case MAVLINK_MSG_ID_SYS_STATUS:
                mav_vbat_A = mavlink_msg_sys_status_get_voltage_battery(&msg.m) ; //Battery voltage, in millivolts (1 = 1 millivolt)
                break;

            case MAVLINK_MSG_ID_GPS_RAW_INT:
                coord.lat = mavlink_msg_gps_raw_int_get_lat(&msg.m);
                coord.lon = mavlink_msg_gps_raw_int_get_lon(&msg.m);
                mav_fix_type = mavlink_msg_gps_raw_int_get_fix_type(&msg.m);
                mav_satellites_visible = mavlink_msg_gps_raw_int_get_satellites_visible(&msg.m);
        
                got_coords=true; // мы получили координаты!
                lflags.pointDirty = true; // есть свежие координаты
                break; 

            case MAVLINK_MSG_ID_VFR_HUD:
                mav_throttle = (uint8_t)mavlink_msg_vfr_hud_get_throttle(&msg.m); // 0-100
                mav_alt_rel = (long)(mavlink_msg_vfr_hud_get_alt(&msg.m) * 100); //< Current altitude (MSL), in meters * 100
                mav_climb = (long)(mavlink_msg_vfr_hud_get_climb(&msg.m) * 100); //< Current climb rate in meters/second * 100
    
		extern void chute_got_climb();
                chute_got_climb();
                break;


            case MAVLINK_MSG_ID_ATTITUDE:
// сразу преобразуем в градусы дабы не хранить float
                mav_pitch = ToDeg(mavlink_msg_attitude_get_pitch(&msg.m));
                mav_roll = ToDeg(mavlink_msg_attitude_get_roll(&msg.m));

		extern void chute_got_atti();
		chute_got_atti();
                break;

	    case MAVLINK_MSG_ID_HIGHRES_IMU:
		// 
		last_baro_alt = mav_baro_alt; // запомним предыдущее значение
		mav_baro_alt = (long)(mavlink_msg_highres_imu_get_pressure_alt(&msg.m) * 100);
		
		mav_xacc = (int)(mavlink_msg_highres_imu_get_xacc(&msg.m) * 1000); ///< X acceleration (m/s^2) * 1000
		mav_yacc = (int)(mavlink_msg_highres_imu_get_yacc(&msg.m) * 1000); ///< Y acceleration (m/s^2) * 1000
		mav_zacc = (int)(mavlink_msg_highres_imu_get_zacc(&msg.m) * 1000); ///< Z acceleration (m/s^2) * 1000

		extern void chute_got_imu();
		chute_got_imu();
		break;


            case MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT: //desired values
//              xtrack_error = mavlink_msg_nav_controller_output_get_xtrack_error(&msg.m);
                mav_alt_error = (long)(mavlink_msg_nav_controller_output_get_alt_error(&msg.m) * 100); // in meters * 100
    
		extern void chute_got_alterr();
                chute_got_alterr();
                break;

#if 0
            case MAVLINK_MSG_ID_SCALED_PRESSURE:
		press_abs  = mavlink_msg_scaled_pressure_get_press_abs(&msg.m);
		press_diff = mavlink_msg_scaled_pressure_get_press_diff(&msg.m);
                break;
#endif

	    case MAVLINK_MSG_ID_STATUSTEXT:
		mav_severity =  mavlink_msg_statustext_get_severity(&msg.m);
		if(SEVERITY_HIGH >= mav_severity) { // обрабатываем новое системное сообщение только высокой важности
		    byte len= mavlink_msg_statustext_get_text(&msg.m, (char *)buf);
		    buf[len]=0;
		//"Crash: Disarming"
		    static const char PROGMEM pat[]="crash:";
		    if(strncasecmp_P( (char *)buf, pat, sizeof(pat)-1 )==0){
			lflags.crash=true;	// дизарм будет чуть позже
		    }
		}
		break;

            default:
                //Do nothing
                break;
            }
        }
        if(!Serial.available_S())
            delayMicroseconds((1000000/TELEMETRY_SPEED*10));
        //next one
    }
    // Update global packet drops counter
//    packet_drops += status.packet_rx_drop_count;
//    parse_error += status.parse_error;

    return got_coords;
}

#endif
