


using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.IO.Ports;
using System.IO;
using ArdupilotMega;
using System.Xml;
using System.Globalization;
using System.Text.RegularExpressions;
using System.Net;
using System.Diagnostics;
using System.Threading;
using System.Reflection;

namespace CT {
    using uint32_t = System.UInt32;
    using uint16_t = System.UInt16;
    using uint8_t = System.Byte;
    using int8_t = System.SByte;




    
    public partial class CTool : Form {
        

        //*****************************************/		
        public const string VERSION = "r001";


        
             /*------------------------------------------------*/
        public const int MAVLINK_MAX_PAYLOAD_LEN = 255;
        public const int MAVLINK_NUM_CHECKSUM_BYTES = 2;
        public const int MAVLINK_STX = 254;
        public const int X25_INIT_CRC = 0xffff;

        public struct mavlink_message {
            public uint16_t checksum; /// sent at end of packet
            public uint8_t magic;   /// protocol magic marker
            public uint8_t len;     /// Length of payload
            public uint8_t seq;     /// Sequence of packet
            public uint8_t sysid;   /// ID of message sender system/aircraft
            public uint8_t compid;  /// ID of the message sender component
            public uint8_t msgid;   /// ID of message in payload
            public uint8_t[] payload; // MAVLINK_MAX_PAYLOAD_LEN + MAVLINK_NUM_CHECKSUM_BYTES
        };

        public enum mavlink_parse_state_t {
            MAVLINK_PARSE_STATE_UNINIT = 0,
            MAVLINK_PARSE_STATE_IDLE,
            MAVLINK_PARSE_STATE_GOT_STX,
            MAVLINK_PARSE_STATE_GOT_SEQ,
            MAVLINK_PARSE_STATE_GOT_LENGTH,
            MAVLINK_PARSE_STATE_GOT_SYSID,
            MAVLINK_PARSE_STATE_GOT_COMPID,
            MAVLINK_PARSE_STATE_GOT_MSGID,
            MAVLINK_PARSE_STATE_GOT_PAYLOAD,
            MAVLINK_PARSE_STATE_GOT_CRC1
        }; ///< The state machine for the comm parser

        public struct mavlink_status {
            public uint8_t msg_received;               /// Number of received messages
            public uint8_t buffer_overrun;             /// Number of buffer overruns
            public uint8_t parse_error;                /// Number of parse errors
            public mavlink_parse_state_t parse_state;  /// Parsing state machine
            public uint8_t packet_idx;                 /// Index in current packet
            public uint8_t current_rx_seq;             /// Sequence number of last packet received
            public uint8_t current_tx_seq;             /// Sequence number of last packet sent
            public uint16_t packet_rx_success_count;   /// Received packets
            public uint16_t packet_rx_drop_count;      /// Number of packet drops
        };
        
   
        string processingpanel = "";
        /// <summary>
        /// use to draw the red outline box is currentlyselected matchs
        /// </summary>
        bool selectedrectangle = false;
        /// <summary>
        /// use to as a invalidator
        /// </summary>
        bool startup = false;

          

        public SerialPort comPort = new SerialPort();


        public mavlink_status status = new mavlink_status() { msg_received = 0, buffer_overrun = 0, parse_error = 0, parse_state = mavlink_parse_state_t.MAVLINK_PARSE_STATE_IDLE, packet_idx = 0, current_rx_seq = 0, current_tx_seq = 0, packet_rx_success_count = 0, packet_rx_drop_count = 0 };

        public mavlink_message rxmsg;

     
        bool fReenter=false;
        bool fDone=false;

        private bool tlog_run = false;
        public byte[] tlog_data;
        System.Threading.Thread tlog_thread;
        public System.Threading.Thread com_thread;

        public bool comBusy=false;
        public bool com_run=false;

        string CurrentCOM;

        const int PARAMS_END=30; // количество целых параметров

        public CTool() {
           

            InitializeComponent();
            setupFunctions(); 
        }

 

        //Set item boxes
        void setupFunctions() {
            processingpanel = "";

            com_thread = new System.Threading.Thread(com_thread_proc);
            com_thread.Start();
      }

        public  string[] GetPortNames() {
            string[] devs = new string[0];

            if (Directory.Exists("/dev/"))
                devs = Directory.GetFiles("/dev/", "*ACM*");

            string[] ports = SerialPort.GetPortNames();

            string[] all = new string[devs.Length + ports.Length];

            devs.CopyTo(all, 0);
            ports.CopyTo(all, devs.Length);

            return all;
        }


        string currentVersion = "";

        private void OSD_Load(object sender, EventArgs e) {
            string strVersion = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version.ToString();
            this.Text = this.Text + " " + strVersion + " - " + VERSION;
            currentVersion = strVersion + VERSION;

            CMB_ComPort.Items.AddRange(GetPortNames());

            if (CMB_ComPort.Items.Count > 0)
                CMB_ComPort.SelectedIndex = 0;

            xmlconfig(false);

            Translate(this);

            EnableControls(false);

        }    


        //Write data to  EPPROM
        private void BUT_WriteOSD_Click(object sender, EventArgs e) {
            this.BeginInvoke((MethodInvoker)delegate {
                //Sub_WriteOSD();
            });
        }

 
        

        //Write data to  EPPROM
        private int BUT_ResetOSD_EEPROM() {
            return 0;   
        }



        private void comboBox1_Click(object sender, EventArgs e) {
            CMB_ComPort.Items.Clear();
            CMB_ComPort.Items.AddRange(GetPortNames());
        }



   

        private void OSD_Resize(object sender, EventArgs e) {

            try {
             

            } catch { }
        }



 


        byte[] readIntelHEXv2(StreamReader sr) {
            byte[] FLASH = new byte[1024 * 256];

            int optionoffset = 0;
            int total = 0;
            bool hitend = false;

            while (!sr.EndOfStream) {
                toolStripProgressBar1.Value = (int)(((float)sr.BaseStream.Position / (float)sr.BaseStream.Length) * 100);

                string line = sr.ReadLine();

                if (line.StartsWith(":")) {
                    int length = Convert.ToInt32(line.Substring(1, 2), 16);
                    int address = Convert.ToInt32(line.Substring(3, 4), 16);
                    int option = Convert.ToInt32(line.Substring(7, 2), 16);
                    Console.WriteLine("len {0} add {1} opt {2}", length, address, option);

                    if (option == 0) {
                        string data = line.Substring(9, length * 2);
                        for (int i = 0; i < length; i++) {
                            byte byte1 = Convert.ToByte(data.Substring(i * 2, 2), 16);
                            FLASH[optionoffset + address] = byte1;
                            address++;
                            if ((optionoffset + address) > total)
                                total = optionoffset + address;
                        }
                    } else if (option == 2) {
                        optionoffset = (int)Convert.ToUInt16(line.Substring(9, 4), 16) << 4;
                    } else if (option == 1) {
                        hitend = true;
                    }
                    int checksum = Convert.ToInt32(line.Substring(line.Length - 2, 2), 16);

                    byte checksumact = 0;
                    for (int z = 0; z < ((line.Length - 1 - 2) / 2); z++) { // minus 1 for : then mins 2 for checksum itself 
                        checksumact += Convert.ToByte(line.Substring(z * 2 + 1, 2), 16);
                    }
                    checksumact = (byte)(0x100 - checksumact);

                    if (checksumact != checksum) {
                        MessageBox.Show("The hex file loaded is invalid, please try again.");
                        throw new Exception("Checksum Failed - Invalid Hex");
                    }
                }              
            }

            if (!hitend) {
                MessageBox.Show("The hex file did no contain an end flag. aborting");
                throw new Exception("No end flag in file");
            }

            Array.Resize<byte>(ref FLASH, total);

            return FLASH;
        }

        void sp_Progress(int progress) {
            toolStripStatusLabel1.Text = "Uploading " + progress + " %";
            toolStripProgressBar1.Value = progress;

            statusStrip1.Refresh();
        }


  

   

