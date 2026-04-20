#include <AsyncEventSource.h>
#include <AsyncJson.h>
#include <AsyncWebSocket.h>
#include <AsyncWebSynchronization.h>
#include <ESPAsyncWebServer.h>
#include <StringArray.h>
#include <WebAuthentication.h>
#include <WebHandlerImpl.h>
#include <WebResponseImpl.h>

#include <AsyncEventSource.h>
#include <AsyncJson.h>
#include <AsyncWebSocket.h>
#include <AsyncWebSynchronization.h>
#include <ESPAsyncWebServer.h>
#include <StringArray.h>
#include <WebAuthentication.h>
#include <WebHandlerImpl.h>
#include <WebResponseImpl.h>

/*
 * ═══════════════════════════════════════════════════════════════════
 *  SyringePump_ESP32.ino
 *  ─────────────────────────────────────────────────────────────────
 *  Complete ESP32 firmware for a DIY stepper-motor syringe pump.
 *
 *  Features:
 *    • Wi-Fi SoftAP ("SyringePump_Control")
 *    • ESPAsyncWebServer hosting the dashboard from PROGMEM
 *    • Non-blocking AccelStepper motor control
 *    • Linear potentiometer volume sensing (ADC)
 *    • Occlusion & syringe-empty limit switches (active LOW)
 *    • MPU6050 accelerometer tremor detection (I2C)
 *    • JSON REST API: POST /set_parameters, GET /status,
 *      POST /emergency_stop
 *
 *  Required Libraries (install via Arduino Library Manager):
 *    1. ESPAsyncWebServer  (by me-no-dev)
 *    2. AsyncTCP           (by me-no-dev)
 *    3. AccelStepper       (by Mike McCauley)
 *    4. Wire               (built-in)
 *    5. ArduinoJson        (by Benoit Blanchon, v6+)
 *
 *  Hardware:
 *    Motor Driver : A4988 — STEP=14, DIR=27
 *    Potentiometer: GPIO 34  (ADC1, analog volume sensing)
 *    Occlusion SW : GPIO 32  (active LOW, internal pull-up)
 *    Empty SW     : GPIO 33  (active LOW, internal pull-up)
 *    MPU6050      : SDA=21, SCL=22 (default I2C)
 *
 *  Author : Auto-generated for Ziad's Medical Equipment project
 *  Date   : 2026-04-18
 * ═══════════════════════════════════════════════════════════════════
 */

// ─────────────────────────────────────────────
//  INCLUDES
// ─────────────────────────────────────────────
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AccelStepper.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include "webpage.h"                       // PROGMEM HTML dashboard

// ─────────────────────────────────────────────
//  PIN DEFINITIONS
// ─────────────────────────────────────────────
#define STEP_PIN       14                  // A4988 STEP input
#define DIR_PIN        27                  // A4988 DIR  input
#define POT_PIN        34                  // Linear potentiometer (volume)
#define OCCLUSION_PIN  32                  // Limit switch – occlusion (NC, active LOW)
#define EMPTY_PIN      33                  // Limit switch – syringe empty (NC, active LOW)

// ─────────────────────────────────────────────
//  CALIBRATION CONSTANTS  *** ADJUST THESE ***
// ─────────────────────────────────────────────

/*
 *  POT_MIN_RAW / POT_MAX_RAW:
 *    Record the analogRead() value when the syringe plunger is
 *    fully retracted (0 mL delivered) and fully depressed (max mL).
 *    Use Serial.println(analogRead(POT_PIN)) to find these.
 */
#define POT_MIN_RAW     0                  // ADC value at 0 mL
#define POT_MAX_RAW     4095               // ADC value at full volume

/*
 *  SYRINGE_MAX_ML:
 *    The total volume (mL) of your syringe barrel.
 *    E.g. 10.0 for a 10 mL syringe, 60.0 for a 60 mL syringe.
 */
#define SYRINGE_MAX_ML  10.0

/*
 *  STEPS_PER_ML:
 *    Number of stepper motor steps required to dispense 1 mL.
 *    Depends on: motor step angle, micro-stepping setting on A4988,
 *    lead-screw pitch, and syringe barrel diameter.
 *    Example: 200 steps/rev * 16 microsteps / 0.8mm pitch / barrel area
 *    *** You MUST calibrate this with your hardware. ***
 */
#define STEPS_PER_ML    200.0

