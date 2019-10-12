#ifndef Simple485_h
#define Simple485_h

#include "Arduino.h"
#include <LinkedList.h>

#define SOH 1
#define STX 2
#define ETX 3
#define EOT 4
#define LF 10
#define LINE_READY_TIME 10

typedef struct Message {
  uint8_t addr;
  uint8_t len;
  uint8_t * bytes;
} Message;

class Simple485
{
  public:
    Simple485() {};
    Simple485(Stream * serial, uint8_t addr);
    Simple485(Stream * serial, uint8_t addr, uint8_t pin);
    ~Simple485();
    void receive();
    void transmitt();
    void loop() { receive(); transmitt(); }
    void send(uint8_t dst, uint8_t len, uint8_t * bytes);
    void send(Message message);
    void send(uint8_t dst, String text);
    int received() { return receivedMessages.size(); }
    Message read() { return receivedMessages.shift(); }
  private:
    Stream * serial;
    uint8_t addr;
    LinkedList<Message> outputMessages = LinkedList<Message>();
    LinkedList<Message> receivedMessages = LinkedList<Message>();
    unsigned long last_receive = 0;
    uint8_t stat = 0, len, src, crc, pos, dst;
    bool first_nibble = true;
    uint8_t * buff = NULL;
    uint8_t pin = 0;
};

#endif
