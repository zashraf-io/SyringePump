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

// Alerts
#define BUZZER_PIN     23                  // Passive buzzer on GPIO 23

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
 *    Calibrated as: 2048 steps per 1.25 mL = 1638.4 steps/mL.
 */
#define STEPS_PER_ML    1638.4

// ─────────────────────────────────────────────
//  FSR PRESSURE SENSOR CONSTANTS
// ─────────────────────────────────────────────

/*
 *  FSR_ALARM_THRESHOLD:
 *    Raw ADC value (0–4095) above which the FSR indicates
 *    an occlusion (pressure increase) while infusing.
 *    Higher pressure → higher ADC value (with voltage divider).
 *
 *    Start with a LOW value (e.g. 500) and raise it if you get
 *    false triggers. Check Serial Monitor for "[FSR]" debug lines
 *    to see the actual ADC readings from your sensor.
 *    *** CALIBRATE with your hardware. ***
 */
#define FSR_ALARM_THRESHOLD 1000

/*
trying the push button to detect the occlusion
*/
const int pushButtonPin = 4;

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

float targetVolume_mL  = 0.0;             // Desired infusion volume (mL)
float flowRate_mLmin   = 0.0;             // Desired flow rate (mL/min)
double deliveredVol_mL  = 0.0;             // Current delivered volume (mL)

// Motor direction: -1 = push (dispense), +1 = pull (retract)
int   motorDirection   = -1;

bool  mpuAvailable     = false;           // Did MPU6050 init succeed?

// Buzzer and user alert timer state
enum TuneType { TUNE_NONE, TUNE_ALERT, TUNE_TIMER };
TuneType activeTune = TUNE_NONE;
uint8_t tuneIndex = 0;
unsigned long tuneStepStarted_ms = 0;
bool lastCriticalAlarmState = false;

bool alertTimerActive = false;
bool alertTimerTriggered = false;
unsigned long alertTimerDue_ms = 0;
unsigned long alertTimerDuration_ms = 0;

bool scheduledStartActive = false;
bool scheduledStartTriggered = false;
unsigned long scheduledStartDue_ms = 0;
unsigned long scheduledStartDuration_ms = 0;
float scheduledTargetVolume_mL = 0.0f;
float scheduledFlowRate_mLmin = 0.0f;

// ── FSR pressure reading ──
int   fsrRawValue      = 0;               // Latest FSR ADC reading (0–4095)
float fsrPressure      = 0.0;             // Mapped pressure (arbitrary units)

// ── Stepper position tracking ──
long lastStepperPosition = 0;

// ── YF-S401 flow rate sensor (Replaced with Stepper Estimation) ──
unsigned long lastFlowCalc_ms  = 0;        // Last time we computed flow rate
float measuredFlowRate_mLmin   = 0.0;      // Computed flow rate from stepper
const unsigned long FLOW_CALC_INTERVAL_MS = 1000; // Recalculate every 1 sec

// Timing for non-blocking sensor reads
unsigned long lastSensorCheck_ms = 0;
const unsigned long SENSOR_INTERVAL_MS = 50;  // Check sensors every 50 ms

// ADC smoothing (FSR only)

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
void   readSensors();
int    readFSR();
void   computeFlowRate();
float  readMPU6050Magnitude();
void   haltMotor(const char* reason);
void   startMotor();
void   startTune(TuneType tune);
void   updateBuzzer();
void   stopBuzzerTone();
void   updateAlertTimer();
void   updateScheduledStart();

