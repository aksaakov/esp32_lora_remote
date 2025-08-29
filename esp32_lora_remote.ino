#include "DisplaySuite.h"
#include "LoraController.h"
#include "LoRaWan_APP.h"
#include "BatteryMonitoring.h"
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include "DeepSleep.h"

volatile bool isAlarmOn = false;

int VOLTAGE_PIN = 2;
int PIR_PIN = 5;
int ALARM_RELAY = 45;

uint8_t MOTION_CODE = 0x01;
uint8_t ALARM_STATUS = 0x20;
uint8_t BATTERY_REPORT_CODE = 0x80;

static const uint8_t MOTION_TX_PKG[1] = { MOTION_CODE };
static const uint8_t ALARM_STATUS_PKG[1] = { ALARM_STATUS };
static const uint8_t BATTERY_REPORT_PKG[1] = { BATTERY_REPORT_CODE };

uint32_t lastStatusMs = 0;
uint32_t alarmStartedMs = 0;
const uint32_t STATUS_INTERVAL_MS = 3000;
const uint32_t ALARM_EXPIRES_AFTER_MS = 3UL * 60UL * 1000UL; // 3 minutes

uint32_t lastMotionMs = 0;
const uint32_t MOTION_COOLDOWN_MS = 1500; 

void sendAndAwait(const uint8_t* payload, size_t len, uint32_t ms) {
  sendMessage(payload, len);
  radioAwaitTxDone(ms);
}

void toggleAlarmSound(bool shouldSoundAlarm) {
  if (shouldSoundAlarm && !isAlarmOn) {
    alarmStartedMs = millis();
    digitalWrite(ALARM_RELAY, HIGH);
    isAlarmOn = true;
  } else if (!shouldSoundAlarm && isAlarmOn) {
    alarmStartedMs = 0;
    digitalWrite(ALARM_RELAY, LOW);
    isAlarmOn = false;
  }
}

void sendBatteryUpdate() {
  float v = readBatteryVoltage();
  uint8_t pct = (uint8_t)batteryPercent(v);
  uint8_t battery_payload[2] = { BATTERY_REPORT_CODE, pct };
  sendAndAwait(battery_payload, 2, 600);
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  delay(200);

  pinMode(PIR_PIN, INPUT_PULLDOWN);
  pinMode(ALARM_RELAY, OUTPUT);
  radioInit();

  // Wake cause:
  auto cause = esp_sleep_get_wakeup_cause();
  switch (cause) {
    case ESP_SLEEP_WAKEUP_TIMER:
      {
        Serial.println("Wake cause: TIMER");
        sendBatteryUpdate();
        break;
      }
    case ESP_SLEEP_WAKEUP_EXT0:
      {
        Serial.println("wake cause: MOTION (EXT0)");
        displayMotionIcon();
        sendAndAwait(MOTION_TX_PKG, 1, 600);
        lastMotionMs = millis();
        break;
      }
    default:
      Serial.println("Wake cause: POWER-ON/OTHER");
      displayLogo();
      sendBatteryUpdate();
      break;
  }

  armWakeSources();
}

void processReceivedPacket(const uint8_t* data, uint16_t len, int16_t rssi, int8_t snr) {
  Serial.printf("data=%u, rssi=%d, snr=%d\n", data[0], rssi, snr);

  switch (data[0]) {
    case 0x10:
      Serial.println("received -> 0x10 code: ALARM ON");
      toggleAlarmSound(true);
      break;
    case 0x11:
      Serial.println("received -> 0x11 code: ALARM OFF");
      toggleAlarmSound(false);
      break;
    default:
      Serial.println("received -> UNKNOWN code or data not binary.");
      break;
  }
}

void loop() {
  if (isAlarmOn) {
    if (millis() - lastStatusMs >= STATUS_INTERVAL_MS) {
      lastStatusMs = millis();
      sendAndAwait(ALARM_STATUS_PKG, 1, 300);
    }

    if (millis() - alarmStartedMs >= ALARM_EXPIRES_AFTER_MS) {
      Serial.println("ALARM AUTO OFF");
      toggleAlarmSound(false);
    }

    radioService();
    delay(10);
    return;
  }

  int motionState = digitalRead(PIR_PIN);

  if (motionState == HIGH && millis() - lastMotionMs >= MOTION_COOLDOWN_MS) {
    displayMotionIcon();
    sendAndAwait(MOTION_TX_PKG, 1, 600);
    lastMotionMs = millis();
  }

  radioService();

  if (!isAlarmOn && esp_timer_get_time() - g_awake_start_us >= AWAKE_TIME) {
    goDeepSleep();
  }

  delay(50);
}
