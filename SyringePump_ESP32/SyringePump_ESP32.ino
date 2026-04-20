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
 *    • Non-blocking AccelStepper motor control (28BYJ-48 via ULN2003)
 *    • Linear potentiometer volume sensing (ADC)
 *    • FSR pressure sensor for occlusion detection (ADC)
 *    • YF-S401 flow rate sensor (interrupt-driven pulse counting)
 *    • Syringe-empty limit switch (active LOW)
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
 *    Motor       : 28BYJ-48 via ULN2003 driver
 *                   IN1=14, IN2=27, IN3=26, IN4=25
 *    Potentiometer: GPIO 34  (ADC1, analog volume sensing)
 *    FSR Sensor  : GPIO 35  (ADC1, analog pressure sensing)
 *    Flow Sensor : GPIO 4   (YF-S401, digital pulse output)
 *    Empty SW    : GPIO 33  (active LOW, internal pull-up)
 *    MPU6050     : SDA=21, SCL=22 (default I2C)
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

// 28BYJ-48 stepper via ULN2003 driver board
#define MOTOR_IN1      14                  // ULN2003 IN1
#define MOTOR_IN2      27                  // ULN2003 IN2
#define MOTOR_IN3      26                  // ULN2003 IN3
#define MOTOR_IN4      25                  // ULN2003 IN4

// Sensors
#define POT_PIN        34                  // Linear potentiometer (volume) — ADC1
#define FSR_PIN        35                  // FSR pressure sensor — ADC1
#define FLOW_PIN       4                   // YF-S401 flow rate sensor (digital pulse)

// Switches
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
 *    The 28BYJ-48 has 2048 half-steps per revolution (with its
 *    built-in 1:64 gear reduction). Actual value depends on your
 *    lead-screw pitch and syringe barrel diameter.
 *    *** You MUST calibrate this with your hardware. ***
 */
#define STEPS_PER_ML    2048.0

// ─────────────────────────────────────────────
//  FSR PRESSURE SENSOR CONSTANTS
// ─────────────────────────────────────────────

/*
 *  FSR_EMPTY_THRESHOLD:
 *    Raw ADC value (0–4095) above which the FSR indicates
 *    that the syringe plunger has bottomed out (syringe empty).
 *    The FSR is placed so the plunger presses it at end of travel.
 *    Higher pressure → higher ADC value (with voltage divider).
 *
 *    Start with a LOW value (e.g. 500) and raise it if you get
 *    false triggers. Check Serial Monitor for "[FSR]" debug lines
 *    to see the actual ADC readings from your sensor.
 *    *** CALIBRATE with your hardware. ***
 */
#define FSR_EMPTY_THRESHOLD 500

// ─────────────────────────────────────────────
//  YF-S401 FLOW RATE SENSOR CONSTANTS
// ─────────────────────────────────────────────

/*
 *  FLOW_CALIBRATION_FACTOR:
 *    The YF-S401 outputs ~98 pulses per litre (datasheet: 98 pulses/L).
 *    Frequency (Hz) = flow rate (L/min) × 98.
 *    So: flow (L/min) = frequency / 98
 *        flow (mL/min) = frequency / 98 * 1000 = frequency * 10.204
 *    *** Adjust based on your calibration. ***
 */
#define FLOW_CALIBRATION_FACTOR 98.0

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

// 28BYJ-48 stepper in half-step mode via ULN2003 (4 pins)
// Pin order for AccelStepper HALF4WIRE: IN1, IN3, IN2, IN4
AccelStepper stepper(AccelStepper::HALF4WIRE, MOTOR_IN1, MOTOR_IN3, MOTOR_IN2, MOTOR_IN4);

// Async web server on port 80
AsyncWebServer server(80);

// ─────────────────────────────────────────────
//  SYSTEM STATE (volatile where ISR-adjacent)
// ─────────────────────────────────────────────
volatile bool motorRunning    = false;     // Is the motor actively stepping?
volatile bool alarmOcclusion  = false;     // Occlusion detected (via FSR)?
volatile bool alarmEmpty      = false;     // Syringe empty detected?
volatile bool alarmTremor     = false;     // Tremor spike detected?

float targetVolume_mL  = 0.0;             // Desired infusion volume (mL)
float flowRate_mLmin   = 0.0;             // Desired flow rate (mL/min)
float deliveredVol_mL  = 0.0;             // Current delivered volume (mL)

