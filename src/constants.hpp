#pragma once

//#define AHTx0
#define SHT31
//#define SWSERIAL

typedef enum : uint8_t {
  DisplayState1, // TODO
  DisplayStates
} DisplayState;

const uint8_t PinButton = 0;
const uint8_t PinPMS5003Enable = 13;
const uint8_t PinPMS5003SerialRx = 16;
const uint8_t PinPMS5003SerialTx = 17;
const uint8_t PinMHZ19SerialRx = 18;
const uint8_t PinMHZ19SerialTx = 19;

const uint8_t DisplayBrightness = 64;

const uint8_t DebounceInterval = 20;
const uint16_t UpdateInterval = 1000; // ms per tick
const uint32_t DataHistoryUpdateInterval = 30000; // ms
const uint32_t DataPushToServerInterval = 15000; // ms
const uint32_t DisplayTimeout = 30000; // ms

const uint8_t DataHistoryLength = 80; // samples

// Sensor stuff
const float TemperatureOffset = -10.0; // degrees C, compensate for sensor self heating
const uint32_t SGP30BaselineCheckInterval = 60000; // ms

// Calibration values - specific to each SGP30 sensor
const uint16_t SGP30BaselineECO2 = 0x941d;
const uint16_t SGP30BaselineTVOC = 0x953f;

// CO2 sensor preheat
const uint16_t MHZ19StartupPeriod = 60; // s

// Particulate matter sensor schedule (to extend sensor life)
const uint32_t PMMeasurementInterval = 120000; // ms, interval between readings
const uint32_t PMWakeDelay = 25000; // ms, time to wait after waking up the sensor
const uint32_t PMReadPeriod = 5000; // ms, time to sample the sensor after the wake delay

