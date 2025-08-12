#include "DisplaySuite.h"
#include "HT_SSD1306Wire.h"
#include "Images.h"

// Heltec V3: Vext powers the OLED. LOW = ON.
static inline void vextPower(bool on) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, on ? LOW : HIGH); // LOW turns Vext ON
}

// addr, i2c freq, SDA, SCL, geometry, RST
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

bool isDisplayOn;

void displayClear() {
  display.clear();
}

void displayOff() {
  display.clear();
  display.display();
  vextPower(false);
  isDisplayOn = false;
}

void displayOn() {
  vextPower(true);
  delay(50);
  display.init(); 
  isDisplayOn = true;
}

void displayLogo() {
  displayOn();
  display.clear();
  display.drawXbm(0, 0, logo_width, logo_height, logo_bits);
  display.display();
  delay(5000);
  displayOff();
}

void displayMotionIcon() {
  displayOn();
  display.clear();
  display.drawXbm(0, 0, logo_width, logo_height, motion_bits);
  display.display();
  delay(1000);
  displayOff();
}

void displayShow(const int msDelay, const String& line1, const String& line2) {
  Serial.printf("isDisplayOn?: %s\n", isDisplayOn ? "true" : "false");
  if (!isDisplayOn) {
    displayOn();
  }

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);

  int cx = display.getWidth() / 2;
  int y  = 18;

  display.setFont(ArialMT_Plain_16);
  display.drawString(cx, y, line1);
  Serial.println("display showing message heading: " + line1);

  if (line2.length()) {
    display.setFont(ArialMT_Plain_10);
    display.drawString(cx, y + 20, line2);
    Serial.println("and message detail: " + line2);
  }
  display.display();
  delay(msDelay);
  displayOff();
}