//INCLUDE_ASSEMBLY System.dll
//INCLUDE_ASSEMBLY System.Windows.Forms.dll
//INCLUDE_ASSEMBLY System.Net.dll
//INCLUDE_ASSEMBLY NAudio.dll

//using System;
//using NAudio.Wave;
using IOComm;
using IOComm.Audio.Piper;
using IOComm.Audio.Piper.Models;
using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Text.RegularExpressions;

namespace DXLog.net
{
    public class JeevesMultBell : ScriptClass
    {
        private ContestData _contestData;
        private FrmMain _frmMain;

        public void Initialize(FrmMain main)
        {
            // Store references to DXLog main form and contest data
            _frmMain = main;
            _contestData = main.ContestDataProvider;

            // Subscribe to the event fired whenever a new QSO is saved
            _frmMain.NewQSOSaved += NewQSOSaved;
        }

        public void Deinitialize()
        {
            // Unsubscribe from event to avoid memory leaks or callbacks after unload
            _frmMain.NewQSOSaved -= NewQSOSaved;
        }

        private void NewQSOSaved(DXQSO newQso)
        {
            // Check whether the new QSO is a multiplier on any of the 3 multiplier types
            if (newQso.IsMultiplier(1) || newQso.IsMultiplier(2) || newQso.IsMultiplier(3))
            {
                // Host running the ESP32 relay trigger
                //const string host = "jeeves.local";   // mDNS version (optional)
                const string host = "192.168.10.176";   // Direct IP version

                const int port = 73;                    // TCP port the ESP32 listens on

                try
                {
                    // Open a TCP connection to the ESP32
                    using (var client = new TcpClient(host, port))
                   
				   // Get the network stream for sending data
                    using (var stream = client.GetStream())
                    {
                        // Payload to send (ESP32 only cares about the connection)
                        byte[] dataBytes = new byte[] { 0 };

                        // Send the single-byte trigger packet
                        stream.Write(dataBytes, 0, dataBytes.Length);
                    }
                }
                catch (Exception ex)
                {
                    // Log or display any connection errors
                    Console.WriteLine($"Error: {ex.Message}");
                }
            }
        }

        public async void Main(FrmMain main, ContestData cdata, COMMain comMain)
        {
            // DXLog requires this method but it is unused in this script
            if (main == null)
            {
                return;
            }
        }
    }
}