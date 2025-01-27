#ifndef CONFIG_H
#define CONFIG_H

#include <map>
#include <IPAddress.h>

#define WIFI_SSID ""
#define WIFI_PASSWORD ""

extern const IPAddress local_desired_IP;
extern const IPAddress gateway_IP;
extern const IPAddress subnet;
extern const IPAddress dns_IP;

#define MDNS_ENDPOINT "esp32_temperature"

extern const char *ntpServer1;
extern const char *ntpServer2;
extern const long gmtOffset_sec;
extern const int daylightOffset_sec;

#define LOKI_URL ""
extern const std::map<String, String> OBSERVABILITY_LABELS;

#define SYSTEM_METRIC true
#define WIFI_METRIC true
#define METRICS_REFRESH_RATE 15000

#endif
