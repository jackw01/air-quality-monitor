#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_SHT31.h>
#include <sgp30_patched.hpp> // For some reason, adafruit's library won't work with my sensor
#include <Adafruit_PM25AQI.h>
#include <MHZ.h>
#include <U8g2lib.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#include "constants.hpp"
#include "credentials.hpp"

#ifdef SWSERIAL
#include <SoftwareSerial.h>
#else
#include <HardwareSerial.h>
#endif

#ifdef ESP8266_WIFI
#include <ESP8266WiFiMulti.h>
#else
#include <WiFiMulti.h>
#endif

#ifdef ESP8266_HOMEKIT
#include <arduino_homekit_server.h>
#endif

typedef struct {
  float temperature;         // C - temperature compensated for sensor self-heating
  float humidity;            // %RH - humidity compensated for sensor self-heating
  float temperatureRaw;      // C - raw temperature - for debug purposes
  float humidityRaw;         // %RH - raw relative humidity - for debug purposes
  float absoluteHumidity;    // g/m^3 - always correct, independent of sensor temperature
  float dewPoint;            // C - always correct, independent of sensor temperature
  uint16_t tvoc;             // ppb
  uint16_t eco2;             // ppm
  uint16_t co2;              // ppm
  uint16_t pm10;             // μg/m^3
  uint16_t pm25;             // μg/m^3
  uint16_t pm100;            // μg/m^3
} sensor_data;

class System {
  public:
    System();
    void init();
    void tick();

  private:
    void updateDisplay();

    void drawLineGraph(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                       int32_t values[], uint8_t count, int32_t min, int32_t max,
                       bool showRange, uint8_t decimalPlaces = 0, uint16_t scaling = 1);
    void drawLineGraph(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                       float values[], uint8_t count, float min, float max,
                       bool showRange, uint8_t decimalPlaces = 1);
    void setDisplayBrightness(uint8_t brightness, uint8_t p1 = 1, uint8_t p2 = 10);
    void sendSensorData(Point sensor);
    uint8_t getSubjectiveAirQuality();

    Point temperaturePoint = Point("Temperature");
    Point humidityPoint = Point("Humidity");
    Point vocPoint = Point("Volatile Organic Compounds");
    Point co2Point = Point("Carbon Dioxide");
    Point pmPoint = Point("Particulate Matter");

#ifdef ESP8266_WIFI
    ESP8266WiFiMulti wifiMulti;
#else
    WiFiMulti wifiMulti;
#endif

    InfluxDBClient client = InfluxDBClient(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

#ifdef AHTx0
    Adafruit_AHTX0 aht = Adafruit_AHTX0();
#endif
#ifdef SHT31
    Adafruit_SHT31 sht = Adafruit_SHT31();
#endif

    Adafruit_SGP30 sgp30 = Adafruit_SGP30();

#ifdef SWSERIAL
    SoftwareSerial *co2Serial = new SoftwareSerial();
    SoftwareSerial *pmSerial = new SoftwareSerial();
#else
    HardwareSerial *co2Serial = new HardwareSerial(1);
    HardwareSerial *pmSerial = new HardwareSerial(2);
#endif

    MHZ mhz19 = MHZ(co2Serial, MHZ19C);
    Adafruit_PM25AQI pms5003 = Adafruit_PM25AQI();

    U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2 = U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C(U8G2_R2);

    bool firstUpdate = true;
    uint32_t lastUpdate = 0;
    uint32_t lastBaselineCheck = 0;
    uint32_t lastDataHistoryUpdate = 0;
    uint32_t lastDataPoint = 0;
    uint32_t lastDisplayCycle = 0;
    uint32_t lastDisplayOn = 0;
    uint32_t lastPMWake = 0;
    bool pmMeasurementDone = false;

    bool prevButtonState = false;
    uint32_t lastButtonChange = 0;

    bool displayOn = false;
    bool displayCycle = true;
    bool displayNeedsUpdate = true;
    uint8_t displayState = 0;

    // Sensor data
    sensor_data currentSensorData;
    sensor_data oldSensorData[DataHistoryLength];

    uint16_t sgp30TvocBase = 0;
    uint16_t sgp30Eco2Base = 0;

    sensor_data pmTempData;
    uint8_t pmSampleCount = 0;
};
