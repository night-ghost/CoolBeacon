using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Collections;

namespace CT {

 
    public partial class frmTerm : Form {
        /// <summary>
        /// Class to keep track of string and color for lines in output window.
        /// </summary>
    
        //

        ArrayList lines = new ArrayList();

        System.Threading.Thread com_thread;

        Font origFont;
        Font monoFont;

        private CTool ct;
        bool fDone=false;

        public frmTerm(CTool form) {
            ct=form;
            InitializeComponent();

            splitContainer1.FixedPanel = FixedPanel.Panel1;
            splitContainer2.FixedPanel = FixedPanel.Panel2;

            AcceptButton = button5; //Send
            CancelButton = button4; //Close

            outputList_Initialize();

           // TopMost = Settings.Option.StayOnTop;

            // let form use multiple fonts
            origFont = Font;
            FontFamily ff = new FontFamily("Courier New");
            monoFont = new Font(ff, 8, FontStyle.Regular);
            Font = /* Settings.Option.MonoFont ? */ monoFont /* : origFont*/ ;

            com_thread = new System.Threading.Thread(com_thread_proc);
            com_thread.Start();
        }

        string filterString = "";
        bool scrolling = true;
        Color receivedColor = Color.Green;
        Color sentColor = Color.Blue;

        /// <summary>
        /// context menu for the output window
        /// </summary>
        ContextMenu popUpMenu;

        void com_thread_proc(){
            while (!fDone) {
                    try {
                        if (ct.comPort.IsOpen && ct.comPort.BytesToRead > 0) {
                            string s = ct.comPort.ReadExisting();
                            OnDataReceived(s);
                        }
                    } catch { }
                System.Threading.Thread.Sleep(10);
            }
        }

        /// <summary>
        /// Initialize the output window
        /// </summary>
        private void outputList_Initialize() {
            // owner draw for listbox so we can add color
            outputList.DrawMode = DrawMode.OwnerDrawFixed;
            outputList.DrawItem += new DrawItemEventHandler(outputList_DrawItem);
            outputList.ClearSelected();

            // build the outputList context menu
            popUpMenu = new ContextMenu();
            popUpMenu.MenuItems.Add("&Copy", new EventHandler(outputList_Copy));
            popUpMenu.MenuItems[0].Visible = true;
            popUpMenu.MenuItems[0].Enabled = false;
            popUpMenu.MenuItems[0].Shortcut = Shortcut.CtrlC;
            popUpMenu.MenuItems[0].ShowShortcut = true;
            popUpMenu.MenuItems.Add("Copy All", new EventHandler(outputList_CopyAll));
            popUpMenu.MenuItems[1].Visible = true;
            popUpMenu.MenuItems.Add("Select &All", new EventHandler(outputList_SelectAll));
            popUpMenu.MenuItems[2].Visible = true;
            popUpMenu.MenuItems[2].Shortcut = Shortcut.CtrlA;
            popUpMenu.MenuItems[2].ShowShortcut = true;
            popUpMenu.MenuItems.Add("Clear Selected", new EventHandler(outputList_ClearSelected));
            popUpMenu.MenuItems[3].Visible = true;
            outputList.ContextMenu = popUpMenu;
        }

        bool outputList_ApplyFilter(String s) {
            if (filterString == "") {
                return true;
            } else if (s == "") {
                return false;
            } else if (/* Settings.Option.FilterUseCase */ false) {
                return (s.IndexOf(filterString) != -1);
            } else {
                string upperString = s.ToUpper();
                string upperFilter = filterString.ToUpper();
                return (upperString.IndexOf(upperFilter) != -1);
            }
        }

        void outputList_ClearAll() {
            lines.Clear();
            partialLine = null;

            outputList.Items.Clear();
        }

        /// <summary>
        /// refresh the output window
        /// </summary>
        void outputList_Refresh() {
            outputList.BeginUpdate();
            outputList.Items.Clear();
            foreach (Line line in lines) {
                if (outputList_ApplyFilter(line.Str)) {
                    outputList.Items.Add(line);
                }
            }
            outputList.EndUpdate();
            outputList_Scroll();
        }

        /// <summary>
        /// add a new line to output window
        /// </summary>
        Line outputList_Add(string str, Color color) {
            Line newLine = new Line(str, color);
            lines.Add(newLine);

            if (outputList_ApplyFilter(newLine.Str)) {
                outputList.Items.Add(newLine);
                outputList_Scroll();
            }

            return newLine;
        }

