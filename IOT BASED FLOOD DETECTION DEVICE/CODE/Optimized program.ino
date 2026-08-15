/*******************************************************
 * ESP8266 IoT Based Flood Monitoring System
 *
 * Sensors:
 *  - DHT11              -> Temperature & Humidity
 *  - HC-SR04            -> Water Level / Distance
 *  - Rain Sensor        -> Rain Intensity
 *  - Water Flow Sensor  -> Flow Rate & Total Flow
 *  - Buzzer             -> Flood Alert
 *
 * Cloud:
 *  - Blynk IoT
 *
 * Board:
 *  - NodeMCU ESP8266
 *
 * Author: Swapnil Varma
 *******************************************************/

#define BLYNK_TEMPLATE_ID   "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN    "YOUR_AUTH_TOKEN"

#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>

// Keep credentials in a separate file.
// Do NOT upload secrets.h to GitHub.
#include "secrets.h"


/********************************************************
 * PIN CONFIGURATION
 ********************************************************/

#define BUZZER_PIN       D2
#define DHT_PIN          D1
#define FLOW_SENSOR_PIN  D3
#define TRIGGER_PIN      D7
#define ECHO_PIN         D6
#define RAIN_SENSOR_PIN  A0

#define DHT_TYPE DHT11


/********************************************************
 * SENSOR OBJECTS
 ********************************************************/

DHT dht(DHT_PIN, DHT_TYPE);
BlynkTimer timer;


/********************************************************
 * FLOW SENSOR VARIABLES
 ********************************************************/

volatile unsigned long flowPulseCount = 0;

float flowRate = 0.0;                 // L/min
float totalFlowLitres = 0.0;          // Litres

unsigned long lastFlowCalculation = 0;


/********************************************************
 * WATER LEVEL CONFIGURATION
 ********************************************************/

// Distance from ultrasonic sensor to the reference
// water level in centimeters.
//
// CHANGE THIS VALUE according to your tank/river setup.
const float SENSOR_REFERENCE_HEIGHT_CM = 100.0;


/********************************************************
 * SENSOR VALUES
 ********************************************************/

float temperature = 0.0;
float humidity = 0.0;

float distanceCm = 0.0;
float waterLevelCm = 0.0;

int rainValue = 0;


/********************************************************
 * FLOOD ALERT
 ********************************************************/

bool manualBuzzerState = false;


/********************************************************
 * FLOW SENSOR INTERRUPT
 ********************************************************/

ICACHE_RAM_ATTR void flowPulseISR()
{
    flowPulseCount++;
}


/********************************************************
 * SETUP
 ********************************************************/

void setup()
{
    Serial.begin(115200);
    delay(100);

    Serial.println();
    Serial.println(F("======================================"));
    Serial.println(F(" ESP8266 Flood Monitoring System"));
    Serial.println(F("======================================"));

    /****************************************************
     * GPIO CONFIGURATION
     ****************************************************/

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    pinMode(TRIGGER_PIN, OUTPUT);
    digitalWrite(TRIGGER_PIN, LOW);

    pinMode(ECHO_PIN, INPUT);

    pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);


    /****************************************************
     * SENSOR INITIALIZATION
     ****************************************************/

    dht.begin();


    /****************************************************
     * FLOW SENSOR INTERRUPT
     ****************************************************/

    attachInterrupt(
        digitalPinToInterrupt(FLOW_SENSOR_PIN),
        flowPulseISR,
        RISING
    );


    /****************************************************
     * BLYNK CONNECTION
     ****************************************************/

    Blynk.begin(
        BLYNK_AUTH_TOKEN,
        WIFI_SSID,
        WIFI_PASSWORD
    );


    /****************************************************
     * TIMERS
     ****************************************************/

    // DHT11 + Rain sensor
    timer.setInterval(2000L, readTemperatureHumidity);

    // Ultrasonic sensor
    timer.setInterval(1000L, readWaterLevel);

    // Flow sensor
    timer.setInterval(1000L, calculateFlow);

    // Rain sensor
    timer.setInterval(2000L, readRainSensor);

    // Send data to Blynk
    timer.setInterval(2000L, sendDataToBlynk);


    Serial.println(F("System initialized."));
}


/********************************************************
 * MAIN LOOP
 ********************************************************/

void loop()
{
    Blynk.run();
    timer.run();
}


/********************************************************
 * DHT11
 ********************************************************/

void readTemperatureHumidity()
{
    float newHumidity = dht.readHumidity();
    float newTemperature = dht.readTemperature();

    // DHT11 sometimes returns NaN.
    // Keep previous valid value if reading fails.
    if (isnan(newHumidity) || isnan(newTemperature))
    {
        Serial.println(F("DHT11: Reading failed."));
        return;
    }

    humidity = newHumidity;
    temperature = newTemperature;

    Serial.print(F("Temperature: "));
    Serial.print(temperature);
    Serial.print(F(" °C | Humidity: "));
    Serial.print(humidity);
    Serial.println(F(" %"));
}


