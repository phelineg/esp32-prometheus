#include <WebServer.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "time.h"
#include "esp_sntp.h"

#include <metric.h>
#include <config.h>

// Custom Sensor libs
#include <DHT.h> // DHT Sensor Library by Adafruit

static const int DHT_SENSOR_PIN = 2;
DHT dht11(DHT_SENSOR_PIN, DHT11);

float dhtTemperature = NAN;
float dhtHumidity = NAN;
unsigned long lastSensorUpdate = 0;

const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;
const char *time_zone = "CET-1CEST,M3.5.0,M10.5.0/3";

HTTPClient httpClient;
WebServer server(80);

long long getLokiTimestamp() {

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("No time available (yet)");
    return 0;
  }

  time_t now = mktime(&timeinfo);
  long long timestamp_ns = (long long)now * 1000000000;

  return timestamp_ns;
}

void timeavailable(struct timeval *t) {
  Serial.println("Got time adjustment from NTP!");
}

void handleMetrics() {

  lokiLog("esp32_temperature", "INFO", "Metric poll");

  String metrics;

  metrics += createPrometheusMetricString("system_uptime_total","Uptime in milliseconds",COUNTER,OBSERVABILITY_LABELS,String(millis()));

  if(WIFI_METRIC){
    metrics += createPrometheusMetricString("wifi_rssi", "Received Signal Strength Indicator value measures the strength of the Wi-Fi signal received by your ESP32", GAUGE, OBSERVABILITY_LABELS, String(WiFi.RSSI()));
    metrics += createPrometheusMetricString("wifi_mac_address", "MAC address of the ESP32's Wi-Fi interface", GAUGE, extendLabels(OBSERVABILITY_LABELS, {{"mac_address", WiFi.macAddress()}}), "1");
    metrics += createPrometheusMetricString("wifi_local_ip", "Current IP address of the ESP32", GAUGE, extendLabels(OBSERVABILITY_LABELS, {{"local_ip", WiFi.localIP().toString()}}), "1");
  }

  if(SYSTEM_METRIC){
    metrics += createPrometheusMetricString("system_heap_size", "Total heap size", GAUGE, OBSERVABILITY_LABELS, String(ESP.getHeapSize()));
    metrics += createPrometheusMetricString("system_free_heap", "Available free heap size", GAUGE, OBSERVABILITY_LABELS, String(ESP.getFreeHeap()));
    metrics += createPrometheusMetricString("system_min_free_heap", "Minimum free heap size since boot", GAUGE, OBSERVABILITY_LABELS, String(ESP.getMinFreeHeap()));
    metrics += createPrometheusMetricString("system_max_alloc_heap", "Largest allocable block of heap", GAUGE, OBSERVABILITY_LABELS, String(ESP.getMaxAllocHeap()));
    metrics += createPrometheusMetricString("system_psram_size", "Total SPI RAM size", GAUGE, OBSERVABILITY_LABELS, String(ESP.getPsramSize()));
    metrics += createPrometheusMetricString("system_free_psram", "Available free SPI RAM", GAUGE, OBSERVABILITY_LABELS, String(ESP.getFreePsram()));
    metrics += createPrometheusMetricString("system_min_free_psram", "Minimum free SPI RAM size since boot", GAUGE, OBSERVABILITY_LABELS, String(ESP.getMinFreePsram()));
    metrics += createPrometheusMetricString("system_max_alloc_psram", "Largest allocable block of SPI RAM", GAUGE, OBSERVABILITY_LABELS, String(ESP.getMaxAllocPsram()));
    metrics += createPrometheusMetricString("system_chip_revision", "ESP chip revision", GAUGE, OBSERVABILITY_LABELS, String(ESP.getChipRevision()));
    metrics += createPrometheusMetricString("system_chip_model", "ESP chip model", GAUGE, extendLabels(OBSERVABILITY_LABELS, {{"chip_model", String(ESP.getChipModel())}}), "1");
    metrics += createPrometheusMetricString("system_chip_cores", "Number of CPU cores on the ESP chip", GAUGE, OBSERVABILITY_LABELS, String(ESP.getChipCores()));
    metrics += createPrometheusMetricString("system_cpu_freq", "CPU frequency in MHz", GAUGE, OBSERVABILITY_LABELS, String(ESP.getCpuFreqMHz()));
    metrics += createPrometheusMetricString("system_cycle_count_total", "Cycle count since boot", COUNTER, OBSERVABILITY_LABELS, String(ESP.getCycleCount()));
    metrics += createPrometheusMetricString("system_sdk_version", "ESP-IDF SDK version", GAUGE, extendLabels(OBSERVABILITY_LABELS, {{"sdk_version", String(ESP.getSdkVersion())}}), "1");
    metrics += createPrometheusMetricString("system_core_version", "ESP core version", GAUGE, extendLabels(OBSERVABILITY_LABELS, {{"sdk_version", String(ESP.getCoreVersion())}}), "1");
    metrics += createPrometheusMetricString("system_flash_chip_size", "Flash chip size", GAUGE, OBSERVABILITY_LABELS, String(ESP.getFlashChipSize()));
    metrics += createPrometheusMetricString("system_flash_chip_speed", "Flash chip speed", GAUGE, OBSERVABILITY_LABELS, String(ESP.getFlashChipSpeed()));
    metrics += createPrometheusMetricString("system_flash_chip_mode", "Flash chip mode", GAUGE, OBSERVABILITY_LABELS, String(ESP.getFlashChipMode()));
    metrics += createPrometheusMetricString("system_sketch_size", "Sketch size in bytes", GAUGE, OBSERVABILITY_LABELS, String(ESP.getSketchSize()));
    metrics += createPrometheusMetricString("system_sketch_md5", "Sketch MD5 checksum", GAUGE, extendLabels(OBSERVABILITY_LABELS, {{"sketch_md5", String(ESP.getSketchMD5())}}), "1");
    metrics += createPrometheusMetricString("system_free_sketch_space", "Free space in the sketch", GAUGE, OBSERVABILITY_LABELS, String(ESP.getFreeSketchSpace()));
    metrics += createPrometheusMetricString("system_mac_address", "MAC address of the device", GAUGE, OBSERVABILITY_LABELS, String(ESP.getEfuseMac()));
  }

  if (!isnan(dhtTemperature)) {
    metrics += createPrometheusMetricString("dht_temperature", "Temperature in deg. C", GAUGE, OBSERVABILITY_LABELS, String(dhtTemperature));
  }
  if (!isnan(dhtHumidity)) {
    metrics += createPrometheusMetricString("dht_humidity", "Humidity %", GAUGE, OBSERVABILITY_LABELS, String(dhtHumidity));
  }

  server.send(200, "text/plain", metrics);
}

