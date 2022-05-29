#include "system.hpp"

// Approximate absolute humidity formula (grams / m^3)
static float getAbsoluteHumidity(float t, float h) {
  float b = 17.62;
  float c = 243.12;
  return 216.7f * ((h / 100.0f) * 6.112f * exp((b * t) / (c + t)) / (273.15f + t));
}

// Approximate dew point formula
static float getDewPoint(float t, float h) {
  float b = 17.62;
  float c = 243.12;
  return c * (log(h / 100.0f) + (b * t) / (c + t)) / (b - log(h / 100.0f) - (b * t) / (c + t));
}

// Approximate relative humidity formula
static float getRelativeHumidity(float t, float td) {
  float b = 17.62;
  float c = 243.12;
  return 100.0 * exp((c * b * (td - t)) / ((c + t) * (c + td)));
}

// Celsius to Fahrenheit
static float cToF(float t) {
  return t * 1.8 + 32.0;
}

System::System() {
}

// Initialize everything
void System::init() {
  Serial.begin(115200);
  Serial.printf("Initializing...\n");

  // Inputs
  pinMode(PinButton, INPUT_PULLUP);

  // Display
  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setFontMode(1);
  setDisplayBrightness(DisplayBrightness);

  // Display startup screen
  u8g2.setFont(u8g2_font_profont10_tf);
  u8g2.drawStr(0, 6, "Connecting to WiFi...");
  u8g2.sendBuffer();

  // Connect to wifi
  Serial.printf("Connecting to WiFi...");
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  for (uint8_t i = 0; i < WiFiCount; i++) wifiMulti.addAP(WiFiSSID[i], WiFiPassword[i]);
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.printf(".");
    delay(100);
  }
  Serial.printf("\n");
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  temperaturePoint.addTag("ssid", WiFi.SSID());
  humidityPoint.addTag("ssid", WiFi.SSID());
  vocPoint.addTag("ssid", WiFi.SSID());
  co2Point.addTag("ssid", WiFi.SSID());
  pmPoint.addTag("ssid", WiFi.SSID());

  // Check server connection
  if (client.validateConnection()) {
    Serial.printf("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.printf("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  // Check for sensors

  // Serial ports
#ifdef SWSERIAL
    co2Serial->begin(9600, SWSERIAL_8N1, PinMHZ19SerialRx, PinMHZ19SerialTx);
    pmSerial->begin(9600, SWSERIAL_8N1, PinPMS5003SerialRx, PinPMS5003SerialTx);
#else
    co2Serial->begin(9600, SERIAL_8N1, PinMHZ19SerialRx, PinMHZ19SerialTx);
    pmSerial->begin(9600, SERIAL_8N1, PinPMS5003SerialRx, PinPMS5003SerialTx);
#endif

  // Temp/Humidity
#ifdef AHTx0
  if (! aht.begin()) Serial.printf("Failed to communicate with AHTx0 sensor\n");
#endif
#ifdef SHT31
  if (! sht.begin()) Serial.printf("Failed to communicate with AHTx0 sensor\n");
#endif

  // VOC
  if (!sgp30.begin()) Serial.printf("Failed to communicate with SGP30 sensor\n");
  Serial.printf("SGP30 Serial number: %04x%04x%04x\n",
                sgp30.serialnumber[0], sgp30.serialnumber[1], sgp30.serialnumber[2]);
  sgp30.setIAQBaseline(SGP30BaselineECO2, SGP30BaselineTVOC);

  // CO2
  mhz19.setAutoCalibrate(false);

  // PM
  pinMode(PinPMS5003Enable, OUTPUT);
  digitalWrite(PinPMS5003Enable, true);
  pms5003.begin_UART(pmSerial);
  digitalWrite(PinPMS5003Enable, false);

  // Wait for CO2 sensor
  Serial.printf("CO2 sensor preheat...\n");
  u8g2.drawStr(0, 6 + 10, "CO2 sensor preheat...");
  u8g2.drawFrame(0, 24, MHZ19StartupPeriod + 4, 8);
  u8g2.sendBuffer();
  for (uint8_t i = 0; i < MHZ19StartupPeriod; i++) {
    u8g2.drawBox(2, 26, i, 4);
    u8g2.sendBuffer();
    delay(1000);
  }

  Serial.printf("\n\n");
}

// Update function, called in a loop
void System::tick() {
  uint32_t time = millis();

  // Check button
  bool buttonState = !digitalRead(PinButton);
  if (buttonState != prevButtonState) {
    if (time - lastButtonChange > DebounceInterval && buttonState) {
      lastDisplayOn = time;
      if (displayOn == false) {
        // If display is off, turn it on
        setDisplayBrightness(DisplayBrightness);
        displayOn = true;
        lastDisplayCycle = time;
      } else if (displayCycle == true) {
        // If display is on, stop display from cycling automatically
        displayCycle = false;
      } else if (displayCycle == false) {
        // Manually cycle display state
        if (++displayState == DisplayStates) displayState = 0;
      }
    }
    lastButtonChange = time;
  }
  prevButtonState = buttonState;

  // Rate limit the loop
  if (time - lastUpdate >= UpdateInterval) {

    // Temp/Humidity
#ifdef AHTx0
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);
    currentSensorData.temperatureRaw = temp.temperature;
    currentSensorData.temperature = temp.temperature;
    currentSensorData.humidityRaw = humidity.relative_humidity;
#endif
#ifdef SHT31
    sht.readBoth(&currentSensorData.temperatureRaw, &currentSensorData.humidityRaw);
    currentSensorData.temperature = currentSensorData.temperatureRaw + TemperatureOffset;
#endif

    // Compensate humidity readings for sensor self-heating
    currentSensorData.absoluteHumidity = getAbsoluteHumidity(currentSensorData.temperatureRaw,
                                                             currentSensorData.humidityRaw);
    currentSensorData.dewPoint = getDewPoint(currentSensorData.temperatureRaw,
                                             currentSensorData.humidityRaw);
    currentSensorData.humidity = getRelativeHumidity(currentSensorData.temperature,
                                                     currentSensorData.dewPoint);

    // VOC
    sgp30.setHumidity((uint32_t)(1000.0 * currentSensorData.absoluteHumidity));
    if (!sgp30.IAQmeasure()) {
      Serial.printf("SGP30 Failed to read VOC level\n");
    } else {
      currentSensorData.tvoc = sgp30.TVOC;
      currentSensorData.eco2 = sgp30.eCO2;
    }

    // CO2
    int16_t co2 = mhz19.readCO2UART();
    if (co2 >= 0) {
      currentSensorData.co2 = co2;
      // Send data
      // TODO: since co2 sensor only produces data every 120 seconds, send it here
      co2Point.clearFields();
      co2Point.addField("CO2 PPM", currentSensorData.co2);
      sendSensorData(co2Point);
    } else {
      Serial.printf("MHZ19 No new data available\n");
    }

    // Particulate matter
    bool pmRead;
    PM25_AQI_Data pm;
    if (time - lastPMWake >= PMMeasurementInterval || firstUpdate) {
      Serial.printf("PMS5003 Wake\n");
      digitalWrite(PinPMS5003Enable, true);
      lastPMWake = time;
      pmMeasurementDone = false;
    } else if (time - lastPMWake >= PMWakeDelay + PMReadPeriod && !pmMeasurementDone) {
      Serial.printf("PMS5003 Sleep\n");
      digitalWrite(PinPMS5003Enable, false);
      pmMeasurementDone = true;
      currentSensorData.pm10 = pmTempData.pm10 / pmSampleCount;
      currentSensorData.pm25 = pmTempData.pm25 / pmSampleCount;
      currentSensorData.pm100 = pmTempData.pm100 / pmSampleCount;
      currentSensorData.part003 = pmTempData.part003;
      currentSensorData.part005 = pmTempData.part005;
      currentSensorData.part010 = pmTempData.part010;
      currentSensorData.part025 = pmTempData.part025;
      currentSensorData.part050 = pmTempData.part050;
      currentSensorData.part100 = pmTempData.part100;
      pmTempData.pm10 = 0;
      pmTempData.pm25 = 0;
      pmTempData.pm100 = 0;
      pmSampleCount = 0;

      // Send data
      // TODO: since pm sensor only produces data every 120 seconds, send it here
      pmPoint.clearFields();
      pmPoint.addField("PM 1.0 μg/m^3", currentSensorData.pm10);
      pmPoint.addField("PM 2.5 μg/m^3", currentSensorData.pm25);
      pmPoint.addField("PM 10 μg/m^3", currentSensorData.pm100);
      sendSensorData(pmPoint);
    }

    pmRead = pms5003.read(&pm);
    if (pmRead) {
      if (time - lastPMWake >= PMWakeDelay &&
          time - lastPMWake < PMWakeDelay + PMReadPeriod) {
        Serial.printf("PMS5003 Sample %d\n", ++pmSampleCount);
        pmTempData.pm10 += pm.pm10_standard;
        pmTempData.pm25 += pm.pm25_standard;
        pmTempData.pm100 += pm.pm100_standard;
        pmTempData.part003 = pm.particles_03um;
        pmTempData.part005 = pm.particles_05um;
        pmTempData.part010 = pm.particles_10um;
        pmTempData.part025 = pm.particles_25um;
        pmTempData.part050 = pm.particles_50um;
        pmTempData.part100 = pm.particles_100um;
      }
    } else {
      Serial.printf("PMS5003 No new data available\n");
    }

    // Serial output
    Serial.printf("Temperature C       : %f\n", currentSensorData.temperature);
    Serial.printf("Temperature C (raw) : %f\n", currentSensorData.temperatureRaw);
    Serial.printf("Humidity %%RH        : %f\n", currentSensorData.humidity);
    Serial.printf("Humidity %%RH (raw)  : %f\n", currentSensorData.humidityRaw);
    Serial.printf("Humidity g/m^3      : %f\n", currentSensorData.absoluteHumidity);
    Serial.printf("Dew Point C         : %f\n", currentSensorData.dewPoint);
    Serial.printf("TVOC ppb            : %d\n", currentSensorData.tvoc);
    Serial.printf("eCO2 ppm            : %d\n", currentSensorData.eco2);
    Serial.printf("CO2 ppm             : %d\n", currentSensorData.co2);
    if (pmRead) {
      Serial.printf("PM1.0 μg/m^3 (live) : %d\n", pm.pm10_standard);
      Serial.printf("PM2.5 μg/m^3 (live) : %d\n", pm.pm25_standard);
      Serial.printf("PM10 μg/m^3 (live)  : %d\n", pm.pm100_standard);
    } else {
      Serial.printf("PM1.0 μg/m^3 (old)  : %d\n", currentSensorData.pm10);
      Serial.printf("PM2.5 μg/m^3 (old)  : %d\n", currentSensorData.pm25);
      Serial.printf("PM10 μg/m^3 (old)   : %d\n", currentSensorData.pm100);
    }
    if (pmSampleCount > 0) {
      Serial.printf("PM1.0 μg/m^3 (avg)  : %d\n", pmTempData.pm10 / pmSampleCount);
      Serial.printf("PM2.5 μg/m^3 (avg)  : %d\n", pmTempData.pm25 / pmSampleCount);
      Serial.printf("PM10 μg/m^3 (avg)   : %d\n", pmTempData.pm100 / pmSampleCount);
    }
    Serial.printf("\n");

    // Send data to server
    if (time - lastDataPoint >= DataPushToServerInterval) {

      // TODO
      temperaturePoint.clearFields();
      temperaturePoint.addField("Temperature C", currentSensorData.temperature);
      temperaturePoint.addField("Dew Point C", currentSensorData.dewPoint);
      sendSensorData(temperaturePoint);

      humidityPoint.clearFields();
      humidityPoint.addField("Relative Humidity %", currentSensorData.humidity);
      humidityPoint.addField("Absolute Humidity g/m^3", currentSensorData.absoluteHumidity);
      sendSensorData(humidityPoint);

      vocPoint.clearFields();
      vocPoint.addField("TVOC PPB", currentSensorData.tvoc);
      sendSensorData(vocPoint);

      lastDataPoint = time;
    }

    // Save historical data
    if (time - lastDataHistoryUpdate >= DataHistoryUpdateInterval || firstUpdate) {
      for (uint8_t i = 0; i < DataHistoryLength - 1; i++) {
        if (firstUpdate) oldSensorData[i] = currentSensorData;
        else oldSensorData[i] = oldSensorData[i + 1];
      }
      oldSensorData[DataHistoryLength - 1] = currentSensorData;
      firstUpdate = false;
      lastDataHistoryUpdate = time;
    }

    // SGP30 baseline check
    if (time - lastBaselineCheck >= SGP30BaselineCheckInterval) {
      if (!sgp30.getIAQBaseline(&sgp30Eco2Base, &sgp30TvocBase)) {
        Serial.printf("SGP30 Failed to read baseline values\n");
      }
      Serial.printf("SGP30 Baseline eCO2 %04x TVOC %04x\n\n", sgp30Eco2Base, sgp30TvocBase);
      lastBaselineCheck = time;
    }

    // Cycle display states
    if (displayCycle && time - lastDisplayCycle >= DisplayAutoCycleInterval) {
      if (++displayState == DisplayStates) displayState = 0;
      lastDisplayCycle = time;
    }

    // Display timeout
    if (time - lastDisplayOn >= DisplayTimeout) {
      setDisplayBrightness(0, 7, 7);
      displayOn = false;
      displayCycle = true;
    }
    updateDisplay();
    lastUpdate = time;
  }
}