// Motor direction: -1 = push (dispense), +1 = pull (retract)
int   motorDirection   = -1;

bool  mpuAvailable     = false;           // Did MPU6050 init succeed?

// ── FSR pressure reading ──
int   fsrRawValue      = 0;               // Latest FSR ADC reading (0–4095)
float fsrPressure      = 0.0;             // Mapped pressure (arbitrary units)

// ── YF-S401 flow rate sensor ──
volatile unsigned long flowPulseCount = 0; // ISR-incremented pulse counter
unsigned long lastFlowCalc_ms  = 0;        // Last time we computed flow rate
float measuredFlowRate_mLmin   = 0.0;      // Computed flow rate from sensor
const unsigned long FLOW_CALC_INTERVAL_MS = 1000; // Recalculate every 1 sec

// Timing for non-blocking sensor reads
unsigned long lastSensorCheck_ms = 0;
const unsigned long SENSOR_INTERVAL_MS = 50;  // Check sensors every 50 ms

// ADC smoothing (potentiometer)
#define ADC_SAMPLES 8
int adcBuffer[ADC_SAMPLES];
int adcIndex = 0;

// ADC smoothing (FSR)
#define FSR_SAMPLES 8
int fsrBuffer[FSR_SAMPLES];
int fsrIndex = 0;

