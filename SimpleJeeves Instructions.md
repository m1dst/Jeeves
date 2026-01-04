# Jeeves – ESP32 Multiplier Bell Controller

Jeeves is an ESP32‑based relay controller designed to integrate with DXLog.  
It connects to WiFi, listens for TCP connections on port **73**, and pulses a relay + LED whenever DXLog logs a multiplier.

## 🔧 What the Device Does

- Connects to WiFi using **WiFiManager**
- Listens for **TCP connections on port 73**
- Pulses:
  - **Relay on GPIO16**
  - **LED on GPIO23**
  for **50 ms**
- Advertises itself on the network as **`jeeves.local`** via mDNS
- Supports **WiFi reset** by holding a button on **GPIO22** during boot
- Integrates with DXLog to ring a bell when a multiplier is logged

## 📡 First‑Time WiFi Setup

1. Power on the device.  
2. If no WiFi credentials are stored, it creates an access point named **`Jeeves`**.  
3. Connect to this AP using a phone or laptop.  
4. A captive portal will appear automatically.  
5. Select your WiFi network and enter the password.  
6. The device reboots and connects to your network.

Once connected, it becomes reachable as:

jeeves.local

(Windows users may need Apple Bonjour installed for `.local` resolution.)

## 🔄 Resetting WiFi Settings

To erase stored WiFi credentials:

1. Power off the device.  
2. Press and hold the **reset button wired to GPIO22**.  
3. Power the device back on while holding the button.  
4. Keep holding for about 5 seconds.
5. Release the button.  
6. The device reboots and starts the WiFiManager AP again.

You can now reconfigure WiFi from scratch.

## 🔔 Triggering the Relay Manually

The relay activates whenever a TCP connection is made to **port 73**.

If you run the comment in a terminal you should hear the bell ring.

curl telnet://jeeves.local:73

## 🖥️ DXLog Integration

1. Add the script to DXLog Script Manager. Do not bind it to a key.
2. Restart DXLog.

Whenever a QSO is a multiplier is logged, the bell should now ring.