// ═════════════════════════════════════════════
//  SETUP
// ═════════════════════════════════════════════
void setup() {
  // ── Serial for debugging ──
  Serial.begin(115200);
  delay(100);
  Serial.println("\n══════════════════════════════════════");
  Serial.println("  Syringe Pump Controller — Booting");
  Serial.println("══════════════════════════════════════");

  // ── Pin modes ──
  pinMode(EMPTY_PIN,     INPUT_PULLUP);    // Limit switch, NC to GND
  pinMode(pushButtonPin, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  stopBuzzerTone();
  // POT_PIN (34) and FSR_PIN (35) are ADC — no pinMode needed for analogRead

  // ── Stepper defaults (28BYJ-48 is slow: ~500 half-steps/sec max) ──
  stepper.setMaxSpeed(500);                // Half-steps/sec upper limit
  stepper.setAcceleration(200);            // Half-steps/sec² (smooth ramp)
  stepper.setSpeed(0);                     // Start stopped

  // ── Sync stepper position tracking ──
  lastStepperPosition = stepper.currentPosition();

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

  // â”€â”€ 4. Non-blocking buzzer and alert timer â”€â”€
  updateScheduledStart();
  updateAlertTimer();
  updateBuzzer();
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

        lastStepperPosition = stepper.currentPosition(); // Sync steps to current position
        
        Serial.printf("[API] /set_parameters — target=%.2f mL, rate=%.2f mL/min\n",
                      targetVolume_mL, flowRate_mLmin);

        // Clear any previous alarms before starting
        alarmOcclusion = false;
        alarmEmpty     = false;

        // Calculate stepper speed and start motor
        startMotor();

        request->send(200, "application/json", "{\"status\":\"ok\"}");
      }
    }
  );

  // ── POST /reset_volume ──
  server.on("/reset_volume", HTTP_POST, [](AsyncWebServerRequest *request) {
    deliveredVol_mL = 0.0;
    lastStepperPosition = stepper.currentPosition();
    Serial.println("[API] Volume reset to 0 mL");
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // ── POST /emergency_stop ──
  server.on("/emergency_stop", HTTP_POST, [](AsyncWebServerRequest *request) {
    haltMotor("EMERGENCY STOP via web");
    startTune(TUNE_ALERT);
    request->send(200, "application/json", "{\"status\":\"stopped\"}");
  });

  // â”€â”€ POST /set_alert_timer â”€â”€
  // Expects JSON: { "minutes": 10 }
  server.on("/set_alert_timer", HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (index == 0) {
        request->_tempObject = malloc(total + 1);
      }
      memcpy((uint8_t*)request->_tempObject + index, data, len);

      if (index + len == total) {
        ((char*)request->_tempObject)[total] = '\0';

        StaticJsonDocument<128> doc;
        DeserializationError err = deserializeJson(doc, (char*)request->_tempObject);
        free(request->_tempObject);
        request->_tempObject = NULL;

        if (err) {
          request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
          return;
        }

        float minutes = doc["minutes"] | 0.0f;
        if (minutes <= 0.0f || minutes > 1440.0f) {
          request->send(400, "application/json", "{\"error\":\"Timer minutes must be between 0 and 1440\"}");
          return;
        }

        alertTimerDuration_ms = (unsigned long)(minutes * 60000.0f);
        if (alertTimerDuration_ms < 1000UL) alertTimerDuration_ms = 1000UL;
        alertTimerDue_ms = millis() + alertTimerDuration_ms;
        alertTimerActive = true;
        alertTimerTriggered = false;

        Serial.printf("[Timer] Alert timer set for %.2f minutes\n", minutes);
        request->send(200, "application/json", "{\"status\":\"timer_set\"}");
      }
    }
  );

  // â”€â”€ POST /cancel_alert_timer â”€â”€
  server.on("/cancel_alert_timer", HTTP_POST, [](AsyncWebServerRequest *request) {
    alertTimerActive = false;
    alertTimerTriggered = false;
    request->send(200, "application/json", "{\"status\":\"timer_cancelled\"}");
  });

  // â”€â”€ POST /schedule_start â”€â”€
  // Expects JSON: { "seconds": 30, "minutes": 0.5, "target_vol": 5.0, "flow_rate": 0.5 }
  server.on("/schedule_start", HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (index == 0) {
        request->_tempObject = malloc(total + 1);
      }
      memcpy((uint8_t*)request->_tempObject + index, data, len);

      if (index + len == total) {
        ((char*)request->_tempObject)[total] = '\0';

        StaticJsonDocument<192> doc;
        DeserializationError err = deserializeJson(doc, (char*)request->_tempObject);
        free(request->_tempObject);
        request->_tempObject = NULL;

        if (err) {
          request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
          return;
        }

        float seconds = doc["seconds"] | 0.0f;
        float minutes = doc["minutes"] | 0.0f;
        float targetVol = doc["target_vol"] | 0.0f;
        float flowRate = doc["flow_rate"] | 0.0f;

        if (seconds <= 0.0f && minutes > 0.0f) {
          seconds = minutes * 60.0f;
        }

        if (motorRunning) {
          request->send(409, "application/json", "{\"error\":\"Motor already running\"}");
          return;
        }

        if (seconds <= 0.0f || seconds > 86400.0f) {
          request->send(400, "application/json", "{\"error\":\"Timer seconds must be between 0 and 86400\"}");
          return;
        }

        if (targetVol <= 0.0f || flowRate <= 0.0f) {
          request->send(400, "application/json", "{\"error\":\"Invalid target volume or flow rate\"}");
          return;
        }

        scheduledStartDuration_ms = (unsigned long)(seconds * 1000.0f);
        if (scheduledStartDuration_ms < 1000UL) scheduledStartDuration_ms = 1000UL;
        scheduledStartDue_ms = millis() + scheduledStartDuration_ms;
        scheduledStartActive = true;
        scheduledStartTriggered = false;
        scheduledTargetVolume_mL = targetVol;
        scheduledFlowRate_mLmin = flowRate;

        Serial.printf("[Schedule] Start in %.2f seconds (target=%.2f mL, rate=%.2f mL/min)\n",
                seconds, targetVol, flowRate);
        request->send(200, "application/json", "{\"status\":\"scheduled\"}");
      }
    }
  );

  // â”€â”€ POST /cancel_scheduled_start â”€â”€
  server.on("/cancel_scheduled_start", HTTP_POST, [](AsyncWebServerRequest *request) {
    scheduledStartActive = false;
    scheduledStartTriggered = false;
    request->send(200, "application/json", "{\"status\":\"schedule_cancelled\"}");
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
    StaticJsonDocument<768> doc;
    doc["delivered_vol"]     = deliveredVol_mL;
    doc["occlusion"]         = alarmOcclusion;
    doc["empty"]             = alarmEmpty;
    doc["running"]           = motorRunning;
    doc["direction"]         = (motorDirection < 0) ? "push" : "pull";
    doc["fsr_raw"]           = fsrRawValue;
    doc["fsr_pressure"]      = fsrPressure;
    doc["measured_flow_rate"] = measuredFlowRate_mLmin;
    doc["alert_timer_active"] = alertTimerActive;
    doc["alert_timer_triggered"] = alertTimerTriggered;
    long timerRemaining_ms = 0;
    if (alertTimerActive) {
      timerRemaining_ms = (long)(alertTimerDue_ms - millis());
      if (timerRemaining_ms < 0) timerRemaining_ms = 0;
    }
    doc["alert_timer_remaining_ms"] = timerRemaining_ms;

    doc["scheduled_start_active"] = scheduledStartActive;
    doc["scheduled_start_triggered"] = scheduledStartTriggered;
    long scheduledRemaining_ms = 0;
    if (scheduledStartActive) {
      scheduledRemaining_ms = (long)(scheduledStartDue_ms - millis());
      if (scheduledRemaining_ms < 0) scheduledRemaining_ms = 0;
    }
    doc["scheduled_start_remaining_ms"] = scheduledRemaining_ms;

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

  // ── 1. Update delivered volume from stepper motor steps ──
  long currentSteps = stepper.currentPosition();
  long deltaSteps = currentSteps - lastStepperPosition;
  lastStepperPosition = currentSteps;
  
  if (deltaSteps != 0) {
    // Calculate the absolute volume change
    double stepVolDelta = (double)(abs(deltaSteps)) / STEPS_PER_ML;
    
    // Always increase delivered volume regardless of direction
    // so target volume works for both pull and push commands
    deliveredVol_mL += stepVolDelta; 
  }

  // ── 2. Check if target volume reached ──
  if (motorRunning && deliveredVol_mL >= targetVolume_mL) {
    haltMotor("Target volume reached");
    startTune(TUNE_ALERT);
  }

  // ── 3. Push button (empty indicator) ──
  static bool lastButtonState = HIGH;
  bool buttonState = digitalRead(pushButtonPin);
  bool buttonPressed = (lastButtonState == HIGH && buttonState == LOW);
  lastButtonState = buttonState;

  if (buttonPressed && !alarmEmpty) {
    if (alarmOcclusion) {
      alarmOcclusion = false;
      alarmEmpty = true;
      haltMotor("SYRINGE EMPTY confirmed (button)");
      Serial.println("[Button] Occlusion switched to EMPTY");
    } else {
      alarmEmpty = true;
      haltMotor("SYRINGE EMPTY detected (button)");
      Serial.println("[Button] EMPTY TRIGGERED");
    }
  }

  // ── 4. FSR pressure sensor → syringe empty & occlusion detection ──
  fsrRawValue = readFSR();
  fsrPressure = (float)fsrRawValue / 4095.0 * 100.0; // 0–100 arbitrary units

  // Debug: print FSR value periodically
  static unsigned long lastFsrDebug = 0;
  if (millis() - lastFsrDebug >= 1000) {
    lastFsrDebug = millis();
    Serial.printf("[FSR] Raw: %d | (Occlusion Thresh: %d)\n",
                  fsrRawValue, FSR_ALARM_THRESHOLD);
  }

  // Occlusion detection (empty alarm only via button)
  if (fsrRawValue >= FSR_ALARM_THRESHOLD) {
    if (!alarmOcclusion && !alarmEmpty) {
      alarmOcclusion = true;
      haltMotor("OCCLUSION detected (FSR pressure)");
      Serial.printf("[FSR] OCCLUSION TRIGGERED! Raw: %d\n", fsrRawValue);
    }
  } else if (alarmOcclusion) {
    alarmOcclusion = false;
    Serial.println("[FSR] Occlusion cleared — pressure returned to normal.");
  }
}

// ═════════════════════════════════════════════
//  POTENTIOMETER → VOLUME (with smoothing)
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
 *  computeFlowRate()
 *  Called every FLOW_CALC_INTERVAL_MS (1 sec).
 *  Estimates the flow rate directly from the stepper motor speed
 *  since the physical flow sensor is inaccurate at low rates.
 */
void computeFlowRate() {
  if (motorRunning) {
    // stepper.speed() is in steps/sec
    float stepsPerSec = abs(stepper.speed());
    // Convert to mL/min: (steps/sec / STEPS_PER_ML) * 60
    measuredFlowRate_mLmin = (stepsPerSec / STEPS_PER_ML) * 60.0;
  } else {
    measuredFlowRate_mLmin = 0.0;
  }

  // Print estimated flow rate for debugging
  Serial.printf("[FlowRate] Estimated from stepper: %.2f mL/min\n", measuredFlowRate_mLmin);
}

// ═════════════════════════════════════════════
//  BUZZER ALERTS AND USER TIMER
void startTune(TuneType tune) {
  activeTune = tune;
  tuneIndex = 0;
  tuneStepStarted_ms = 0;
}

void stopBuzzerTone() {
  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, LOW);
}

