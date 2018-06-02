
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

int timezone = 0;
int dst = 0;
int tubeStatusTimer = 300;
String tubeStatus = "";

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x64)
  display.clearDisplay();

  display.setTextColor(WHITE);
  
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

  if (tubeStatusTimer == 300) {
    tubeStatusTimer = 0;

    String districtStatus = getLineDelay("district");
    String piccadillyStatus = getLineDelay("piccadilly");
    
    tubeStatus = "District: \n\t" + districtStatus + ",\n\nPiccadilly: \n\t" + piccadillyStatus;    
    Serial.println(tubeStatus);
  }
  
  displayTimeAndTube(time, tubeStatus);
  delay(1000);

  tubeStatusTimer++; 
}

void displayTimeAndTube(String time, String tube) {
  display.clearDisplay();

  display.setTextSize(2);
  display.setCursor(15,0);
  display.println(time);

  display.setTextSize(1);
  display.setCursor(0,25);
  display.println(tube);
  
  display.display();
}

String getLineDelay(String line) {
  String status = "Error Connecting";

  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    HTTPClient http;  //Declare an object of class HTTPClient
    String thumbprint = "75 8A 5F 6E 00 87 72 89 9E 17 2E 62 66 59 07 9C 3E BF E8 52";
    String url = "https://api.tfl.gov.uk/Line/" + line + "/Status?app_id=34252ee6&app_key=aeedebd1d41961c42db6c7b4aee7f6ac";
    Serial.println(url);
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
  JsonObject& root_0_lineStatuses0 = root_0["lineStatuses"][0];
  const char* root_0_lineStatuses0_statusSeverityDescription = root_0_lineStatuses0["statusSeverityDescription"]; // "Good Service"
  

  return root_0_lineStatuses0_statusSeverityDescription;
}

String getTime() {
  time_t now = time(nullptr);
  struct tm foo;
  char outstr[200];
  strftime(outstr, sizeof(outstr), "%H:%M:%S", localtime_r (&now, &foo));
  return outstr;
}

