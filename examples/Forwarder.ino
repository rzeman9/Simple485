// Simple485 Forwarder Example
//
// Input from serial port is forwarded to RS485 to specifided destination and vice versa.
// RS485 on software serial to keep default serial for user's input

#include "Simple485.h"
#include <AltSoftSerial.h>

// AltSoftSerial
// Arduino Uno
// TX 9 RX 8

#define BAUDRATE 9600
#define REDEPIN 7

Simple485 rs485;
AltSoftSerial altSerial;

String s = "";
uint8_t dst = 0;
uint8_t addr;

void setup() {
  Serial.begin(BAUDRATE);
  while (!Serial);
  altSerial.begin(BAUDRATE);

  randomSeed(analogRead(0));
  addr = random(1, 256);
  rs485 = Simple485(&altSerial, addr, REDEPIN);
  
  Serial.println("Ready. Addr " + String(addr));
}

void loop() {
  rs485.loop();

  // from Serial to RS485
  char c;
  while (Serial.available() > 0) {
    c = Serial.read();
    if (c == 10) {
      if (s.startsWith("to=")) {
        dst = s.substring(3).toInt();
        Serial.println("New dst: " + String(dst));
      } else {
        uint8_t * b = new uint8_t[s.length()+1];
        s.getBytes(b, s.length()+1);
        rs485.send(dst, s.length()+1, b);
        Serial.println("Sending " + String(s.length()+1) + " bytes");
      }
      s = "";
    } else {
      s += c;
    }
  }

  // RS485 to Serial
  while (rs485.received() > 0) {
    Message m = rs485.read();
    Serial.println("Message: src: " + String(m.addr) + ", len: " + String(m.len) + ".");
    Serial.write(m.bytes, m.len);
    Serial.println("");
    delete [] m.bytes;
  }
  
}
