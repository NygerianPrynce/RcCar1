F1 RC Telemetry Dashboard
A sophisticated remote-controlled car with real-time telemetry monitoring, featuring a Formula 1-inspired web dashboard with live camera feed, sensor data visualization, and holographic-style controls.
Features
Remote Control

Web-based dashboard with F1-inspired design
Keyboard controls (WASD) for throttle and steering
Real-time responsiveness via WebSocket communication
Safety collision avoidance system with ultrasonic detection

Live Telemetry

Speed & Velocity monitoring with MPH display
Proximity detection using ultrasonic sensor (collision avoidance)
G-Force measurement via MPU6050 accelerometer/gyroscope
Pitch & Roll attitude display with IMU fusion
Battery monitoring for dual power systems
Connection status indicators

Live Camera Feed

Real-time video streaming from ESP32-CAM
Cockpit view integrated into dashboard
Auto-discovery camera IP via UART communication

Dashboard Design

Holographic-style UI with animated elements
Particle effects and glowing controls
Real-time data visualization
Responsive design for mobile and desktop
F1-inspired color scheme and typography

Repository Structure
This repository contains the main RC car controller code and web dashboard. The camera module firmware is maintained separately:

Main Controller: This repository (ESP32-S3 firmware + web dashboard)
Camera Module: RCCarCam Repository (ESP32-CAM firmware)

Hardware Requirements
Main Controller (ESP32-S3)

ESP32-S3 Development Board
MPU6050 IMU (Accelerometer/Gyroscope)
HC-SR04 Ultrasonic Sensor
SSD1306 OLED Display (128x32)
TB6612FNG Motor Driver
2x DC Motors
Servo Motor (steering)
Buzzer (collision warning)
2x Battery voltage monitors

Camera Module

ESP32-CAM with OV2640 camera
Separate power supply
Camera firmware available at: RCCarCam Repository

Power System

2S LiPo batteries (dual battery setup)
Voltage divider circuits for battery monitoring

Pin Configuration
ESP32-S3 Main Controller
cpp// Motor Driver (TB6612FNG)
#define AIN1 18     // Motor A direction
#define AIN2 8      // Motor A direction  
#define PWMA 17     // Motor A speed
#define BIN1 46     // Motor B direction
#define BIN2 9      // Motor B direction
#define PWMB 16     // Motor B speed
#define STBY 11     // Standby pin

// Sensors
#define ECHO_PIN 13 // Ultrasonic echo
#define TRIG_PIN 14 // Ultrasonic trigger
#define SERVO 4     // Steering servo

// Communication & Audio
#define TX_PIN 43   // UART to camera
#define RX_PIN 44   // UART from camera
#define BUZZER 7    // Collision buzzer

// Battery Monitoring
#define BATTERY1_PIN 5  // Battery 1 voltage
#define BATTERY2_PIN 6  // Battery 2 voltage

// I2C (MPU6050 & OLED)
// SDA1: 1, SCL1: 2 (OLED)
// SDA2: 38, SCL2: 39 (MPU6050)
ESP32-CAM

Standard AI-Thinker ESP32-CAM pinout
UART communication on pins 1 (TX) and 3 (RX)

Installation & Setup
1. Hardware Assembly

Connect all components according to the pin configuration
Set up dual I2C buses for OLED and MPU6050
Install voltage dividers for battery monitoring
Mount camera module with clear view

2. Software Installation
For ESP32-S3 (Main Controller):
bash# Install required libraries in Arduino IDE:
# - SparkFun TB6612FNG Motor Driver
# - ESP32Servo
# - ArduinoJson
# - Adafruit MPU6050
# - Adafruit SSD1306
# - ESPAsyncWebServer
# - Madgwick filter library
For ESP32-CAM:
bash# Camera module firmware is maintained in a separate repository
# Clone and flash from: https://github.com/NygerianPrynce/RCCarCam
git clone https://github.com/NygerianPrynce/RCCarCam.git
# Follow the setup instructions in that repository
3. Configuration

Update WiFi credentials in both ESP32 files:
cppconst char *WIFI_SSID = "YourWiFiName";
const char *WIFI_PASS = "YourWiFiPassword";

Upload SPIFFS data (HTML, CSS, JS files) to ESP32-S3
Flash firmware to both ESP32 boards:

Main controller: Use the code in this repository
Camera module: Clone and flash from RCCarCam Repository



4. File Upload to SPIFFS
Upload these files to the ESP32-S3 SPIFFS:

index.html - Main dashboard
style.css - F1-inspired styling
socket.js - WebSocket communication

Usage
Starting the System

Power on both ESP32 modules
Wait for WiFi connection (check OLED display)
Navigate to http://rccar.local or the displayed IP address
Dashboard will show connection status and live telemetry

Controls

W - Forward throttle
S - Reverse throttle
A - Steer left
D - Steer right
Mouse/Touch - Use dashboard sliders for precise control

Safety Features

Collision avoidance: Automatically stops forward movement when obstacle detected within 12cm
Safe distance resumption: Allows forward movement again when 20cm+ clearance
Audio alerts: Buzzer sounds during collision detection
Battery monitoring: Real-time voltage and percentage display

Technical Details
Communication Protocol

WebSocket for real-time bidirectional communication
JSON message format for telemetry data
UART communication between ESP32-S3 and ESP32-CAM
mDNS for easy network discovery

Sensor Fusion

Madgwick filter for IMU data fusion
Kalman-style smoothing for ultrasonic readings
Calibrated accelerometer for accurate velocity estimation

Performance Optimizations

Non-blocking ultrasonic measurements
Efficient servo control with minimal PWM conflicts
Optimized WebSocket updates (15ms intervals)
Smart collision detection with hysteresis

Calibration
IMU Calibration
The system automatically calibrates pitch/roll offsets at startup. Place the car on a level surface during boot.
Battery Voltage
Adjust these constants for your battery setup:
cppconst float BATTERY_MIN = 6.0;  // 2S LiPo minimum
const float BATTERY_MAX = 8.4;  // 2S LiPo maximum
Troubleshooting
Common Issues

Camera not connecting: Check UART wiring and baud rates
Servo conflicts: Ensure proper PWM channel separation
WiFi connection fails: Verify credentials and signal strength
Collision sensor false triggers: Adjust COLLISION_DISTANCE threshold

Debug Information

OLED display shows current system status
Serial monitor provides detailed logging
Built-in LED indicates connection status:

Green: Client connected
Blue/Purple: No clients
Red: Connection error



Future Enhancements

 GPS tracking and waypoint navigation
 Telemetry data logging and replay
 Multiple camera angles
 Mobile app development
 Machine learning for autonomous driving
 Race timing and lap counter

Contributing
Feel free to submit issues, fork the repository, and create pull requests for any improvements.
License
This project is open source. Please credit if you use this code in your own projects.
Acknowledgments

SparkFun for the TB6612FNG library
Adafruit for sensor libraries
Madgwick for the IMU fusion algorithm
ESP32 community for WebSocket examples


Built with love for the RC and maker community
