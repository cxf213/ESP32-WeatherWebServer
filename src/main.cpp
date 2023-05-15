/* ESP32 HTTP IoT Server
TODO Plan:
点击实体按钮改变网页按钮，并控制显示屏开关-Finished
点击网页按钮调整GPIO口-Finished
实现不同状态，并显示在显示屏上
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
#define DEVELOP

#define GPIO_BUTTON 4
#define GPIO_NOWIFI_LED 15
#define SCREEN_ADDRESS 0x3C
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define WIFI_connect_overTime 10000

#define PRE_PARAM_INPUT_1 "ssid"
#define PRE_PARAM_INPUT_2 "pass"
#define PRE_PARAM_INPUT_3 "ip"
#define PRE_PARAM_INPUT_4 "gateway"
#define ssidPath "/ssid.txt"
#define passPath "/pass.txt"
#define ipPath "/ip.txt"
#define gatewayPath "/gateway.txt"

// WebServer 配置
AsyncWebServer server(80);
String ssid;
String pass;
String ip;
String gateway;
IPAddress localIP;
IPAddress localGateway;
IPAddress subnet(255, 255, 0, 0);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
ClosedCube_HDC1080 hdc1080;
Adafruit_BMP085 bmp;

bool screenState = true;
double temperature;
double huimidity;
int pressure;
unsigned long previousMillis = 0;
bool haveConnected = false;
bool haveReconnect = false;
const char *PARAM_INPUT_1 = "output";
const char *PARAM_INPUT_2 = "state";
String Weatherdata = "NO DATA";

// Initialize WiFi
bool initWiFi()
{
  if (ssid == "" || ip == "")
  {
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  localIP.fromString(ip.c_str());
  localGateway.fromString(gateway.c_str());

  if (!WiFi.config(localIP, localGateway, subnet))
  {
    Serial.println("STA Failed to configure");
    return false;
  }
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("Connecting to WiFi...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while (WiFi.status() != WL_CONNECTED)
  {
    currentMillis = millis();
    if (currentMillis - previousMillis >= WIFI_connect_overTime)
    {
      Serial.println("Failed to connect.");
      return false;
    }
  }

  Serial.println(WiFi.localIP());
  return true;
}

String outputState(int output)
{
  if (digitalRead(output))
    return "checked";
  else
    return "";
}
String screenStateChanger()
{
  if (screenState)
    return "checked";
  else
    return "";
}

String processor(const String &var)
{
  if (var == "BUTTONPLACEHOLDER")
  {
    String buttons = "";
    buttons += "<h4>Output - Screen</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"screenState\" " + screenStateChanger() + "><span class=\"slider\"></span></label>";
    buttons += "<h4>Output - GPIO 2</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"2\" " + outputState(2) + "><span class=\"slider\"></span></label>";
    // buttons += "<h4>Output - GPIO 33</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"15\" " + outputState(15) + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  return String();
}

void putText(String s, int xpos = 10, int ypos = 10, int size = 1)
{
  display.setTextSize(size); // set text size
  display.setTextColor(WHITE);
  display.setCursor(xpos, ypos);
  display.print(s);
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(GPIO_NOWIFI_LED, OUTPUT);
  pinMode(GPIO_BUTTON, INPUT);

  initSPIFFS();
#if defined(DEVELOP)
  ssid = "cxf";
  pass = "12345678";
  ip = "192.168.137.10";
  gateway = "192.168.137.1";
#else
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);
  ip = readFile(SPIFFS, ipPath);
  gateway = readFile(SPIFFS, gatewayPath);
#endif

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

  if (initWiFi())
  {
    haveConnected = true;
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/index.html", "text/html", false, processor); });
    server.serveStatic("/", SPIFFS, "/");
    // 提供传感器数据
    server.on("/sensor", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plane", Weatherdata); 
              Serial.println(Weatherdata); });
    // 读取按钮消息
    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
              {
      String inputMessage1;
      String inputMessage2;
      // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
      if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
        inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
        inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
        if(inputMessage1 == "screenState"){
          screenState = inputMessage2=="1"?true:false;
        }else{
          digitalWrite(inputMessage1.toInt(), inputMessage2.toInt());
        }
      }
      else {
        inputMessage1 = "No message sent";
        inputMessage2 = "No message sent";
      }
      Serial.print("GPIO: ");
      Serial.print(inputMessage1);
      Serial.print(" - Set to: ");
      Serial.println(inputMessage2);
      request->send(200, "text/plain", "OK"); });

    // 提供屏幕按钮数据
    server.on("/stateLED", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", String(screenState ? 1 : 0).c_str()); });
    server.begin();
  }
  else
  {
    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("ESP-WIFI-MANAGER", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/wifimanager.html", "text/html"); });

    server.serveStatic("/", SPIFFS, "/");

    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
              {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PRE_PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PRE_PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(SPIFFS, passPath, pass.c_str());
          }
          // HTTP POST ip value
          if (p->name() == PRE_PARAM_INPUT_3) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            // Write file to save value
            writeFile(SPIFFS, ipPath, ip.c_str());
          }
          // HTTP POST gateway value
          if (p->name() == PRE_PARAM_INPUT_4) {
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
      ESP.restart(); });
    server.begin();
  }
  Serial.println(F("Server Start."));
}

int count = 0;
unsigned int overtimecount = 0;
int buttonState;
int lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
void loop()
{
  delay(20);
  count++;
  if (count >= 5)
  {
    count = 0;
    temperature = hdc1080.readTemperature();
    huimidity = hdc1080.readHumidity();
    pressure = bmp.readPressure();
    Weatherdata = "{\"temperature\" : " + String(temperature) + ", \"huimidity\" : " + String(huimidity) + ", \"pressure\" : " + String(pressure) + "}";
    if (screenState)
    {
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
    }
    else
    {
      display.clearDisplay();
    }
    display.display();

    if (haveConnected)
    {
      if (haveReconnect)
        overtimecount++;
      else
        overtimecount = 0;
      if (overtimecount > 50)
      {
        haveReconnect = false;
        overtimecount = 0;
      };
      if ((WiFi.status() != WL_CONNECTED) && !haveReconnect)
      {
        digitalWrite(GPIO_NOWIFI_LED, HIGH);
        Serial.print(millis());
        Serial.println("Reconnecting to WIFI network");
        WiFi.disconnect();
        WiFi.reconnect();
        haveReconnect = true;
      }
      else if ((WiFi.status() == WL_CONNECTED) && haveReconnect)
      {
        digitalWrite(GPIO_NOWIFI_LED, LOW);
        haveReconnect = false;
        Serial.print(millis());
        Serial.println("Reconnected");
      }
    }
  }

#pragma region ButtonProcess
  int reading = digitalRead(GPIO_BUTTON);
  if (reading != lastButtonState)
  {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    if (reading != buttonState)
    {
      buttonState = reading;
      if (buttonState == HIGH)
      {
        screenState = !screenState;
      }
    }
  }
  lastButtonState = reading;
#pragma endregion
}