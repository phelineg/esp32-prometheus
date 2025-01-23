#include <WebServer.h>
#include <WiFi.h>
#include <map>

#include <metric.h>
#include <config.h>

// Custom Sensor libs
#include <DHT.h> // DHT Sensor Library by Adafruit

static const int DHT_SENSOR_PIN = 2;
DHT dht11(DHT_SENSOR_PIN, DHT11);

float dhtTemperature = NAN;
float dhtHumidity = NAN;
unsigned long lastSensorUpdate = 0;

WebServer server(80);

void handleMetrics() {

  String metrics;

  std::map<String, String> labels = {
    {"endpoint", "esp32_temperature"},
    {"location", "office"}
  };

  metrics += createPrometheusMetricString("system_uptime","Uptime in milliseconds",COUNTER,labels,String(millis()));

  if(WIFI_METRIC){
    metrics += createPrometheusMetricString("wifi_rssi", "Received Signal Strength Indicator value measures the strength of the Wi-Fi signal received by your ESP32", GAUGE, labels, String(WiFi.RSSI()));
    metrics += createPrometheusMetricString("wifi_mac_address", "MAC address of the ESP32's Wi-Fi interface", GAUGE, extendLabels(labels, {{"mac_address", WiFi.macAddress()}}), "1");
    metrics += createPrometheusMetricString("wifi_local_ip", "Current IP address of the ESP32", GAUGE, extendLabels(labels, {{"local_ip", WiFi.localIP().toString()}}), "1");
  }

  if(SYSTEM_METRIC){
    metrics += createPrometheusMetricString("system_heap_size", "Total heap size", GAUGE, labels, String(ESP.getHeapSize()));
    metrics += createPrometheusMetricString("system_free_heap", "Available free heap size", GAUGE, labels, String(ESP.getFreeHeap()));
    metrics += createPrometheusMetricString("system_min_free_heap", "Minimum free heap size since boot", GAUGE, labels, String(ESP.getMinFreeHeap()));
    metrics += createPrometheusMetricString("system_max_alloc_heap", "Largest allocable block of heap", GAUGE, labels, String(ESP.getMaxAllocHeap()));
    metrics += createPrometheusMetricString("system_psram_size", "Total SPI RAM size", GAUGE, labels, String(ESP.getPsramSize()));
    metrics += createPrometheusMetricString("system_free_psram", "Available free SPI RAM", GAUGE, labels, String(ESP.getFreePsram()));
    metrics += createPrometheusMetricString("system_min_free_psram", "Minimum free SPI RAM size since boot", GAUGE, labels, String(ESP.getMinFreePsram()));
    metrics += createPrometheusMetricString("system_max_alloc_psram", "Largest allocable block of SPI RAM", GAUGE, labels, String(ESP.getMaxAllocPsram()));
    metrics += createPrometheusMetricString("system_chip_revision", "ESP chip revision", GAUGE, labels, String(ESP.getChipRevision()));
    metrics += createPrometheusMetricString("system_chip_model", "ESP chip model", GAUGE, extendLabels(labels, {{"chip_model", String(ESP.getChipModel())}}), "1");
    metrics += createPrometheusMetricString("system_chip_cores", "Number of CPU cores on the ESP chip", GAUGE, labels, String(ESP.getChipCores()));
    metrics += createPrometheusMetricString("system_cpu_freq", "CPU frequency in MHz", GAUGE, labels, String(ESP.getCpuFreqMHz()));
    metrics += createPrometheusMetricString("system_cycle_count", "Cycle count since boot", GAUGE, labels, String(ESP.getCycleCount()));
    metrics += createPrometheusMetricString("system_sdk_version", "ESP-IDF SDK version", GAUGE, extendLabels(labels, {{"sdk_version", String(ESP.getSdkVersion())}}), "1");
    metrics += createPrometheusMetricString("system_core_version", "ESP core version", GAUGE, extendLabels(labels, {{"sdk_version", String(ESP.getCoreVersion())}}), "1");
    metrics += createPrometheusMetricString("system_flash_chip_size", "Flash chip size", GAUGE, labels, String(ESP.getFlashChipSize()));
    metrics += createPrometheusMetricString("system_flash_chip_speed", "Flash chip speed", GAUGE, labels, String(ESP.getFlashChipSpeed()));
    metrics += createPrometheusMetricString("system_flash_chip_mode", "Flash chip mode", GAUGE, labels, String(ESP.getFlashChipMode()));
    metrics += createPrometheusMetricString("system_sketch_size", "Sketch size in bytes", GAUGE, labels, String(ESP.getSketchSize()));
    metrics += createPrometheusMetricString("system_sketch_md5", "Sketch MD5 checksum", GAUGE, extendLabels(labels, {{"sketch_md5", String(ESP.getSketchMD5())}}), "1");
    metrics += createPrometheusMetricString("system_free_sketch_space", "Free space in the sketch", GAUGE, labels, String(ESP.getFreeSketchSpace()));
    metrics += createPrometheusMetricString("system_mac_address", "MAC address of the device", GAUGE, labels, String(ESP.getEfuseMac()));
  }

  if (!isnan(dhtTemperature)) {
    metrics += createPrometheusMetricString("dht_temperature", "Temperature in deg. C", GAUGE, labels, String(dhtTemperature));
  }
  if (!isnan(dhtHumidity)) {
    metrics += createPrometheusMetricString("dht_humidity", "Humidity %", GAUGE, labels, String(dhtHumidity));
  }

  server.send(200, "text/plain", metrics);
}

void updateSensorReadings() {
  dhtTemperature = dht11.readTemperature();
  dhtHumidity = dht11.readHumidity();
}

void setup() {
  Serial.begin(115200);
  delay(100);

  connectToWifi();
  startMdns();

  server.on("/metrics", handleMetrics);

  server.begin();
  Serial.println("HTTP server started!");
}

void loop() {
  server.handleClient();

  if (millis() - lastSensorUpdate >= METRICS_REFRESH_RATE) {
      lastSensorUpdate = millis();
      updateSensorReadings();
  }
}