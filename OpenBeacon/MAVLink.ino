//#include "compat.h"

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

        //trying to grab msg  
        if(mavlink_parse_char(MAVLINK_COMM_0, c, &msg, &status)) {
            lastMAVBeat = millis();
            // 
            lflags.mavlink_active = lflags.mavlink_got = true;

//digitalWrite(LEDPIN, !digitalRead(LEDPIN)); // Эта строка мигает светодиодом на плате. Удобно и прикольно :)

            //handle msg
            switch(msg.msgid) {
            case MAVLINK_MSG_ID_HEARTBEAT:
                {
                    lflags.mavbeat = 1;
                    apm_mav_system    = msg.sysid;
                    apm_mav_component = msg.compid;
                 //   apm_mav_type      = mavlink_msg_heartbeat_get_type(&msg);            
                    //mav_mode = (uint8_t)mavlink_msg_heartbeat_get_custom_mode(&msg);
                    //Mode (arducoper armed/disarmed)
                    base_mode = mavlink_msg_heartbeat_get_base_mode(&msg);

                    lflags.motor_armed = getBit(base_mode,7);
//                    if(lflags.motor_armed) {
//                	if(!lflags.motor_was_armed) // если только что заармили
//                	    armTime=uptime;		// запомнить время
//                	lflags.motor_was_armed=1;
//                    }
                }
                break;
                
            case MAVLINK_MSG_ID_SYS_STATUS:
                {
//                    if(!flags.useExtVbattA){
                        mav_vbat_A = mavlink_msg_sys_status_get_voltage_battery(&msg) ; //Battery voltage, in millivolts (1 = 1 millivolt)
//                        mav_battery_remaining_A = mavlink_msg_sys_status_get_battery_remaining(&msg); //Remaining battery energy: (0%: 0, 100%: 100)
//                    }
//                    if (!flags.useExtCurr)
//                        mav_curr_A = mavlink_msg_sys_status_get_current_battery(&msg); //Battery current, in 10*milliamperes (1 = 10 milliampere)

                    //mav_mode = apm_mav_component;//Debug
                }
                break;

            case MAVLINK_MSG_ID_GPS_RAW_INT:
                {
//                    mav_alt_gps = mavlink_msg_gps_raw_int_get_alt(&msg);  // *1000
                    coord.lat = mavlink_msg_gps_raw_int_get_lat(&msg);
                    coord.lon = mavlink_msg_gps_raw_int_get_lon(&msg);
                    mav_fix_type = mavlink_msg_gps_raw_int_get_fix_type(&msg);
                    mav_satellites_visible = mavlink_msg_gps_raw_int_get_satellites_visible(&msg);
                    
                    got_coords=true; // мы получили координаты!
                    lflags.pointDirty = true; // есть свежие координаты
                    
//                    mav_cog = mavlink_msg_gps_raw_int_get_cog(&msg);
//                    eph = mavlink_msg_gps_raw_int_get_eph(&msg);
                }
                break; 

            case MAVLINK_MSG_ID_VFR_HUD:
                {
//                    mav_airspeed = mavlink_msg_vfr_hud_get_airspeed(&msg);
//                    mav_groundspeed = mavlink_msg_vfr_hud_get_groundspeed(&msg);
//                    mav_heading = mavlink_msg_vfr_hud_get_heading(&msg); // 0..360 deg, 0=north
                    mav_throttle = (uint8_t)mavlink_msg_vfr_hud_get_throttle(&msg);
                    mav_alt_rel = mavlink_msg_vfr_hud_get_alt(&msg);
                    mav_climb = mavlink_msg_vfr_hud_get_climb(&msg);
                }
                break;


            case MAVLINK_MSG_ID_ATTITUDE:
                {
// сразу преобразуем в градусы дабы не хранить float
                    mav_pitch = ToDeg(mavlink_msg_attitude_get_pitch(&msg));
                    mav_roll = ToDeg(mavlink_msg_attitude_get_roll(&msg));
//                    mav_yaw = ToDeg(mavlink_msg_attitude_get_yaw(&msg));
                }
                break;

/*
            case MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT:
                { //desired values
                  wp_target_bearing = mavlink_msg_nav_controller_output_get_target_bearing(&msg);
                  wp_dist = mavlink_msg_nav_controller_output_get_wp_dist(&msg);
                  xtrack_error = mavlink_msg_nav_controller_output_get_xtrack_error(&msg);
                }
                break;

            case MAVLINK_MSG_ID_MISSION_CURRENT:
                {
                    wp_number = (uint8_t)mavlink_msg_mission_current_get_seq(&msg);
                }
                break;

            case MAVLINK_MSG_ID_RC_CHANNELS_RAW:
                {
                    chan_raw[0] = mavlink_msg_rc_channels_raw_get_chan1_raw(&msg);
                    chan_raw[1] = mavlink_msg_rc_channels_raw_get_chan2_raw(&msg);
                    chan_raw[2] = mavlink_msg_rc_channels_raw_get_chan3_raw(&msg);
                    chan_raw[3] = mavlink_msg_rc_channels_raw_get_chan4_raw(&msg);
                    chan_raw[4] = mavlink_msg_rc_channels_raw_get_chan5_raw(&msg);
                    chan_raw[5] = mavlink_msg_rc_channels_raw_get_chan6_raw(&msg);
                    chan_raw[6] = mavlink_msg_rc_channels_raw_get_chan7_raw(&msg);
                    chan_raw[7] = mavlink_msg_rc_channels_raw_get_chan8_raw(&msg);
                    mav_rssi = mavlink_msg_rc_channels_raw_get_rssi(&msg);
                }
                break;           
            case MAVLINK_MSG_ID_WIND:
                {
                    mav_winddirection = mavlink_msg_wind_get_direction(&msg); // 0..360 deg, 0=north
                    mav_windspeed = mavlink_msg_wind_get_speed(&msg); //m/s
//                    mav_windspeedz = mavlink_msg_wind_get_speed_z(&msg); //m/s
                }
                break;

*/
#if 0
            case MAVLINK_MSG_ID_SCALED_PRESSURE:
                {
//                    temperature = mavlink_msg_scaled_pressure_get_temperature(&msg);
		    press_abs = mavlink_msg_scaled_pressure_get_press_abs(&msg);
		    press_diff = mavlink_msg_scaled_pressure_get_press_diff(&msg);
/*

packet.press_abs = press_abs;
packet.press_diff = press_diff;

*/
                    
                }
                break;
#endif

#if 0
            case MAVLINK_MSG_ID_SCALED_PRESSURE2:
                    temperature = mavlink_msg_scaled_pressure2_get_temperature(&msg);
/*

packet.press_abs = press_abs;
packet.press_diff = press_diff;

*/
                break;
#endif


	    case MAVLINK_MSG_ID_STATUSTEXT:
		mav_severity =  mavlink_msg_statustext_get_severity(&msg);
		if(SEVERITY_HIGH == mav_severity) { // обрабатываем новое системное сообщение только высокой важности
		    byte len= mavlink_msg_statustext_get_text(&msg, mav_txtbuf);
		    mav_txtbuf[len]=0;
		} else
		    mav_severity = 0; // иначе игнорируем

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