// ─────────────────────────────────────────────
//  FORWARD DECLARATIONS
// ─────────────────────────────────────────────
void   setupWiFiAP();
void   setupWebServer();
void   setupMPU6050();
void   setupFlowSensor();
void   readSensors();
float  readPotVolume();
int    readFSR();
void   computeFlowRate();
float  readMPU6050Magnitude();
void   haltMotor(const char* reason);
void   startMotor();
void   IRAM_ATTR flowPulseISR();

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
  pinMode(EMPTY_PIN,     INPUT_PULLUP);    // Limit switch, NC to GND
  // POT_PIN (34) and FSR_PIN (35) are ADC — no pinMode needed for analogRead

  // ── Stepper defaults (28BYJ-48 is slow: ~500 half-steps/sec max) ──
  stepper.setMaxSpeed(500);                // Half-steps/sec upper limit
  stepper.setAcceleration(200);            // Half-steps/sec² (smooth ramp)
  stepper.setSpeed(0);                     // Start stopped

  // ── Initialize ADC buffers ──
  for (int i = 0; i < ADC_SAMPLES; i++) {
    adcBuffer[i] = analogRead(POT_PIN);
  }
  for (int i = 0; i < FSR_SAMPLES; i++) {
    fsrBuffer[i] = analogRead(FSR_PIN);
  }

  // ── Flow sensor (YF-S401) ──
  setupFlowSensor();

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

  // ── 3. Compute flow rate from YF-S401 (every 1 sec) ──
  if (now - lastFlowCalc_ms >= FLOW_CALC_INTERVAL_MS) {
    computeFlowRate();
    lastFlowCalc_ms = now;
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

  // ── POST /reverse_direction ──
  //    Toggles motor direction between push (dispense) and pull (retract).
  //    If the motor is running, the speed is immediately reapplied with the new direction.
  server.on("/reverse_direction", HTTP_POST, [](AsyncWebServerRequest *request) {
    motorDirection *= -1;
    const char* dirLabel = (motorDirection < 0) ? "PUSH (dispense)" : "PULL (retract)";
    Serial.printf("[Motor] Direction reversed → %s\n", dirLabel);

    // If motor is currently running, reapply speed with new direction
    if (motorRunning) {
      float stepsPerSec = (flowRate_mLmin * STEPS_PER_ML) / 60.0;
      if (stepsPerSec > stepper.maxSpeed()) stepsPerSec = stepper.maxSpeed();
      stepper.setSpeed(stepsPerSec * motorDirection);
    }

    String resp = "{\"status\":\"ok\",\"direction\":\"";
    resp += dirLabel;
    resp += "\"}";
    request->send(200, "application/json", resp);
  });

  // ── GET /status ──
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Build JSON response
    StaticJsonDocument<512> doc;
    doc["delivered_vol"]     = deliveredVol_mL;
    doc["occlusion"]         = alarmOcclusion;
    doc["empty"]             = alarmEmpty;
    doc["tremor"]            = alarmTremor;
    doc["running"]           = motorRunning;
    doc["direction"]         = (motorDirection < 0) ? "push" : "pull";
    doc["fsr_raw"]           = fsrRawValue;
    doc["fsr_pressure"]      = fsrPressure;
    doc["measured_flow_rate"] = measuredFlowRate_mLmin;

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

  stepper.setSpeed(stepsPerSec * motorDirection);
  motorRunning = true;

  Serial.printf("[Motor] Started — %.1f steps/sec (%.2f mL/min, dir=%s)\n",
                stepsPerSec, flowRate_mLmin,
                (motorDirection < 0) ? "PUSH" : "PULL");
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

  // ── 3. FSR pressure sensor → syringe empty detection ──
  //    The FSR is positioned so the plunger presses it when the
  //    syringe is empty. When force is detected → emergency stop.
  fsrRawValue = readFSR();
  fsrPressure = (float)fsrRawValue / 4095.0 * 100.0; // 0–100 arbitrary units

  // Debug: print FSR value periodically so user can calibrate threshold
  static unsigned long lastFsrDebug = 0;
  if (millis() - lastFsrDebug >= 1000) {
    lastFsrDebug = millis();
    Serial.printf("[FSR] Raw: %d / 4095  (threshold: %d)\n", fsrRawValue, FSR_EMPTY_THRESHOLD);
  }

  if (fsrRawValue >= FSR_EMPTY_THRESHOLD) {
    if (!alarmEmpty) {
      alarmEmpty = true;
      haltMotor("SYRINGE EMPTY detected (FSR pressure)");
      Serial.printf("[FSR] TRIGGERED! Raw: %d, Pressure: %.1f%%\n", fsrRawValue, fsrPressure);
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
//  FSR PRESSURE SENSOR (with smoothing)
// ═════════════════════════════════════════════

/*
 *  readFSR()
 *  Reads the FSR analog value, applies a rolling average filter,
 *  and returns the smoothed ADC value (0–4095).
 *
 *  Wiring: FSR in series with a 10kΩ resistor (voltage divider)
 *    FSR one leg → 3.3V
 *    FSR other leg → FSR_PIN (GPIO 35) AND through 10kΩ to GND
 */
int readFSR() {
  // Store new sample in circular buffer
  fsrBuffer[fsrIndex] = analogRead(FSR_PIN);
  fsrIndex = (fsrIndex + 1) % FSR_SAMPLES;

  // Compute rolling average
  long sum = 0;
  for (int i = 0; i < FSR_SAMPLES; i++) {
    sum += fsrBuffer[i];
  }
  return (int)(sum / FSR_SAMPLES);
}

// ═════════════════════════════════════════════
//  YF-S401 FLOW RATE SENSOR
// ═════════════════════════════════════════════

/*
 *  flowPulseISR()
 *  Interrupt Service Routine — increments pulse count on each
 *  rising edge from the YF-S401 Hall-effect sensor.
 */
void IRAM_ATTR flowPulseISR() {
  flowPulseCount++;
}

/*
 *  setupFlowSensor()
 *  Configures the flow sensor pin and attaches the interrupt.
 *  The YF-S401 outputs an open-collector signal — use INPUT_PULLUP.
 */
void setupFlowSensor() {
  Serial.print("[FlowSensor] Initializing YF-S401... ");
  pinMode(FLOW_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), flowPulseISR, RISING);
  lastFlowCalc_ms = millis();
  Serial.println("OK");
}

/*
 *  computeFlowRate()
 *  Called every FLOW_CALC_INTERVAL_MS (1 sec).
 *  Reads the accumulated pulse count, computes frequency,
 *  and converts to mL/min using the calibration factor.
 *
 *  YF-S401: ~98 pulses per litre
 *    frequency (Hz) = pulses / elapsed_seconds
 *    flow (L/min)   = frequency / FLOW_CALIBRATION_FACTOR
 *    flow (mL/min)  = flow (L/min) * 1000
 */
void computeFlowRate() {
  // Atomically read and reset the pulse counter
  noInterrupts();
  unsigned long pulses = flowPulseCount;
  flowPulseCount = 0;
  interrupts();

  // Compute flow rate
  float frequency = (float)pulses; // We calculate every 1 sec, so Hz ≈ count
  measuredFlowRate_mLmin = (frequency / FLOW_CALIBRATION_FACTOR) * 1000.0;

  if (measuredFlowRate_mLmin > 0.01) {
    Serial.printf("[FlowSensor] %.2f mL/min (%lu pulses)\n",
                  measuredFlowRate_mLmin, pulses);
  }
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
