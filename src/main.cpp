//Necessary libraries and dat
#include <Arduino.h>
#include <SparkFun_TB6612.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <esp_sleep.h>
#include <Preferences.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <MadgwickAHRS.h>
#include <ESPmDNS.h>


Adafruit_MPU6050 mpu;
Madgwick filter;

//WIFI info -- gotta  hide during git commit!
const char *WIFI_SSID = "Ebialayo";
const char *WIFI_PASS = "alhamdulillahi2024$";

//HTTP & Webserver STUFF
#define HTTP_PORT 80
AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/ws");
// Motor pins & Declarations
#define AIN1 18
#define AIN2 8
#define PWMA 17
#define BIN1 46
#define BIN2 9
#define PWMB 16
#define STBY 11
const int offsetA = 1;
const int offsetB = 1;
Motor motor1 = Motor(AIN1, AIN2, PWMA, offsetA, STBY);
Motor motor2 = Motor(BIN1, BIN2, PWMB, offsetB, STBY);
int currentSpeed = 0;
//Ultrasonic Detector pins & Declarations
#define ECHO_PIN 13
#define TRIG_PIN 14
int duration;
float distance; 
//Horn  pin
#define BUZZER 7
//Servo steering pin & Declarations
#define SERVO 4
Servo myServo;
int currentSteer = 1500;
bool connected = false;
//UART stuff
#define TX_PIN 43
#define RX_PIN 44
//Camera stuff 
String camIP = "";
bool sentIP = false;
//AccGy stuff
float roll = 0;   // left/right tilt (degrees)
float pitch = 0; 
float ax = 0;
float velocity = 0;

String b1Percent;
String b2Percent;


TwoWire I2CBus1 = TwoWire(0);
TwoWire I2CBus2 = TwoWire(1);

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2CBus1, OLED_RESET);


const int BATTERY1_PIN = 5;  // GPIO4
const int BATTERY2_PIN = 6;  // GPIO5
const int ADC_MAX = 4095;
const float ESP_VOLTAGE = 3.3;
const float VOLTAGE_DIVIDER_RATIO = 4.0;  // 30kΩ + 10kΩ = 4:1 ratio

// 2S LiPo thresholds
const float BATTERY_MIN = 6.0;   // Empty (3.0V per cell)
const float BATTERY_MAX = 8.4;   // Full (4.2V per cell)

float pitchOffset = 0;
float rollOffset = 0;

float readBattery1Voltage() {
    int rawValue = analogRead(BATTERY1_PIN);
    return (rawValue / (float)ADC_MAX) * ESP_VOLTAGE * VOLTAGE_DIVIDER_RATIO;
}

float readBattery2Voltage() {
    int rawValue = analogRead(BATTERY2_PIN);
    return (rawValue / (float)ADC_MAX) * ESP_VOLTAGE * VOLTAGE_DIVIDER_RATIO;
}

int getBattery1Percent() {
    float voltage = readBattery1Voltage();
    if (voltage >= BATTERY_MAX) return 100;
    if (voltage <= BATTERY_MIN) return 0;
    return constrain(((voltage - BATTERY_MIN) / (BATTERY_MAX - BATTERY_MIN)) * 100, 0, 100);
}

int getBattery2Percent() {
    float voltage = readBattery2Voltage();
    if (voltage >= BATTERY_MAX) return 100;
    if (voltage <= BATTERY_MIN) return 0;
    return constrain(((voltage - BATTERY_MIN) / (BATTERY_MAX - BATTERY_MIN)) * 100, 0, 100);
}


void writeMessage(String msg){
  static String lastMsg = "";
  if (msg == lastMsg) return;
  lastMsg = msg;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0); 
  display.println(msg);
  display.display(); 
}

//blicky hit the spiffy uh -- DONE
void initSPIFFS() {
  if (!SPIFFS.begin()) {
    Serial.println("Cannot mount SPIFFS volume...");
  }
  Serial.println("SPIFFS SUCCESS!!!");
  writeMessage("SPIFFS SUCCESS");
}
//wifi stuff -- remove serial monitor stuff once done testing && see if ESP doesnt have to restart
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  Serial.printf("Trying to connect [%s] ", WiFi.macAddress().c_str());
  unsigned long startAttemptTime = millis();
  const unsigned long wifiTimeout = 30000;  // 15 seconds
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < wifiTimeout) {
    Serial.print(".");
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf(" %s\n", WiFi.localIP().toString().c_str());
    Serial.println("WIFI SUCCESS!!!");
    Serial.println("http://" + WiFi.localIP().toString());
    Serial.println("\n✅ WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("Subnet: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());
    if (!MDNS.begin("rccar")) {  // this sets your name: mycar.local
    Serial.println("Error starting mDNS");
    while (1) {
      delay(1000);
    }
    }
    Serial.println("mDNS responder started");
  } else {
    Serial.println("\nWiFi FAILED. Restarting ESP...");
    Serial.println();
    Serial.print("WiFi status code: ");
    Serial.println(WiFi.status());
    delay(2000);
    ESP.restart();
  }
}