void updateAlertTimer() {
  unsigned long now = millis();
  if (alertTimerActive && (long)(now - alertTimerDue_ms) >= 0) {
    alertTimerActive = false;
    alertTimerTriggered = true;
    Serial.println("[Timer] Alert timer reached");
  }

  bool criticalAlarmActive = alarmOcclusion || alarmEmpty;
  if (criticalAlarmActive && !lastCriticalAlarmState) {
    startTune(TUNE_ALERT);
  }
  lastCriticalAlarmState = criticalAlarmActive;
}

void updateScheduledStart() {
  if (!scheduledStartActive) return;

  unsigned long now = millis();
  if ((long)(now - scheduledStartDue_ms) < 0) return;

  scheduledStartActive = false;
  scheduledStartTriggered = true;

  targetVolume_mL = scheduledTargetVolume_mL;
  flowRate_mLmin = scheduledFlowRate_mLmin;
  deliveredVol_mL = 0.0;
  lastStepperPosition = stepper.currentPosition();
  alarmOcclusion = false;
  alarmEmpty = false;

  Serial.printf("[Schedule] Triggered. Starting infusion: %.2f mL at %.2f mL/min\n",
                targetVolume_mL, flowRate_mLmin);
  startTune(TUNE_TIMER);
  startMotor();
}