/********************************************************
 * ULTRASONIC SENSOR
 ********************************************************/

void readWaterLevel()
{
    // Generate 10 µs trigger pulse
    digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(2);

    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(10);

    digitalWrite(TRIGGER_PIN, LOW);


    // Timeout prevents pulseIn() from blocking forever.
    unsigned long duration = pulseIn(
        ECHO_PIN,
        HIGH,
        30000UL
    );


    // No echo received
    if (duration == 0)
    {
        Serial.println(F("Ultrasonic: No echo received."));
        return;
    }


    // Speed of sound ≈ 0.0343 cm/µs
    distanceCm = (duration * 0.0343f) / 2.0f;


    /****************************************************
     * WATER LEVEL CALCULATION
     *
     * If sensor is mounted 100 cm above the reference
     * bottom:
     *
     * Water Level = 100 - measured distance
     ****************************************************/

    waterLevelCm =
        SENSOR_REFERENCE_HEIGHT_CM - distanceCm;


    // Prevent negative water level
    if (waterLevelCm < 0)
        waterLevelCm = 0;


    // Prevent level greater than sensor reference
    if (waterLevelCm > SENSOR_REFERENCE_HEIGHT_CM)
        waterLevelCm = SENSOR_REFERENCE_HEIGHT_CM;


    Serial.print(F("Distance: "));
    Serial.print(distanceCm);
    Serial.print(F(" cm | Water Level: "));
    Serial.print(waterLevelCm);
    Serial.println(F(" cm"));
}


/********************************************************
 * FLOW SENSOR
 *
 * For YF-S201 type sensors:
 *
 * Frequency = 7.5 × Flow Rate (L/min)
 *
 * Therefore:
 *
 * Flow Rate = Frequency / 7.5
 ********************************************************/

void calculateFlow()
{
    unsigned long currentTime = millis();

    if (currentTime - lastFlowCalculation < 1000)
        return;


    /****************************************************
     * Safely copy pulse count
     ****************************************************/

    noInterrupts();

    unsigned long pulses = flowPulseCount;
    flowPulseCount = 0;

    interrupts();


    unsigned long elapsedTime =
        currentTime - lastFlowCalculation;

    lastFlowCalculation = currentTime;


    /****************************************************
     * Calculate frequency
     ****************************************************/

    float frequency =
        (pulses * 1000.0f) / elapsedTime;


    /****************************************************
     * Flow rate in L/min
     ****************************************************/

    flowRate = frequency / 7.5f;


    /****************************************************
     * Calculate total volume
     *
     * L/min × time(min) = litres
     ****************************************************/

    float elapsedMinutes =
        elapsedTime / 60000.0f;

    totalFlowLitres +=
        flowRate * elapsedMinutes;


    Serial.print(F("Flow Rate: "));
    Serial.print(flowRate, 2);

    Serial.print(F(" L/min | Total Flow: "));
    Serial.print(totalFlowLitres, 3);

    Serial.println(F(" L"));
}


/********************************************************
 * RAIN SENSOR
 ********************************************************/

void readRainSensor()
{
    rainValue = analogRead(RAIN_SENSOR_PIN);


    Serial.print(F("Rain Sensor: "));
    Serial.println(rainValue);
}


/********************************************************
 * SEND DATA TO BLYNK
 *
 * Virtual Pin Mapping:
 *
 * V0 -> Manual Buzzer Control
 * V1 -> Temperature
 * V2 -> Humidity
 * V3 -> Water Level
 * V4 -> Flow Rate
 * V5 -> Total Flow
 * V6 -> Rain Sensor
 * V7 -> Ultrasonic Distance
 ********************************************************/

void sendDataToBlynk()
{
    Blynk.virtualWrite(V1, temperature);
    Blynk.virtualWrite(V2, humidity);
    Blynk.virtualWrite(V3, waterLevelCm);
    Blynk.virtualWrite(V4, flowRate);
    Blynk.virtualWrite(V5, totalFlowLitres);
    Blynk.virtualWrite(V6, rainValue);
    Blynk.virtualWrite(V7, distanceCm);
}


/********************************************************
 * BLYNK V0
 *
 * Manual buzzer control
 ********************************************************/

BLYNK_WRITE(V0)
{
    manualBuzzerState = param.asInt();

    digitalWrite(
        BUZZER_PIN,
        manualBuzzerState ? HIGH : LOW
    );


    if (manualBuzzerState)
    {
        Serial.println(F("Buzzer: ON"));

        // Blynk Event
        Blynk.logEvent(
            "flooding_detected",
            "Manual flood alert activated."
        );
    }
    else
    {
        Serial.println(F("Buzzer: OFF"));
    }
}