#include "LoraSuite.h"
#include "DisplaySuite.h"

uint8_t motion_pkg[] = { 0x01 };

void sendMotionMessage() {
  loraSend(motion_pkg, sizeof(motion_pkg), 3000);
}

//receiving strings now TODO: change to bytes
void startReceiver() {
  Serial.println("TODO: implement receiver");
  // uint8_t buf[128];
  // int16_t rssi; int8_t snr;
  // int n = loraReceive(buf, sizeof(buf), &rssi, &snr, 30*1000);
  // if (n > 0) {
  //   Serial.println((char*)buf); 
  // } else {
  //   Serial.println("No messages received");
  // };
}

int pirPin = 48;

void setup() {
  Serial.begin(115200);
  // VextON();
  delay(200);

  // displayInit();
  displayLogo();
  pinMode(pirPin, INPUT);
}

void loop() {
  int state = digitalRead(pirPin);  // Read sensor
  Serial.println(state);
  if (state == HIGH) {

    String message = "Motion Detected. ";
    Serial.println(message);
    displayMotionIcon();
    sendMotionMessage();
    Serial.println("GOT HERE");
    startReceiver();
  }

  delay(1000);
}

  // print text on display:
  // displayShow(2000, "HEADING TEXT", "detail text");

  // Send lora strings:
  // const char* msg = "PING message";
  // if (loraSend((const uint8_t*)msg, strlen(msg), 3000)) {
  //   Serial.println("TX OK");
  // } else {
  //   Serial.println("TX FAIL");
  // }

  // Receive lora strings:
  // uint8_t buf[128];
  // int16_t rssi; int8_t snr;
  // int n = loraReceive(buf, sizeof(buf), &rssi, &snr, 1500);
  // if (n > 0) {
  //   Serial.println((char*)buf); 
  // }
