#include "config.h"
#include <Wire.h>
// #include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#include <WiFi.h>

IPAddress staticIP(192,168,1,36);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */
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

  display.display();
}

void callback(){
  //placeholder callback function
}


void setup() {
  Serial.begin(115200);
  Wire.begin(5, 4);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C, bootCount == 0)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  if(bootCount == 0) {
    display.dim(true);
  }
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);

  ++bootCount;
  display.println("Boot number: " + String(bootCount));

  print_wakeup_reason();

  display.printf("Connecting to %s ", ssid);
  display.display();
  WiFi.begin(ssid, password);
  WiFi.config(staticIP, gateway, subnet);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    display.print(".");
    display.display();
  }
  display.println();
  display.println(WiFi.localIP());
  display.display();

  //Setup interrupt on Touch Pad 3 (GPIO15)
  touchAttachInterrupt(T3, callback, Threshold);

  //Configure Touchpad as wakeup source
  esp_sleep_enable_touchpad_wakeup();
}

void loop() {
  display.println("Going to sleep now");
  display.display();
  delay(1000);
  esp_deep_sleep_start();
}