        private void saveToFileToolStripMenuItem_Click(object sender, EventArgs e) {
            SaveFileDialog sfd = new SaveFileDialog() { Filter = "*.osd|*.osd" };

            sfd.ShowDialog();

            if (sfd.FileName != "") {
                try {
/*
                    using (StreamWriter sw = new StreamWriter(sfd.OpenFile())) {       //Write                    
                        //Panel 1
                        for (int k = 0; k < npanel; k++) {
                            sw.WriteLine("{0}", "Panel " + k);
                            foreach (var item in this.scr[k].panelItems) {
                                if (item != null) {
                                    TreeNode[] tnArray = scr[k].LIST_items.Nodes.Find(item.name, true);
                                    if (tnArray.Length > 0)
                                        sw.WriteLine("{0}\t{1}\t{2}\t{3}\t{4}", item.name, item.x, item.y, tnArray[0].Checked.ToString(), item.sign);
                                }
                            }
                        }

                        //Config 
                        sw.WriteLine("{0}", "Configuration");
                        sw.WriteLine("{0}\t{1}", "Model Type", (byte)(ModelType)cbxModelType.SelectedItem); //We're just saving what's in the config screen, not the eeprom model type
                        sw.WriteLine("{0}\t{1}", "Units", pan.converts);
                        sw.WriteLine("{0}\t{1}", "Overspeed", pan.overspeed);
                        sw.WriteLine("{0}\t{1}", "Stall", pan.stall);
                        sw.WriteLine("{0}\t{1}", "Battery", pan.battv);
                        sw.WriteLine("{0}\t{1}", "RSSI High", pan.rssical);
                        sw.WriteLine("{0}\t{1}", "RSSI Low", pan.rssipersent);
                        sw.WriteLine("{0}\t{1}", "RSSI Enable Raw", pan.rssiraw_on);
                        sw.WriteLine("{0}\t{1}", "Toggle Channel", pan.ch_toggle);
                        sw.WriteLine("{0}\t{1}", "Auto Screen Switch", pan.auto_screen_switch);
                        sw.WriteLine("{0}\t{1}", "Chanel Rotation Switching", pan.switch_mode);
                        sw.WriteLine("{0}\t{1}", "Video Mode", pan.pal_ntsc);
                        sw.WriteLine("{0}\t{1}", "Auto Mode", pan.mode_auto);
                        sw.WriteLine("{0}\t{1}", "Battery Warning Level", pan.batt_warn_level);
                        sw.WriteLine("{0}\t{1}", "RSSI Warning Level", pan.rssi_warn_level);
                        sw.WriteLine("{0}\t{1}", "OSD Brightness", pan.osd_brightness);
                        sw.WriteLine("{0}\t{1}", "Call Sign", pan.callsign_str);
                        sw.WriteLine("{0}\t{1}", "flgHUD", pan.flgHUD);
                        sw.WriteLine("{0}\t{1}", "flgTrack", pan.flgTrack);
                        //                        sw.WriteLine("{0}\t{1}", "Sign Air Speed", pan.sign_air_speed);
                        //                        sw.WriteLine("{0}\t{1}", "Sign Ground  Speed", pan.sign_ground_speed);
                        //                        sw.WriteLine("{0}\t{1}", "Sign Home Altitude", pan.sign_home_altitude);
                        //                        sw.WriteLine("{0}\t{1}", "Sign MSL Altitude", pan.sign_msl_altitude);

                        
                        sw.WriteLine("{0}\t{1}", "BattB", pan.battBv);
                        sw.WriteLine("{0}\t{1}", "rssi_k", pan.rssi_koef);
                        sw.WriteLine("{0}\t{1}", "curr_k", pan.Curr_koef);
                        sw.WriteLine("{0}\t{1}", "batt_a_k", pan.battA_koef);
                        sw.WriteLine("{0}\t{1}", "batt_b_k", pan.battB_koef);
                        sw.WriteLine("{0}\t{1}", "roll_k", pan.roll_k);
                        sw.WriteLine("{0}\t{1}", "pitch_k", pan.pitch_k);
                        sw.WriteLine("{0}\t{1}", "roll_kn", pan.roll_k_ntsc);
                        sw.WriteLine("{0}\t{1}", "pitch_kn", pan.pitch_k_ntsc);


                        sw.WriteLine("{0}\t{1}", "fBattA", pan.flgBattA);
                        sw.WriteLine("{0}\t{1}", "fBattB", pan.flgBattB);
                        sw.WriteLine("{0}\t{1}", "fCurr", pan.flgCurrent);

                        sw.WriteLine("{0}\t{1}", "fRadar", pan.flgRadar);
                        sw.WriteLine("{0}\t{1}", "fILS", pan.flgILS);
                        sw.WriteLine("{0}\t{1}", "HOS", pan.horiz_offs);
                        sw.WriteLine("{0}\t{1}", "VOS", pan.vert_offs);
                        // выходной PWM
                        sw.WriteLine("{0}\t{1}", "PWMSRC", pan.pwm_src);
                        sw.WriteLine("{0}\t{1}", "PWMDST", pan.pwm_dst);
                        //
                        sw.WriteLine("{0}\t{1}", "NSCREENS", pan.n_screens);

                        sw.Close();
                    }
 */ 
                } catch (Exception ex) {
                    MessageBox.Show("Error writing file: " + ex.Message);
                }
            }
        }

        private Boolean updatingRSSI = false;

        private void loadFromFileToolStripMenuItem_Click(object sender, EventArgs e) {
            OpenFileDialog ofd = new OpenFileDialog() { Filter = "*.osd|*.osd" };
            //const int nosdfunctions = 29;
            ofd.ShowDialog();

            startup = true;

            string[] strings = { "" };

            if (ofd.FileName != "") {
                try {
/*
                    using (StreamReader sr = new StreamReader(ofd.OpenFile())) {
                        while (!sr.EndOfStream) {
                            //Panel 1
                            //string stringh = sr.ReadLine(); //
                            string[] hdr = sr.ReadLine().Split(new char[] { '\x20' }, StringSplitOptions.RemoveEmptyEntries);
                            int k = 0;

                            if (hdr[0] != "Panel")
                                break;

                            k = int.Parse(hdr[1]);
   
                            for (int i = 0; i < osd_functions_N; i++) {
                                strings = sr.ReadLine().Split(new char[] { '\t' }, StringSplitOptions.RemoveEmptyEntries);
                                for (int a = 0; a < scr[k].panelItems.Length; a++) {
                                    if (this.scr[k].panelItems[a] != null && scr[k].panelItems[a].name == strings[0]) {
                                        var pi = scr[k].panelItems[a];
                                        // incase there is an invalid line number or to shore
                                        try {
                                            //scr[k].panelItems[a] = new Panel(pi.name, pi.show, int.Parse(strings[1]), int.Parse(strings[2]), pi.pos);
                                            pi.x = int.Parse(strings[1]);
                                            pi.y = int.Parse(strings[2]);
                                            try {
                                                pi.sign = int.Parse(strings[4]);
                                            } catch { }

                                            TreeNode[] tnArray = scr[k].LIST_items.Nodes.Find(scr[k].panelItems[a].name, true);
                                            if (tnArray.Length > 0)
                                                tnArray[0].Checked = (strings[3] == "True");
                                        } catch {
                                        }
                                    }
                                }
                            }
                        }
                        //                        stringh = sr.ReadLine(); //config
                        while (!sr.EndOfStream) {
                            strings = sr.ReadLine().Split(new char[] { '\t' }, StringSplitOptions.RemoveEmptyEntries);

                            if (strings[0] == "Units")
                                try {
                                    pan.converts = byte.Parse(strings[1]) != 0;
                                } catch {
                                    pan.converts = bool.Parse(strings[1]);
                                } else if (strings[0] == "Overspeed")
                                pan.overspeed = byte.Parse(strings[1]);
                            else if (strings[0] == "Stall") pan.stall = byte.Parse(strings[1]);
                            else if (strings[0] == "Battery") pan.battv = byte.Parse(strings[1]);
                            else if (strings[0] == "RSSI High")
                                pan.rssical = (UInt16)(int.Parse(strings[1]));
                            else if (strings[0] == "RSSI Low")
                                pan.rssipersent = (UInt16)(int.Parse(strings[1]));
                            else if (strings[0] == "RSSI Enable Raw") pan.rssiraw_on = byte.Parse(strings[1]);
                            else if (strings[0] == "Toggle Channel") pan.ch_toggle = byte.Parse(strings[1]);
                            else if (strings[0] == "Auto Screen Switch") pan.auto_screen_switch = byte.Parse(strings[1]);
                            else if (strings[0] == "Chanel Rotation Switching") pan.switch_mode = byte.Parse(strings[1]);
                            else if (strings[0] == "Video Mode")
                                try {
                                    pan.pal_ntsc = byte.Parse(strings[1]) != 0;
                                } catch {
                                    pan.pal_ntsc = bool.Parse(strings[1]);
                                }
                                //sw.WriteLine("{0}\t{1}", "Auto Mode", pan.mode_auto);
                            else if (strings[0] == "Auto Mode")
                                try {
                                    pan.mode_auto = byte.Parse(strings[1]) != 0;
                                } catch {
                                    pan.mode_auto = bool.Parse(strings[1]);
                                } else if (strings[0] == "Battery Warning Level") pan.batt_warn_level = byte.Parse(strings[1]);
                            else if (strings[0] == "RSSI Warning Level") pan.rssi_warn_level = byte.Parse(strings[1]);
                            else if (strings[0] == "OSD Brightness") pan.osd_brightness = byte.Parse(strings[1]);
                            else if (strings[0] == "Call Sign") pan.callsign_str = strings[1];
                            else if (strings[0] == "Model Type") cbxModelType.SelectedItem = (ModelType)(pan.model_type = byte.Parse(strings[1])); //we're not overwriting "eeprom" model type
                            //                            else if (strings[0] == "Sign Air Speed") pan.sign_air_speed = byte.Parse(strings[1]);
                            //                            else if (strings[0] == "Sign Ground  Speed") pan.sign_ground_speed = byte.Parse(strings[1]);
                            //                            else if (strings[0] == "Sign Home Altitude") pan.sign_home_altitude = byte.Parse(strings[1]);
                            //                            else if (strings[0] == "Sign MSL Altitude") pan.sign_msl_altitude = byte.Parse(strings[1]);
                            else if (strings[0] == "BattB") pan.battBv = byte.Parse(strings[1]);
                            else if (strings[0] == "rssi_k") pan.rssi_koef = float.Parse(strings[1]);
                            else if (strings[0] == "curr_k") pan.Curr_koef = float.Parse(strings[1]);
                            else if (strings[0] == "batt_a_k") pan.battA_koef = float.Parse(strings[1]);
                            else if (strings[0] == "batt_b_k") pan.battB_koef = float.Parse(strings[1]);
                            else if (strings[0] == "roll_k") pan.roll_k = float.Parse(strings[1]);
                            else if (strings[0] == "pitch_k") pan.pitch_k = float.Parse(strings[1]);
                            else if (strings[0] == "roll_kn") pan.roll_k_ntsc = float.Parse(strings[1]);
                            else if (strings[0] == "pitch_kn") pan.pitch_k_ntsc = float.Parse(strings[1]);
                            else if (strings[0] == "fBattA") pan.flgBattA = bool.Parse(strings[1]);
                            else if (strings[0] == "fBattB") pan.flgBattB = bool.Parse(strings[1]);
                            else if (strings[0] == "fCurr") pan.flgCurrent = bool.Parse(strings[1]);
                            else if (strings[0] == "fRadar") pan.flgRadar = bool.Parse(strings[1]);
                            else if (strings[0] == "fILS") pan.flgILS = bool.Parse(strings[1]);
                            else if (strings[0] == "HOS") pan.horiz_offs = (byte)int.Parse(strings[1]);
                            else if (strings[0] == "VOS") pan.vert_offs = (byte)int.Parse(strings[1]);
                            else if (strings[0] == "PWMSRC") pan.pwm_src = (byte)int.Parse(strings[1]);
                            else if (strings[0] == "PWMDST") pan.pwm_dst = (byte)int.Parse(strings[1]);
                            else if (strings[0] == "NSCREENS") pan.n_screens = (byte)int.Parse(strings[1]);
                            else if (strings[0] == "flgHUD") pan.flgHUD = bool.Parse(strings[1]);
                            else if (strings[0] == "flgTrack") pan.flgTrack = bool.Parse(strings[1]);
                        }



                        //pan.model_type = (byte)cbxModelType.SelectedItem;

                        //Modify units
                        if (!pan.converts) {
                            UNITS_combo.SelectedIndex = 0; //metric
                            STALL_label.Text = cbxModelType.SelectedItem.ToString() == "Copter" ? "Max VS (m/min) / 10" : "Stall Speed (km/h)";
                            OVERSPEED_label.Text = "Overspeed (km/h)";
                        } else {
                            UNITS_combo.SelectedIndex = 1; //imperial
                            STALL_label.Text = cbxModelType.SelectedItem.ToString() == "Copter" ? "Max VS (ft/min) / 10" : "Stall Speed (mph)";
                            OVERSPEED_label.Text = "Overspeed (mph)";
                        }

                        OVERSPEED_numeric.Value = pan.overspeed;
                        STALL_numeric.Value = pan.stall;
                        MINVOLT_numeric.Value = Convert.ToDecimal(pan.battv) / Convert.ToDecimal(10.0);

                        //RSSI_numeric_max.Value = pan.rssical;
                        //RSSI_numeric_min.Value = pan.rssipersent;

                        updatingRSSI = true;
                        RSSI_numeric_min.Minimum = 0;
                        RSSI_numeric_min.Maximum = 2047;
                        RSSI_numeric_max.Minimum = 0;
                        RSSI_numeric_max.Maximum = 2047;
                        RSSI_numeric_min.Value = 0;
                        RSSI_numeric_max.Value = 0;
                        RSSI_RAW.Checked = Convert.ToBoolean(pan.rssiraw_on % 2);
                        if ((int)(pan.rssiraw_on / 2) == 0 || pan.rssiraw_on / 2 == 1){ // analog 
                          } else { // pwm 
 
                        }

                        try {
                            RSSI_numeric_min.Value = pan.rssipersent;
                        } catch {
                            RSSI_numeric_min.Value = RSSI_numeric_min.Minimum;
                        }
                        try {
                            RSSI_numeric_max.Value = pan.rssical;
                        } catch {
                            RSSI_numeric_max.Value = RSSI_numeric_max.Minimum;
                        }

                        cbxRSSIChannel.SelectedIndex = rssi_decode(pan.rssiraw_on);

                        if (pan.ch_toggle >= toggle_offset && pan.ch_toggle < 9) ONOFF_combo.SelectedIndex = pan.ch_toggle - toggle_offset;
                        else ONOFF_combo.SelectedIndex = 0; //reject garbage from the red file

                        cbxWarningsAutoPanelSwitch.SelectedItem = (PanelsAutoSwitch)pan.auto_screen_switch;
                        TOGGLE_BEH.Checked = Convert.ToBoolean(pan.switch_mode);

                        CHK_pal.Checked = Convert.ToBoolean(pan.pal_ntsc);
                        CHK_auto.Checked = Convert.ToBoolean(pan.mode_auto);

                        chkHUD.Checked = Convert.ToBoolean(pan.flgHUD);
                        chkTrack.Checked = Convert.ToBoolean(pan.flgTrack);

                        BATT_WARNnumeric.Value = pan.batt_warn_level;
                        RSSI_WARNnumeric.Value = pan.rssi_warn_level;

                        BRIGHTNESScomboBox.SelectedIndex = pan.osd_brightness;

                        try {
                            numHOS.Value = pan.horiz_offs - 0x20;
                            numVOS.Value = pan.vert_offs - 0x10;
                        } catch {
                            pan.horiz_offs = (byte)numHOS.Value;
                            pan.vert_offs = (byte)numVOS.Value;
                        }

                        txtCallSign.Text = pan.callsign_str;

                        //                        cbxAirSpeedSign.Checked = pan.sign_air_speed!=0;
                        //                        cbxGroundSpeedSign.Checked = pan.sign_ground_speed!=0;
                        //                        cbxHomeAltitudeSign.Checked = pan.sign_home_altitude!=0 ;
                        //                        cbxMslAltitudeSign.Checked = pan.sign_msl_altitude!=0 ;

                        this.CHK_pal_CheckedChanged(EventArgs.Empty, EventArgs.Empty);
                        this.pALToolStripMenuItem_CheckStateChanged(EventArgs.Empty, EventArgs.Empty);
                        this.nTSCToolStripMenuItem_CheckStateChanged(EventArgs.Empty, EventArgs.Empty);
                        // new!
                        numMinVoltB.Text = (pan.battBv / 10.0).ToString();
                        txtRSSI_k.Text = pan.rssi_koef.ToString();
                        txtCurr_k.Text = pan.Curr_koef.ToString();
                        txtBattA_k.Text = pan.battA_koef.ToString();
                        txtBattB_k.Text = pan.battB_koef.ToString();
                        txtRollPal.Text = pan.roll_k.ToString();
                        txtPitchPal.Text = pan.pitch_k.ToString();
                        txtRollNtsc.Text = pan.roll_k_ntsc.ToString();
                        txtPitchNtsc.Text = pan.pitch_k_ntsc.ToString();


                        cbBattA_source.SelectedIndex = pan.flgBattA ? 1 : 0;
                        cbCurrentSoure.SelectedIndex = pan.flgCurrent ? 1 : 0;

                        chkRadar.Checked = pan.flgRadar;
                        chkILS.Checked = pan.flgILS;

                        try {
                            cbOutSource.SelectedIndex = pan.pwm_src;
                            cbOutPin.SelectedIndex = pan.pwm_dst;
                        } catch (Exception ex) { }

                        try {
                            cbNscreens.SelectedIndex = pan.n_screens - 1;
                        } catch (Exception ex) { }

                    }
 */ 
                } catch (Exception ex) {
                    MessageBox.Show("Error Reading file at " + ex.Message + " str=" + strings[0] + " val=" + strings[1]);
                } finally {
                    updatingRSSI = false;
                }
            }
            startup = false;
           
        }

