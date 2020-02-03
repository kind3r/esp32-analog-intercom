#define U8X8_USE_PINS
#define DISPLAY_CLOCK 4
#define DISPLAY_DATA 5
#define U8LOG_WIDTH 32
#define U8LOG_HEIGHT 10

const char* ssid = "**********";
const char* password = "**********";

const String ha_url = "http://YOUR_HA_SERVER_IP:8123";
const String ha_token = "LONG_LIVED_ACCESS_TOKEN";
const String ha_device_id = "sensor_id";
const String ha_device_name = "sensor name";

const String device_token = "************";

// Talk relay
const int TALK = 12;
// Open relay
const int OPEN = 13;
// Talk switch
const int TALK_SW = 14;
// Open switch
const int OPEN_SW = 15;