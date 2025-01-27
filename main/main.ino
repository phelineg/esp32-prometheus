#include <WebServer.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <time.h>
#include <esp_sntp.h>

#include "MetricUtil.h"
#include "LogUtil.h"
#include "Config.h"

// Custom Sensor libs
#include <DHT.h> // DHT Sensor Library by Adafruit

static const int DHT_SENSOR_PIN = 2;
DHT dht11(DHT_SENSOR_PIN, DHT11);

float dhtTemperature = NAN;
float dhtHumidity = NAN;
unsigned long lastSensorUpdate = 0;

WebServer server(80);

void timeavailable(struct timeval *t) {
  Serial.println("Got time adjustment from NTP!");
}

void handleMetrics() {

  log("INFO", "Metric endpoint polled");

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

  log("INFO", "Collecting DHT sensor information");

  dhtTemperature = dht11.readTemperature();
  dhtHumidity = dht11.readHumidity();

  if (isnan(dhtTemperature) || isnan(dhtHumidity)) {
    log("WARNING", "Failed to collect DHT sensor information");
  }
}

void setup() {
  Serial.begin(115200);

  /**
  * Wifi
  */

  if (!WiFi.config(local_desired_IP, gateway_IP, subnet, dns_IP)) {
    log("ERROR", "Failed to configure Static IP");
  }

  log("INFO", "Connecting to Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  log("INFO","Connected to Wi-Fi! IP Address: " + WiFi.localIP().toString());

  /**
  * MDNS
  */

  if (MDNS.begin(MDNS_ENDPOINT)) {
    log("INFO", "mDNS responder started!");
    log("INFO", "You can now access the ESP32 at http://" + String(MDNS_ENDPOINT) + ".local");
  } else {
    log("ERROR", "Error setting up mDNS responder!");
  }

  /**
  * NTP
  */

  sntp_set_time_sync_notification_cb(timeavailable);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

  /**
  * HTTP Endpoint
  */

  server.on("/metrics", handleMetrics);
  server.begin();

  log("INFO", "HTTP server started!");
}

void loop() {

  server.handleClient();

  if (millis() - lastSensorUpdate >= METRICS_REFRESH_RATE) {
      lastSensorUpdate = millis();
      updateSensorReadings();
  }

}