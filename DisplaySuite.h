#pragma once
#include <Arduino.h>

// Init OLED (powers Vext, sets up I2C, shows a splash)
void displayInit();

// Show up to two lines of text (centered)
void displayShow(const int msDelay, const String& line1, const String& line2 = "");

// Show your XBM logo from Logo.h
void displayLogo();

// Show your XBM motion icon from MotionIcon.h
void displayMotionIcon();

// Optional power helpers
void displayOff();   // clear + power Vext OFF
void displayOn();    // power Vext ON (keeps prior buffer)