// ─────────────────────────────────────────────
//  MPU6050 CONSTANTS
// ─────────────────────────────────────────────
#define MPU6050_ADDR        0x68           // Default I2C address
#define MPU6050_PWR_MGMT_1  0x6B           // Power management register
#define MPU6050_ACCEL_XOUT  0x3B           // First accel data register

/*
 *  TREMOR_THRESHOLD_G:
 *    Acceleration magnitude (in g) above which we flag a tremor.
 *    Normal gravity ≈ 1.0 g, so a spike of 2.5+ g is significant.
 *    Tune this based on your mounting and patient scenario.
 */
#define TREMOR_THRESHOLD_G  2.5

// ─────────────────────────────────────────────
//  WI-FI ACCESS POINT SETTINGS
// ─────────────────────────────────────────────
const char* AP_SSID     = "SyringePump_Control";
const char* AP_PASSWORD = "";              // Open network (no password)

// ─────────────────────────────────────────────
//  GLOBAL OBJECTS
// ─────────────────────────────────────────────

// Stepper motor (driver interface: step + direction pins)
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// Async web server on port 80
AsyncWebServer server(80);

// ─────────────────────────────────────────────
//  SYSTEM STATE (volatile where ISR-adjacent)
// ─────────────────────────────────────────────
volatile bool motorRunning    = false;     // Is the motor actively stepping?
volatile bool alarmOcclusion  = false;     // Occlusion detected?
volatile bool alarmEmpty      = false;     // Syringe empty detected?
volatile bool alarmTremor     = false;     // Tremor spike detected?

float targetVolume_mL  = 0.0;             // Desired infusion volume (mL)
float flowRate_mLmin   = 0.0;             // Desired flow rate (mL/min)
float deliveredVol_mL  = 0.0;             // Current delivered volume (mL)

bool  mpuAvailable     = false;           // Did MPU6050 init succeed?

// Timing for non-blocking sensor reads
unsigned long lastSensorCheck_ms = 0;
const unsigned long SENSOR_INTERVAL_MS = 50;  // Check sensors every 50 ms

// ADC smoothing
#define ADC_SAMPLES 8
int adcBuffer[ADC_SAMPLES];
int adcIndex = 0;

// ─────────────────────────────────────────────
//  FORWARD DECLARATIONS
// ─────────────────────────────────────────────
void   setupWiFiAP();
void   setupWebServer();
void   setupMPU6050();
void   readSensors();
float  readPotVolume();
float  readMPU6050Magnitude();
void   haltMotor(const char* reason);
void   startMotor();

// ═════════════════════════════════════════════
//  SETUP
// ═════════════════════════════════════════════
void setup() {
  // ── Serial for debugging ──
  Serial.begin(115200);
  delay(200);
  Serial.println("\n══════════════════════════════════════");
  Serial.println("  Syringe Pump Controller — Booting");
  Serial.println("══════════════════════════════════════");

  // ── Pin modes ──
  pinMode(OCCLUSION_PIN, INPUT_PULLUP);    // Limit switch, NC to GND
  pinMode(EMPTY_PIN,     INPUT_PULLUP);    // Limit switch, NC to GND
  // POT_PIN is ADC — no pinMode needed for analogRead

  // ── Stepper defaults ──
  stepper.setMaxSpeed(2000);               // Steps/sec upper limit
  stepper.setAcceleration(500);            // Steps/sec² (smooth ramp)
  stepper.setSpeed(0);                     // Start stopped

  // ── Initialize ADC buffer ──
  for (int i = 0; i < ADC_SAMPLES; i++) {
    adcBuffer[i] = analogRead(POT_PIN);
  }

  // ── I2C & MPU6050 ──
  Wire.begin();                            // SDA=21, SCL=22
  setupMPU6050();

  // ── Wi-Fi Access Point ──
  setupWiFiAP();

  // ── Web Server Routes ──
  setupWebServer();

  Serial.println("[OK] System ready. Connect to Wi-Fi SSID: " + String(AP_SSID));
  Serial.println("     Dashboard: http://192.168.4.1");
  Serial.println("══════════════════════════════════════\n");
}

// ═════════════════════════════════════════════
//  LOOP — Must remain non-blocking!
// ═════════════════════════════════════════════
void loop() {
  // ── 1. Run stepper (non-blocking single step) ──
  if (motorRunning) {
    stepper.runSpeed();
  }

  // ── 2. Periodic sensor checks ──
  unsigned long now = millis();
  if (now - lastSensorCheck_ms >= SENSOR_INTERVAL_MS) {
    lastSensorCheck_ms = now;
    readSensors();
  }
}

