#include <WiFi.h>
#include <config.h>

IPAddress local_desired_IP(10, 4, 117, 129);
IPAddress gateway_IP(10, 4, 117, 102);
IPAddress subnet(255, 255, 255, 0);

void connectToWifi(){
  if (!WiFi.config(local_desired_IP, gateway_IP, subnet)) {
    Serial.println("Failed to configure Static IP");
  }

  Serial.print("Connecting to Wi-Fi");
  Serial.print(WIFI_SSID);
  Serial.print(WIFI_PASSWORD);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nConnected to Wi-Fi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}