#include <WiFi.h>

// Type your WiFi name and password between the quotes below.
// Before uploading this code to GitHub, change them back to the placeholders!
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
}

void loop() {
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength: ");
  Serial.println(rssi);
  delay(2000);
}
