#include <map>

#ifndef CONFIG_H
#define CONFIG_H

#define WIFI_SSID ""
#define WIFI_PASSWORD ""

#define MDNS_ENDPOINT "esp32_temperature"

#define LOKI_URL ""

const std::map<String, String> OBSERVABILITY_LABELS = {
    {"endpoint", MDNS_ENDPOINT},
    {"location", "office"}
};

#define SYSTEM_METRIC true
#define WIFI_METRIC true

#define METRICS_REFRESH_RATE 15000

#endif
