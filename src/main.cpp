/* ESP32 HTTP IoT Server Example for Wokwi.com
  https://wokwi.com/arduino/projects/320964045035274834
  When running it on Wokwi for VSCode, you can connect to the 
  simulated ESP32 server by opening http://localhost:8180
  in your browser. This is configured by wokwi.toml.
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <uri/UriBraces.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "ClosedCube_HDC1080.h"
#include <Adafruit_BMP085.h>
#include "SPIFFS.h"

#define WIFI_SSID "cxf"
#define WIFI_PASSWORD "12345678"
#define WIFI_CHANNEL 6
IPAddress localIP(192, 168, 137, 10); // hardcoded
IPAddress localGateway(192, 168, 137, 1); //hardcoded
IPAddress subnet(255, 255, 0, 0);
WebServer server(80);

#define SCREEN_ADDRESS 0x3C 
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
ClosedCube_HDC1080 hdc1080;
Adafruit_BMP085 bmp;

const int LED1 = LED_BUILTIN;
const int LED2 = 15;
bool led1State = false;
bool led2State = false;

String HTMLstr;
void sendHtml() {
  String response = String(HTMLstr);
  response.replace("LED1_TEXT", led1State ? "ON" : "OFF");
  response.replace("LED2_TEXT", led2State ? "ON" : "OFF");
  server.send(200, "text/html", response);
}

void putText(String s,int xpos=10,int ypos=10,int size=1){
  display.setTextSize(size);         // set text size
  display.setTextColor(WHITE);
  display.setCursor(xpos,ypos);
  display.print(s);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  // 显示初始化
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {Serial.println(F("SSD1306 failed"));return;}
  delay(200);
  display.clearDisplay(); 
  display.display(); 
  putText("Hello!",30,22,2);
  display.display();

  //WIFI初始化
  WiFi.config(localIP, localGateway, subnet);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  Serial.print("Connecting to WiFi ");
  Serial.print(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println(" Connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //SPIFFS存储初始化
  if(!SPIFFS.begin(true)){Serial.println("An Error has occurred while mounting SPIFFS");return;
  }
  File file = SPIFFS.open("/index.html");
  if(!file){Serial.println("Failed to open file!");return;}
  while(file.available()){
    //Serial.write(file.read());
    HTMLstr.concat(char(file.read()));
  }
  file.close();

  //传感器初始化
  hdc1080.begin(0x40);
  bmp.begin();

  //WebSever初始化
  server.on("/", sendHtml);
  server.on(UriBraces("/toggle/{}"), []() {
    String led = server.pathArg(0);
    Serial.print("Toggle LED #");
    Serial.println(led);
    switch (led.toInt()) {
      case 1:
        led1State = !led1State;
        display.clearDisplay();
        putText("Toggle LED 1");
        display.display();
        digitalWrite(LED1, led1State);
        break;
      case 2:
        led2State = !led2State;
        display.clearDisplay();
        putText("Toggle LED 2");
        display.display();
        digitalWrite(LED2, led2State);
        break;
    }

    sendHtml();
  });
  server.begin();
  Serial.println("HTTP server started (http://localhost:80)");
}
int in = 0;
void loop() {
  server.handleClient();
  delay(200);
  double temperature = hdc1080.readTemperature();
  double huimidity   = hdc1080.readHumidity();
  int    pressure    = bmp.readPressure();
  double altitude    = bmp.readAltitude(101500);

  display.clearDisplay();
  putText("Temperature: ",2,2,1);
  putText("Huimidity: ",2,36,1);
  putText("C",95,15,2);
  putText("%",95,50,2);
  display.setTextSize(2);
  display.setCursor(28,15);
  display.println(temperature);
  display.setCursor(28,50);
  display.println(huimidity);
  display.display();
}