        /// <summary>
        /// Update a line in the output window.
        /// </summary>
        /// <param name="line">line to update</param>
        void outputList_Update(Line line) {
            // should we add to output?
            if (outputList_ApplyFilter(line.Str)) {
                // is the line already displayed?
                bool found = false;
                for (int i = 0; i < outputList.Items.Count; ++i) {
                    int index = (outputList.Items.Count - 1) - i;
                    if (line == outputList.Items[index]) {
                        // is item visible?
                        int itemsPerPage = (int)(outputList.Height / outputList.ItemHeight);
                        if (index >= outputList.TopIndex &&
                            index < (outputList.TopIndex + itemsPerPage)) {
                            // is there a way to refresh just one line
                            // without redrawing the entire listbox?
                            // changing the item value has no effect
                            outputList.Refresh();
                        }
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    // not found, so add it
                    outputList.Items.Add(line);
                }
            }
        }

        void outputList_DrawItem(object sender, DrawItemEventArgs e) {
            e.DrawBackground();
            if (e.Index >= 0 && e.Index < outputList.Items.Count) {
                Line line = (Line)outputList.Items[e.Index];

                // if selected, make the text color readable
                Color color = line.ForeColor;
                if ((e.State & DrawItemState.Selected) == DrawItemState.Selected) {
                    color = Color.Black;	// make it readable
                }

                e.Graphics.DrawString(line.Str, e.Font, new SolidBrush(color),
                    e.Bounds, StringFormat.GenericDefault);
            }
            e.DrawFocusRectangle();
        }

        /// <summary>
        /// Scroll to bottom of output window
        /// </summary>
        void outputList_Scroll() {
            if (scrolling) {
                int itemsPerPage = (int)(outputList.Height / outputList.ItemHeight);
                outputList.TopIndex = outputList.Items.Count - itemsPerPage;
            }
        }

        /// <summary>
        /// Enable/Disable copy selection in output window
        /// </summary>
        private void outputList_SelectedIndexChanged(object sender, EventArgs e) {
            popUpMenu.MenuItems[0].Enabled = (outputList.SelectedItems.Count > 0);
        }

        /// <summary>
        /// copy selection in output window to clipboard
        /// </summary>
        private void outputList_Copy(object sender, EventArgs e) {
            int iCount = outputList.SelectedItems.Count;
            if (iCount > 0) {
                String[] source = new String[iCount];
                for (int i = 0; i < iCount; ++i) {
                    source[i] = ((Line)outputList.SelectedItems[i]).Str;
                }

                String dest = String.Join("\r\n", source);
                Clipboard.SetText(dest);
            }
        }

        /// <summary>
        /// copy all lines in output window
        /// </summary>
        private void outputList_CopyAll(object sender, EventArgs e) {
            int iCount = outputList.Items.Count;
            if (iCount > 0) {
                String[] source = new String[iCount];
                for (int i = 0; i < iCount; ++i) {
                    source[i] = ((Line)outputList.Items[i]).Str;
                }

                String dest = String.Join("\r\n", source);
                Clipboard.SetText(dest);
            }
        }

        /// <summary>
        /// select all lines in output window
        /// </summary>
        private void outputList_SelectAll(object sender, EventArgs e) {
            outputList.BeginUpdate();
            for (int i = 0; i < outputList.Items.Count; ++i) {
                outputList.SetSelected(i, true);
            }
            outputList.EndUpdate();
        }

        /// <summary>
        /// clear selected in output window
        /// </summary>
        private void outputList_ClearSelected(object sender, EventArgs e) {
            outputList.ClearSelected();
            outputList.SelectedItem = -1;
        }


        private String PrepareData(String StringIn) {
            // The names of the first 32 characters
            string[] charNames = { "NUL", "SOH", "STX", "ETX", "EOT",
				"ENQ", "ACK", "BEL", "BS", "TAB", "LF", "VT", "FF", "CR", "SO", "SI",
				"DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB", "CAN", "EM", "SUB",
				"ESC", "FS", "GS", "RS", "US", "Space"};

            string StringOut = "";

            foreach (char c in StringIn) {
                if (chkHEX.Checked) {
                    StringOut = StringOut + String.Format("{0:X2} ", (int)c);
                } else if (c < 32 && c != 9) {
                    StringOut = StringOut + "<" + charNames[c] + ">";

                    //Uglier "Termite" style
                    //StringOut = StringOut + String.Format("[{0:X2}]", (int)c);
                } else {
                    StringOut = StringOut + c;
                }
            }
            return StringOut;
        }

        /// <summary>
        /// Partial line for AddData().
        /// </summary>
        private Line partialLine = null;

        /// <summary>
        /// Add data to the output.
        /// </summary>
        /// <param name="StringIn"></param>
        /// <returns></returns>
        private Line AddData(String StringIn) {
            String StringOut = PrepareData(StringIn);

            // if we have a partial line, add to it.
            if (partialLine != null) {
                // tack it on
                partialLine.Str = partialLine.Str + StringOut;
                outputList_Update(partialLine);
                return partialLine;
            }

            return outputList_Add(StringOut, receivedColor);
        }

        // delegate used for Invoke
        internal delegate void StringDelegate(string data);

        /// <summary>
        /// Handle data received event from serial port.
        /// </summary>
        /// <param name="data">incoming data</param>
        public void OnDataReceived(string dataIn) {
            //Handle multi-threading
            if (InvokeRequired) {
                Invoke(new StringDelegate(OnDataReceived), new object[] { dataIn });
                return;
            }

            // pause scrolling to speed up output of multiple lines
            bool saveScrolling = scrolling;
            scrolling = false;

            // if we detect a line terminator, add line to output
            int index;
            while (dataIn.Length > 0 &&
                ((index = dataIn.IndexOf("\r")) != -1 ||
                (index = dataIn.IndexOf("\n")) != -1)) {
                String StringIn = dataIn.Substring(0, index);
                
                dataIn = dataIn.Remove(0, index + 1);
                AddData(StringIn);
                partialLine = null;	// terminate partial line
            }

            // if we have data remaining, add a partial line
            if (dataIn.Length > 0) {
                partialLine = AddData(dataIn);
            }

            // restore scrolling
            scrolling = saveScrolling;
            outputList_Scroll();
        }

        /// <summary>
        /// Update the connection status
        /// </summary>
        public void OnStatusChanged(string status) {
            //Handle multi-threading
            if (InvokeRequired) {
                Invoke(new StringDelegate(OnStatusChanged), new object[] { status });
                return;
            }

            textBox1.Text = status;
        }

        /// <summary>
        /// If character 0-9 a-f A-F, then return hex digit value ?
        /// </summary>
        private static int GetHexDigit(char c) {
            if ('0' <= c && c <= '9') return (c - '0');
            if ('a' <= c && c <= 'f') return (c - 'a') + 10;
            if ('A' <= c && c <= 'F') return (c - 'A') + 10;
            return 0;
        }

        /// <summary>
        /// Parse states for ConvertEscapeSequences()
        /// </summary>
        public enum Expecting : byte {
            ANY = 1,
            ESCAPED_CHAR,
            HEX_1ST_DIGIT,
            HEX_2ND_DIGIT
        };

        /// <summary>
        /// Convert escape sequences
        /// </summary>
        private string ConvertEscapeSequences(string s) {
            Expecting expecting = Expecting.ANY;

            int hexNum = 0;
            string outs = "";
            foreach (char c in s) {
                switch (expecting) {
                case Expecting.ANY:
                    if (c == '\\')
                        expecting = Expecting.ESCAPED_CHAR;
                    else
                        outs += c;
                    break;
                case Expecting.ESCAPED_CHAR:
                    if (c == 'x') {
                        expecting = Expecting.HEX_1ST_DIGIT;
                    } else {
                        char c2 = c;
                        switch (c) {
                        case 'n': c2 = '\n'; break;
                        case 'r': c2 = '\r'; break;
                        case 't': c2 = '\t'; break;
                        }
                        outs += c2;
                        expecting = Expecting.ANY;
                    }
                    break;
                case Expecting.HEX_1ST_DIGIT:
                    hexNum = GetHexDigit(c) * 16;
                    expecting = Expecting.HEX_2ND_DIGIT;
                    break;
                case Expecting.HEX_2ND_DIGIT:
                    hexNum += GetHexDigit(c);
                    outs += (char)hexNum;
                    expecting = Expecting.ANY;
                    break;
                }
            }
            return outs;
        }


        private void button2_Click(object sender, EventArgs e) {
            outputList_ClearAll();
        }

        private void button5_Click(object sender, EventArgs e) {
            string command = comboBox1.Text;
            comboBox1.Items.Add(comboBox1.Text);
            comboBox1.Text = "";
            comboBox1.Focus();
            
            if (command =="") return;

            if (command=="." || command[0] == '.' && (command[0] == '\n' || command[0] == '\r')) {
                Close();
                return;
            }
            if (command.Length > 0) {// не отправляем пустую строку
                command = ConvertEscapeSequences(command);
                
                ct.comPort.WriteLine  (command);

                if (chkEcho.Checked ) {
                    outputList_Add(command + "\n", sentColor);
                }
            }
        }

        private void button7_Click(object sender, EventArgs e) {

            scrolling = !scrolling;
            outputList_Scroll();
        }

        private void button4_Click(object sender, EventArgs e) {
            Close();
        }

        private void comboBox1_SelectedIndexChanged(object sender, EventArgs e) {

        }

        private void frmTerm_FormClosing(object sender, FormClosingEventArgs e) {
            fDone=true;
            try {
                ct.comPort.WriteLine(""); // выход по пустой строке
                ct.comPort.WriteLine(""); // выход по пустой строке
                ct.comPort.WriteLine("."); // выход по точке
            } catch{}

            System.Threading.Thread.Sleep(50);
            //ct.readOut();
            com_thread.Abort ();
        }
    }

    public class Line {
        public string Str;
        public Color ForeColor;

        public Line(string str, Color color) {
            Str = str;
            ForeColor = color;
        }
    }

}