void onRootRequest(AsyncWebServerRequest *request) {
  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    request->send(500, "text/plain", "Failed to open index.html");
    return;
  }
  String html = file.readString();
  file.close();
  html.replace("CAM_IP_PLACEHOLDER", camIP);  // Cam stuff -- disposable
  request->send(200, "text/html", html);
}

void initWebServer() {
    server.on("/", onRootRequest);
    server.serveStatic("/", SPIFFS, "/");
    server.begin();
}

void notifyClients() {
  if (ws.count() == 0) return;
  static int lastSpeed = -999;
  static int lastSteer = -999;
  static float lastDistance = -999;
  if (currentSpeed != lastSpeed || currentSteer != lastSteer || abs(distance - lastDistance) > 1.0) {
    DynamicJsonDocument doc(128);
    doc["speed"] = currentSpeed;
    doc["distance"] = distance;
    doc["steering"] = currentSteer;
    doc["ax"] = ax;
    doc["velocity"] = velocity * 2.237;
    doc["pitch"] = pitch;
    doc["roll"]  = roll;
    doc["b1Percent"] = b1Percent;
    doc["b2Percent"] = b2Percent;

    String json;
    serializeJson(doc, json);
    ws.textAll(json);

    lastSpeed = currentSpeed;
    lastSteer = currentSteer;
    lastDistance = distance;
  }
}
//DO NOT TOUCH BRUH
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    char *msg = (char *)malloc(len + 1);
    if (!msg) {
      Serial.println("Memory allocation failed!");
      return;
    }
    memcpy(msg, data, len);
    msg[len] = '\0';  // safely null-terminate
    Serial.print("Received WS message: ");
    Serial.println(msg);
    DynamicJsonDocument doc(128);
    DeserializationError error = deserializeJson(doc, msg);
    free(msg);  // free memory after parsing
    if (!error) {
      if (doc.containsKey("speed")) {
        currentSpeed = constrain(doc["speed"].as<int>(), -255, 255);
      }

      if (doc.containsKey("steering")) {
        currentSteer = constrain(doc["steering"].as<int>(), 500, 2500);
      }
    } else {
      Serial.print("JSON parse error: ");
      Serial.println(error.c_str());
    }
  }
}


void onEvent(AsyncWebSocket       *server,
             AsyncWebSocketClient *client,
             AwsEventType          type,
             void                 *arg,
             uint8_t              *data,
             size_t                len) {

    switch (type) {
        case WS_EVT_CONNECT:
            if (connected) {
                // Disconnect all existing clients to make room for new one
                Serial.println("New client connecting, disconnecting existing clients");
                server->closeAll(1000, "New client connecting");
            }
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            connected = true;
            break;
            
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            connected = false;
            break;
            
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;

            
        case WS_EVT_PONG:
          break;
        case WS_EVT_ERROR:
          Serial.println("WebSocket error occurred");
          connected = false;
          break;
    }
}

void initWebSocket() {
    ws.onEvent(onEvent);
    server.addHandler(&ws);
}

void initUltrasonic(){
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  pinMode(ECHO_PIN, INPUT);
  delay(6000);
}

void runUltrasonic(){
 digitalWrite(TRIG_PIN, HIGH);
 delayMicroseconds(10); 
 digitalWrite(TRIG_PIN, LOW);
 duration = pulseIn(ECHO_PIN, HIGH);
 if(duration<=38000){
  distance = duration/58;
 }
}

void initServo(){
  myServo.setPeriodHertz(50); 
  myServo.attach(SERVO, 500, 2500); 
  myServo.writeMicroseconds(currentSteer);
}

void initCamera(){
  Serial.println("✅ ENABLING CAM CONNECTION");
  Serial.println("📡 Sending handshake to CAM...");
  Serial2.println("REQ_IP");
  unsigned long startTime = millis();
  bool sentIP = false;

  while (millis() - startTime < 15000 && !sentIP) {  // 5-second timeout
    if (Serial2.available()) {
      String msg = Serial2.readStringUntil('\n');
      msg.trim();
      if (msg.length() > 0 && msg[0] == '\0') {
        msg.remove(0, 1);
      }

      if (msg.startsWith("CAM_IP:")) {
        camIP = msg.substring(7);
        if(camIP == "0.0.0.0"){
          break;
        } else{
          Serial.println("✅ CAM IP received: " + camIP);
          String camURL = "http://" + camIP + "/";
          Serial.println("🌐 URL: " + camURL);
          sentIP = true;
          writeMessage("CAM IS RUNNING: " + camURL);
        }
        
      }
    }
  }

  if (!sentIP) {
    Serial.println("⚠️ Failed to receive CAM IP within timeout.");
    writeMessage("CAM SETUP FAILURE");
  }
  Serial2.end();
  delay(1000);

}

void initDisplay(){
  I2CBus1.begin(1, 2);     // SDA = GPIO 1, SCL = GPIO 2
  I2CBus1.setClock(100000);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();                      // Clear the screen buffer
  display.setTextSize(2);                      // Text size (1 = normal, 2 = 2x, etc.)
  display.setTextColor(SSD1306_WHITE);         // Text color
  display.setCursor(0, 0); 
  display.println("SCREEN IS ACTIVE ");
  display.display();  
}

