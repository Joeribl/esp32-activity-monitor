# ESP32 Activity Monitor

This project is a real-time hardware activity monitor that fetches system metrics from a macOS device using a Python script and displays them on an external 128x64 SSD1306 OLED screen controlled by an ESP32.

The UI was designed using **Lopaka** and implemented via `U8g2` library.

![Demo PNG](monitor.png)

## Features

* **Live CPU Tracker:** Real-time CPU usage percentage with a dynamic graphical progress bar.
* **RAM Monitor:** Displays currently used memory against total system memory (e.g., `3.3 / 8 GB`).
* **Battery Status:** Displays current battery level and dynamically renders a battery icon when the MacBook is charging (`is_plugged_in`).

### Pin Configuration (Default Hardware I2C)
| OLED Pin | ESP32 GPIO |
| :--- | :--- |
| **VCC** | 3.3V / 5V |
| **GND** | GND |
| **SCL** | GPIO 22 |
| **SDA** | GPIO 21 |

## Setup
### 1. ESP32 (Firmware)
* Built using **PlatformIO** inside VS Code.
* Uses the **U8g2** library for crisp pixel rendering and custom bitmaps.
* To deploy, simply clone this repository, open the firmware folder in PlatformIO, and hit **Upload**.

### 2. Python
* The background client relies on `psutil` to extract system diagnostics and `pyserial` to stream it over the USB-Serial bridge.

```bash
pip install -r requirements.txt
```
* Run the script on the terminal using:
```bash
python monitor.py
```
* To run the script in the background (so you can close the terminal window):
```bash
nohup python monitor.py &
```
* To quit enter **control + c** or 
```bash
pkill -f monitor.py
```
to kill the terminal.