void updateBuzzer() {
  static const uint16_t alertNotes[] = { 988, 0, 988, 0, 784, 0, 988, 0 };
  static const uint16_t alertDurations[] = { 140, 70, 140, 70, 220, 90, 300, 0 };
  static const uint16_t timerNotes[] = { 659, 784, 988, 1175, 988, 1175, 1319, 0 };
  static const uint16_t timerDurations[] = { 120, 120, 120, 180, 120, 120, 260, 0 };

  if (activeTune == TUNE_NONE) return;

  const uint16_t* notes = (activeTune == TUNE_ALERT) ? alertNotes : timerNotes;
  const uint16_t* durations = (activeTune == TUNE_ALERT) ? alertDurations : timerDurations;
  const uint8_t tuneLength = 8;
  unsigned long now = millis();

  if (tuneStepStarted_ms == 0 || now - tuneStepStarted_ms >= durations[tuneIndex - 1]) {
    if (tuneIndex >= tuneLength || durations[tuneIndex] == 0) {
      stopBuzzerTone();
      activeTune = TUNE_NONE;
      tuneIndex = 0;
      return;
    }

    if (notes[tuneIndex] == 0) {
      stopBuzzerTone();
    } else {
      tone(BUZZER_PIN, notes[tuneIndex]);
    }

    tuneStepStarted_ms = now;
    tuneIndex++;
  }
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
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