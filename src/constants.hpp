#pragma once

//#define AHTx0
#define SHT31
//#define SWSERIAL

const uint8_t PinButton = 0;
const uint8_t PinPMS5003Enable = 13;

#ifdef SWSERIAL
const uint8_t PinPMS5003SerialRx = 14;
const uint8_t PinPMS5003SerialTx = 15;
const uint8_t PinMHZ19SerialRx = 12;
const uint8_t PinMHZ19SerialTx = 2;
#else
const uint8_t PinPMS5003SerialRx = 16;
const uint8_t PinPMS5003SerialTx = 17;
const uint8_t PinMHZ19SerialRx = 18;
const uint8_t PinMHZ19SerialTx = 19;
#endif

typedef enum : uint8_t {
  DisplayStateTempHumidity,
  DisplayStateVOC,
  DisplayStateCO2,
  DisplayStatePM,
  DisplayStates
} DisplayState;

const bool DisplayAlwaysOn = true;
const uint8_t DisplayBrightness = 128;

const uint8_t DebounceInterval = 20;
const uint16_t UpdateInterval = 1000; // ms per tick
const uint32_t DataHistoryUpdateInterval = 60000; // ms
const uint32_t DataPushToServerInterval = 15000; // ms
const uint32_t DisplayAutoCycleInterval = 5000; // ms
const uint32_t DisplayTimeout = 30000; // ms

const uint8_t DataHistoryLength = 60; // samples

// Sensor stuff
const float TemperatureOffset = -10.0; // degrees C, compensate for sensor self heating
const uint32_t SGP30BaselineCheckInterval = 60000; // ms

// Calibration values - specific to each SGP30 sensor
const uint16_t SGP30BaselineECO2 = 0x941d;
const uint16_t SGP30BaselineTVOC = 0x953f;

// CO2 sensor preheat
const uint16_t MHZ19StartupPeriod = 2;//60; // s

// Particulate matter sensor schedule (to extend sensor life)
const uint32_t PMMeasurementInterval = 120000; // ms, interval between readings
const uint32_t PMWakeDelay = 25000; // ms, time to wait after waking up the sensor
const uint32_t PMReadPeriod = 5000; // ms, time to sample the sensor after the wake delay

// for influxdb
const char* WIFI_SSID("ResNet Mobile Access");  // WiFi AP SSID
const char* WIFI_PASSWORD("");  // WiFi password
const char* INFLUXDB_URL("https://eastus-1.azure.cloud2.influxdata.com");  // InfluxDB v2 server url
const char* INFLUXDB_TOKEN("oTCuGzMSCrZrmrMO6xAZRvUDXFLWwpaWZkJDmj_mnAY-uqH8daolpmiDBuxGWOMa-lZB7rSq_tKFNaYn4lvUaQ==");  // InfluxDB v2 server or cloud API token
const char* INFLUXDB_ORG("cs147group11");// InfluxDB v2 organization id
const char* INFLUXDB_BUCKET("Indoor Air Quality Monitoring");  // InfluxDB v2 bucket name
const char* TZ_INFO("CET-1CEST,M3.5.0,M10.5.0/3");  // not needeed if not using influxdb cloud
