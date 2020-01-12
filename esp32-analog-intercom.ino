#include "config.h"

#include <U8g2lib.h>
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, DISPLAY_CLOCK, DISPLAY_DATA);
U8G2LOG display;
uint8_t u8log_buffer[U8LOG_WIDTH*U8LOG_HEIGHT];

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <ArduinoJson.h>

volatile bool timerInterrupted = false;
hw_timer_t * timer = NULL;

#define Threshold 40 /* Greater the value, more the sensitivity */

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR uint8_t ip[4];
RTC_DATA_ATTR uint8_t gateway[4];
RTC_DATA_ATTR uint8_t subnet[4];
RTC_DATA_ATTR uint8_t dns[4];

/**
 * At first boot, we use DHCP and store the ip/gw/subnet values in RTC
 * to use them during the next wakeups
 */
void startWifi() {
  display.printf("Connecting to %s", ssid);
  display.setRedrawMode(1);
  WiFi.begin(ssid, password);
  if(bootCount != 0) { // restore settings from RTC
    WiFi.config(IPAddress(ip), IPAddress(gateway), IPAddress(subnet), IPAddress(dns));
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
    display.print("DNS:"); display.println(WiFi.dnsIP());
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
    dns[0] = WiFi.dnsIP()[0];
    dns[1] = WiFi.dnsIP()[1];
    dns[2] = WiFi.dnsIP()[2];
    dns[3] = WiFi.dnsIP()[3];
  }
}

void setClock() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  display.print("Waiting for NTP time sync: ");
  time_t nowSecs = time(nullptr);
  display.setRedrawMode(1);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    display.print(".");
    yield();
    nowSecs = time(nullptr);
  }
  display.setRedrawMode(0);
  display.println();
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  display.print("Current time: ");
  display.println(asctime(&timeinfo));
}

String createSensorPayload(const String &state) {
  const size_t capacity = JSON_OBJECT_SIZE(10);
  DynamicJsonDocument doc(capacity);

  JsonObject attributes = doc.createNestedObject("attributes");
  attributes["friendly_name"] = ha_device_name;
  doc["entity_id"] = "sensor." + ha_device_id;
  doc["state"] = state;

  String out;
  serializeJson(doc, out);
  return out;
}

void updateHASensor(const String &state) {
  WiFiClientSecure *client = new WiFiClientSecure;
  if(client) {

    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
      HTTPClient https;
  
      // display.print("[HTTPS] begin...\n");
      if (https.begin(*client, ha_url + "/api/states/sensor." + ha_device_id)) {
        https.addHeader("Authorization", "Bearer " + ha_token);
        String body = createSensorPayload(state);
        // display.println("[HTTPS] POST... " + body);
        int httpCode = https.POST(body);
        if (httpCode > 0) {
          display.printf("[HTTPS] POST... code: %d\n", httpCode);
          if(httpCode != 200) {
            String payload = https.getString();
            display.println(payload);
          }
        } else {
          display.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }
  
        https.end();
      } else {
        display.printf("[HTTPS] Unable to connect\n");
      }

      // End extra scoping block
    }
  
    delete client;
  } else {
    display.println("[HTTPS] Unable to create client");
  }

}

void lowPowerMode() {
  updateHASensor("idle");
  
  //Setup interrupt on Touch Pad 3 (GPIO15) to simulate ringing
  touchAttachInterrupt(T3, callback, Threshold);
  esp_sleep_enable_touchpad_wakeup();

  u8g2.setPowerSave(1);
  esp_deep_sleep_start();
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

void IRAM_ATTR onTimer() {
  timerInterrupted = true;
}

void setTimer(const uint64_t seconds) {
  timerInterrupted = false;
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, seconds * 1000000, false);
  timerAlarmEnable(timer);
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

  display.println("Boot number: " + String(bootCount));

  print_wakeup_reason();

  startWifi();

  if(bootCount == 0) {
    setClock();
  } else {
    updateHASensor("ringing");
  }

  bootCount++;

  // set a timer to simulate ringing stopped
  setTimer(10);
  display.println("Going to sleep in 10s");
}

void loop() {
  delay(100);
  if(timerInterrupted == true) {
    lowPowerMode();
  }
}
