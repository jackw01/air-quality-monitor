#ifndef UPLOAD_HPP
#define UPLOAD_HPP
#include <Arduino.h>

#include <WiFiMulti.h>
extern WiFiMulti wifiMulti;

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

// WiFi AP SSID
#define WIFI_SSID "ResNet Mobile Access"
// WiFi password
#define WIFI_PASSWORD ""
// InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
#define INFLUXDB_URL "https://eastus-1.azure.cloud2.influxdata.com"
// InfluxDB v2 server or cloud API token (Use: InfluxDB UI -> Data -> API Tokens -> Generate API Token)
#define INFLUXDB_TOKEN "oTCuGzMSCrZrmrMO6xAZRvUDXFLWwpaWZkJDmj_mnAY-uqH8daolpmiDBuxGWOMa-lZB7rSq_tKFNaYn4lvUaQ=="
// InfluxDB v2 organization id (Use: InfluxDB UI -> User -> About -> Common Ids )
#define INFLUXDB_ORG "cs147group11"
// InfluxDB v2 bucket name (Use: InfluxDB UI ->  Data -> Buckets)
#define INFLUXDB_BUCKET "Indoor Air Quality Monitoring"

// not needeed if not using influxdb cloud
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

extern InfluxDBClient client;

void connectToDatabase();  // call in setup

template <typename T>
void sendSensorData(String measurementName, T& measurementValue, String sensorName)
{
  // measurementName   - what is being measured (e.g., degreesC). _field on influx
  // measurementValue  - the amount of whatever was measured (e.g., 32). _value on influx
  // sensorName        - name of the sensor (e.g., AHTX0). _measurement on influx
  /*if (sensorName == ""){
      sensorName = measurementName;
  }*/
  
  // Data point
  Point sensor(sensorName);
  sensor.clearFields();
  sensor.addField(measurementName, measurementValue);
  // Check WiFi connection and reconnect if needed
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Wifi connection lost");
  }

  // Write point
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}

#endif
