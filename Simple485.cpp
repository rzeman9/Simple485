#include "Simple485.h"

Simple485::Simple485(Stream * _serial, uint8_t _addr)
{
  serial = _serial;
  addr = _addr;
}

Simple485::Simple485(Stream * _serial, uint8_t _addr, uint8_t _pin)
{
  serial = _serial;
  addr = _addr;
  pin = _pin;
  pinMode(pin, OUTPUT); 
  digitalWrite(pin, LOW);
}

void Simple485::send(uint8_t dst, uint8_t len, uint8_t * bytes) {
  uint8_t * buff = new uint8_t[2*len + 13];
  buff[0] = LF; buff[1] = LF; buff[2] = LF; buff[3] = SOH;
  buff[4] = dst;
  buff[5] = addr;
  buff[6] = len;
  buff[7] = STX;
  uint8_t b, crc = addr ^ dst ^ len;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= bytes[i];
    b = (bytes[i] & 0xf0);
    b = b | ((~b & 0xf0) >> 4);
    buff[8+2*i] = b;
    b = (bytes[i] & 0x0f);
    b = b | (~b << 4);
    buff[9+2*i] = b;
  }
  buff[8 + 2*len] = ETX;
  buff[9 + 2*len] = crc;
  buff[10 + 2*len] = EOT;
  buff[11 + 2*len] = LF;
  buff[12 + 2*len] = LF;
  Message o = {dst, 2*len + 13, buff};
  outputMessages.add(o);
}

void Simple485::send(Message m) {
  send(m.addr, m.len, m.bytes);
}

void Simple485::send(uint8_t dst, String text) {
  uint8_t * buff = new uint8_t[text.length() + 1];
  text.getBytes(buff, text.length()+1);
  send(dst, text.length() + 1, buff);
  delete [] buff;
}

void Simple485::transmitt() {
  if(millis() > last_receive + LINE_READY_TIME) {
    while(outputMessages.size() > 0) {
      Message o = outputMessages.shift();
      if (pin > 0) digitalWrite(pin, HIGH);
      serial->write(o.bytes, o.len);
      delete [] o.bytes;
      serial->flush();
      if (pin > 0) digitalWrite(pin, LOW);
    }
  }
}

void Simple485::receive() {
  uint8_t b;
  while (serial->available()) {
    last_receive = millis();
    b = serial->read();
    if (b == SOH) {
      stat = 1;       // awainting message
      continue;
    }
    switch (stat) {
      case 0:
        continue;
      case 1:         // awaiting dst address
        dst = b;
        if (dst == addr || dst == 0) stat++; // for us
        else stat = 0;
        break;
      case 2:         // awaiting src address
        src = b;
        stat++;
        break;
      case 3:         // awaiting length
        len = b;
        stat++;
        break;
      case 4:
        if (b == STX) stat++;  // awaiting STX
        else stat = 0;
        delete [] buff;
        buff = new uint8_t[len];
        crc = len ^ dst ^ src;
        first_nibble = true; pos = 0;
        break;
      case 5:         // awaiting payload
        if (((~(b >> 4 | b << 4)) & 0xff) == b) { // expected code
          if (first_nibble) {
            buff[pos] = (b & 0xf0);
            first_nibble = false;
          } else {
            first_nibble = true;
            buff[pos] |= (b & 0x0f);
            crc ^= buff[pos];
            pos ++;
          }
        } else if (b == ETX) {    // termination received
          if (pos == len) stat++; // payload complete
          else stat = 0;          // or drop
        } else stat = 0;
        break;
      case 6:         // awaiting crc
        if (b == crc) stat++; // crc match
        else stat = 0;        // drop
        break;
      case 7:         // awaiting EOT
        if (b == EOT) {
          Message m = {src, len, buff};
          receivedMessages.add(m);
          buff = NULL;
          stat = 0;
        } else stat = 0;
        break;
    }
  }
}

Simple485::~Simple485() {
  delete [] buff;
  Message m;
  while (receivedMessages.size() > 0) {
    m = receivedMessages.shift();
    delete [] m.bytes;
  }
  while (outputMessages.size() > 0) {
    m = outputMessages.shift();
    delete [] m.bytes;
  }
}
