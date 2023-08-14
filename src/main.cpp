/*

*/

#include "WiFi.h"

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.flush();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("home_show_alone - show ip address");
}

void loop() {

}
