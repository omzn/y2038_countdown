/*************************************************** 
  This is a library for our I2C LED Backpacks

  Designed specifically to work with the Adafruit LED Matrix backpacks 
  ----> http://www.adafruit.com/products/
  ----> http://www.adafruit.com/products/

  These displays use I2C to communicate, 2 pins are required to 
  interface. There are multiple selectable I2C addresses. For backpacks
  with 2 Address Select pins: 0x70, 0x71, 0x72 or 0x73. For backpacks
  with 3 Address Select pins: 0x70 thru 0x77

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  MIT license, all text above must be included in any redistribution
 ****************************************************/

/*
 * This version of Adafruit_LEDBackpack is modified 
 * for use of 10 x anode-common 7 segment LEDs (Adafruit_7segment_10) 
 * For this purpose, we use A0-A9 to select digits and C0-C7 to select segments.
 * 
 * Written by Osamu Mizuno
 */
 
#ifndef anodecommon_7seg_h
#define anodecommon_7seg_h

#include "Adafruit_LEDBackpack.h"

#define SEVENSEG_DIGITS 10

class Anodecommon_7seg : public Adafruit_LEDBackpack {
 public:
  Anodecommon_7seg(void);
  size_t write(uint8_t c);
  void clear();
  void print(char, int = BYTE);
  void print(unsigned char, int = BYTE);
  void print(int, int = DEC);
  void print(unsigned int, int = DEC);
  void print(long, int = DEC);
  void print(unsigned long, int = DEC);
  void print(double, int = 2);
  void println(char, int = BYTE);
  void println(unsigned char, int = BYTE);
  void println(int, int = DEC);
  void println(unsigned int, int = DEC);
  void println(long, int = DEC);
  void println(unsigned long, int = DEC);
  void println(double, int = 2);
  void println(void);
  
  void writeDigitRaw(uint8_t x, uint8_t bitmask);
  void writeDigitNum(uint8_t x, uint8_t num, boolean dot = false);
  void printNumber(long, uint8_t = 2);
  void printFloat(double, uint8_t = 2, uint8_t = DEC);
  void printError(void);
    
  void writeDisplay(void);
  uint16_t displaybuffer[SEVENSEG_DIGITS];
 private:
  uint8_t position;
};
#endif

