#define HardwareSerial_h

#include <GCS_MAVLink.h>

//#include "../GCS_MAVLink/include/mavlink/v1.0/mavlink_types.h"
//#include "../GCS_MAVLink/include/mavlink/v1.0/ardupilotmega/mavlink.h"

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



bool read_mavlink(){
    static mavlink_message_t msg; 
    mavlink_status_t status;

    uint8_t base_mode=0;
    bool got_coords=0; // to return flag

    lflags.mavlink_got=0;	// нет данных


    //grabing data 
    while(serial.available() > 0) { 
        uint8_t c = serial.read();
	byte mav_severity;

        //trying to grab msg  
        if(mavlink_parse_char(MAVLINK_COMM_0, c, &msg, &status)) {
            lastMAVBeat = millis();
            lflags.mavlink_active = lflags.mavlink_got = true;

            //handle msg
            switch(msg.msgid) {
            case MAVLINK_MSG_ID_HEARTBEAT:
                
                lflags.mavbeat = 1;
//                   apm_mav_system    = msg.sysid;
//                    apm_mav_component = msg.compid;
                 //   apm_mav_type      = mavlink_msg_heartbeat_get_type(&msg);
                mav_mode = (uint8_t)mavlink_msg_heartbeat_get_custom_mode(&msg); // flight mode
            
                //Mode (arducoper armed/disarmed)
                base_mode = mavlink_msg_heartbeat_get_base_mode(&msg);
                lflags.motor_armed = (base_mode & MAV_MODE_FLAG_SAFETY_ARMED)!=0;
                break;
                
            case MAVLINK_MSG_ID_SYS_STATUS:
                mav_vbat_A = mavlink_msg_sys_status_get_voltage_battery(&msg) ; //Battery voltage, in millivolts (1 = 1 millivolt)
                break;

            case MAVLINK_MSG_ID_GPS_RAW_INT:
                coord.lat = mavlink_msg_gps_raw_int_get_lat(&msg);
                coord.lon = mavlink_msg_gps_raw_int_get_lon(&msg);
                mav_fix_type = mavlink_msg_gps_raw_int_get_fix_type(&msg);
                mav_satellites_visible = mavlink_msg_gps_raw_int_get_satellites_visible(&msg);
        
                got_coords=true; // мы получили координаты!
                lflags.pointDirty = true; // есть свежие координаты
                break; 

            case MAVLINK_MSG_ID_VFR_HUD:
                mav_throttle = (uint8_t)mavlink_msg_vfr_hud_get_throttle(&msg); // 0-100
                mav_alt_rel = (long)(mavlink_msg_vfr_hud_get_alt(&msg) * 100); //< Current altitude (MSL), in meters * 100
                mav_climb = (long)(mavlink_msg_vfr_hud_get_climb(&msg) * 100); //< Current climb rate in meters/second * 100
    
                chute_got_climb();
                break;


            case MAVLINK_MSG_ID_ATTITUDE:
// сразу преобразуем в градусы дабы не хранить float
                mav_pitch = ToDeg(mavlink_msg_attitude_get_pitch(&msg));
                mav_roll = ToDeg(mavlink_msg_attitude_get_roll(&msg));
                break;

	    case MAVLINK_MSG_ID_HIGHRES_IMU:
		// 
		last_baro_alt = mav_baro_alt; // запомним предыдущее значение
		mav_baro_alt = (long)(mavlink_msg_highres_imu_get_pressure_alt(&msg) * 100);
		
		mav_xacc = (int)(mavlink_msg_highres_imu_get_xacc(&msg) * 1000); ///< X acceleration (m/s^2) * 1000
		mav_yacc = (int)(mavlink_msg_highres_imu_get_yacc(&msg) * 1000); ///< Y acceleration (m/s^2) * 1000
		mav_zacc = (int)(mavlink_msg_highres_imu_get_zacc(&msg) * 1000); ///< Z acceleration (m/s^2) * 1000

		chute_got_imu();
		break;


            case MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT: //desired values
//              xtrack_error = mavlink_msg_nav_controller_output_get_xtrack_error(&msg);
                mav_alt_error = (long)(mavlink_msg_nav_controller_output_get_alt_error(&msg) * 100); // in meters * 100
    
                chute_got_alterr();
                break;

#if 0
            case MAVLINK_MSG_ID_SCALED_PRESSURE:
		press_abs = mavlink_msg_scaled_pressure_get_press_abs(&msg);
		press_diff = mavlink_msg_scaled_pressure_get_press_diff(&msg);
                break;
#endif

	    case MAVLINK_MSG_ID_STATUSTEXT:
		mav_severity =  mavlink_msg_statustext_get_severity(&msg);
		if(SEVERITY_HIGH <= mav_severity) { // обрабатываем новое системное сообщение только высокой важности
		    byte len= mavlink_msg_statustext_get_text(&msg, (char *)buf);
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
        delayMicroseconds(138);
        //next one
    }
    // Update global packet drops counter
//    packet_drops += status.packet_rx_drop_count;
//    parse_error += status.parse_error;

    return got_coords;
}