void System::updateDisplay() {
  u8g2.clearBuffer();
  char line[32];
  uint8_t width;

  if (displayOn || DisplayAlwaysOn) {
    if (displayState == DisplayStateTempHumidity) {
      // Big text
      u8g2.setFont(u8g2_font_profont22_tf);
      sprintf(line, "%2.1f", currentSensorData.temperature);
      width = u8g2.drawUTF8(0, 14, line);
      u8g2.setFont(u8g2_font_profont12_tf);
      u8g2.drawUTF8(width, 11, "°C");

      u8g2.setFont(u8g2_font_profont22_tf);
      sprintf(line, "%2.0f", currentSensorData.humidity);
      width = u8g2.drawUTF8(66, 14, line);
      u8g2.setFont(u8g2_font_profont12_tf);
      u8g2.drawUTF8(66 + width, 11, "%RH");

      // Line graphs
      float values[DataHistoryLength];
      float min = 100;
      float max = -100;
      for (uint8_t i = 0; i < DataHistoryLength; i++) {
        values[i] = oldSensorData[i].temperature;
        if (values[i] < min) min = values[i];
        if (values[i] > max) max = values[i];
      }
      drawLineGraph(0, 16, 62, 15, values, DataHistoryLength, min - 1, max + 1, false);

      min = 100;
      max = -100;
      for (uint8_t i = 0; i < DataHistoryLength; i++) {
        values[i] = oldSensorData[i].humidity;
        if (values[i] < min) min = values[i];
        if (values[i] > max) max = values[i];
      }
      drawLineGraph(64, 16, 62, 15, values, DataHistoryLength, min - 1, max + 1, false);
    } else if (displayState == DisplayStateVOC) {
      // Big text
      u8g2.setFont(u8g2_font_profont22_tf);
      sprintf(line, "%5d", currentSensorData.tvoc);
      width = u8g2.drawUTF8(0, 14, line);
      u8g2.setFont(u8g2_font_profont12_tf);
      u8g2.drawUTF8(width, 11, "ppb VOC");

      // Line graph
      int32_t values[DataHistoryLength];
      int32_t min = 65535;
      int32_t max = 0;
      for (uint8_t i = 0; i < DataHistoryLength; i++) {
        values[i] = oldSensorData[i].tvoc;
        if (values[i] < min) min = values[i];
        if (values[i] > max) max = values[i];
      }
      drawLineGraph(0, 16, 128, 15, values, DataHistoryLength, 0, max, true);
    } else if (displayState == DisplayStateCO2) {
      // Big text
      u8g2.setFont(u8g2_font_profont22_tf);
      sprintf(line, "%4d", currentSensorData.co2);
      width = u8g2.drawUTF8(0, 14, line);
      u8g2.setFont(u8g2_font_profont12_tf);
      u8g2.drawUTF8(width, 11, "ppm CO2");

      // Line graph
      int32_t values[DataHistoryLength];
      int32_t min = 65535;
      int32_t max = 0;
      for (uint8_t i = 0; i < DataHistoryLength; i++) {
        values[i] = oldSensorData[i].co2;
        if (values[i] < min) min = values[i];
        if (values[i] > max) max = values[i];
      }
      drawLineGraph(0, 16, 128, 15, values, DataHistoryLength, min, max, true);
    } else if (displayState == DisplayStatePM) {
      // Big text
      u8g2.setFont(u8g2_font_profont22_tf);
      sprintf(line, "%4d", min((int32_t)currentSensorData.pm25, 9999));
      width = u8g2.drawUTF8(0, 14, line);
      u8g2.setFont(u8g2_font_profont12_tf);
      u8g2.drawUTF8(width, 11, "ug/m^3 PM2.5");

      // Line graph
      int32_t values[DataHistoryLength];
      int32_t min = 65535;
      int32_t max = 0;
      for (uint8_t i = 0; i < DataHistoryLength; i++) {
        values[i] = oldSensorData[i].pm25;
        if (values[i] < min) min = values[i];
        if (values[i] > max) max = values[i];
      }
      drawLineGraph(0, 16, 128, 15, values, DataHistoryLength, 0, max + 1, true);
    }
  }

  u8g2.sendBuffer();
}

