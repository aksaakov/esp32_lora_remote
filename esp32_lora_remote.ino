#include "DisplaySuite.h"
#include "loraController.h"
#include "LoRaWan_APP.h"
#include "BatteryMonitoring.h"

bool isAlarmArmed = false; 
bool isAlarmOn = false;
bool isMotionTriggered = false;

int VOLTAGE_PIN = 2; // Pin connected to the voltage sensor output
int PIR_PIN = 48;
int ALARM_LED_PIN = 45;

uint8_t motion_pkg = 0x01;
volatile uint8_t pendingAck = 0;

void sendMessage(const uint8_t* package) {
  Radio.Standby();
  Serial.printf(">>> Sending package: 0x%02X <<<\n",  package[0]);
  Radio.Send( (uint8_t *)package, 1);
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12); // ESP32 default is 12 bits
  delay(200);
  displayLogo();
  pinMode(PIR_PIN, INPUT);
  pinMode(ALARM_LED_PIN, OUTPUT);
  radioInit();
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
      pendingAck = 0x10;
      digitalWrite(ALARM_LED_PIN, HIGH);
      isAlarmArmed = true;
      if (isMotionTriggered) {
        isAlarmOn = true;
      }
      break;
    case 0x11:
      Serial.println("received -> 0x11 code: ALARM DIDSARMED");
      pendingAck = 0x11;
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

  Serial.print("Battery Voltage: ");
  Serial.print(voltage, 2);
  Serial.print(" V  |  Battery: ");
  Serial.print(percent);
  Serial.println("%");

  int motionState = digitalRead(PIR_PIN);

  if (pendingAck) {
    uint8_t ackPackage = pendingAck;
    pendingAck = 0;
    sendMessage(&ackPackage);
  }

  if (motionState == HIGH) {
    displayMotionIcon();
    sendMessage(&motion_pkg);
    if (isAlarmArmed) isMotionTriggered = true;
  }

  if (isAlarmOn) {
    Serial.println("!!! ALARM LOUD !!!");
  }

  Radio.IrqProcess();

  if (!pendingAck) delay(1000);
  // receive();
}
