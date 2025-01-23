#include <ESPmDNS.h>
#include <config.h>

void startMdns(){
  if (MDNS.begin(MDNS_ENDPOINT)) {
    Serial.println("mDNS responder started!");
    Serial.println("You can now access the ESP32 at http://" + String(MDNS_ENDPOINT) + ".local");
  } else {
    Serial.println("Error setting up mDNS responder!");
  }
}