// ═════════════════════════════════════════════
//  WI-FI ACCESS POINT
// ═════════════════════════════════════════════
void setupWiFiAP() {
  Serial.print("[WiFi] Starting SoftAP... ");
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  IPAddress ip = WiFi.softAPIP();
  Serial.println("OK — IP: " + ip.toString());
}

// ═════════════════════════════════════════════
//  WEB SERVER ROUTES
// ═════════════════════════════════════════════
void setupWebServer() {

  // ── Serve the dashboard HTML ──
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", INDEX_HTML);
  });

  // ── POST /set_parameters ──
  // Expects JSON: { "target_vol": 5.0, "flow_rate": 0.5 }
  // flow_rate is in mL/min
  server.on("/set_parameters", HTTP_POST,
    // onRequest handler (called after body is received)
    [](AsyncWebServerRequest *request) {},
    // onUpload — not used
    NULL,
    // onBody — parse JSON from request body
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {

      // Accumulate body (for chunked transfers)
      if (index == 0) {
        request->_tempObject = malloc(total + 1);
      }
      memcpy((uint8_t*)request->_tempObject + index, data, len);

      if (index + len == total) {
        // Null-terminate the body string
        ((char*)request->_tempObject)[total] = '\0';

        // Parse JSON
        StaticJsonDocument<256> doc;
        DeserializationError err = deserializeJson(doc, (char*)request->_tempObject);
        free(request->_tempObject);
        request->_tempObject = NULL;

        if (err) {
          Serial.println("[API] /set_parameters — JSON parse error");
          request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
          return;
        }

        // Extract parameters
        targetVolume_mL = doc["target_vol"] | 0.0f;
        flowRate_mLmin  = doc["flow_rate"]  | 0.0f;

        Serial.printf("[API] /set_parameters — target=%.2f mL, rate=%.2f mL/min\n",
                      targetVolume_mL, flowRate_mLmin);

        // Clear any previous alarms before starting
        alarmOcclusion = false;
        alarmEmpty     = false;
        alarmTremor    = false;

        // Calculate stepper speed and start motor
        startMotor();

        request->send(200, "application/json", "{\"status\":\"ok\"}");
      }
    }
  );

  // ── POST /emergency_stop ──
  server.on("/emergency_stop", HTTP_POST, [](AsyncWebServerRequest *request) {
    haltMotor("EMERGENCY STOP via web");
    request->send(200, "application/json", "{\"status\":\"stopped\"}");
  });

  // ── GET /status ──
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Build JSON response
    StaticJsonDocument<256> doc;
    doc["delivered_vol"] = deliveredVol_mL;
    doc["occlusion"]     = alarmOcclusion;
    doc["empty"]         = alarmEmpty;
    doc["tremor"]        = alarmTremor;
    doc["running"]       = motorRunning;

    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
  });

  // ── Start listening ──
  server.begin();
  Serial.println("[Server] Async web server started on port 80");
}

// ═════════════════════════════════════════════
//  MOTOR CONTROL
// ═════════════════════════════════════════════

/*
 *  startMotor()
 *  Converts the flow rate (mL/min) into a stepper speed (steps/sec)
 *  and begins constant-speed movement.
 */
void startMotor() {
  if (flowRate_mLmin <= 0 || targetVolume_mL <= 0) {
    Serial.println("[Motor] Invalid parameters — not starting.");
    return;
  }

  // ── Convert mL/min → steps/sec ──
  //    steps/sec = (mL/min) * (steps/mL) / 60
  float stepsPerSec = (flowRate_mLmin * STEPS_PER_ML) / 60.0;

  // Clamp to max speed
  if (stepsPerSec > stepper.maxSpeed()) {
    stepsPerSec = stepper.maxSpeed();
    Serial.printf("[Motor] Speed clamped to max: %.1f steps/sec\n", stepsPerSec);
  }

  stepper.setSpeed(stepsPerSec);
  motorRunning = true;

  Serial.printf("[Motor] Started — %.1f steps/sec (%.2f mL/min)\n",
                stepsPerSec, flowRate_mLmin);
}

/*
 *  haltMotor()
 *  Immediately stops the stepper and logs the reason.
 */
void haltMotor(const char* reason) {
  stepper.setSpeed(0);
  stepper.stop();
  motorRunning = false;
  Serial.printf("[Motor] HALTED — %s\n", reason);
}

