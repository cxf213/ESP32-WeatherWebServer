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

#define WIFI_SSID "cxf"
#define WIFI_PASSWORD "12345678"
// Defining the WiFi channel speeds up the connection:
#define WIFI_CHANNEL 6


WebServer server(80);

#define SCREEN_ADDRESS 0x3C 
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int LED1 = LED_BUILTIN;
const int LED2 = 15;

bool led1State = false;
bool led2State = false;

void sendHtml() {
  String response = R"(
    <!DOCTYPE html><html>
      <head>
        <title>ESP32 Web Server Demo</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          html { font-family: sans-serif; text-align: center; }
          body { display: inline-flex; flex-direction: column; }
          h1 { margin-bottom: 1.2em; } 
          h2 { margin: 0; }
          div { display: grid; grid-template-columns: 1fr 1fr; grid-template-rows: auto auto; grid-auto-flow: column; grid-gap: 1em; }
          .btn { background-color: #5B5; border: none; color: #fff; padding: 0.5em 1em;
                 font-size: 2em; text-decoration: none }
          .btn.OFF { background-color: #333; }
        </style>
      </head>
            
      <body>
        <h1>ESP32 Web Server</h1>
        <div>
          <h2>LED 1</h2>
          <a href="/toggle/1" class="btn LED1_TEXT">LED1_TEXT</a>
          <h2>LED 2</h2>
          <a href="/toggle/2" class="btn LED2_TEXT">LED2_TEXT</a>
        </div>
      </body>
    </html>
  )";
  response.replace("LED1_TEXT", led1State ? "ON" : "OFF");
  response.replace("LED2_TEXT", led2State ? "ON" : "OFF");
  server.send(200, "text/html", response);
}

void displayText(String s){
  display.clearDisplay();
  display.setTextSize(1);         // set text size
  display.setTextColor(WHITE);
  display.setCursor(10,10);
  display.print(s);
  display.display();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  //Wire.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }
  Serial.println(F("SSD1306 allocation"));
  delay(200);
  display.clearDisplay(); // Clear the buffer
  display.display(); // Display the buffer
  display.setTextSize(1);         // set text size
  display.setTextColor(WHITE);
  display.setCursor(10,10);
  display.print("Hello.");
  display.display();


  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  Serial.print("Connecting to WiFi ");
  Serial.print(WIFI_SSID);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println(" Connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  server.on("/", sendHtml);

  server.on(UriBraces("/toggle/{}"), []() {
    String led = server.pathArg(0);
    Serial.print("Toggle LED #");
    Serial.println(led);

    switch (led.toInt()) {
      case 1:
        led1State = !led1State;
        digitalWrite(LED1, led1State);
        displayText("LED 1 TOGGLE");
        break;
      case 2:
        led2State = !led2State;
        digitalWrite(LED2, led2State);
        displayText("LED 2 TOGGLE");
        break;
    }

    sendHtml();
  });

  server.begin();
  Serial.println("HTTP server started (http://localhost:80)");
}


// #include <Arduino.h>
// void setup() {
//   // put your setup code here, to run once:
//   pinMode(LED_BUILTIN, OUTPUT);
//   Serial.begin(9600);
// }

// void loop() {
//   // put your main code here, to run repeatedly:
//   digitalWrite(LED_BUILTIN, HIGH);
//   Serial.println("Hello world!");
//   delay(100);
//   digitalWrite(LED_BUILTIN, LOW);
//   delay(100);
// }
int in = 0;
void loop() {
  // display.clearDisplay();
  // display.setTextSize(1);         // set text size
  // display.setTextColor(WHITE);
  // display.setCursor(10,10);
  // display.print(in);
  // display.display();
  server.handleClient();
  // in++;
  delay(100);
}