void updateSensorReadings() {

  lokiLog("esp32_temperature", "INFO", "Collecting DHT Sensor Information");

  dhtTemperature = dht11.readTemperature();
  dhtHumidity = dht11.readHumidity();
}

void lokiLog(const char* endpoint, const char* logLevel, const char* message) {

  long long ts = getLokiTimestamp();
  if (ts == 0) {
    Serial.println("Failed to retrieve timestamp. Will not push to Loki.");
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {

    String labelsJson = "";
    for (const auto& label : extendLabels(OBSERVABILITY_LABELS, {{"level", String(logLevel)}})) {
      labelsJson += "\"" + label.first + "\": \"" + label.second + "\", ";
    }

    if (labelsJson.length() > 0) {
      labelsJson.remove(labelsJson.length() - 2, 2);
    }

    String payload = String(
      "{"
      "  \"streams\": [{"
      "    \"stream\": {" + labelsJson + "},"
      "    \"values\": [[ \"" + String(ts) + "\",\"" + String(message) + "\"]]"
      "  }]"
      "}"
    );

    Serial.println("Log try: " + payload);

    httpClient.begin(LOKI_URL);
    httpClient.addHeader("Content-Type", "application/json");
    int httpResponseCode = httpClient.POST(payload);

    if (httpResponseCode > 0) {
      Serial.println("Log sent successfully: " + String(httpResponseCode));
    } else {
      Serial.println("Error sending log: " + String(httpClient.errorToString(httpResponseCode).c_str()));
    }

    httpClient.end();
  } else {
    Serial.println("WiFi not connected. Cannot send log.");
  }
}

void setup() {
  Serial.begin(115200);

  connectToWifi();
  startMdns();

  sntp_set_time_sync_notification_cb(timeavailable);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

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