        private void loadDefaultsToolStripMenuItem_Click(object sender, EventArgs e) {
            setupFunctions();
        }

        private void exitToolStripMenuItem_Click(object sender, EventArgs e) {
            Application.Exit();
        }



        private void updateFirmwareToolStripMenuItem_Click(object sender, EventArgs e) {
            toolStripProgressBar1.Style = ProgressBarStyle.Continuous;
            this.toolStripStatusLabel1.Text = "";

            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Filter = "*.hex|*.hex";
            if (ofd.ShowDialog() == System.Windows.Forms.DialogResult.Cancel)
                return;

            if (ofd.FileName != "") {
                byte[] FLASH;
                bool spuploadflash_flag = false;
                try {
                    toolStripStatusLabel1.Text = "Reading Hex File";

                    statusStrip1.Refresh();

                    FLASH = readIntelHEXv2(new StreamReader(ofd.FileName));
                } catch {
                    MessageBox.Show("Bad Hex File");
                    return;
                }

                //bool fail = false;
                ArduinoSTK sp = OpenArduino();

                toolStripStatusLabel1.Text = "Connecting to Board";

                if (sp != null && sp.connectAP()) {

                    sp.Progress += new ArduinoSTK.ProgressEventHandler(sp_Progress);//////
                    try {
                        for (int i = 0; i < 3; i++) { //try to upload 3 times //try to upload n times if it fail
                            spuploadflash_flag = sp.uploadflash(FLASH, 0, FLASH.Length, 0);
                            if (!spuploadflash_flag) {
                                if (sp.keepalive())
                                    Console.WriteLine("keepalive successful (iter " + i + ")");
                                else
                                    Console.WriteLine("keepalive fail (iter " + i + ")");
                            } else
                                break;
                        }

                    } catch (Exception ex) {

                        MessageBox.Show(ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }

                } else {
                    MessageBox.Show("Failed to talk to bootloader");
                }

                sp.Close();

                if (spuploadflash_flag) {
                    // TODO: update pan.fw_version = conf.eeprom.FW_version ;
                    //		pan.cs_version = conf.eeprom.CS_version;

                    toolStripStatusLabel1.Text = "Done";

                    MessageBox.Show("Done!");
                } else {
                    MessageBox.Show("Upload failed. Lost sync. Try using Arduino to upload instead",
                                "Error",
                                MessageBoxButtons.OK,
                                MessageBoxIcon.Warning);
                    toolStripStatusLabel1.Text = "Failed";
                }
            }            

        }

 
        private void sendTLogToolStripMenuItem_Click(object sender, EventArgs e) {

            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Filter = "Tlog|*.tlog";

            ofd.ShowDialog();

            if (ofd.FileName == "")
                return;



            BinaryReader br = new BinaryReader(ofd.OpenFile());

            this.toolStripProgressBar1.Style = ProgressBarStyle.Marquee;
            this.toolStripStatusLabel1.Text = "Loading TLOG data...";

            try {
                toolStripProgressBar1.Style = ProgressBarStyle.Continuous;
                toolStripStatusLabel1.Text = "";
            } catch { }
        }

        private void OSD_FormClosed(object sender, FormClosedEventArgs e) {
            fDone=true;

            System.Threading.Thread.Sleep(50);

            if (tlog_run)
                tlog_thread.Abort();


            try {
                com_thread.Abort ();
            } catch{}

            if (comPort.IsOpen) comPort.Close();

            xmlconfig(true);
        }

        private String arduinoIDEPath = "Arduino-1.6.5";
        private String planeSketchPath = "ArduCAM_OSD";
        private String copterSketchPath = "ArduCAM_OSD";
        private bool autoUpdate = false;
        private bool checkForUpdates = true;

        private void xmlconfig(bool write) {
            if (write || !File.Exists(Path.GetDirectoryName(Application.ExecutablePath) + Path.DirectorySeparatorChar + @"config.xml")) {
                try {
                    XmlTextWriter xmlwriter = new XmlTextWriter(Path.GetDirectoryName(Application.ExecutablePath) + Path.DirectorySeparatorChar + @"config.xml", Encoding.ASCII);
                    xmlwriter.Formatting = Formatting.Indented;

                    xmlwriter.WriteStartDocument();

                    xmlwriter.WriteStartElement("Config");

                    xmlwriter.WriteElementString("comport", CMB_ComPort.Text);

                    xmlwriter.WriteElementString("AutoUpdate", autoUpdate.ToString());

                    xmlwriter.WriteElementString("CheckForUpdates", checkForUpdates.ToString());

                    xmlwriter.WriteEndElement();

                    xmlwriter.WriteEndDocument();
                    xmlwriter.Close();

                    //appconfig.Save();
                } catch (Exception ex) { MessageBox.Show(ex.ToString()); }
            } else {
                try {
                    using (XmlTextReader xmlreader = new XmlTextReader(Path.GetDirectoryName(Application.ExecutablePath) + Path.DirectorySeparatorChar + @"config.xml")) {
                        while (xmlreader.Read()) {
                            xmlreader.MoveToElement();
                            try {
                                switch (xmlreader.Name) {
                                case "comport":
                                    string temp = xmlreader.ReadString();
                                    CMB_ComPort.Text = temp;
                                    break;
                                 case "AutoUpdate":
                                    autoUpdate = (xmlreader.ReadString().ToUpper() == "TRUE");
                                    cbxShowUpdateDialog.Checked = !autoUpdate;
                                    break;
                                case "CheckForUpdates":
                                    checkForUpdates = (xmlreader.ReadString().ToUpper() == "TRUE");
                                    cbxAutoUpdate.Checked = checkForUpdates;
                                    break;
                                case "xml":
                                    break;
                                default:
                                    if (xmlreader.Name == "") // line feeds
                                        break;
                                    break;
                                }
                            } catch (Exception ee) { Console.WriteLine(ee.Message); } // silent fail on bad entry
                        }
                    }
                } catch (Exception ex) { Console.WriteLine("Bad Config File: " + ex.ToString()); } // bad config file
            }
        }

 

        private void updateFontToolStripMenuItem_Click(object sender, EventArgs e) {
            Application.DoEvents();

            this.BeginInvoke((MethodInvoker)delegate {
                sub_updateCharset();
            });
        }

        private void sub_updateCharset() {
            toolStripProgressBar1.Style = ProgressBarStyle.Continuous;
            toolStripStatusLabel1.Text = "";

            if (tlog_run)
                tlog_thread.Abort();

            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Filter = "mcm|*.mcm";

            DialogResult dialogResp = ofd.ShowDialog();
            if ((dialogResp != System.Windows.Forms.DialogResult.OK) || (ofd.FileName.Trim() == ""))
                return;


            Application.DoEvents();

            //Get file version
            string fileVersion = "000";
            string tempFileName = ofd.SafeFileName.ToUpper();
            if (tempFileName.StartsWith("MINIMOSD_")) {
                tempFileName = tempFileName.Remove(0, 9);
                if (tempFileName.EndsWith(".MCM")) {
                    tempFileName = tempFileName.Remove(tempFileName.Length - 4, 3);
                    string[] versionArray = tempFileName.Split('.');
                    Int16 version1, version2, version3;
                    if (versionArray.Length > 2) {
                        if (Int16.TryParse(versionArray[0], out version1) &&
                           Int16.TryParse(versionArray[1], out version2) &&
                           Int16.TryParse(versionArray[2], out version3))
                            fileVersion = version1.ToString().Substring(0, 1).Trim() + version2.ToString().Substring(0, 1).Trim() + version3.ToString().Substring(0, 1).Trim();
                    }
                }
            }

            Application.DoEvents();

            if (ofd.FileName != "") {
                if (comPort.IsOpen)
                    comPort.Close();

                try {
                    comBusy=true;
                    comPort.PortName = CMB_ComPort.Text;
                    comPort.BaudRate = 57600;

                    comPort.Open();

                    comPort.DtrEnable = false;
                    comPort.RtsEnable = false;

                    System.Threading.Thread.Sleep(50);

                    comPort.DtrEnable = true;
                    comPort.RtsEnable = true;

                    System.Threading.Thread.Sleep(1000);
                    Application.DoEvents();

                    while (comPort.BytesToRead != 0)
                        parseInput(comPort.ReadExisting());

                    comPort.WriteLine("");
                    comPort.WriteLine("");
               

                    int timeout = 0;

                    while (comPort.BytesToRead == 0) {
                        System.Threading.Thread.Sleep(100);
                        comPort.WriteLine("");
                        Console.WriteLine("Waiting... " + timeout.ToString());
                        timeout++;
                        Application.DoEvents();

                        if (timeout > 60) {
                            MessageBox.Show("Error entering font mode - No Data");
                            comPort.Close();
                            comBusy = false;
                            return;
                        }
                    }
                    string readFont = comPort.ReadLine();
                    if (!readFont.Contains("RFF")) {
                        MessageBox.Show("Error entering CharSet upload mode - invalid data: " + readFont);
                        comPort.Close();
                        comBusy = false;
                        return;
                    }

                } catch { MessageBox.Show("Error opening com port", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error); return; }

                using (var stream = ofd.OpenFile()) {

                    BinaryReader br = new BinaryReader(stream);
                    StreamReader sr2 = new StreamReader(br.BaseStream);

                    string device = sr2.ReadLine();

                    if (device != "MAX7456") {
                        MessageBox.Show("Invalid MCM");
                        comPort.Close();
                        comBusy = false;
                        return;
                    }

                    br.BaseStream.Seek(0, SeekOrigin.Begin);

                    toolStripStatusLabel1.Text = "CharSet Uploading";

                    long length = br.BaseStream.Length;
                    while (br.BaseStream.Position < br.BaseStream.Length && !this.IsDisposed) {
                        try {
                            toolStripProgressBar1.Value = (int)((br.BaseStream.Position / (float)br.BaseStream.Length) * 100);


                            int read = 256 * 3;// 163847 / 256 + 1; // 163,847 font file
                            if ((br.BaseStream.Position + read) > br.BaseStream.Length) {
                                read = (int)(br.BaseStream.Length - br.BaseStream.Position);
                            }
                            length -= read;
                            byte[] buffer = br.ReadBytes(read);
                            Console.WriteLine("write " + buffer.ToString());
                            comPort.Write(buffer, 0, buffer.Length);
                            int timeout = 0;

                            while (comPort.BytesToRead == 0 && read == 768) {
                                System.Threading.Thread.Sleep(10);
                                Application.DoEvents();
                                timeout++;

                                if (timeout > 100) {
                                    MessageBox.Show("CharSet upload failed - no response");
                                    comPort.Close();
                                    comBusy = false;
                                    return;
                                }
                            }

                            string resp = comPort.ReadExisting(); //CDn
                            

                        } catch {
                            break;
                        }

                        Application.DoEvents();
                    }
                    comPort.WriteLine("\r\n");
                    //Wait for last char acknowledge
                    int t = 0;
                    while (comPort.BytesToRead == 0) {
                        System.Threading.Thread.Sleep(10);
                        Application.DoEvents();
                        t++;

                        if (t > 10) {
                            MessageBox.Show("No end");
                            comPort.Close();
                            comBusy = false;
                            return;
                        }
                    }
                    //Console.WriteLine(comPort.ReadExisting());
                    if (comPort.BytesToRead != 0)
                        comPort.ReadLine();

                    comPort.WriteLine("\r\n\r\n\r\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");

                    comPort.DtrEnable = false;
                    comPort.RtsEnable = false;

                    System.Threading.Thread.Sleep(50);
                    Application.DoEvents();

                    comPort.DtrEnable = true;
                    comPort.RtsEnable = true;

                    System.Threading.Thread.Sleep(50);
                    Application.DoEvents();

                    comPort.Close();

                    comPort.DtrEnable = false;
                    comPort.RtsEnable = false;

                    toolStripProgressBar1.Value = 100;
                    toolStripStatusLabel1.Text = "CharSet Done";
                }

                toolStripStatusLabel1.Text = "CharSet Done!!!";
            }
            comBusy = false;
        }


       


        private void gettingStartedToolStripMenuItem_Click(object sender, EventArgs e) {
            try {
                System.Diagnostics.Process.Start("https://github.com/night-ghost/https://github.com/night-ghost/CoolBeacon/blob/master/wiki/Config_Tool.md");
            } catch { MessageBox.Show("Webpage open failed... do you have a virus?"); }
        }

        private void aboutToolStripMenuItem_Click(object sender, EventArgs e) {
            AboutBox1 about = new AboutBox1();
            about.Show();
        }



        private bool UploadFirmware(string fileName) {
            if (string.IsNullOrEmpty(fileName))
                return false;

            if (tlog_run)
                tlog_thread.Abort();

            byte[] FLASH;
            bool spuploadflash_flag = false;
            try {
                toolStripStatusLabel1.Text = "Reading Hex File";

                statusStrip1.Refresh();

                FLASH = readIntelHEXv2(new StreamReader(fileName));
            } catch { MessageBox.Show("Bad Hex File"); return false; }

            comBusy = true;
            ArduinoSTK sp = OpenArduino();

            toolStripStatusLabel1.Text = "Connecting to Board";

            if (sp != null && sp.connectAP()) {
                sp.Progress += new ArduinoSTK.ProgressEventHandler(sp_Progress);
                try {
                    for (int i = 0; i < 3; i++) { //try to upload 3 times  //try to upload n times if it fail
                        spuploadflash_flag = sp.uploadflash(FLASH, 0, FLASH.Length, 0);
                        if (!spuploadflash_flag) {
                            if (sp.keepalive()) Console.WriteLine("keepalive successful (iter " + i + ")");
                            else Console.WriteLine("keepalive fail (iter " + i + ")");
                            //toolStripStatusLabel1.Text = "Lost sync. Reconnecting...";
                        } else break;
                    }

                } catch (Exception ex) {
                    //fail = true;
                    MessageBox.Show(ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            } else {
                MessageBox.Show("Failed to talk to bootloader");
            }

            sp.Close();
            comBusy = false;

            if (spuploadflash_flag) {

                toolStripStatusLabel1.Text = "Done";

                MessageBox.Show("Done!");
            } else {
                MessageBox.Show("Upload failed. Lost sync. Try using Arduino to upload instead",
                            "Error",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Warning);
                toolStripStatusLabel1.Text = "Failed";
            }
            return true;
        }


 


        private void GetFwFromOSD() {
            byte[] FLASH = new byte[32 * 1024];
            //byte[] FLASH = new byte[30382];

            comBusy = true;
            ArduinoSTK sp = OpenArduino();


            if (sp != null && sp.connectAP()) {
                try {
                    int start = 0;
                    short length = 0x100;

                    while (start < FLASH.Length) {
                        sp.setaddress(start);
                        sp.downloadflash(length).CopyTo(FLASH, start);
                        start += length;
                    }

                    StreamWriter sw = new StreamWriter(Path.GetDirectoryName(Application.ExecutablePath) + Path.DirectorySeparatorChar + "FW" + Path.DirectorySeparatorChar + @"flash.bin", false);
                    BinaryWriter bw = new BinaryWriter(sw.BaseStream);
                    bw.Write(FLASH, 0, FLASH.Length);
                    bw.Close();

                    sw = new StreamWriter(Path.GetDirectoryName(Application.ExecutablePath) + Path.DirectorySeparatorChar + "FW" + Path.DirectorySeparatorChar + @"flash.hex", false);
                    for (int i = 0; i < FLASH.Length; i += 16) {
                        string add = string.Format("{0:X4}", i);
                        if (i % (0x1000 << 4) == 0) {
                            if (i != 0)
                                sw.WriteLine(":02000002{0:X4}{1:X2}", ((i >> 4) & 0xf000), 0x100 - (2 + 2 + (((i >> 4) & 0xf000) >> 8) & 0xff));
                        }
                        if (add.Length == 5) {
                            add = add.Substring(1);
                        }
                        sw.Write(":{0:X2}{1}00", 16, add);
                        byte ck = (byte)(16 + (i & 0xff) + ((i >> 8) & 0xff));
                        for (int a = 0; a < 16; a++) {
                            ck += FLASH[i + a];
                            sw.Write("{0:X2}", FLASH[i + a]);
                        }
                        sw.WriteLine("{0:X2}", (byte)(0x100 - ck));
                    }

                    sw.Close();
                } catch (Exception ex) {
                    MessageBox.Show(ex.Message);
                }

            }
            sp.Close();
            comBusy = false;
        }

  
        private void CALLSIGNmaskedText_Validating(object sender, CancelEventArgs e) {
            string validString = "";
            foreach (char c in txtCallSign.Text) {

                if ((c == '-') || ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || ((c >= '0') && (c <= '9')))
                    validString += c;
            }
            txtCallSign.Text = validString;
        }

        private void CALLSIGNmaskedText_Validated(object sender, EventArgs e) {
            string s = txtCallSign.Text;
            //convert to lowercase on validate
            s = s.ToLower(new CultureInfo("en-US", false));

            txtCallSign.Text = s;

        }

        private void OnlyNumbers_Validating(object sender, CancelEventArgs e) {
            
            Int32 v=(Int32)  myConvert(txtCallSign.Text);

            txtCallSign.Text = v.ToString();
        }


        int Constrain(int value, int min, int max) {
            if (value < min)
                return (int)min;
            if (value > max)
                return (int)max;

            return (int)value;
        }

        public string convertChars(string s) {
            string so = "";
            try {
                for (int i = 0; i < s.Length; i++) {
                    char c = s[i];
                    if (c == '\\') {
                        i++;
                        if (i == s.Length) break;
                        switch (s[i]) {
                        case 'n':
                            so += "\n";
                            break;
                        case 'r':
                            so += "\r";
                            break;
                        case 't':
                            so += "\n";
                            break;

                        case '0': // octal string
                        case '1':
                            string oct = "";
                            oct += s[i];
                            i++;
                            if (i == s.Length) break;
                            oct += s[i];
                            i++;
                            if (i == s.Length) break;
                            oct += s[i];
                            so += Convert.ToByte(oct, 8);
                            break;

                        case 'x': // hex string
                            string hex = "";
                            i++;
                            if (i == s.Length) break;
                            hex += s[i];
                            i++;
                            if (i == s.Length) break;
                            hex += s[i];
                            so += Convert.ToChar(Convert.ToByte(hex, 16));
                            break;

                        }
                    } else {
                        so += c;
                    }
                }
                return so;
            } catch {
                return s;
            }
        }

        public string myDecode(string s) {
            string so = "";
            try {
                for (int i = 0; i < s.Length; i++) {
                    char c = s[i];
                    if (c == 0) break;
                    if (c < 0x20 || c >= 0x80) {
                        string hex = Convert.ToString(c, 16);
                        if (hex.Length % 2 != 0)
                            hex = '0' + hex;
                        so += "\\x" + hex;
                    } else
                        so += c;
                }
                return so;
            } catch {
                return s;
            }
        }
        public float myConvert(string s) {
            bool f = false;
            double v = 0;


            do {
                try {
                    v = Convert.ToDouble(s);
                    f = true;
                } catch {
                };

                if (!f) {
                    string s1 = s.Replace('.', ',');
                    try {
                        v = Convert.ToDouble(s1);
                        f = true;
                    } catch {
                    };
                }
                if (!f && s.Length != 0) s = s.Substring(0, s.Length - 1);
            } while (!f && s != "");

            return (float)v;
        }
 
        public ArduinoSTK OpenArduino() {
            ArduinoSTK sp;
            try {
                if (comPort.IsOpen)
                    comPort.Close();

                sp = new ArduinoSTK();
                sp.PortName = CMB_ComPort.Text;
                sp.BaudRate = 57600;
                sp.DataBits = 8;
                sp.StopBits = StopBits.One;
                sp.Parity = Parity.None;
                //				sp.DtrEnable = true;
                //				sp.RtsEnable = false; //added

                sp.Open();
                return sp;
            } catch {
                MessageBox.Show("Error opening com port", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error); return null;
            }

        }

        public static ushort crc_accumulate(byte b, ref ushort crc) {
            unchecked {
                byte ch = (byte)(b ^ (byte)(crc & 0x00ff));
                ch = (byte)(ch ^ (ch << 4));
                crc = (ushort)((crc >> 8) ^ (ch << 8) ^ (ch << 3) ^ (ch >> 4));
                return crc;
            }
        }
        static void crc_init(ref uint16_t crcAccum) {
            crcAccum = X25_INIT_CRC;
        }

        private void mavlink_start_checksum(ref mavlink_message msg) {
            crc_init(ref msg.checksum);
        }

        void mavlink_update_checksum(ref mavlink_message msg, uint8_t c) {
            crc_accumulate(c, ref msg.checksum);
        }

        public byte mavlink_parse_char(uint8_t c) {
            /*
                default message crc function. You can override this per-system to
                put this data in a different memory segment
            */

            //mavlink_message_t* rxmsg = mavlink_get_channel_buffer(chan); ///< The currently decoded message
            //mavlink_status_t* status = mavlink_get_channel_status(chan); ///< The current decode status

            int bufferIndex = 0;

            status.msg_received = 0;

            switch (status.parse_state) {


            case mavlink_parse_state_t.MAVLINK_PARSE_STATE_IDLE:
                if (c == MAVLINK_STX) {
                    status.parse_state = mavlink_parse_state_t.MAVLINK_PARSE_STATE_GOT_STX;
                    rxmsg.len = 0;
                    //			rxmsg->magic = c;
                    mavlink_start_checksum(ref rxmsg);
                    //mavlink_comm_0_port->printf_P(PSTR("\n\ngot STX!"));
                    return (byte)1;
                } else {
                    status.parse_state = mavlink_parse_state_t.MAVLINK_PARSE_STATE_IDLE;
                }
                break;

            case mavlink_parse_state_t.MAVLINK_PARSE_STATE_GOT_STX:
                if (status.msg_received != 0
                    /* Support shorter buffers than the
                        default maximum packet size */
                        ) {
                    status.buffer_overrun++;
                    status.parse_error++;
                    status.msg_received = 0;
                    status.parse_state = mavlink_parse_state_t.MAVLINK_PARSE_STATE_IDLE;
                } else {
                    //mavlink_comm_0_port->printf_P(PSTR(" got Length!"));

                    // NOT counting STX, LENGTH, SEQ, SYSID, COMPID, MSGID, CRC1 and CRC2
                    rxmsg.len = c;
                    status.packet_idx = 0;
                    mavlink_update_checksum(ref rxmsg, c);
                    status.parse_state = mavlink_parse_state_t.MAVLINK_PARSE_STATE_GOT_LENGTH;
                }
                break;

            case mavlink_parse_state_t.MAVLINK_PARSE_STATE_GOT_LENGTH:
                //mavlink_comm_0_port->printf_P(PSTR(" got Seq!"));

                rxmsg.seq = c;
                mavlink_update_checksum(ref rxmsg, c);
                status.parse_state = mavlink_parse_state_t.MAVLINK_PARSE_STATE_GOT_SEQ;
                break;

            case mavlink_parse_state_t.MAVLINK_PARSE_STATE_GOT_SEQ:
                //mavlink_comm_0_port->printf_P(PSTR(" got Sysid!"));

                rxmsg.sysid = c;
                mavlink_update_checksum(ref rxmsg, c);
                status.parse_state = mavlink_parse_state_t.MAVLINK_PARSE_STATE_GOT_SYSID;
                break;

            case mavlink_parse_state_t.MAVLINK_PARSE_STATE_GOT_SYSID:
                //mavlink_comm_0_port->printf_P(PSTR(" got Compid!"));

                rxmsg.compid = c;
                mavlink_update_checksum(ref rxmsg, c);
                status.parse_state = mavlink_parse_state_t.MAVLINK_PARSE_STATE_GOT_COMPID;
                break;

            case mavlink_parse_state_t.MAVLINK_PARSE_STATE_GOT_COMPID:
                rxmsg.msgid = c;
                mavlink_update_checksum(ref rxmsg, c);
                if (rxmsg.len == 0) {
                    status.parse_state = mavlink_parse_state_t.MAVLINK_PARSE_STATE_GOT_PAYLOAD;
                } else {
                    status.parse_state = mavlink_parse_state_t.MAVLINK_PARSE_STATE_GOT_MSGID;
                }
                break;

            case mavlink_parse_state_t.MAVLINK_PARSE_STATE_GOT_MSGID:
                rxmsg.payload[status.packet_idx++] = (byte)c;
                mavlink_update_checksum(ref rxmsg, c);
                if (status.packet_idx == rxmsg.len) {
                    status.parse_state = mavlink_parse_state_t.MAVLINK_PARSE_STATE_GOT_PAYLOAD;

                }
                break;

            case mavlink_parse_state_t.MAVLINK_PARSE_STATE_GOT_PAYLOAD:

                if (false && c != (rxmsg.checksum & 0xFF)) {
                    //mavlink_comm_0_port->printf_P(PSTR("\n CRC1 err! want=%d got=%d"), rxmsg->checksum & 0xFF, c);
                    // Check first checksum byte
                    status.parse_error++;
                    status.msg_received = 0;
                    if (c == MAVLINK_STX) {
                        status.parse_state = mavlink_parse_state_t.MAVLINK_PARSE_STATE_GOT_STX;
                        rxmsg.len = 0;
                        mavlink_start_checksum(ref rxmsg);

                    } else {
                        status.parse_state = mavlink_parse_state_t.MAVLINK_PARSE_STATE_IDLE;
                    }
                } else {
                    //  {
                    status.parse_state = mavlink_parse_state_t.MAVLINK_PARSE_STATE_GOT_CRC1;
                    rxmsg.payload[status.packet_idx] = (byte)c;
                }
                break;

            case mavlink_parse_state_t.MAVLINK_PARSE_STATE_GOT_CRC1:
                /*		        if (c != (rxmsg.checksum >> 8)) {
                                    // Check second checksum byte
                        //mavlink_comm_0_port->printf_P(PSTR("\nCRC2 err! want=%d got=%d"), rxmsg->checksum >> 8, c);

                                    status.parse_error++;
                                    status.msg_received = 0;
			
                                    if (c == MAVLINK_STX) {
                                        status.parse_state = mavlink_parse_state_t.MAVLINK_PARSE_STATE_GOT_STX;
                                        rxmsg.len = 0;                        
                                        mavlink_start_checksum(ref rxmsg);
                                    } else {
                                        //status->parse_state = MAVLINK_PARSE_STATE_IDLE;     
                                        status.parse_state = mavlink_parse_state_t.MAVLINK_PARSE_STATE_IDLE;
                                    }
                                } else {
                //*/ {
                    // Successfully got message
                    status.msg_received = 1;
                    status.parse_state = mavlink_parse_state_t.MAVLINK_PARSE_STATE_IDLE;
                    rxmsg.payload[status.packet_idx + 1] = (byte)c;
                }
     break;
            }

            bufferIndex++;
            // If a message has been sucessfully decoded, check index
            if (status.msg_received != 0) {
                //while(status->current_seq != rxmsg->seq)
                //{
                //	status->packet_rx_drop_count++;
                //               status->current_seq++;
                //}
                status.current_rx_seq = rxmsg.seq;
                // Initial condition: If no packet has been received so far, drop count is undefined
                if (status.packet_rx_success_count == 0) status.packet_rx_drop_count = 0;
                // Count this packet as received
                status.packet_rx_success_count++;
            }

            //status.parse_error = 0;
            status.current_rx_seq += 1;
            //status.packet_rx_success_count = status->packet_rx_success_count;
            status.packet_rx_drop_count = status.parse_error;
            //r_mavlink_status->buffer_overrun = status->buffer_overrun;

            return status.msg_received != 0 ? (byte)2 : (byte)0;
        }


        private void btnTLog_Click(object sender, EventArgs e) {
            tlog_run = !tlog_run;
            btnTLog.Text = tlog_run ? "Stop" : "Start";

            if (tlog_run) {
                CurrentCOM = CMB_ComPort.Text;
                tlog_thread = new System.Threading.Thread(thread_proc);
                tlog_thread.Start();
            } else {
                tlog_thread.Abort();
                comBusy = false;
                //if (comPort.IsOpen)
                //    comPort.Close();
            }
        }


        UInt64 get_timestamp(byte[] bytes, int ptr) {
            UInt64 time = 0;
            byte[] datearray = new Byte[8];
            for (int i = 0; i < 8; i++) {
                datearray[i] = bytes[ptr + i];
            }
            Array.Reverse(datearray);
            time = (ulong)BitConverter.ToInt64(datearray, 0);
            return time;
        }

        void com_thread_proc (){
            while(!fDone){
                if(!comBusy){
                    try{
                        if (comPort.IsOpen && comPort.BytesToRead > 0) {
                            string s = comPort.ReadExisting();
                            parseInput (s);
                        }
                    } catch{}
                }
                System.Threading.Thread.Sleep(10);
            }
        }

        void thread_proc() {
            byte[] bytes = tlog_data;
            int frStart = 0;
            int frEnd = 0;
            int frameIndex = 0;
            int np = 0;
            int[] last_seq = new int[256];
            string message;


            status.packet_rx_drop_count = 0;
            status.parse_state = mavlink_parse_state_t.MAVLINK_PARSE_STATE_IDLE;
            status.packet_idx = 0;
            status.packet_rx_success_count = 0;
            status.current_rx_seq = 0;
            status.buffer_overrun = 0;
            status.parse_error = 0;

            for (int i = 0; i < 256; i++) last_seq[i] = 0xff;

            if (comPort.IsOpen)
                comPort.Close();

            try {

                comPort.PortName = CurrentCOM;
                comPort.BaudRate = 57600;
                //comPort.BaudRate = 115200;

                comBusy =true;
                comPort.Open();

                comPort.DtrEnable = true;

            } catch {
                MessageBox.Show("Error opening com port", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            while (!fDone) {
                np = 0;
                UInt64 time, start_time;
                DateTime localtime = DateTime.Now;
                UInt64 stamp = (UInt64)((DateTime.UtcNow - new DateTime(1970, 1, 1)).TotalMilliseconds * 1000);

                int byteIndex;
                try {
                    // MP writes as
                    // byte[] datearray = BitConverter.GetBytes((UInt64)((DateTime.UtcNow - new DateTime(1970, 1, 1)).TotalMilliseconds * 1000));
                    // Array.Reverse(datearray);

                    time = start_time = get_timestamp(bytes, 0);
                    for (byteIndex = 8; byteIndex < bytes.Length; byteIndex++) {
                        if (comPort.BytesToRead != 0)
                            parseInput(comPort.ReadExisting());


                        byte c = bytes[byteIndex];

                        switch (mavlink_parse_char(c)) {
                        case 1: // got STX
                            frStart = byteIndex;
                            break;
                        case 2: // got packet
                            frameIndex++;
                            frEnd = byteIndex;

                            while (comPort.BytesToWrite != 0) // подождем передачи пакета
                                System.Threading.Thread.Sleep(1);

                            comPort.Write(bytes, frStart, frEnd - frStart + 1);

                            if (((last_seq[rxmsg.sysid] + 1) & 0xff) == rxmsg.seq) { // поймали синхронизацию
                                time = get_timestamp(bytes, byteIndex + 1);  // skip parsed CRC2                              
                                byteIndex += 8; // skip timestamp
                            }
                            last_seq[rxmsg.sysid] = rxmsg.seq;
                            np++;
                            try {
                                this.Invoke((MethodInvoker)delegate {
                                    lblTLog.Text = np.ToString(); // runs on UI thread
                                });

                            } catch { };

                            double time_w = (DateTime.UtcNow - new DateTime(1970, 1, 1)).TotalMilliseconds;
                            while (!fDone) {
                                UInt64 diff_log = (time - start_time);
                                UInt64 diff_real = ((UInt64)((DateTime.UtcNow - new DateTime(1970, 1, 1)).TotalMilliseconds * 1000)) - stamp; // если время лога опережает реальное - задерживаемся
                                if (diff_log < diff_real) {
                                    //Console.WriteLine("go");
                                    break;
                                }

                                if (((DateTime.UtcNow - new DateTime(1970, 1, 1)).TotalMilliseconds - time_w) > 100) { // но не реже 10 раз в секунду
                                    start_time = time; // ждали слишком долго, сместим метку времени в логе
                                    break;
                                }
                                //Console.WriteLine("wait");
                                System.Threading.Thread.Sleep(1);
                            }

                            if(fDone) break;

                            message = "";
                            message += "Payload length: " + rxmsg.len.ToString();
                            message += "Packet sequence: " + rxmsg.seq.ToString();
                            message += "System ID: " + rxmsg.sysid.ToString();
                            message += "Component ID: " + rxmsg.compid.ToString();
                            message += "Message ID: " + rxmsg.msgid.ToString();
                            message += "Message: ";
                            for (int x = 0; x < rxmsg.len; x++) {
                                message += rxmsg.payload[x].ToString();
                            }
                            message += "CRC1: " + rxmsg.seq.ToString();
                            message += "CRC2: " + ((int)bytes[byteIndex]).ToString();
                            message += Environment.NewLine;

                            //Console.Write(message);
                            break;
                        }

                        while (comPort.BytesToRead != 0)
                            parseInput(comPort.ReadExisting());
                    }
                } catch {
                    continue;
                }
            }

            comBusy = false;
        }

        private void EnableControls(bool flag){
            foreach(var c in tabPageConfig.Controls){
                ((System.Windows.Forms.Control)c).Enabled =flag;
            }
            foreach (var c in tabPageAlt.Controls) {
                ((System.Windows.Forms.Control)c).Enabled = flag;
            }
            btnDisconnect.Enabled =flag;
            btnConnect.Enabled = ! flag;
        }

        private string delCr(string s){
            if (s.Length == 0) {
                return s;
            }
            if (s[s.Length - 1] == '\r')
                return s.Substring(0, s.Length - 1);
            return s;

        }

        private void parseInput(string s) {
            Console.WriteLine(delCr(s));
        
        }

        private void   getConfig(bool fWaitHeader=true)    {
            
            string s="";

            if(fWaitHeader){
                try {
    again:
                    parseInput(s);
                    if(!waitComAnswer("Error getting config - No Data")){
                        EnableControls(false);
                        comBusy = false;
                        return;
                    }

                    //
                    if (!waitComAnswer("timeout"))
                        return ;
                    s = comPort.ReadLine();
                    if(s=="" || s=="\r") goto again;
                    if (s[0] == '#') goto again;

                    if (!s.Contains("[CONFIG]")) {
                        MessageBox.Show("Error getting config - invalid data: " + s);
                        
                        EnableControls(false);
                        comBusy = false;
                        return;
                    }

                } catch{
                    EnableControls(false);
                    comBusy = false;
                    return;
                }
            }
            while (true) {
                if (!waitComAnswer("Error getting params")) {
                    comBusy = false;
                    return;
                }
                s = comPort.ReadLine();
                if (s[0] == '#') {
                    parseInput(s);
                    continue;
                }

                if (s == ">") break;

                parseParam(delCr(s));
            }

            comBusy = false;
            return;

        }

        private bool waitComAnswer(string error){
            int timeout=0;
            try {
                while (comPort.BytesToRead == 0) {
                    System.Threading.Thread.Sleep(100);
                    Console.WriteLine("Waiting... " + timeout.ToString());
                    timeout++;
                    Application.DoEvents();

                    if (timeout > 60) {
                        MessageBox.Show(error);

                        return false;
                    }
                }
                return true;
            } catch{
                return false ;
            }
        }

        string readBeaconLine(){
            string s;
            while (true) {
                if (!waitComAnswer("timeout"))
                    return "";
                s = delCr(comPort.ReadLine());
                if(s=="") continue;
                if (s[0] == '#') {// skip debug messages
                    parseInput(s);
                    continue;
                }
                if (s == "[CONFIG]") { //  вместо ожидаемого результата пришел полный конфиг
                    getConfig(false);
                    return "";
                }
                break;                    
            }
            return s;
        }

        private void btnConnet_Click(object sender, EventArgs e) {
                       
            btnConnect.Enabled =false ;
            try {

                try {
                    if(comPort.IsOpen)
                        comPort.Close();
                } catch { }

                comPort.PortName = CMB_ComPort.Text;
                comPort.BaudRate = 57600;

                comPort.ReadTimeout =1000; // 1s

                comBusy = true;
                comPort.Open();

                comPort.DtrEnable = false;
                comPort.RtsEnable = false;

                System.Threading.Thread.Sleep(50);

                comPort.DtrEnable = true;
                comPort.RtsEnable = true;

                System.Threading.Thread.Sleep(100);
                Application.DoEvents();

                while (comPort.BytesToRead != 0)
                    parseInput(comPort.ReadExisting());

                
                string s;
                if(!waitComAnswer("Error connecting to beacon - No Data")) {
                    comPort.Close();
                    comBusy = false;
                    btnConnect.Enabled =true;
                    return;
                }

               
                while(true) {
                    s = comPort.ReadLine();
                    if(s[0]!='#') break;    // skip debug messages
                    parseInput(s);
                }
                if (!s.Contains("cBeacon")) {
                    MessageBox.Show("Error connecting to beacon - invalid data: " +s );
                    comPort.Close();
                    comBusy = false;
                    btnConnect.Enabled = true;
                    return;
                }
                string rel=s.Substring(7);
                lblRel.Text ="Release: " + rel;

            } catch { 
                MessageBox.Show("Error opening com port", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                comBusy = false;
                btnConnect.Enabled = true;
                return; 
            }


            comPort.WriteLine("cBeacon");

            getConfig();
            EnableControls(true);
            
        }

        private void beaconCommand(char c){
            comBusy = true;
            readOut ();
            try {
                comPort.WriteLine(c.ToString());
            } catch{
                Console.WriteLine("Error writing to port!");
                EnableControls(false);
            };
        }

        private void getTrack(){
        
        }

        private void openConsole() {
            comBusy = true;
            frmTerm frm = new frmTerm(this);
            //frm.Show();
            frm.ShowInTaskbar = false;
            frm.ShowDialog(); // modal
            //comBusy = false; eats [config], let it be parsed
        }

        public void readOut(){
            try {
                while (comPort.BytesToRead != 0) {
                    parseInput(comPort.ReadExisting());
                    System.Threading.Thread.Sleep(100);
                }
            }catch{}
        }

        void delay(int t){
            System.Threading.Thread.Sleep(t);
        }

        int millis(){
            System.DateTime moment = DateTime.UtcNow;
            
            return moment.Millisecond;
        }

        private void btnDisconnect_Click(object sender, EventArgs e) {
            beaconCommand('q');
            EnableControls(false);
            if(comPort.IsOpen) comPort.Close();
            comBusy = false;
        }

        private void btnSMS_Click(object sender, EventArgs e) {
            beaconCommand('e');
            if (waitComAnswer("Timeout to get"))
                MessageBox.Show("Result: " + readBeaconLine(), "SMS result", MessageBoxButtons.OK, MessageBoxIcon.Information); 
 
        }

        private void BUT_ResetOSD_EEPROM_click(object sender, EventArgs e) {
            beaconCommand('d');
        }


        private void btnCalibrate_Click(object sender, EventArgs e) {
            beaconCommand('c');
            if(waitComAnswer("Timeout to get"))
                Console.WriteLine("Got " + readBeaconLine());
        }

        private void btnTrack_Click(object sender, EventArgs e) {
            beaconCommand('t');
            if (waitComAnswer("Timeout to get"))
                getTrack();
        }

        private void btnConsole_Click(object sender, EventArgs e) {
            beaconCommand('g');
            openConsole();
        }

        private void btnBalance_Click(object sender, EventArgs e) {
            beaconCommand('m');
            if (waitComAnswer("Timeout to get"))
                MessageBox.Show("Balance: " + readBeaconLine(), "Balance", MessageBoxButtons.OK, MessageBoxIcon.Information); 
        }

        private void btnSay_Click(object sender, EventArgs e) {
            beaconCommand('s');
        }

        private void tnSayBuzzer_Click(object sender, EventArgs e) {
            beaconCommand('z');
        }


        private void btnDTMF_Click(object sender, EventArgs e) {
            beaconCommand('f');
        }

        private void btnBeep_Click(object sender, EventArgs e) {
            beaconCommand('b');
        }


        private void btnClearSMS_Click(object sender, EventArgs e) {
            beaconCommand('g'); // direct SIM800 connection
            GSM_command("+CMGD=1,4");
            comPort.WriteLine(".");
        }

        private void btnGetSms_Click(object sender, EventArgs e) {
            beaconCommand('g'); // direct SIM800 connection

            GSM_command("+CMGL=0,0");

            comPort.WriteLine(".");
            MessageBox.Show(gsm_answer, "Balance", MessageBoxButtons.OK, MessageBoxIcon.Information); 
            
        }

        string gsm_answer="";

        // отправка AT-команд
        // usage  command("+SAPBR=3,1,\"CONTYPE\",\"GPRS\""), ["OK"], [""], [10000]);
        int GSM_command(string cmd, string answer, string answer2, uint16_t time){
            readOut();
               
            comPort.WriteLine(cmd);    // Send the command.

            return wait_answer(answer, answer2, time)==1?1:0; // 10 sec
        }

        int GSM_command(string cmd, string answer, string answer2) {
            return GSM_command(cmd, answer, answer2,  10000);
        }
    

        int GSM_command(string cmd, string answer, uint16_t time){
            return GSM_command(cmd, answer, "", time);
        }       

        int GSM_command(string cmd, string answer){
            return GSM_command(cmd, answer, 10000);
        }

        int GSM_command(string cmd){
            return GSM_command(cmd, "OK");
        }

        int GSM_command(string cmd, uint16_t time) {
            return GSM_command(cmd, "OK", time);
        }


        int wait_answer(string answer, string answer2,  int timeout){
            string response="";

            

            int deadtime = millis() + timeout;
            int has_answer=0;            
            int result2_ptr=0;
    
            int hasAnswer2 = (answer2 == "")?1:0;

            delay(100); // модуль не особо шустрый

       
            do { // this loop waits for the answer
        
                delay(10); // чуть подождать чтобы что-то пришло

                if(comPort.BytesToRead>0) {// за время ожидания что-то пришло: 38400 бод - 4800 байт/с, за 10мс придет 48 байт
                    string s=comPort.ReadExisting();
                    response+=s;
                    Console.Write(s);
                    
                    // данные закончились, можно и проверить, если еще ответ не получен
                    if(has_answer==0) {
                        // check if the desired answer  is in the response of the module
                        if( response.Contains(answer))  { // окончательный ответ
                            has_answer = 1;
        
                        } else
                        if(response.Contains("ERROR"))  { // окончательная ошибка
                            has_answer = 3;
        
                        }
                    }
            
                    if( answer2 !="") {
                        if(response.Contains(answer2)) {
                            result2_ptr= response.IndexOf(answer2); // промежуточный ответ
                            hasAnswer2 = 1;
                        }        
                    }
            
                    // TODO: контролировать разбиение на строки
                } else if(has_answer!=0 && hasAnswer2!=0) // за время ожидания ничего не пришло - данные кончились, если ответ получен - готово
             break;
            } while( millis() < deadtime ); // Waits for the asnwer with time out

            if (hasAnswer2 != 0 && result2_ptr!=0) {
                gsm_answer = response.Substring(result2_ptr + answer2.Length);
            }

            return has_answer;
        }



        private void parseParam(string s) {
            if(s=="") return;
            if(s=="cBeacon") return;
            //if(s=="[CONFIG]") return;
            if(s[0] == '#') {parseInput (s); return;}
            if (s == ">") return;
            

            // params in form RN=val
            //Console.WriteLine("ParseParams s="+s);
            string[] sa = s.Split('=');

            int reg = int.Parse(sa[0].Substring(1));
            if(sa.Length<2) {
                Console.WriteLine ("Error parsing param string="+s);
                return;
            }
            if(sa.Length >2){
                sa[1] +="=" + sa[2];
            }
            string v=delCr(sa[1]);

            fReenter = true;

            TextBox tbx = this.Controls.Find("txtParam" + reg.ToString(), true).FirstOrDefault() as TextBox;
            if (tbx == null) {                
                switch(reg){
                case 16:    // no SearchGPS_time
                    break;

                case 17:
                    try {
                        chkParam17.Checked=int.Parse(v)!=0;
                    } catch{}
                    break;
                case 18:
                    try {
                        chkParam18.Checked = int.Parse(v) != 0;
                    } catch { }
                    break;

                case 19: // WakeBuzzerParams  старший байт - номер ноги, младший - тон
                    int val=0;
                    try {
                       val = int.Parse(v);
                    } catch{
                        Console.WriteLine("param19 error converting s=" + v);
                    }
                    txtParam19a.Text = (val / 256).ToString();
                    txtParam19b.Text = (val % 256).ToString();
                    break;

                case 29: // старшие 2байт - LAST_POINT_DISTANCE, младшие - MIN_HOME_DISTANCE
                    val = 0;
                    try {
                        val = int.Parse(v);
                    } catch {
                        Console.WriteLine("param9 error converting s=" + v);
                    }
                    txtParam29a.Text = (val % 65536).ToString(); 
                    txtParam29b.Text = (val >> 16).ToString();
                    break;


                
                case 21:    // no HighSavePower
                    break;

                case 26:    // call sign as string
                case 27:    // call sign as string
                    break;

            // strings
                case PARAMS_END+0:    
                    txtPhone1.Text = v;
                    break;
                case PARAMS_END+1:
                    txtPhone2.Text = v;
                    break;
                case PARAMS_END+2:
                    txtPhone3.Text = v;
                    break;
                case PARAMS_END+3:
                    txtPhone4.Text = v;
                    break;

                case PARAMS_END+4:
                    txtCallSign.Text = v;
                    break;

                case PARAMS_END+5:
                    txtURL.Text = v;
                    break;

                default:
                    Console.WriteLine("Error searhing textbox n=" + reg.ToString());
                    break;
                }
                fReenter = false;
                return;
            }

            switch(sa[0].Substring(0,1)){
            case "S":
                tbx.Text = v;
                break;

            case "R":
                int val = 0;
                try {
                    val = int.Parse(v);
                } catch{
                    Console.WriteLine("Reg" + reg + " error converting s=" + v);
                }
                tbx.Text = val.ToString();
                break;
            }

            //Console.WriteLine("ParseParams r=" + reg.ToString() + " val=" + tbx.Text);
            fReenter = false;
        }


        private void sendParam(int n) {
            int val=0;
            string s="";

            if(fReenter) return;

            TextBox tbx = this.Controls.Find("txtParam" + n, true).FirstOrDefault() as TextBox;
            if (tbx == null) {
                switch(n){
                case 10:  // no GPS format
                case 16:    // no SearchGPS_time
                case 21:    // no HighSavePower
                case 26:    // call sign as string
                case 27:    // call sign as string
                    return;

                case 17:
                    val = chkParam17.Checked ? 1: 0;
                    goto as_number;

                case 18:
                    val = chkParam18.Checked ? 1 : 0;
                    goto as_number;
                    
 
                case 19: // WakeBuzzerParams  старший байт - номер ноги, младший - тон
                    int k=0;
                    try {
                        k = int.Parse(txtParam19a.Text);
                    } catch{
                        Console.WriteLine("send error converting s=" + txtParam19a.Text);
                    }
                    int f=0;
                    try {
                        f = int.Parse(txtParam19b.Text);
                    } catch{
                        Console.WriteLine("send error converting s=" + txtParam19b.Text);
                    }

                    val = k*256 + f;
                    
                    goto as_number;

                case 29:
                    int hd=0;
                    try {
                        hd = int.Parse(txtParam29a.Text);
                    } catch{
                        Console.WriteLine("send error converting s=" + txtParam29a.Text);
                    }
                    int lp=0;
                    try {
                        lp = int.Parse(txtParam29b.Text);
                    } catch{
                        Console.WriteLine("send error converting s=" + txtParam29b.Text);
                    }

                    val = lp<<16 + hd;
                    
                    goto as_number;
                    

            // strings
                case PARAMS_END+0:    
                    s=txtPhone1.Text;
                    break;
                case PARAMS_END + 1:
                    s = txtPhone2.Text;
                    break;
                case PARAMS_END + 2:
                    s = txtPhone3.Text;
                    break;
                case PARAMS_END + 3:
                    s = txtPhone4.Text;
                    break;

                case PARAMS_END + 4:
                    s = txtCallSign.Text;
                    break;

                case PARAMS_END + 5:
                    s = txtURL.Text ;
                    break;

                default:
                    Console.WriteLine("Send Error searhing textbox n=" + n.ToString());
                    return;
                }
                readOut ();
                comBusy = true;
                comPort.WriteLine(String.Format("S{0}={1}", n, s));
                //parseParam(readBeaconLine());
                getConfig(); // full config here
                return;
            
            }

            val=int.Parse (tbx.Text);
as_number:
            readOut();
            comBusy=true ;
            comPort.WriteLine(String.Format("R{0}={1}",n,val));
            //parseParam(readBeaconLine());
            getConfig(); // full config here
            return;


        }
       

        private void connectComPortToolStripMenuItem_Click(object sender, EventArgs e) {
            /*
            frmComPort frm = new frmComPort(this);
            //frm.Show();
            frm.ShowInTaskbar = false;
            frm.ShowDialog(); // modal
            */
        }

      
        private void mnuComPort_Click(object sender, EventArgs e) {
            frmComPort frm = new frmComPort(this);
            //frm.Show();
            frm.ShowInTaskbar = false;

        }

        private void btnSave_Click(object sender, EventArgs e) {
            beaconCommand('w');
            if (waitComAnswer("Timeout to save"))
                MessageBox.Show("Result: " + readBeaconLine(), "EEPROM save", MessageBoxButtons.OK, MessageBoxIcon.Information); 
 
        }

        private void txtParam0_Leave(object sender, EventArgs e) {
            string name = ((TextBox)sender).Name.Substring ("txtParam".Length );
            int n = int.Parse(name);
            sendParam(n);
        }

        private void txtParam19a_Leave(object sender, EventArgs e) {
            sendParam(19);
        }

        private void txtParam19b_Leave(object sender, EventArgs e) {
            sendParam(19);
        }

        private void txtPhone1_Leave(object sender, EventArgs e) {
            sendParam(PARAMS_END + 0);
        }

        private void txtPhone2_Leave(object sender, EventArgs e) {
            sendParam(PARAMS_END + 1);
        }

        private void txtPhone3_Leave(object sender, EventArgs e) {
            sendParam(PARAMS_END + 2);
        }

        private void txtPhone4_Leave(object sender, EventArgs e) {
            sendParam(PARAMS_END + 3);
        }

        private void txtURL_Leave(object sender, EventArgs e) {
            sendParam(PARAMS_END + 5);
        }

        private void txtCallSign_Leave(object sender, EventArgs e) {
            sendParam(PARAMS_END+4);
        }

        private void chkParam17_Leave(object sender, EventArgs e) {
            sendParam (17);
        }

        private void chkParam18_Leave(object sender, EventArgs e) {
            sendParam(18);
        }

        private void txtParam29a_Leave(object sender, EventArgs e) {
            sendParam(29);
        }

        private void txtParam29b_Leave(object sender, EventArgs e) {
            sendParam(29);
        }

        private void button1_Click(object sender, EventArgs e) {
            beaconCommand('s');
        }

       
        private void Translate(Form f){
            bool found=false;
            string fn=Path.GetDirectoryName(Application.ExecutablePath) + Path.DirectorySeparatorChar + "lang.txt";
            if(!File.Exists(fn)) {
                generateLang();
                   return;
            }
            
            using (StreamReader sr = new StreamReader(fn)) {
                while (!sr.EndOfStream) {

                    string s = sr.ReadLine();
                    if(!found){
                        if(s == "["+ f.Name  +"]")
                            found=true ;
                        continue;
                    }
                    if(s[0]=='[') break; // next section
                    // found
                    string[] sa=s.Split('=');

                    string nm=sa[0].Substring(sa[0].Length -3);
                    if(nm=="-tt"){ // tooltip
                        Control ct = this.Controls.Find(sa[0].Substring(0,sa[0].Length -3), true).FirstOrDefault() as Control;
                        if(ct!=null)
                            hint.SetToolTip(ct,sa[1]);
                    } else {
                        Control c=this.Controls.Find(sa[0], true).FirstOrDefault() as Control ;
                        if(c==null) continue;
                        if (c.ToString().Contains("System.Windows.Forms.TextBox")) continue;
                        if (c.ToString().Contains("System.Windows.Forms.ComboBox")) continue;
                        if (c.ToString().Contains("System.Windows.Forms.NumericUpDown")) continue;
                        if (c.ToString().Contains("System.Windows.Forms.MaskedTextBox")) continue;
                            
                        c.Text =sa[1];
                    }
                }
            }
        }

        void parseControls(StreamWriter sw, Control.ControlCollection ctls) {
            foreach (Control c in ctls) {

                if (c.Controls != null && c.Controls.Count > 0) {
                    parseControls(sw,c.Controls);
                }

                if (c.Name == c.Text) continue;
                string tt = hint.GetToolTip(c);
                if(tt!=""){
                    sw.WriteLine(c.Name+"-tt" + "=" + tt);
                }

                if (c.Text == "") continue;
                //txtParam
                if(c.ToString ().Contains("System.Windows.Forms.TextBox")) continue;
                if (c.ToString().Contains("System.Windows.Forms.ComboBox")) continue;
                if (c.ToString().Contains("System.Windows.Forms.NumericUpDown")) continue;
                if (c.ToString().Contains("System.Windows.Forms.MaskedTextBox")) continue;
                sw.WriteLine(c.Name + "=" + c.Text);
            }

/*
            foreach (System.Windows.Forms.ToolTip p in hint.tools) {

            }
*/
        }

        void generateLang(){
/*
            List<Type> forms = new List<Type>();
            foreach (Assembly asm in AppDomain.CurrentDomain.GetAssemblies()) {
                forms.AddRange(from t in asm.GetTypes() where t.IsSubclassOf(typeof(Form)) select t);
            }
            foreach (object f in forms) {
                Console.WriteLine ("f="+f.GetType());
                string n = ((System.RuntimeType)f).
            }
  */

            

            string fn=Path.GetDirectoryName(Application.ExecutablePath) + Path.DirectorySeparatorChar + "lang.txt";
            using (StreamWriter sw = new StreamWriter(fn)) {       //Write   
                sw.WriteLine ("["+this.Name +"]");

                parseControls(sw, this.Controls);

                sw.Close();
            }
            

        }


        

        


       

        

     


       

       



      

      

      
  

       




    }
}
