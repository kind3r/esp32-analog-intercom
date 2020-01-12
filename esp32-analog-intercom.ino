#include "config.h"
#include <U8g2lib.h>

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, DISPLAY_CLOCK, DISPLAY_DATA);
U8G2LOG display;
uint8_t u8log_buffer[U8LOG_WIDTH*U8LOG_HEIGHT];


#include <WiFi.h>
IPAddress staticIP(192,168,1,36);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);


#define Threshold 40 /* Greater the value, more the sensitivity */


RTC_DATA_ATTR int bootCount = 0;


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
  Serial.begin(115200);

  u8g2.begin();
  u8g2.setFont(u8g2_font_tom_thumb_4x6_mf);
  display.begin(u8g2, U8LOG_WIDTH, U8LOG_HEIGHT, u8log_buffer);
  display.setLineHeightOffset(0);	// set extra space between lines in pixel, this can be negative
  display.setRedrawMode(0);		// 0: Update screen with newline, 1: Update screen for every char 

  if(bootCount == 0) {
  }

  ++bootCount;
  display.println("Boot number: " + String(bootCount));

  print_wakeup_reason();

  display.printf("Connecting to %s", ssid);
  display.setRedrawMode(1);
  WiFi.begin(ssid, password);
  WiFi.config(staticIP, gateway, subnet);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(50);
    display.print(".");
  }
  display.setRedrawMode(0);
  display.println();
  display.println(WiFi.localIP());

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
