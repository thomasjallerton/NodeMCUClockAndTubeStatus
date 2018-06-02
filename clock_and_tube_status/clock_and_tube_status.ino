
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <time.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

#define OLED_RESET LED_BUILTIN
Adafruit_SSD1306 display(OLED_RESET);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2
const char* ssid = "SKY01B9D";
const char* password = "CFRDQFSDPP";
const int MAX_TUBER_TIMER = 60000;
const int SLEEP_TIME = 5;

int maxLength, minX;
int timezone = 0;
int dst = 0;
int tubeStatusTimer = MAX_TUBER_TIMER;

struct TubeStatus {
  String name;
  String status;
  int position;
} tubeStatuses[2];

void setup() {
  tubeStatuses[0].name = "District: ";
  tubeStatuses[1].name = "Piccadilly: ";
  tubeStatuses[0].position = 10;
  tubeStatuses[1].position = 10;
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x64)
  display.clearDisplay();

  display.setTextColor(WHITE);
  display.setTextWrap(false);
  maxLength = display.width() / 6;
    
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    display.fillCircle(display.width()/2, display.height()/2, 10, WHITE);
    display.display();
    Serial.print(".");
    delay(1000);
  }

  configTime(0, 3600, "pool.ntp.org", "time.nist.gov");
  Serial.println("\nWaiting for time");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("");
}

void loop() {
  String time = getTime();

  if (tubeStatusTimer == MAX_TUBER_TIMER) {
    Serial.println("Getting tube status");
    tubeStatusTimer = 0;

    tubeStatuses[0].status = "\t" + getLineDelay("district");
    tubeStatuses[1].status = "\t" + getLineDelay("piccadilly");
  }

  handlePosition(&tubeStatuses[0]);
  handlePosition(&tubeStatuses[1]);
  
  displayTimeAndTube(time);
  delay(SLEEP_TIME);

  tubeStatusTimer++; 
}

void displayTimeAndTube(String time) {
  display.clearDisplay();

  display.setTextSize(2);
  display.setCursor(15, 0);
  display.println(time);

  display.setTextSize(1);
  display.setCursor(0,25);
  display.println(tubeStatuses[0].name);
  display.setCursor(tubeStatuses[0].position, 33);
  display.println(tubeStatuses[0].status);

  display.setCursor(0,47);
  display.println(tubeStatuses[1].name);
  display.setCursor(tubeStatuses[1].position, 56);
  display.println(tubeStatuses[1].status);
  
  display.display();
}

String getLineDelay(String line) {
  String status = "Error Connecting";

  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    HTTPClient http;  //Declare an object of class HTTPClient
    String thumbprint = "75 8A 5F 6E 00 87 72 89 9E 17 2E 62 66 59 07 9C 3E BF E8 52";
    String url = "https://api.tfl.gov.uk/Line/" + line + "/Status?app_id=34252ee6&app_key=aeedebd1d41961c42db6c7b4aee7f6ac";
    http.begin(url, thumbprint);  
    int httpCode = http.GET();                                                                

    if (httpCode = HTTP_CODE_OK) { //Check the returning code

      status = readLineStatus(http.getString());   //Get the request response payload

    } else {
      Serial.println("Error connecting to tfl, status code: " + httpCode);
    }

    http.end();   //Close connection
  }
  return status;
}


String readLineStatus(String payload) {
  const size_t bufferSize = 3*JSON_ARRAY_SIZE(0) + 2*JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(11) + 900;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  
  JsonArray& root = jsonBuffer.parseArray(payload);
  JsonObject& root_0 = root[0];
  int size = root_0["lineStatuses"].size();
  String result = "";
  for (int i = 0; i < size; i++) {
    String status = root_0["lineStatuses"][i]["statusSeverityDescription"];
    if (i < size - 1) {
      result += status + " + ";
    } else {
      result += status;
    }
  }
  
  return result;
}

String getTime() {
  time_t now = time(nullptr);
  struct tm foo;
  char outstr[200];
  strftime(outstr, sizeof(outstr), "%H:%M:%S", localtime_r (&now, &foo));
  return outstr;
}

boolean isTooLong(TubeStatus* status) {
  return status->status.length() > maxLength;
}

void handlePosition(TubeStatus* status) {
  if (isTooLong(status)) {
    int pixelLength = -(status->status.length() * 6);
    if (status->position < pixelLength) {
      status->position = display.width();
    } else {
      status->position--;
    }
  } else {
    status->position = 0;
  }
}

