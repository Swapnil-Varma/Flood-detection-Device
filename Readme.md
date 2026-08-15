# ESP8266 IoT Based Flood Detection & Monitoring System

An IoT-based flood detection and environmental monitoring system built using **ESP8266 NodeMCU**. The system combines multiple sensors to monitor temperature, humidity, rain, water level, and water flow, while providing remote monitoring and alert functionality through **Blynk IoT**.

## Overview

The system continuously collects data from:

- DHT11 — Temperature and Humidity
- HC-SR04 — Water Level / Distance
- Rain Sensor — Rain Detection
- Water Flow Sensor — Flow Rate and Total Flow
- Buzzer — Local Alert

The ESP8266 processes the sensor readings and sends them to the Blynk IoT dashboard over Wi-Fi.

## Features

- 🌡️ Real-time temperature monitoring
- 💧 Humidity monitoring
- 🌊 Ultrasonic water-level monitoring
- 🌧️ Rain sensor monitoring
- 🚰 Water flow-rate measurement
- 📊 Total water-flow measurement
- 🔔 Buzzer alert
- ☁️ Blynk IoT remote monitoring
- 📡 Wi-Fi connectivity using ESP8266
- ⚡ Interrupt-based flow sensor pulse counting

## Hardware Components

| Component | Quantity |
|---|---:|
| ESP8266 NodeMCU | 1 |
| DHT11 Sensor | 1 |
| HC-SR04 Ultrasonic Sensor | 1 |
| Rain Sensor Module | 1 |
| Water Flow Sensor | 1 |
| Buzzer | 1 |
| Jumper Wires | As required |
| Power Supply | 1 |

## Pin Configuration

| Component | ESP8266 Pin |
|---|---|
| DHT11 Data | D1 |
| Buzzer | D2 |
| Flow Sensor | D3 / GPIO0 |
| HC-SR04 Trigger | D7 |
| HC-SR04 Echo | D6 |
| Rain Sensor Analog Output | A0 |

## Blynk Virtual Pin Mapping

| Virtual Pin | Parameter |
|---|---|
| V0 | Buzzer Control |
| V1 | Temperature |
| V2 | Humidity |
| V3 | Calculated Water Level |
| V4 | Flow Rate |
| V5 | Total Flow |
| V6 | Rain Sensor Value |
| V7 | Ultrasonic Distance |

## Working Principle

### 1. Temperature and Humidity Monitoring

The DHT11 sensor measures the surrounding temperature and relative humidity. The ESP8266 reads these values and sends them to Blynk.

### 2. Water-Level Detection

The HC-SR04 ultrasonic sensor measures the distance between the sensor and the water surface.

The project calculates an estimated water level using:

```text
Water Level = Reference Height - Measured Distance
```

The reference height must be calibrated according to the actual installation.

### 3. Rain Detection

The rain sensor provides an analog reading through the ESP8266 A0 pin. The value is transmitted to Blynk for monitoring.

The raw sensor value should be calibrated for the particular rain sensor module before converting it into a percentage or rainfall intensity.

### 4. Water Flow Monitoring

The flow sensor generates pulses when water passes through it. The ESP8266 counts these pulses using an interrupt.

For a typical YF-S201-type flow sensor, a commonly used relationship is:

```text
Flow Rate (L/min) = Frequency (Hz) / 7.5
```

The project can use this value to calculate both instantaneous flow rate and accumulated flow.

### 5. Flood Alert

A buzzer is connected to the ESP8266 and can be controlled through Blynk.

The Blynk event:

```text
flooding_detected
```

can be configured for flood-alert notifications.

## Software Requirements

- Arduino IDE
- ESP8266 Board Package
- Blynk Library
- DHT Sensor Library
- Adafruit Unified Sensor Library

## Required Libraries

```cpp
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
```

## Blynk Configuration

Create a Blynk IoT template and configure the required virtual pins.

Set the following definitions in the source code:

```cpp
#define BLYNK_TEMPLATE_ID   "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN    "YOUR_AUTH_TOKEN"
```

### Security

**Do not upload your actual Blynk Auth Token or Wi-Fi password to GitHub.**

A recommended project structure is:

