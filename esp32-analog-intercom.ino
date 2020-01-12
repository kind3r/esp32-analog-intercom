#include "config.h"

#include <U8g2lib.h>
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, DISPLAY_CLOCK, DISPLAY_DATA);
U8G2LOG display;
uint8_t u8log_buffer[U8LOG_WIDTH*U8LOG_HEIGHT];

#include <WiFi.h>

#define Threshold 40 /* Greater the value, more the sensitivity */

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR uint8_t ip[4];
RTC_DATA_ATTR uint8_t gateway[4];
RTC_DATA_ATTR uint8_t subnet[4];

/**
 * At first boot, we use DHCP and store the ip/gw/subnet values in RTC
 * to use them during the next wakeups
 */
void startWifi() {
  display.printf("Connecting to %s", ssid);
  display.setRedrawMode(1);
  WiFi.begin(ssid, password);
  if(bootCount != 0) { // restore settings from RTC
    WiFi.config(IPAddress(ip), IPAddress(gateway), IPAddress(subnet));
  }
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(50);
    display.print(".");
  }
  display.setRedrawMode(0);
  display.println();
  if(bootCount == 0) { // store settings in RTC
    display.print("IP: "); display.println(WiFi.localIP());
    display.print("GW: "); display.println(WiFi.gatewayIP());
    display.print("NET:"); display.println(WiFi.subnetMask());
    ip[0] = WiFi.localIP()[0];
    ip[1] = WiFi.localIP()[1];
    ip[2] = WiFi.localIP()[2];
    ip[3] = WiFi.localIP()[3];
    gateway[0] = WiFi.gatewayIP()[0];
    gateway[1] = WiFi.gatewayIP()[1];
    gateway[2] = WiFi.gatewayIP()[2];
    gateway[3] = WiFi.gatewayIP()[3];
    subnet[0] = WiFi.subnetMask()[0];
    subnet[1] = WiFi.subnetMask()[1];
    subnet[2] = WiFi.subnetMask()[2];
    subnet[3] = WiFi.subnetMask()[3];
  }
}

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : display.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : display.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : display.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : display.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : display.println("Wakeup caused by ULP program"); break;
    default : display.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void callback(){
  //placeholder callback function
}


void setup() {
  u8g2.begin();
  u8g2.setFont(u8g2_font_tom_thumb_4x6_mf);
  display.begin(u8g2, U8LOG_WIDTH, U8LOG_HEIGHT, u8log_buffer);
  display.setLineHeightOffset(0);	// set extra space between lines in pixel, this can be negative
  display.setRedrawMode(0);		// 0: Update screen with newline, 1: Update screen for every char 

  if(bootCount == 0) {
  }

  display.println("Boot number: " + String(bootCount));

  print_wakeup_reason();

  startWifi();

  ++bootCount;

  //Setup interrupt on Touch Pad 3 (GPIO15)
  touchAttachInterrupt(T3, callback, Threshold);

  //Configure Touchpad as wakeup source
  esp_sleep_enable_touchpad_wakeup();
}

void loop() {
  display.println("Going to sleep now");
  delay(1000);
  u8g2.setPowerSave(1);
  esp_deep_sleep_start();
}