void System::drawLineGraph(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                           int32_t values[], uint8_t count, int32_t min, int32_t max,
                           bool showRange, uint8_t decimalPlaces, uint16_t scaling) {
  uint8_t xOffset = 0;
  if (showRange) {
    char line[6];
    u8g2.setFont(u8g2_font_tom_thumb_4x6_mn);
    u8g2.setFontMode(0);

    sprintf(line, "%.*f", decimalPlaces, (float)max / (float)scaling);
    u8g2.drawStr(x, y + 6, line);
    xOffset = u8g2.getStrWidth(line);

    sprintf(line, "%.*f", decimalPlaces, (float)min / (float)scaling);
    u8g2.drawStr(x, y + h + 1, line);
    uint8_t xOffset2 = u8g2.getStrWidth(line);
    if (xOffset2 > xOffset) xOffset = xOffset2;

    u8g2.setFontMode(1);
  }

  uint8_t graphW = w - xOffset;
  if (min != max && graphW / count >= 1) {
    uint8_t lastY = map(values[0], min, max, y + h, y);
    for (uint8_t i = 1; i < count; i++) {
      uint8_t thisY = map(values[i], min, max, y + h, y);
      u8g2.drawLine(x + xOffset + (i - 1) * graphW / count, lastY,
                    x + xOffset + i * graphW / count, thisY);
      lastY = thisY;
    }
  }
}