```text
Flood-Detection-System/
│
├── ESP8266-Flood-Detection-System.ino
├── secrets.h.example
├── .gitignore
└── README.md
```

Use a local `secrets.h` file for Wi-Fi credentials and exclude it using `.gitignore`.

Example:

```cpp
#ifndef SECRETS_H
#define SECRETS_H

#define WIFI_SSID      "YOUR_WIFI_NAME"
#define WIFI_PASSWORD  "YOUR_WIFI_PASSWORD"

#endif
```

## System Flow

```text
             ┌─────────────────┐
             │   DHT11 Sensor  │
             │ Temp + Humidity  │
             └────────┬────────┘
                      │
             ┌────────▼────────┐
             │                 │
┌───────────┐│                 │┌───────────────┐
│Rain Sensor├┼──►   ESP8266    ◄┤ HC-SR04       │
└───────────┘│                 ││ Water Level   │
             │                 │└───────────────┘
┌───────────┐│                 │
│Flow Sensor├┼──►             │
└───────────┘│                 │
             └────────┬────────┘
                      │
               Wi-Fi / Blynk
                      │
             ┌────────▼────────┐
             │   Blynk IoT     │
             │    Dashboard    │
             └─────────────────┘
                      │
                 Flood Alert
                      │
                 ┌────▼────┐
                 │ Buzzer  │
                 └─────────┘
```

## Important Hardware Considerations

### HC-SR04 Echo Pin

The HC-SR04 commonly operates with a 5 V logic-level echo output, while ESP8266 GPIOs are 3.3 V logic.

Use an appropriate **voltage divider or logic-level interface** between:

```text
HC-SR04 ECHO → ESP8266 D6
```

Do not directly apply an unsuitable voltage to the ESP8266 GPIO.

### ESP8266 A0

Check the ADC input range of your particular ESP8266 development board before connecting the rain sensor analog output directly to A0. Different ESP8266 boards may provide different input scaling.

### Flow Sensor on D3 / GPIO0

D3 corresponds to GPIO0, which is also an ESP8266 boot-strap pin. The flow sensor should not force GPIO0 into an invalid boot state during startup.

If boot reliability becomes an issue, consider moving the flow sensor to another appropriate GPIO.

## Calibration

The following parameters should be calibrated for the actual hardware installation:

### Water-Level Reference

```cpp
const float SENSOR_REFERENCE_HEIGHT_CM = 100.0;
```

The value should correspond to the physical distance between the ultrasonic sensor and the chosen zero/reference water level.

### Flow Sensor Calibration

The `7.5` factor is commonly associated with YF-S201-type sensors:

```text
Flow Rate = Frequency / 7.5
```

For other flow sensors, use the manufacturer's calibration factor or experimentally determine the correct factor.

### Rain Sensor

The raw ADC value should be experimentally calibrated against the actual sensor behavior. Avoid assuming that one fixed ADC value universally represents a particular rainfall intensity.

## Example Data

The Blynk dashboard can display:

```text
Temperature       → °C
Humidity          → %
Water Level       → cm
Ultrasonic Dist.  → cm
Flow Rate         → L/min
Total Flow        → L
Rain Sensor       → ADC value
Buzzer            → ON/OFF
```

## Project Applications

This project demonstrates concepts that can be applied to:

- Flood warning systems
- Water-level monitoring
- Drainage monitoring
- Agricultural water monitoring
- Tank monitoring
- IoT-based environmental monitoring
- Remote water-flow monitoring

## Future Improvements

Possible improvements include:

- Automatic flood-level threshold detection
- Automatic buzzer activation based on water level
- Blynk push notifications
- SMS/email alerts
- OLED/LCD local display
- SD-card data logging
- Historical sensor graphs
- Battery backup
- Solar-powered operation
- GPS-based location reporting
- Multiple water-level warning thresholds
- Improved rain-sensor calibration
- Mobile dashboard optimization

## Project Status

**Status:** Completed / Educational Prototype

## Author

**Swapnil Varma**

Bachelor's Engineering in Electronics Engineering  
Faculty of Technology & Engineering  
The Maharaja Sayajirao University of Baroda

## License

This project is intended for educational and academic purposes. You may modify and reuse the project with appropriate attribution.
