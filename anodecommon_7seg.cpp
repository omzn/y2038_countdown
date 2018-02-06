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
 * for use of 10 x anode-common 7 segment LEDs (Anodecommon_7seg) 
 * For this purpose, we use A0-A9 to select digits and C0-C7 to select segments.
 * 
 * Written by Osamu Mizuno
 */


#include <Wire.h>
#include "anodecommon_7seg.h"

#ifndef _BV
#define _BV(bit) (1<<(bit))
#endif

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif

/*
 *    --0
     |5 |1
      --6
     |4 |2
      --3
*/

static const uint8_t numbertable[] = {
  0x3F, /* 0 */
  0x06, /* 1 */
  0x5B, /* 2 */
  0x4F, /* 3 */
  0x66, /* 4 */
  0x6D, /* 5 */
  0x7D, /* 6 */
  0x07, /* 7 */
  0x7F, /* 8 */
  0x6F, /* 9 */
  0x77, /* a */
  0x7C, /* b */
  0x39, /* C */
  0x5E, /* d */
  0x79, /* E */
  0x71, /* F */
};

// o -> 0x5c
// n -> 0x54
// E -> 0x79
// c -> 0x58
// t -> 0x44
// L -> 0x38

/******************************* 7 SEGMENT x 10 OBJECT */

Anodecommon_7seg::Anodecommon_7seg(void) {
  position = 0;
}

void Anodecommon_7seg::print(unsigned long n, int base)
{
  if (base == 0) write(n);
  else printNumber(n, base);
}

void Anodecommon_7seg::print(char c, int base)
{
  print((long) c, base);
}

void Anodecommon_7seg::print(unsigned char b, int base)
{
  print((unsigned long) b, base);
}

void Anodecommon_7seg::print(int n, int base)
{
  print((long) n, base);
}

void Anodecommon_7seg::print(unsigned int n, int base)
{
  print((unsigned long) n, base);
}

void  Anodecommon_7seg::println(void) {
  position = 0;
}

void  Anodecommon_7seg::println(char c, int base)
{
  print(c, base);
  println();
}

void  Anodecommon_7seg::println(unsigned char b, int base)
{
  print(b, base);
  println();
}

void  Anodecommon_7seg::println(int n, int base)
{
  print(n, base);
  println();
}

void  Anodecommon_7seg::println(unsigned int n, int base)
{
  print(n, base);
  println();
}

void  Anodecommon_7seg::println(long n, int base)
{
  print(n, base);
  println();
}

void  Anodecommon_7seg::println(unsigned long n, int base)
{
  print(n, base);
  println();
}

void  Anodecommon_7seg::println(double n, int digits)
{
  print(n, digits);
  println();
}

void  Anodecommon_7seg::print(double n, int digits)
{
  printFloat(n, digits);
}


size_t Anodecommon_7seg::write(uint8_t c) {

  uint8_t r = 0;

  if (c == '\n') position = 0;
  if (c == '\r') position = 0;

  if ((c >= '0') && (c <= '9')) {
    writeDigitNum(position, c - '0');
    r = 1;
  }

  position++;
  if (position == 2) position++;

  return r;
}

void Anodecommon_7seg::writeDigitRaw(uint8_t d, uint8_t bitmask) {
  if (d > 9) return;
  displaybuffer[d] = bitmask;
}

void Anodecommon_7seg::writeDigitNum(uint8_t d, uint8_t num, boolean dot) {
  if (d > 9) return;

  writeDigitRaw(d, numbertable[num] | (dot << 7));
}

void Anodecommon_7seg::print(long n, int base)
{
  printNumber(n, base);
}

void Anodecommon_7seg::printNumber(long n, uint8_t base)
{
  printFloat(n, 0, base);
}

void Anodecommon_7seg::printFloat(double n, uint8_t fracDigits, uint8_t base)
{
  uint8_t numericDigits = SEVENSEG_DIGITS;   // available digits on display
  boolean isNegative = false;  // true if the number is negative

  // is the number negative?
  if (n < 0) {
    isNegative = true;  // need to draw sign later
    --numericDigits;    // the sign will take up one digit
    n *= -1;            // pretend the number is positive
  }

  // calculate the factor required to shift all fractional digits
  // into the integer part of the number
  double toIntFactor = 1.0;
  for (int i = 0; i < fracDigits; ++i) toIntFactor *= base;

  // create integer containing digits to display by applying
  // shifting factor and rounding adjustment
  uint32_t displayNumber = n * toIntFactor + 0.5;

  // calculate upper bound on displayNumber given
  // available digits on display
  uint32_t tooBig = 1;
  for (int i = 0; i < numericDigits; ++i) tooBig *= base;

  // if displayNumber is too large, try fewer fractional digits
  while (displayNumber >= tooBig) {
    --fracDigits;
    toIntFactor /= base;
    displayNumber = n * toIntFactor + 0.5;
  }

  // did toIntFactor shift the decimal off the display?
  if (toIntFactor < 1) {
    printError();
  } else {
    // otherwise, display the number
    int8_t displayPos = SEVENSEG_DIGITS - 1;

    if (displayNumber)  //if displayNumber is not 0
    {
      for (uint8_t i = 0; displayNumber || i <= fracDigits; ++i) {
        boolean displayDecimal = (fracDigits != 0 && i == fracDigits);
        writeDigitNum(displayPos--, displayNumber % base, displayDecimal);
        displayNumber /= base;
      }
    }
    else {
      writeDigitNum(displayPos--, 0, false);
    }

    // display negative sign if negative
    if (isNegative) writeDigitRaw(displayPos--, 0x40);

    // clear remaining display positions
    while (displayPos >= 0) writeDigitRaw(displayPos--, 0x00);
  }
}

void Anodecommon_7seg::printError(void) {
  for (uint8_t i = 0; i < SEVENSEG_DIGITS; ++i) {
    writeDigitRaw(i, 0x40);
  }
}

void Anodecommon_7seg::clear(void) {
  for (uint8_t i = 0; i < SEVENSEG_DIGITS; i++) {
    displaybuffer[i] = 0;
  }
}
/*
  address
  00 -> 00(0)02(0)04(0)06(0)08(0)0a(0)0c(0)0e(0)
  01 -> 00(1)02(1)04(1)...0e(1)
  ...
  07 -> 00(7)02(7)04(7)...0e(7)

  08 -> 01(0)03(0)05(0)...0f(0)
  09 -> 01(1)03(1)05(1)...0f(1)
  ...
  0f -> 01(7)03(7)05(7)...0f(7)
*/
void Anodecommon_7seg::writeDisplay(void) {
  uint8_t c[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  // convert displaybuffer(cathode) to new buffer c(anode)
  for (uint8_t address = 0; address < SEVENSEG_DIGITS; address++) {
    uint8_t val = displaybuffer[address] & 0xFF;
    for (uint8_t i = 0; i < 8; i++) {
      if (val >> i & 1) {
        c[(address / 8) + i * 2] |= 1 << (address % 8);
      }
    }
  }
  Wire.beginTransmission(i2c_addr);
  Wire.write((uint8_t)0x00); // start at address $00
  for (uint8_t i = 0; i < 16; i += 2) {
    Wire.write(c[i]);
    Wire.write(c[i + 1]);
  }
  Wire.endTransmission();
}
