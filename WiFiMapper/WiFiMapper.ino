// WiFi Signal Mapper
// The ESP32 joins your WiFi, then hosts its own website.
// Open http://wifimap.local (or the IP printed in Serial Monitor) on any
// phone or laptop on the same WiFi to see a live signal dial and map rooms.

#include <WiFi.h>        // lets the ESP32 join WiFi and read signal strength
#include <WebServer.h>   // lets the ESP32 act as a website
#include <ESPmDNS.h>     // gives the board a friendly name: wifimap.local
#include "secrets.h"     // your WiFi name + password (kept out of GitHub)

WebServer server(80);    // port 80 = the normal port for websites

// The web page lives in page.h (kept separate so Arduino does not edit it).
#include "page.h"

// When someone opens the main address, send them the page.
// Sent in 1 KB pieces so the whole page arrives, even on weak WiFi.
void handleRoot() {
  size_t len = strlen(PAGE);
  server.setContentLength(len);
  server.send(200, "text/html; charset=utf-8", "");
  for (size_t i = 0; i < len; i += 1024) {
    server.sendContent(PAGE + i, min((size_t)1024, len - i));
  }
}

// When the page asks for /data, send back the current signal as JSON.
void handleData() {
  String json = "{\"rssi\":" + String(WiFi.RSSI()) + "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);              // act like a normal device joining WiFi
  WiFi.setSleep(false);             // stay awake so the website answers quickly
  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");

  MDNS.begin("wifimap");            // makes http://wifimap.local work
  server.on("/", handleRoot);       // main page
  server.on("/data", handleData);   // live signal number
  server.begin();

  Serial.print("Open this on your phone: http://");
  Serial.println(WiFi.localIP());
  Serial.println("Or try: http://wifimap.local");
}

void loop() {
  server.handleClient();            // answer any phone/laptop asking for the page

  // Keep printing the address every 5 seconds so you can always find it.
  static unsigned long last = 0;
  if (millis() - last > 5000) {
    last = millis();
    Serial.print("Signal: ");
    Serial.print(WiFi.RSSI());
    Serial.print(" dBm   Dashboard: http://");
    Serial.println(WiFi.localIP());
  }
}
