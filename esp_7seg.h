#ifndef ESP_7SEG_H
#define ESP_7SEG_H

//#define DEBUG
/* 
 *  For testing the year 2038 problem,
 *  uncomment '#define Y2038TEST' and comment out '#define USE_DS1307' 
 */
//#define Y2038TEST
#define USE_DS1307

#define PIN_SDA          (4)
#define PIN_SCL          (5)
#define PIN_BTN          (0)
#define PIN_RTC_INT      (2)

#define DISPLAY_ADDRESS  (0x70)
#define NUM_MODES        (5)

enum {MODE_COUNTDOWN=0, MODE_UNIXTIME, MODE_DATE, MODE_CLOCK, MODE_USER_COUNTDOWN};

//#define PIN_LIGHT         (15)
// pin #15 is hw pulled down

//#define PIN_RX             (4)
//#define PIN_TX            (13)
//#define PIN_TFT_DC       (4)
//#define PIN_TFT_CS      (15)
//#define PIN_SD_CS       (16)
//#define PIN_SPI_CLK   (14)
//#define PIN_SPI_MOSI  (13)
//#define PIN_SPI_MISO  (12)

#define EEPROM_SSID_ADDR             (0)
#define EEPROM_PASS_ADDR            (32)
#define EEPROM_MDNS_ADDR            (96)
#define EEPROM_MODE_ADDR           (128)
#define EEPROM_TZ_ADDR             (129)
#define EEPROM_UCD_ADDR            (133)
#define EEPROM_LAST_ADDR           (137)

#endif
