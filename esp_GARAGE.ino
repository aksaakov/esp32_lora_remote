#include "LoraSuite.h"
#include "DisplaySuite.h"


// void VextON() {
//   pinMode(Vext, OUTPUT);
//   digitalWrite(Vext, LOW);  // LOW = ON
// }

int pirPin = 48;
// int pirValue; 

void setup() {
  Serial.begin(115200);
  // VextON();
  delay(200);

  displayInit();
  displayLogo();
  pinMode(pirPin, INPUT); 
}

void loop() {
  // Send
  // const char* msg = "PING message";
  // if (loraSend((const uint8_t*)msg, strlen(msg), 3000)) {
  //   Serial.println("TX OK");
  // } else {
  //   Serial.println("TX FAIL");
  // }

  // Receive for up to 1500 ms
  // uint8_t buf[128];
  // int16_t rssi; int8_t snr;
  // int n = loraReceive(buf, sizeof(buf), &rssi, &snr, 1500);
  // if (n > 0) {
  //   Serial.println((char*)buf); 
  // }

  int state = digitalRead(pirPin);  // Read sensor
  Serial.println(state);
  if (state == HIGH) {
    // if (!displayOn) {
    //   displayOn();     // Turn display on if it's off
    //   // displayOn = true;
    // }
    // count++;
    String message = "Motion Detected. ";
    Serial.println(message);
    displayMotion();
  }


  // displayOn();
  // displayShow("TEST TEXT");
  // delay(1000);
  delay(1000);
}
