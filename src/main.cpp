/*

*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <NeoPixelBus.h>
#include <json.h>
#include "secrets.h"

char appname[] = "INFO: home_show_alone start";
char host[] = "192.168.147.1";
char topicResult[] = "home_show/RESULT";
char topicCommand[] = "tele/tasmota_A6C75F/SENSOR";

WiFiClient wificlient;
PubSubClient mqttclient(wificlient);

#define         PIXEL_COUNT         (8 * 64)
#define         PIXEL_PIN           22
#define         COLOR_SATURATION    10

// multiple led strips
// https://github.com/Makuna/NeoPixelBus/discussions/529
NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0Ws2812xMethod> strip(PIXEL_COUNT, PIXEL_PIN);

RgbColor red(COLOR_SATURATION, 0, 0);
RgbColor green(0, COLOR_SATURATION, 0);
RgbColor blue(0, 0, COLOR_SATURATION);
RgbColor white(COLOR_SATURATION);
RgbColor black(0);

HslColor hslRed(red);
HslColor hslGreen(green);
HslColor hslBlue(blue);
HslColor hslWhite(white);
HslColor hslBlack(black);


void wifiConnect() {
  WiFi.begin(ssid, pass);
  Serial.print("WIFI: Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
}

void wifiData() {
  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("\nWIFI: IP Address: ");
  Serial.println(ip);
  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("WIFI: MAC address: ");
  Serial.print(mac[5], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.println(mac[0], HEX);
  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("WIFI: signal strength (RSSI):");
  Serial.println(rssi);
}

void checkWiFi() {
  switch (WiFi.status()) {
    case WL_CONNECT_FAILED:
    case WL_CONNECTION_LOST:
    case WL_DISCONNECTED:
      Serial.println("WIFI: not connected");
      WiFi.begin(ssid, pass);
      return;
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("MQTT: Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void mqttConnect() {
  Serial.print("MQTT: Connecting to mqtt ");
  Serial.print(host);
  Serial.println(" broker...");
  mqttclient.setServer(host, 1883);
  mqttclient.setCallback(mqttCallback);
}

void publishHello() {
  if (mqttclient.connected()) mqttclient.publish(topicResult, appname);
}

void mqttReconnect() {
  // Loop until we're reconnected
  while (!mqttclient.connected()) {
    Serial.println("MQTT: Attempting MQTT connection...");
    // Attempt to connect
    if (mqttclient.connect("home_show_alone")) {
      Serial.println("MQTT: connected");
      // Once connected, publish an announcement...
      mqttclient.publish(topicResult,"MQTT: connected: hello world");
      // ... and resubscribe
      mqttclient.subscribe(topicCommand);
    } else {
      Serial.print("MQTT: failed, rc=");
      Serial.print(mqttclient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void ledInit() {

}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println(appname);
  Serial.flush();
  
  strip.Begin();
  strip.Show();

  //wifiConnect();
  //wifiData();
  //mqttConnect();
}

void loop() {
  // if (!mqttclient.connected()) {
  //   mqttReconnect();
  // }
  // mqttclient.loop();

  // set the colors, 
  // if they don't match in order, you need to use NeoGrbFeature feature
  for (int i = 0; i < PIXEL_COUNT; ++i) {
    strip.SetPixelColor(i, red);
    //v.Show();
    strip.SetPixelColor(i, green);
    //strip.Show();
    strip.SetPixelColor(i, blue);
    //strip.Show();
    strip.SetPixelColor(i, white);
    strip.Show();
    strip.SetPixelColor(i, black);
    strip.Show();
  }
}
