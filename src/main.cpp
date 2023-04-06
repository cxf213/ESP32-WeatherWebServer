/* ESP32 HTTP IoT Server Example for Wokwi.com
  https://wokwi.com/arduino/projects/320964045035274834
  When running it on Wokwi for VSCode, you can connect to the
  simulated ESP32 server by opening http://localhost:8180
  in your browser. This is configured by wokwi.toml.
*/

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "ClosedCube_HDC1080.h"
#include <Adafruit_BMP085.h>
#include "SPIFFSTool/SPIFFSTool.h"

//WebServer 配置
AsyncWebServer server(80);
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";
String ssid;
String pass;
String ip;
String gateway;
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";
IPAddress localIP;
IPAddress localGateway;
IPAddress subnet(255, 255, 0, 0);
unsigned long previousMillis = 0;
const long interval = 10000; 


#define SCREEN_ADDRESS 0x3C
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
ClosedCube_HDC1080 hdc1080;
Adafruit_BMP085 bmp;

const int LED1 = LED_BUILTIN;
String led1State;
double temperature;
double huimidity;
int pressure;
double altitude;

// Initialize WiFi
bool initWiFi() {
  if(ssid=="" || ip==""){
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  localIP.fromString(ip.c_str());
  localGateway.fromString(gateway.c_str());


  if (!WiFi.config(localIP, localGateway, subnet)){
    Serial.println("STA Failed to configure");
    return false;
  }
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("Connecting to WiFi...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while(WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.println("Failed to connect.");
      return false;
    }
  }

  Serial.println(WiFi.localIP());
  return true;
}

void putText(String s, int xpos = 10, int ypos = 10, int size = 1)
{
  display.setTextSize(size); // set text size
  display.setTextColor(WHITE);
  display.setCursor(xpos, ypos);
  display.print(s);
}

String processor(const String& var) {
  if(var == "STATE") {
    if(digitalRead(LED1)) {
      led1State = "ON";
    }
    else {
      led1State = "OFF";
    }
    return led1State;
  }
  return String();
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED1, OUTPUT);

  initSPIFFS();
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);
  ip = readFile(SPIFFS, ipPath);
  gateway = readFile (SPIFFS, gatewayPath);

  // 显示初始化
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 failed"));
    return;
  }
  delay(200);
  display.clearDisplay();
  display.display();
  putText("Hello!", 30, 22, 2);
  display.display();

  // 传感器初始化
  hdc1080.begin(0x40);
  bmp.begin();

  if(initWiFi()) {
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
    });
    server.serveStatic("/", SPIFFS, "/");
    
    // Route to set GPIO state to HIGH
    server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request) {
      digitalWrite(LED1, HIGH);
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
    });

    // Route to set GPIO state to LOW
    server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request) {
      digitalWrite(LED1, LOW);
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
    });
    server.begin();
  }
  else {
    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("ESP-WIFI-MANAGER", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP); 

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/wifimanager.html", "text/html");
    });
    
    server.serveStatic("/", SPIFFS, "/");
    
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(SPIFFS, passPath, pass.c_str());
          }
          // HTTP POST ip value
          if (p->name() == PARAM_INPUT_3) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            // Write file to save value
            writeFile(SPIFFS, ipPath, ip.c_str());
          }
          // HTTP POST gateway value
          if (p->name() == PARAM_INPUT_4) {
            gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
            // Write file to save value
            writeFile(SPIFFS, gatewayPath, gateway.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
      delay(3000);
      ESP.restart();
    });
    server.begin();
  }

}
int in = 0;
void loop()
{
  delay(100);
  temperature = hdc1080.readTemperature();
  huimidity = hdc1080.readHumidity();
  pressure = bmp.readPressure();

  display.clearDisplay();
  putText("Temperature: ", 2, 2, 1);
  putText("Huimidity: ", 2, 36, 1);
  putText("C", 95, 15, 2);
  putText("%", 95, 50, 2);
  display.setTextSize(2);
  display.setCursor(28, 15);
  display.println(temperature);
  display.setCursor(28, 50);
  display.println(huimidity);
  display.display();
}