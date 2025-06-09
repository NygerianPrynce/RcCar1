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
#define PWMA 15
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
#define BUZZER 6
//Servo steering pin & Declarations
#define SERVO 4
Servo myServo;
int currentSteer = 1500;
bool connected = false;
//UART stuff
#define TX_PIN 20
#define RX_PIN 19
//Camera stuff - temp doe
String camIP = "";

//blicky hit the spiffy uh -- DONE
void initSPIFFS() {
  if (!SPIFFS.begin()) {
    Serial.println("Cannot mount SPIFFS volume...");
  }
  Serial.println("SPIFFS SUCCESS!!!");
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
        case WS_EVT_ERROR:
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

void bere(){
  digitalWrite(RGB_BUILTIN, HIGH);
  delay(1500);
  neopixelWrite(RGB_BUILTIN,5,0,0); //Red -- shows wifi stuff happening
  initSPIFFS();
  initWiFi();
  initWebSocket();
  initWebServer();
  neopixelWrite(RGB_BUILTIN, 5, 5, 0); //Yellow -- shows sensors booting and stuff
  initUltrasonic();
  initServo();
  notifyClients();
}

void setup() {
    delay(1000);
  Serial.begin(115200);
  while (!Serial);
  Serial.println("✅ Hello from ESP32-S3");
  Serial.println("✅ Booted up!");
  neopixelWrite(RGB_BUILTIN, 5, 5, 0); //testing bs
  delay(100);
  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  Serial.println("📥 S3 waiting for messages from CAM...");

  //bere();
}




unsigned long lastUpdate = 0;
const unsigned long updateInterval = 100;  
bool hasMoved = false;
static int lastSteer = 1500;
int lastSpeed = 0;
unsigned long lastUltrasonic = 0;
const unsigned long ultrasonicInterval = 200;  

void loop() {
  if (Serial2.available()) {
    String msg = Serial2.readStringUntil('\n');
    msg.trim();
    Serial.println("Received from CAM: " + msg);
  }

  /*if(connected){
    neopixelWrite(RGB_BUILTIN,0,5,0); // Green client exists
  } else{
    	neopixelWrite(RGB_BUILTIN, 2, 0, 5); //No client at page homeboy
  }
  unsigned long now = millis();

  if (now - lastUpdate >= updateInterval) {
    lastUpdate = now;
    int deadZone = 80; 

    // Detect direction change
    bool directionChanged = (lastSpeed > 0 && currentSpeed < 0) || (lastSpeed < 0 && currentSpeed > 0);

    if (directionChanged) {
      brake(motor1, motor2);  // brief brake before reversing
      delay(50);              // optional: short pause to protect motor and stop servo from tweaking
    }

    if (abs(currentSpeed) >= deadZone && distance >= 5) {
      if (currentSpeed > 0) {
        forward(motor1, motor2, currentSpeed);
      } else {
        back(motor1, motor2, currentSpeed);
      }
      hasMoved = true;
    } else if (distance <= 12 && hasMoved) {
      brake(motor1, motor2);
      myServo.detach(); // detaching
      tone(BUZZER, 750);
      delay(1000);
      noTone(BUZZER);
      hasMoved = false;
      delay(75);
      myServo.attach(SERVO, 500, 2500); // reattaching
      myServo.writeMicroseconds(currentSteer);
    } else {
      brake(motor1, motor2);
      hasMoved = false;
    }

    lastSpeed = currentSpeed; 
  }
  if (currentSteer != lastSteer) {
      myServo.writeMicroseconds(currentSteer);
      lastSteer = currentSteer;
  }
  if (now - lastUltrasonic >= ultrasonicInterval) {
    lastUltrasonic = now;
    runUltrasonic();  
  }

  static unsigned long lastNotify = 0;
  const unsigned long notifyInterval = 200;

  if (now - lastNotify >= notifyInterval) {
    lastNotify = now;
    notifyClients();  
  }*/
}
