# ATOMS-Automated_Organ_Monitoring_System

ESP32-based prototype for monitoring temperature, acceleration-derived vibration, and RFID identification during simulated organ transport. A 16×2 LCD provides local readings. Optional server communication can provide transport state and a remaining-time countdown.

## Hardware

| Part | Purpose | Connection |
| --- | --- | --- |
| ESP32 Dev Module | Controller | — |
| DS18B20 | Temperature | GPIO4, 4.7 kΩ pull-up to 3.3 V |
| MPU6050 | Acceleration | I2C SDA21 / SCL22 |
| MFRC522 | RFID tag UID | SPI SS5 / RST15 / SCK18 / MISO19 / MOSI23 |
| 16×2 I2C LCD | Local display | I2C SDA21 / SCL22, typically address 0x27 |

## Firmware setup

1. Install the ESP32 board package in Arduino IDE and select **ESP32 Dev Module**.
2. Install **OneWire**, **DallasTemperature**, **Adafruit MPU6050** (and dependencies), **MFRC522**, **LiquidCrystal I2C**, and **ArduinoJson v6**.
3. Copy `firmware/ATOMS_ESP32/config.example.h` to `firmware/ATOMS_ESP32/config.h`.
4. Leave `ENABLE_BACKEND` as `0` for standalone operation. For optional backend integration, set it to `1` and configure Wi-Fi credentials and your own server endpoint in the private `config.h`.
5. Open `firmware/ATOMS_ESP32/ATOMS_ESP32.ino`, compile, and upload.

## Operation

- Reads temperature every five seconds.
- Estimates vibration from acceleration magnitude deviation relative to 1g (not calibrated vibration measurement).
- Reads an RFID tag UID and displays local sensor measurements.
- With backend enabled, posts readings and receives transport state and remaining minutes. The LCD can continue the last received countdown between server updates.
## Limitations

Educational hardware prototype only, not a validated organ preservation or clinical monitoring system. GPS coordinates and lock status in the optional payload are hardcoded placeholders; the firmware does not measure location or lock state as of now. RFID UID is a demo identifier, not a verified organ ID. 
## Project media


