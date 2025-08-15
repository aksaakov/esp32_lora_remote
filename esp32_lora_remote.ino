#include "DisplaySuite.h"
#include "loraController.h"
#include "LoRaWan_APP.h"

bool isAlarmArmed = false; 
bool isAlarmOn = false;
bool isMotionTriggered = false;

int PIR_PIN = 48;
int ALARM_LED_PIN = 45;
uint8_t motion_pkg = 0x01;

void sendMessage(const uint8_t* package) {
  Radio.Standby();
  Serial.printf(">>> Sending package: 0x%02X <<<\n",  package[0]);
  Radio.Send( (uint8_t *)package, 1);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  displayLogo();
  pinMode(PIR_PIN, INPUT);
  pinMode(ALARM_LED_PIN, OUTPUT);
  radioInit();
}

void processReceivedPacket(const uint8_t* data, uint16_t len, int16_t rssi, int8_t snr) {
  Serial.printf("data=%u, rssi=%d, snr=%d\n", data[0], rssi, snr);

  switch(data[0]) {
    case 0x10:
      Serial.println("received -> 0x10 code: ALARM ARMED");
      digitalWrite(ALARM_LED_PIN, HIGH);
      isAlarmArmed = true;
      if (isMotionTriggered) {
        isAlarmOn = true;
      }
      break;
    case 0x11:
      Serial.println("received -> 0x11 code: ALARM DIDSARMED");
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
  int motionState = digitalRead(PIR_PIN);

  if (motionState == HIGH) {
    displayMotionIcon();
    sendMessage(&motion_pkg);
    isMotionTriggered = true;
  }

  if (isAlarmOn) {
    Serial.println("!!! ALARM LOUD !!!");
  }

  Radio.IrqProcess();

  delay(1000);
  // receive();
}
