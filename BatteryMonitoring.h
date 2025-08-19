#pragma once

extern int VOLTAGE_PIN;

const float MAX_BATTERY_V = 12.6; // full charge for 12V lead acid / LiFePO4 adjust accordingly
const float MIN_BATTERY_V = 11.0; // empty threshold
// Voltage Sensor divider ratio (DC 0-25V sensor = 5:1)
const float DIVIDER_RATIO = 5.0;

float readBatteryVoltage() {
  int raw = analogRead(VOLTAGE_PIN);
  float vOut = (raw / 4095.0) * 3.3;   // ESP32 ADC 0–4095 at 0–3.3V
  float vIn = vOut * DIVIDER_RATIO;    // scale back to actual battery voltage
  return vIn;
}

int batteryPercent(float voltage) {
  if (voltage >= MAX_BATTERY_V) return 100;
  if (voltage <= MIN_BATTERY_V) return 0;
  return (int)( (voltage - MIN_BATTERY_V) * 100 / (MAX_BATTERY_V - MIN_BATTERY_V) );
}
