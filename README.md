# ATOMS | Automated Organ Transport System

ATOMS is an ESP32-based organ transport monitoring prototype developed to improve the safety and traceability of organs during transportation. It combines temperature monitoring, vibration detection, and RFID identification in a compact embedded system.

The project was developed as a Medical Electronics Engineering minor project at Ramaiah Institute of Technology (2025–26).

## Features

- **Temperature Monitoring:** DS18B20 sensor measures transport temperature, with a target range of 4–8°C.
- **Vibration Detection:** MPU6050 accelerometer monitors movement and mechanical shocks during transit.
- **RFID Identification:** MFRC522 reader identifies tagged organ containers using unique RFID IDs.
- **LCD Display:** 16×2 I2C LCD displays live sensor readings and transport status.
- **Remote Monitoring:** The complete prototype also includes a mobile application for viewing sensor data, transport history, patient details and alerts.

## Hardware Components

| Component | Purpose |
|---|---|
| ESP32 | Main microcontroller |
| DS18B20 | Temperature monitoring |
| MPU6050 | Vibration and movement detection |
| MFRC522 | RFID identification |
| 16×2 I2C LCD | Local monitoring display |
| Copper PCB | Hardware assembly and integration |

## System Overview

The ESP32 collects temperature readings through the 1-Wire interface, acceleration data through I2C, and RFID information through SPI.

Vibration is estimated using the acceleration magnitude relative to gravitational acceleration

The LCD provides local monitoring, while the complete system supports wireless communication with a monitoring application.

## Technologies Used

**Hardware:** ESP32, DS18B20, MPU6050, MFRC522, I2C LCD, PCB

**Software:** Embedded C/C++, Arduino IDE, OneWire, DallasTemperature, Adafruit MPU6050, MFRC522

**Protocols:** I2C, SPI, 1-Wire, Wi-Fi

## Repository Scope

This repository contains the ESP32 firmware for the hardware monitoring system. The mobile application and backend are not included.

The current firmware uses RFID identification and sensor monitoring. GPS tracking and electronic locking are planned extensions rather than implemented hardware features.

## Future Improvements

- GPS-based transport tracking
- Electronic locking and tamper detection
- Active temperature regulation
- Improved enclosure and power management

## Project Media

<img width="560" height="467" alt="WhatsApp Image 2026-10-20 at 15 14 27" src="https://github.com/user-attachments/assets/b22f32b0-691a-4ce7-8584-12c918242683" />
<img width="580" height="386" alt="WhatsApp Image 2026-10-20 at 15 13 04" src="https://github.com/user-attachments/assets/2cd62475-131d-453b-88ec-96e1996dc4e1" />




*Academic prototype intended for demonstration and development. Not validated for clinical organ transportation.*
