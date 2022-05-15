#include "upload.hpp"

WiFiMulti wifiMulti;

// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// for influxdb2 (not cloud)
// InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);

void connectToDatabase() {
  // call in setup
  Serial.begin(115200);

  // Setup wifi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.printf("Connecting to wifi");
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.printf("Could not connect to wifi.");
  }
  Serial.println();

  // Add tags - only once
  // sensor.addTag("Sensor", "SensorName");

  // not needeed if not using influxdb cloud
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Check server connection
  if (client.validateConnection()) {
    Serial.printf("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.printf("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}
