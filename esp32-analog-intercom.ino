#include "config.h"

#include <U8g2lib.h>
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, DISPLAY_CLOCK, DISPLAY_DATA);
U8G2LOG display;
uint8_t u8log_buffer[U8LOG_WIDTH*U8LOG_HEIGHT];

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <ArduinoJson.h>

#include <WebServer.h>
#include <ESPmDNS.h>
WebServer server(80);

int bootCount = 0;
uint8_t ip[4];
uint8_t gateway[4];
uint8_t subnet[4];
uint8_t dns[4];

#include <Preferences.h>
Preferences prefs;

int talk_state = 0;
int open_state = 0;
int temp_state = 0;
bool firstLoop = true;

/**
 * Set pin modes
 */
void setPins() {
  pinMode(TALK, OUTPUT);
  digitalWrite(TALK, LOW);
  pinMode(OPEN, OUTPUT);
  digitalWrite(OPEN, LOW);
  pinMode(TALK_SW, INPUT_PULLDOWN);
  pinMode(OPEN_SW, INPUT_PULLDOWN);
}

/**
 * At first boot, we use DHCP and store the ip/gw/subnet values in RTC
 * to use them during the next wakeups
 */
void startWifi() {
  display.printf("Connecting to %s", ssid);
  size_t tempLen = prefs.getBytesLength("wifi_ip");
  if(tempLen != 0) {
    prefs.getBytes("wifi_ip", ip, 4);
    prefs.getBytes("wifi_gw", gateway, 4);
    prefs.getBytes("wifi_mask", subnet, 4);
    prefs.getBytes("wifi_dns", dns, 4);
  }
  WiFi.begin(ssid, password);
  if(tempLen != 0) {
    WiFi.config(IPAddress(ip), IPAddress(gateway), IPAddress(subnet), IPAddress(dns));
    display.print("[SAVED]");
  }
  display.setRedrawMode(1);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(50);
    display.print(".");
  }
  display.setRedrawMode(0);
  display.println();

  if(tempLen == 0) { // store settings in RTC
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
    prefs.putBytes("wifi_ip", ip, 4);
    prefs.putBytes("wifi_gw", gateway, 4);
    prefs.putBytes("wifi_mask", subnet, 4);
    prefs.putBytes("wifi_dns", dns, 4);
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
  attributes["boot_count"] = bootCount;
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

void httpHandleRoot() {
  display.println(server.header("user-agent"));
  server.send(200, "text/plain", "hello from " + ha_device_id);
}

void httpHandleOpen() {
  String authorization = server.header("authorization");
  if(authorization != device_token) {
    server.send(403, "text/plain", "Invalid authorization header");
  } else {
    server.send(200, "text/plain", "Opening door");
    openDoor();
  }
}

void httpHandleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

void openDoor() {
  display.println("Opening door");
  updateHASensor("opening");
  digitalWrite(TALK, HIGH);
  delay(2000);
  digitalWrite(TALK, LOW);
  updateHASensor("idle");
  digitalWrite(OPEN, HIGH);
  delay(2000);
  // we should not reach this state as power will be turned off
  // if power is not off, open sequence did not work
  updateHASensor("open_error");
}


void setup() {
  prefs.begin("IA003ESP", false);

  u8g2.begin();
  u8g2.setFont(u8g2_font_tom_thumb_4x6_mf);
  display.begin(u8g2, U8LOG_WIDTH, U8LOG_HEIGHT, u8log_buffer);
  display.setLineHeightOffset(0);	// set extra space between lines in pixel, this can be negative
  display.setRedrawMode(0);		// 0: Update screen with newline, 1: Update screen for every char 

  setPins();
  startWifi();
  MDNS.begin(ha_device_id.c_str());
  // setClock();

  server.on("/", httpHandleRoot);
  server.on("/open", httpHandleOpen);
  server.onNotFound(httpHandleNotFound);
  const char * headerkeys[] = {"User-Agent", "Authorization"} ;
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
  server.collectHeaders(headerkeys, headerkeyssize);
  server.begin();
  MDNS.addService("http", "tcp", 80);

  prefs.end();

  display.print("BOOT Took: ");
  display.println(millis());

  updateHASensor("ringing");
}

void loop() {
  server.handleClient();
  temp_state = digitalRead(TALK_SW);
  if(temp_state != talk_state) {
    talk_state = temp_state;
    if(talk_state == HIGH) {
      display.println("TALK HIGH");
    } else {
      display.println("TALK LOW");
    }
    digitalWrite(TALK, talk_state);
  }
  temp_state = digitalRead(OPEN_SW);
  if(temp_state != open_state) {
    open_state = temp_state;
    if(open_state == HIGH) {
      display.println("OPEN HIGH");
    } else {
      display.println("OPEN LOW");
    }
    digitalWrite(OPEN, open_state);
  }
  delay(100);
  if(firstLoop) {
    firstLoop = false;
    updateHASensor("idle");
  }
}
