#include "DisplaySuite.h"
#include "loraController.h"
#include "LoRaWan_APP.h"
#include "BatteryMonitoring.h"
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include "DeepSleep.h"

bool isAlarmArmed = false; 
bool isAlarmOn = false;
bool isMotionTriggered = false;

int VOLTAGE_PIN = 2;
int PIR_PIN = 5;
int ALARM_LED_PIN = 45;

uint8_t motion_pkg = 0x01;
const uint8_t akarm_status_req_pkg = 0x90;
volatile uint8_t queuedAck = 0;
constexpr uint64_t MOTION_SUPPRESS_TIME = 5ULL * 1000000ULL; // 5s

void sendMessage(const uint8_t* package) {
  Radio.Standby();
  Serial.printf(">>> Sending package: 0x%02X <<<\n",  package[0]);
  Radio.Send( (uint8_t *)package, 1);
  Radio.IrqProcess();
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12); // ESP32 default is 12 bits
  delay(200);

  // displayLogo();
  pinMode(PIR_PIN, INPUT_PULLDOWN);
  pinMode(ALARM_LED_PIN, OUTPUT);
  radioInit();

  // Wake cause:
  auto cause = esp_sleep_get_wakeup_cause();
  Serial.print("Wake cause: ");
  switch (cause) {
    case ESP_SLEEP_WAKEUP_TIMER: 
      Serial.println("TIMER"); 
      break;
    case ESP_SLEEP_WAKEUP_EXT0:  
      sendMessage(&akarm_status_req_pkg);
      sendMessage(&motion_pkg);
      Serial.println("MOTION (EXT0)"); 
      break;
    // case ESP_SLEEP_WAKEUP_EXT1:  Serial.println("MOTION (EXT1)"); break;
    default:   
      sendMessage(&akarm_status_req_pkg);
      Serial.println("POWER-ON/OTHER"); 
      break;
  }
  
  armWakeSources();
}

void processReceivedPacket(const uint8_t* data, uint16_t len, int16_t rssi, int8_t snr) {
  Serial.printf("data=%u, rssi=%d, snr=%d\n", data[0], rssi, snr);

  switch(data[0]) {
    case 0x01:
      Serial.println("echoed -> 0x01 code: TX failed?");
      radioInit(); //reset radio in case of echo
      break;
    case 0x10:
      Serial.println("received -> 0x10 code: ALARM ARMED");
      queuedAck = 0x10;
      digitalWrite(ALARM_LED_PIN, HIGH);
      isAlarmArmed = true;
      if (isMotionTriggered) {
        isAlarmOn = true;
      }
      break;
    case 0x11:
      Serial.println("received -> 0x11 code: ALARM DIDSARMED");
      queuedAck = 0x11;
      digitalWrite(ALARM_LED_PIN, LOW);
      isAlarmArmed = false;
      isAlarmOn = false;
      isMotionTriggered = false;
      break;      
    default:
      Serial.println("received -> UNKNOWN code or data not binary.");
      break;
  }
}

void loop() {
  float voltage = readBatteryVoltage();
  int percent = batteryPercent(voltage);

  // Serial.print("Battery Voltage: ");
  // Serial.print(voltage, 2);
  // Serial.print(" V  |  Battery: ");
  // Serial.print(percent);
  // Serial.println("%");

  int motionState = digitalRead(PIR_PIN);

  if (queuedAck) {
    uint8_t ackPackage = queuedAck;
    queuedAck = 0;
    sendMessage(&ackPackage);
  }

  const bool shouldPeventMotion = (esp_timer_get_time() - g_awake_start_us) < MOTION_SUPPRESS_TIME;

  if (!shouldPeventMotion && motionState == HIGH) {
    displayMotionIcon();
    sendMessage(&motion_pkg);
    isMotionTriggered = true;
  }

  if (isAlarmOn) {
    Serial.println("!!! ALARM LOUD !!!");
  }

  Radio.IrqProcess();

  if (!isAlarmOn && esp_timer_get_time() - g_awake_start_us >= AWAKE_WINDOW_US) {
    goDeepSleep();
  }

  if (!queuedAck) delay(1000);
}
