#ifndef ADAFRUIT_FINGERPRINT_H
#define ADAFRUIT_FINGERPRINT_H

#include "application.h"

/***************************************************
  This is a library for our optical Fingerprint sensor

  Designed specifically to work with the Adafruit Fingerprint sensor
  ----> http://www.adafruit.com/products/751

  These displays use TTL Serial to communicate, 2 pins are required to
  interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/



#define DEFAULTTIMEOUT 1000  ///< UART reading timeout in milliseconds

///! Helper class to craft UART packets
struct Adafruit_Fingerprint_Packet {

/**************************************************************************/
/*!
    @brief   Create a new UART-borne packet
    @param   type Command, data, ack type packet
    @param   length Size of payload
    @param   data Pointer to bytes of size length we will memcopy into the internal buffer
*/
/**************************************************************************/

  Adafruit_Fingerprint_Packet(uint8_t type, uint16_t length, uint8_t * data) {
    this->start_code = FINGERPRINT_STARTCODE;
    this->type = type;
    this->length = length;
    address[0] = 0xFF; address[1] = 0xFF;
    address[2] = 0xFF; address[3] = 0xFF;
    if(length<64)
      memcpy(this->data, data, length);
    else
      memcpy(this->data, data, 64);
  }

};

///! Helper class to communicate with and keep state for fingerprint sensors
class Adafruit_Fingerprint {
 public:
#if defined(__AVR__) || defined(ESP8266) || defined(FREEDOM_E300_HIFIVE1)
  Adafruit_Fingerprint(SoftwareSerial *ss, uint32_t password = 0x0);
#endif
  Adafruit_Fingerprint(HardwareSerial *hs, uint32_t password = 0x0);

  void begin(uint32_t baud);

  bool verifyPassword(void);
  uint8_t getImage(void);
  uint8_t image2Tz(uint8_t slot = 1);
  uint8_t createModel(void);

  uint8_t emptyDatabase(void);
  uint8_t storeModel(uint16_t id);
  uint8_t loadModel(uint16_t id);
  uint8_t getModel(void);
  uint8_t deleteModel(uint16_t id);
  uint8_t fingerFastSearch(void);
  uint8_t getTemplateCount(void);
  uint8_t setPassword(uint32_t password);
  void writeStructuredPacket(const Adafruit_Fingerprint_Packet & p);
  uint8_t getStructuredPacket(Adafruit_Fingerprint_Packet * p, uint16_t timeout=DEFAULTTIMEOUT);

  /// The matching location that is set by fingerFastSearch()
  uint16_t fingerID;
  /// The confidence of the fingerFastSearch() match, higher numbers are more confidents
  uint16_t confidence;
  /// The number of stored templates in the sensor, set by getTemplateCount()
  uint16_t templateCount;

 private:
  uint8_t checkPassword(void);
  uint32_t thePassword;
  uint32_t theAddress;
    uint8_t recvPacket[20];

  Stream *mySerial;
#if defined(__AVR__) || defined(ESP8266) || defined(FREEDOM_E300_HIFIVE1)
  SoftwareSerial *swSerial;
#endif
  HardwareSerial *hwSerial;
};

#endif
