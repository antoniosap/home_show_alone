/*

*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include <json.h>
#include "secrets.h"

char appname[] = "INFO: home_show_alone start";
char host[] = "192.168.147.1";
char topicResult[] = "home_show/RESULT";
char topicCommand[] = "tele/tasmota_A6C75F/SENSOR";

WiFiClient wificlient;
PubSubClient mqttclient(wificlient);

// make sure to set these panel values to the sizes of yours
const uint8_t PanelWidth  = 64;  // n pixel x m pixel matrix of leds
const uint8_t PanelHeight = 8;
const uint8_t TileWidth   = 1;  // laid out in n1 panels x m2 panels mosaic
const uint8_t TileHeight  = 1;

#define         PIXEL_COUNT         (PanelWidth * PanelHeight * TileWidth * TileHeight)
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

typedef ColumnMajorAlternatingLayout MyPanelLayout;
NeoRgbwCurrentSettings settings = NeoRgbwCurrentSettings(1, 1, 1, 1);
NeoPixelAnimator animations(PIXEL_COUNT, NEO_CENTISECONDS);

typedef ColumnMajorLayout MyTilesLayout;
NeoTiles <MyPanelLayout, MyTilesLayout> tiles(
    PanelWidth,
    PanelHeight,
    TileWidth,
    TileHeight);

//----------------------------------------------------------------------------------
#include <stdint.h>
#include "font8x8_ib8x8u.ino"
#include "font8x8_ic8x8u.ino"

#define FONT8X8 font8x8_ic8x8u

const uint8_t *getImage(uint8_t ch)
{
    if (ch < 128) {
      return FONT8X8[ch];
    }
    if (ch >= 128 && ch < 160) {
      // miscellaneous
      ch -= 128;
      if (ch < sizeof(FONT8X8) / 8) {
        return FONT8X8[ch];
      }
      return FONT8X8[0]; // valeur par défaut
    }
    return FONT8X8[ch - 160 + 128];
}

void rotateChar90(const uint8_t *image, uint8_t newb[8])
{
    for (int i = 0; i < 8; ++i)
    {
        newb[i] = 0;
        for (int j = 0; j < 8; ++j)
        {
            uint8_t b = pgm_read_byte_near(image + j);
            newb[i] |= (b & (1 << i)) ? 1 << (7 - j) : 0;
        }
    }
}

void rotateChar270(const uint8_t *image, uint8_t newb[8])
{
    for (int i = 0; i < 8; ++i)
    {
        newb[i] = 0;
        for (int j = 0; j < 8; ++j)
        {
            uint8_t b = pgm_read_byte_near(image + j);
            newb[i] |= (b & (1 << (7 - i))) ? 1 << j : 0;
        }
    }
}

uint8_t printChar(uint8_t ch, uint16_t cursor)
{
    const uint8_t *image = getImage(ch);
    uint8_t newb1[8];

    rotateChar270(image, newb1);
    for (int i = 0; i < 8; i++) {
        uint8_t b = newb1[i];
        uint8_t mask = 1;

        for (int j = 0; j < 8; j++) {
          if (b & mask) {
            strip.SetPixelColor(tiles.Map(cursor, j), white);
          } else {
            strip.SetPixelColor(tiles.Map(cursor, j), black);
          }
          mask <<= 1;
        }
        cursor++;
    }
    return cursor;
}

// void StrBase::printString(LedControl &lc, unsigned long wait, uint8_t rotate) const
// {
//     for (size_t i = 0; i < length(); ++i)
//     {
//         printChar(lc, at(i), rotate);
//         delay(wait);
//     }
// }

// // wrappers

// // PROGMEM strings
// void displayString_P(LedControl &lc, const char *str, unsigned long wait, bool scroll, uint8_t rotate)
// {
//     StrP(str).displayString(lc, wait, scroll, rotate);
// }

// // DATA strings
// void displayString(LedControl &lc, const char *str, unsigned long wait, bool scroll, uint8_t rotate)
// {
//     StrD(str).displayString(lc, wait, scroll, rotate);
// }
//----------------------------------------------------------------------------------

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

void ledTestLights(RgbColor color) {
  // set the colors, 
  // if they don't match in order, you need to use NeoGrbFeature feature
  // auto maTotal = strip.CalcTotalMilliAmpere(NeoRgbwCurrentSettings(1, 1, 1, 1));
  for (int i = 0; i < PIXEL_COUNT; ++i) {
    strip.SetPixelColor(i, color);
    strip.Show();
    strip.SetPixelColor(i, black);
    strip.Show();
  }
}

void ledTestLightsAll() {
  ledTestLights(red);
  ledTestLights(green);
  ledTestLights(blue);
  ledTestLights(white);
}

void ledTestTiles() {
  // use the topo to map the 2d cordinate to the pixel
  // and use that to SetPixelColor
  const uint16_t left = 0;
  const uint16_t right = PanelWidth - 1;
  const uint16_t top = 0;
  const uint16_t bottom = PanelHeight - 1;

  strip.SetPixelColor(tiles.Map(left, top), white);
  strip.SetPixelColor(tiles.Map(right, top), red);
  strip.SetPixelColor(tiles.Map(right, bottom), green);
  strip.SetPixelColor(tiles.Map(left, bottom), blue);
  strip.Show();
}

void ledClearToBlack() {
  strip.ClearTo(black);
  strip.Show();
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

  //ledTestLightsAll();
  ledTestTiles();
  delay(5000);

  uint16_t cursor = 0;

  cursor = printChar('A', cursor);
  strip.Show();
  delay(1000);

  cursor = printChar('0', cursor);
  strip.Show();
  delay(1000);

  cursor = printChar('1', cursor);
  strip.Show();
  
  //delay(5000);
  //ledClearToBlack();
}

void loop() {
  // if (!mqttclient.connected()) {
  //   mqttReconnect();
  // }
  // mqttclient.loop();
}