void initAccgy(){
  I2CBus2.begin(38, 39);
  I2CBus2.setClock(100000);

  Serial.println("Adafruit MPU6050 test!");
  // Try to initialize!
  if (!mpu.begin(0x68, &I2CBus2)) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  filter.begin(100);  // Assume 100 Hz update rate
  Serial.println("✅ MPU6050 + Madgwick initialized");

}

void calibrateOrientation(int samples = 100) {
  float totalPitch = 0;
  float totalRoll = 0;

  Serial.println("📐 Calibrating pitch and roll...");
  for (int i = 0; i < samples; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    filter.updateIMU(
      g.gyro.x, g.gyro.y, g.gyro.z,
      a.acceleration.x, a.acceleration.y, a.acceleration.z
    );

    totalPitch += filter.getPitch();
    totalRoll  += filter.getRoll();
    delay(10);  // Let filter settle
  }

  pitchOffset = totalPitch / samples;
  rollOffset  = totalRoll  / samples;

  Serial.printf("✅ Pitch offset: %.2f\n", pitchOffset);
  Serial.printf("✅ Roll offset: %.2f\n", rollOffset);
}



void initBattery(){
  analogReadResolution(12);
  b1Percent = String(getBattery1Percent());
  b2Percent = String(getBattery1Percent());
}

void bere(){
  digitalWrite(RGB_BUILTIN, HIGH);
  delay(1500);
  neopixelWrite(RGB_BUILTIN,5,0,0); //Red -- shows wifi stuff happening
  initDisplay();
  initCamera(); //done before webserver so it can be loaded onto it bredeski
  initSPIFFS();
  initWiFi();
  writeMessage("WIFI SUCCESS");
  initWebSocket();
  initWebServer();
  writeMessage("SOCKET & SERVER  SUCCESS");
  neopixelWrite(RGB_BUILTIN, 5, 5, 0); //Yellow -- shows sensors booting and stuff
  initUltrasonic();
  writeMessage("ULTRASONIC SUCCESS");
  initServo();
  writeMessage("SERVO SUCCESS");
  initAccgy();
  calibrateOrientation();
  writeMessage("AccGy SUCCESS");
  initBattery();
  writeMessage("BATTERY STATUS SUCCESS");
  notifyClients();
  writeMessage("CONNECT @ http://" + WiFi.localIP().toString() + " :)");
}



void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  bere();
}


unsigned long lastUpdate = 0;
const unsigned long updateInterval = 200;  
bool hasMoved = false;
static int lastSteer = 1500;
int lastSpeed = 0;
unsigned long lastVelocityUpdate = 0;

void loop() {

  unsigned long now = millis();  // ✅ Declare `now` once

  if (connected) {
    neopixelWrite(RGB_BUILTIN, 0, 5, 0);
    writeMessage("CONNECT @ http://rccar.local :) \n\n CLIENT HAS CONNECTED");
  } else {
    neopixelWrite(RGB_BUILTIN, 2, 0, 5);
    writeMessage("CONNECT @ http://rccar.local:( \n\n NO ACTIVE CLIENTS");
  }

  if (now - lastUpdate >= updateInterval) {
      b1Percent = String(getBattery1Percent());
      b2Percent = String(getBattery2Percent());

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    filter.updateIMU(
      g.gyro.x, g.gyro.y, g.gyro.z,
      a.acceleration.x, a.acceleration.y, a.acceleration.z
    );
    roll  = filter.getRoll() - rollOffset;     // left/right tilt (degrees)
    pitch = filter.getPitch() - pitchOffset;

    lastUpdate = now;
    int deadZone = 80;

    bool directionChanged = (lastSpeed > 0 && currentSpeed < 0) || (lastSpeed < 0 && currentSpeed > 0);
    if (directionChanged) {
      brake(motor1, motor2);
      delay(50);
    }

    if (abs(currentSpeed) >= deadZone && distance >= 5) {
      if (currentSpeed > 0) {
        forward(motor1, motor2, currentSpeed);
      } else {
        back(motor1, motor2, currentSpeed);
      }
      hasMoved = true;

      // ✅ Only compute `dt` now that you know it's moving
      float dt = (now - lastVelocityUpdate) / 1000.0;
      lastVelocityUpdate = now;

      ax = a.acceleration.x - 0.64;
      velocity += ax * dt;

      if (abs(ax) < 0.1) velocity *= 0.95;


    } else if (distance <= 12 && hasMoved) {
      brake(motor1, motor2);
      myServo.detach();
      tone(BUZZER, 750);
      delay(1000);
      noTone(BUZZER);
      hasMoved = false;
      delay(75);
      myServo.attach(SERVO, 500, 2500);
      myServo.writeMicroseconds(currentSteer);

      velocity = 0;  // Reset speed
    } else {
      brake(motor1, motor2);
      hasMoved = false;
      velocity = 0;
      ax = 0;
    }

    lastSpeed = currentSpeed;

    if (currentSteer != lastSteer) {
      myServo.writeMicroseconds(currentSteer);
      lastSteer = currentSteer;
    }
    runUltrasonic();
    notifyClients();
  }
    

}