// ═════════════════════════════════════════════
//  SENSOR READING (called every SENSOR_INTERVAL_MS)
// ═════════════════════════════════════════════
void readSensors() {

  // ── 1. Read potentiometer → delivered volume ──
  deliveredVol_mL = readPotVolume();

  // ── 2. Check if target volume reached ──
  if (motorRunning && deliveredVol_mL >= targetVolume_mL) {
    haltMotor("Target volume reached");
  }

  // ── 3. Occlusion limit switch (active LOW) ──
  //    When the tube is blocked, pressure builds and triggers the switch.
  if (digitalRead(OCCLUSION_PIN) == LOW) {
    if (!alarmOcclusion) {
      alarmOcclusion = true;
      haltMotor("OCCLUSION detected");
    }
  }

  // ── 4. Syringe empty limit switch (active LOW) ──
  //    Triggered when the plunger reaches the end of the barrel.
  if (digitalRead(EMPTY_PIN) == LOW) {
    if (!alarmEmpty) {
      alarmEmpty = true;
      haltMotor("SYRINGE EMPTY detected");
    }
  }

  // ── 5. Tremor detection via MPU6050 ──
  if (mpuAvailable) {
    float accelMag = readMPU6050Magnitude();
    if (accelMag > TREMOR_THRESHOLD_G) {
      if (!alarmTremor) {
        alarmTremor = true;
        haltMotor("TREMOR detected (accel spike)");
        Serial.printf("[Tremor] Magnitude: %.2f g\n", accelMag);
      }
    }
  }
}

// ═════════════════════════════════════════════
//  POTENTIOMETER → VOLUME (with smoothing)
// ═════════════════════════════════════════════

/*
 *  readPotVolume()
 *  Reads the ADC, applies a rolling average filter,
 *  and maps the result to 0..SYRINGE_MAX_ML.
 *
 *  *** CALIBRATION REQUIRED ***
 *  1. Set POT_MIN_RAW to the ADC value when plunger is at 0 mL.
 *  2. Set POT_MAX_RAW to the ADC value when plunger is at max mL.
 *  3. Set SYRINGE_MAX_ML to your syringe size.
 */
float readPotVolume() {
  // Store new sample in circular buffer
  adcBuffer[adcIndex] = analogRead(POT_PIN);
  adcIndex = (adcIndex + 1) % ADC_SAMPLES;

  // Compute rolling average
  long sum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    sum += adcBuffer[i];
  }
  float avgRaw = (float)sum / ADC_SAMPLES;

  // Map to volume (constrain to valid range)
  float volume = ((avgRaw - POT_MIN_RAW) / (POT_MAX_RAW - POT_MIN_RAW)) * SYRINGE_MAX_ML;
  if (volume < 0.0) volume = 0.0;
  if (volume > SYRINGE_MAX_ML) volume = SYRINGE_MAX_ML;

  return volume;
}

// ═════════════════════════════════════════════
//  MPU6050 ACCELEROMETER
// ═════════════════════════════════════════════

/*
 *  setupMPU6050()
 *  Wakes the MPU6050 from sleep mode.
 *  Sets mpuAvailable = true on success.
 */
void setupMPU6050() {
  Serial.print("[MPU6050] Initializing... ");

  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(MPU6050_PWR_MGMT_1);
  Wire.write(0x00);                        // Wake up (clear sleep bit)
  byte error = Wire.endTransmission(true);

  if (error == 0) {
    mpuAvailable = true;
    Serial.println("OK");
  } else {
    mpuAvailable = false;
    Serial.println("FAILED (not connected — tremor detection disabled)");
  }
}

/*
 *  readMPU6050Magnitude()
 *  Reads raw accelerometer X, Y, Z and returns the
 *  total magnitude in g-force units.
 *
 *  Default sensitivity: ±2g range → 16384 LSB/g
 */
float readMPU6050Magnitude() {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(MPU6050_ACCEL_XOUT);         // Start at ACCEL_XOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)MPU6050_ADDR, (uint8_t)6, (uint8_t)true);

  // Read 6 bytes: AX_H, AX_L, AY_H, AY_L, AZ_H, AZ_L
  int16_t ax = (Wire.read() << 8) | Wire.read();
  int16_t ay = (Wire.read() << 8) | Wire.read();
  int16_t az = (Wire.read() << 8) | Wire.read();

  // Convert to g (±2g range, 16384 LSB/g)
  float gx = ax / 16384.0;
  float gy = ay / 16384.0;
  float gz = az / 16384.0;

  // Return magnitude (includes gravity, so ~1g at rest)
  return sqrt(gx * gx + gy * gy + gz * gz);
}