void System::drawLineGraph(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                           float values[], uint8_t count, float min, float max,
                           bool showRange, uint8_t decimalPlaces) {
  uint16_t scaling = 10;
  for (uint8_t i = 0; i < decimalPlaces - 1; i++) scaling *= 10;
  int32_t intValues[count];
  for (uint8_t i = 0; i < DataHistoryLength; i++) {
    intValues[i] = (int32_t)(values[i] * scaling);
  }
  drawLineGraph(x, y, w, h,
                intValues, count, (int32_t)(min * scaling), (int32_t)(max * scaling),
                showRange, decimalPlaces, scaling);
}

void System::setDisplayBrightness(uint8_t brightness, uint8_t p1, uint8_t p2) {
  u8x8_cad_StartTransfer(u8g2.getU8x8());
  // Set Vcom deselect value to 0 to increase range
  u8x8_cad_SendCmd(u8g2.getU8x8(), 0x0db);
  u8x8_cad_SendArg(u8g2.getU8x8(), 0 << 4);
  u8x8_cad_EndTransfer(u8g2.getU8x8());
  // Set precharge period value 1-15 - p1 higher = darker, p2 higher = brighter
  u8x8_cad_StartTransfer(u8g2.getU8x8());
  u8x8_cad_SendCmd(u8g2.getU8x8(), 0x0d9);
  u8x8_cad_SendArg(u8g2.getU8x8(), (p2 << 4) | p1);
  u8x8_cad_EndTransfer(u8g2.getU8x8());
  u8g2.setContrast(brightness);
}

void System::sendSensorData(Point sensor) {
  // Check WiFi connection and reconnect if needed
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.printf("WiFi connection lost\n");
  }

  // Write point
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}
