# ESP32 Smart Home Light Monitoring Node Guide

This directory contains the firmware sketch for the ESP32 microcontroller in the **IoT-Based Smart Home Light Monitoring and Recommendation System**.

---

## 1. Hardware Pinout & Wiring

| Component | Pin on Component | ESP32 GPIO | Description / Notes |
| :--- | :--- | :--- | :--- |
| **LDR Sensor Module** | `VCC` | `3.3V` or `5V` | Power supply |
| | `GND` | `GND` | Common ground |
| | `DO` (Digital Out) | **GPIO 27** | Digital threshold: **HIGH = DARK**, **LOW = LIGHT** |
| **LED Actuator** | Anode (`+` long leg) | **GPIO 23** | Via 220Ω or 330Ω current-limiting resistor |
| | Cathode (`-` short leg)| `GND` | Common ground |

> **Note on Digital LDR:** The onboard potentiometer on the LDR module sets the darkness trigger threshold. Turn the potentiometer until the onboard sensor indicator triggers at your desired room light level.

---

## 2. Firmware Features

1. **Dual Operating Modes**:
   - **`AUTO` (Default)**: Automatically turns the LED ON when darkness is detected (GPIO 27 is `HIGH`) and OFF when light is detected (`LOW`).
   - **`MANUAL`**: Allows user overrides from the local web dashboard while continuing to monitor and upload sensor conditions.
2. **Local Web Interface**:
   - The ESP32 hosts a lightweight HTTP web server on port 80 at `http://<esp32-ip>/`.
   - Allows switching modes between `AUTO` and `MANUAL`, and toggling the LED manually.
3. **Secure Cloud Telemetry**:
   - Transmits telemetry every ~6 seconds using non-blocking `millis()` timers.
   - Communicates securely over HTTPS to the Render FastAPI backend using `WiFiClientSecure`.
   - Uses the Let's Encrypt **ISRG Root X1 CA** certificate to validate Render's SSL/TLS certificate.
   - Sends the `X-Device-Key` header with each POST request.
   - Gracefully handles Wi-Fi reconnects and HTTP timeouts without crashing or locking the CPU.

---

## 3. Configuration & Placeholders

Open [`esp32/esp32_smart_home.ino`](file:///c:/Users/haine/OneDrive/Desktop/Documents/IOT%20Project/esp32/esp32_smart_home.ino) and update lines 33–46:

```cpp
// 1. Wi-Fi Credentials
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// 2. Render Backend URL
const char* SERVER_URL    = "https://your-service-name.onrender.com/api/readings";

// 3. Device Key (must match DEVICE_API_KEY in Render environment)
const char* DEVICE_KEY    = "replace-with-a-long-random-secret";

// 4. Device ID
const char* DEVICE_ID     = "esp32-smart-home-01";
```

---

## 4. Arduino IDE Flashing Instructions

1. Install the latest **Arduino IDE** (v2.x or v1.8.x).
2. Go to **File > Preferences** and add the ESP32 Board Manager URL:
   ```text
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Go to **Tools > Board > Boards Manager**, search for `esp32` by **Espressif Systems**, and click **Install**.
4. Select **Tools > Board > ESP32 Arduino > ESP32 Dev Module** (or your specific ESP32 board).
5. Select the correct COM port under **Tools > Port**.
6. Set upload speed to `921600` or `115200`.
7. Click the **Upload** button (hold down the `BOOT` button on your ESP32 board if uploading hangs at `Connecting.....`).
8. Open **Serial Monitor** at `115200 baud` to see connection status and telemetry logs.
