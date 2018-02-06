/*
   Aqua-tan 7 segment x 10 clock

   /status
   /mode
   /wifireset
   /reset
   /reboot
*/

#include "esp_7seg.h" // Configuration parameters

#include <pgmspace.h>
#include <Wire.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <WiFiClient.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <RTClib.h>
#include <FS.h>
#include "ntp.h"
#include "anodecommon_7seg.h"

const String boolstr[2] = {"false", "true"};

String site_name  = "esp_7seg";
const char* apSSID = "CC2038";
boolean settingMode;
String ssidList;

uint32_t timer_count = 0;
uint32_t p_millis;

DNSServer dnsServer;
MDNSResponder mdns;
const IPAddress apIP(192, 168, 1, 1);
ESP8266WebServer webServer(80);

#ifdef USE_DS1307
RTC_DS1307  rtc;
#else
RTC_Millis rtc;
#endif

Anodecommon_7seg clockdisp;
NTP ntp("ntp.nict.jp");

uint8_t dispmode;
uint8_t rtcint, btnint;
/* Int handler */

void RTCHandler() {
  rtcint = 1;
}

void BTNHandler() {
  detachInterrupt(PIN_BTN);
  delayMicroseconds(10000);
  dispmode++;
  dispmode %= NUM_MODES;
  btnint = 1;
  attachInterrupt(PIN_BTN, BTNHandler, FALLING);
}

/* Setup and loop */

void setup() {
  dispmode = 0;
  rtcint = 0;
  p_millis = millis();
  Serial.begin(115200);
  EEPROM.begin(512);
  delay(10);

  pinMode(PIN_BTN, INPUT_PULLUP);
  digitalWrite(PIN_BTN, HIGH);
  attachInterrupt(PIN_BTN, BTNHandler, FALLING);

  Wire.begin(PIN_SDA, PIN_SCL);
  clockdisp.begin(DISPLAY_ADDRESS);
  clockdisp.setBrightness(4);
  clockdisp.clear();
  clockdisp.writeDisplay();
  SPIFFS.begin();

#ifdef USE_DS1307
  if (! rtc.isrunning() ) {
    rtc.adjust(DateTime(2038, 1, 19, 12, 13, 7));
  }
  rtc.writeSqwPinMode(SquareWave1HZ);
  pinMode(PIN_RTC_INT, INPUT_PULLUP);
  digitalWrite(PIN_RTC_INT, HIGH);
  attachInterrupt(PIN_RTC_INT, RTCHandler, FALLING);
#else
  rtc.begin(DateTime(2038, 1, 19, 12, 13, 7));
#endif
#ifdef DEBUG
  Serial.println("RTC began");
#endif
#ifdef Y2038TEST
  Serial.println("60 secs to year 2038");
#endif
  clockdisp.writeDigitRaw(4, 0x40); // -
  clockdisp.writeDigitRaw(5, 0x40); // -
  clockdisp.writeDisplay();

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  if (restoreConfig()) {
    if (checkConnection()) {
      if (mdns.begin(site_name.c_str(), WiFi.localIP())) {
#ifdef DEBUG
        Serial.println("MDNS responder started.");
#endif
      }
      settingMode = false;
    } else {
      settingMode = true;
    }
  } else {
    settingMode = true;
  }
  clockdisp.writeDigitRaw(3, 0x40); // -
  clockdisp.writeDigitRaw(6, 0x40); // -
  clockdisp.writeDisplay();
  if (settingMode == true) {
#ifdef DEBUG
    Serial.println("Setting mode");
#endif
    //WiFi.mode(WIFI_STA);
    //WiFi.disconnect();
#ifdef DEBUG
    Serial.println("Wifi disconnected");
#endif
    delay(100);
    WiFi.mode(WIFI_AP);
#ifdef DEBUG
    Serial.println("Wifi AP mode");
#endif
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(apSSID);
    dnsServer.start(53, "*", apIP);
#ifdef DEBUG
    Serial.println("Wifi AP configured");
#endif
    clockdisp.writeDigitRaw(0, 0x39); // C
    clockdisp.writeDigitRaw(1, 0x5C); // o
    clockdisp.writeDigitRaw(2, 0x54); // n
    clockdisp.writeDigitRaw(3, 0x54); // n
    clockdisp.writeDigitRaw(4, 0x79); // E
    clockdisp.writeDigitRaw(5, 0x58); // c
    clockdisp.writeDigitRaw(6, 0x44); // t
    clockdisp.writeDigitRaw(7, 0x00); //
    clockdisp.writeDigitRaw(8, 0x77); // A
    clockdisp.writeDigitRaw(9, 0x73); // P
    clockdisp.writeDisplay();

    startWebServer_setting();
#ifdef DEBUG
    Serial.print("Starting Access Point at \"");
    Serial.print(apSSID);
    Serial.println("\"");
#endif
  } else {
    ntp.begin();
    clockdisp.writeDigitRaw(2, 0x40); // -
    clockdisp.writeDigitRaw(7, 0x40); // -
    clockdisp.writeDisplay();
    delay(100);
    startWebServer_normal();
#ifdef DEBUG
    Serial.println("Starting normal operation.");
#endif
  }
  ESP.wdtEnable(100);
}


void loop() {
  ESP.wdtFeed();
  if (settingMode) {
    dnsServer.processNextRequest();
  }
  webServer.handleClient();

  if (btnint == 1) {
    EEPROM.write(EEPROM_MODE_ADDR, char(dispmode));
    EEPROM.commit();
    btnint = 0;
  }
  // 1秒毎に実行
#ifdef USE_DS1307
  if (rtcint == 1) {
    rtcint = 0;
#else
  if (millis() > p_millis + 1000) {
    p_millis = millis();
#endif
    if (timer_count % 3600 == 0) {
      uint32_t epoch = ntp.getTime();
      if (epoch > 0) {
#ifndef Y2038TEST
        rtc.adjust(DateTime(epoch + SECONDS_UTC_TO_JST ));
#endif
      }
#ifdef DEBUG
      Serial.print("epoch:");
      Serial.println(epoch);
#endif
    }
    if (!settingMode) {
      DateTime now = rtc.now();
#ifdef DEBUG
      Serial.println(now.unixtime());
#endif
      clockdisp.clear();
      if (dispmode == MODE_UNIXTIME) {
        dispDigits(now.unixtime()-SECONDS_UTC_TO_JST);
      } else if (dispmode == MODE_COUNTDOWN) {
//        dispDigits((int32_t)(0x7FFFFFFF - (now.unixtime()-SECONDS_UTC_TO_JST)));
        dispDigits((0x7FFFFFFF - (now.unixtime()-SECONDS_UTC_TO_JST))); // y 2038!
      } else if (dispmode == MODE_CLOCK) {
        dispClock(now);
      } else if (dispmode == MODE_DATE) {
        dispDate(now);
      }
      clockdisp.writeDisplay();
    } else {
      if (timer_count % 3 == 0) {
        clockdisp.clear();
        clockdisp.writeDigitRaw(0, 0x39); // C
        clockdisp.writeDigitRaw(1, 0x5C); // o
        clockdisp.writeDigitRaw(2, 0x54); // n
        clockdisp.writeDigitRaw(3, 0x54); // n
        clockdisp.writeDigitRaw(4, 0x79); // E
        clockdisp.writeDigitRaw(5, 0x58); // c
        clockdisp.writeDigitRaw(6, 0x44); // t
        clockdisp.writeDigitRaw(7, 0x00); //
        clockdisp.writeDigitRaw(8, 0x77); // A
        clockdisp.writeDigitRaw(9, 0xF3); // P.
      } else {
        clockdisp.clear();
        clockdisp.writeDigitRaw(2, 0x39); // C
        clockdisp.writeDigitRaw(3, 0x39); // C
        clockdisp.writeDigitNum(4, 2, 0); // 2
        clockdisp.writeDigitNum(5, 0, 0); // 0
        clockdisp.writeDigitNum(6, 3, 0); // 3
        clockdisp.writeDigitNum(7, 8, 0); // 8               
      }
      clockdisp.writeDisplay();
    }
    delay(5);
    timer_count++;
    timer_count %= (86400UL);
  }
  delay(10);
}

void dispDigits(int64_t digits) {
#ifdef DEBUG
      Serial.println((int32_t)digits);
#endif
  uint8_t sign = 0;
  uint8_t pos = SEVENSEG_DIGITS - 1;
  if (digits < 0) {
    sign = 1;
    digits = 0 - digits;
  } else if (digits == 0) {
    clockdisp.writeDigitNum(pos, 0, 0);
  }
  while (digits > 0) {
    int64_t d = digits % 10;
    clockdisp.writeDigitNum(pos, d, 0);
    digits = digits / 10;
    pos--;
  }
  if (sign) {
    clockdisp.writeDigitRaw(pos, 0x40); // show '-'
  }
}

void dispDigitsPos(int64_t digits, int8_t pos = SEVENSEG_DIGITS - 1, int8_t size = 0, uint8_t dot = 0) {
  uint8_t sign = 0;
  if (digits < 0) {
    sign = 1;
    digits = 0 - digits;
  } else if (digits == 0) {
    clockdisp.writeDigitNum(pos, 0, dot);
    dot = 0;
    pos--;
    size--;
  }
  while (digits > 0) {
    int d = digits % 10;
    clockdisp.writeDigitNum(pos, d, dot);
    dot = 0;
    digits = digits / 10;
    pos--;
    size--;
  }
  while (size > 0) {
    clockdisp.writeDigitNum(pos, 0, 0);
    pos--;
    size--;
  }
  if (sign) {
    clockdisp.writeDigitRaw(pos, 0x40); // show '-'
  }
}

void dispClock(DateTime now) {
  dispDigitsPos(now.hour(), 2, 2, 0);
  dispDigitsPos(now.minute(), 5, 2, 0);
  dispDigitsPos(now.second(), 8, 2, 0);
}

void dispDate(DateTime now) {
  dispDigitsPos(now.year(), 3, 4);
  clockdisp.writeDigitRaw(4, 0x40); // show '-'
  dispDigitsPos(now.month(), 6, 2);
  clockdisp.writeDigitRaw(7, 0x40); // show '-'
  dispDigitsPos(now.day(),  9, 2, now.second() % 2 ? 1 : 0);
}


String timestamp(DateTime now) {
  String ts;
  char str[20];
  sprintf(str, "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  ts = str;
  return ts;
}

/***************************************************************
   EEPROM restoring functions
 ***************************************************************/

boolean restoreConfig() {
#ifdef DEBUG
  Serial.println("Reading EEPROM...");
#endif
  String ssid = "";
  String pass = "";

  // Initialize on first boot
  if (EEPROM.read(0) == 255) {
#ifdef DEBUG
    Serial.println("Initialize EEPROM...");
#endif
    for (int i = 0; i < EEPROM_LAST_ADDR; ++i) {
      EEPROM.write(i, 0);
    }
#ifdef DEBUG
    Serial.println("Erasing EEPROM...");
#endif
    EEPROM.commit();
  }

#ifdef DEBUG
  Serial.println("Reading EEPROM(2)...");
#endif

  if (EEPROM.read(EEPROM_SSID_ADDR) != 0) {
    for (int i = EEPROM_SSID_ADDR; i < EEPROM_SSID_ADDR + 32; ++i) {
      ssid += char(EEPROM.read(i));
    }
    for (int i = EEPROM_PASS_ADDR; i < EEPROM_PASS_ADDR + 64; ++i) {
      pass += char(EEPROM.read(i));
    }
#ifdef DEBUG
    Serial.print("ssid:");
    Serial.println(ssid);
    Serial.print("pass:");
    Serial.println(pass);
#endif
    delay(100);
    WiFi.begin(ssid.c_str(), pass.c_str());
#ifdef DEBUG
    Serial.println("WiFi started");
#endif
    delay(100);
    if (EEPROM.read(EEPROM_MDNS_ADDR) != 0) {
      site_name = "";
      for (int i = 0; i < 32; ++i) {
        byte c = EEPROM.read(EEPROM_MDNS_ADDR + i);
        if (c == 0) {
          break;
        }
        site_name += char(c);
      }
    }

    int e_mode  = EEPROM.read(EEPROM_MODE_ADDR);
    dispmode = e_mode % NUM_MODES;

    return true;
  } else {
#ifdef DEBUG
    Serial.println("restore config fails...");
#endif
    return false;
  }
}

/***************************************************************
   Network functions
 ***************************************************************/

boolean checkConnection() {
  int count = 0;
  while ( count < 60 ) {
    if (WiFi.status() == WL_CONNECTED) {
      return true;
    }
    delay(500);
#ifdef DEBUG
    Serial.print(".");
#endif
    count++;
  }
  //  Serial.println("Timed out.");
  return false;
}

/***********************************************************
   WiFi Client functions

 ***********************************************************/

/***************************************************************
   Web server functions
 ***************************************************************/

void startWebServer_setting() {
#ifdef DEBUG
  Serial.print("Starting Web Server at ");
  Serial.println(WiFi.softAPIP());
#endif
  webServer.on("/pure.css", handleCss);
  webServer.on("/setap", []() {
#ifdef DEBUG
    Serial.print("Set AP ");
    Serial.println(WiFi.softAPIP());
#endif
    for (int i = 0; i < EEPROM_MDNS_ADDR; ++i) {
      EEPROM.write(i, 0);
    }
    String ssid = urlDecode(webServer.arg("ssid"));
    String pass = urlDecode(webServer.arg("pass"));
    String site = urlDecode(webServer.arg("site"));
    for (int i = 0; i < ssid.length(); ++i) {
      EEPROM.write(EEPROM_SSID_ADDR + i, ssid[i]);
    }
    for (int i = 0; i < pass.length(); ++i) {
      EEPROM.write(EEPROM_PASS_ADDR + i, pass[i]);
    }
    if (site != "") {
      for (int i = EEPROM_MDNS_ADDR; i < EEPROM_MDNS_ADDR + 32; ++i) {
        EEPROM.write(i, 0);
      }
      for (int i = 0; i < site.length(); ++i) {
        EEPROM.write(EEPROM_MDNS_ADDR + i, site[i]);
      }
    }
    EEPROM.commit();
    String s = "<h2>Setup complete</h2><p>Device will be connected to \"";
    s += ssid;
    s += "\" after the restart.</p><p>Your computer also need to re-connect to \"";
    s += ssid;
    s += "\".</p><p><button class=\"pure-button\" onclick=\"return quitBox();\">Close</button></p>";
    s += "<script>function quitBox() { open(location, '_self').close();return false;};setTimeout(\"quitBox()\",10000);</script>";
    webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
    timer_count = 0;
  });
  webServer.onNotFound([]() {
#ifdef DEBUG
    Serial.println("captive webpage ");
#endif
    int n = WiFi.scanNetworks();
    delay(100);
    ssidList = "";
    for (int i = 0; i < n; ++i) {
      ssidList += "<option value=\"";
      ssidList += WiFi.SSID(i);
      ssidList += "\">";
      ssidList += WiFi.SSID(i);
      ssidList += "</option>";
    }
    String s = R"=====(
<div class="l-content">
<div class="l-box">
<h3 class="if-head">WiFi Setting</h3>
<p>Please enter your password by selecting the SSID.<br />
You can specify site name for accessing a name like http://aquamonitor.local/</p>
<form class="pure-form pure-form-stacked" method="get" action="setap" name="tm"><label for="ssid">SSID: </label>
<select id="ssid" name="ssid">
)=====";
    s += ssidList;
    s += R"=====(
</select>
<label for="pass">Password: </label><input id="pass" name="pass" length=64 type="password">
<label for="site" >Site name: </label><input id="site" name="site" length=32 type="text" placeholder="Site name">
<button class="pure-button pure-button-primary" type="submit">Submit</button></form>
</div>
</div>
)=====";
  webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
  });
  webServer.begin();
}

/*
 * Web server for normal operation
 */
void startWebServer_normal() {
#ifdef DEBUG
  Serial.print("Starting Web Server at ");
  Serial.println(WiFi.localIP());
#endif
  webServer.on("/reset", []() {
    for (int i = 0; i < EEPROM_LAST_ADDR; ++i) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
    String s = "<h3 class=\"if-head\">Reset ALL</h3><p>Cleared all settings. Please reset device.</p>";
    s += "<p><button class=\"pure-button\" onclick=\"return quitBox();\">Close</button></p>";
    s += "<script>function quitBox() { open(location, '_self').close();return false;};</script>";
    webServer.send(200, "text/html", makePage("Reset ALL Settings", s));
    timer_count = 0;
  });
  webServer.on("/wifireset", []() {
    for (int i = 0; i < EEPROM_MDNS_ADDR; ++i) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
    String s = "<h3 class=\"if-head\">Reset WiFi</h3><p>Cleared WiFi settings. Please reset device.</p>";
    s += "<p><button class=\"pure-button\" onclick=\"return quitBox();\">Close</button></p>";
    s += "<script>function quitBox() { open(location, '_self').close();return false;}</script>";
    webServer.send(200, "text/html", makePage("Reset WiFi Settings", s));
    timer_count = 0;
  });
  webServer.on("/", handleRoot);
  webServer.on("/pure.css", handleCss);
  webServer.on("/reboot", handleReboot);
  webServer.on("/mode", handleActionDispmode);
//  webServer.on("/off", handleActionOff);
//  webServer.on("/power", handleActionPower);
  webServer.on("/status", handleStatus);
//  webServer.on("/config", handleConfig);
  webServer.begin();
}

void handleRoot() {
  send_fs("/index.html","text/html");  
}

void handleCss() {
  send_fs("/pure.css","text/css");  
}

void handleReboot() {
  String message;
  message = "{reboot:\"done\"}";
  webServer.send(200, "application/json", message);
  ESP.restart();
}


void handleStatus() {
  String message;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  DateTime now = rtc.now();
  int32_t countdown = (0x7FFFFFFF - (now.unixtime()-SECONDS_UTC_TO_JST));
  json["unixtime"] = now.unixtime();
  json["countdown"] = countdown;
  json["mode"] = dispmode;
  json.printTo(message);
  webServer.send(200, "application/json", message);
}

void handleActionDispmode() {
  String message, argname, argv;
  int p = -1,err;

  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  for (int i = 0; i < webServer.args(); i++) {
    argname = webServer.argName(i);
    argv=webServer.arg(i);
    if (argname == "value") {
      p = argv.toInt();
    }
  }
  if (p >= 0) {
    dispmode = p % NUM_MODES;
  } else {
    dispmode = 0;    
  }
  EEPROM.write(EEPROM_MODE_ADDR, char(dispmode));
  EEPROM.commit();

  json["mode"] = dispmode;
  json.printTo(message);
  webServer.send(200, "application/json", message);
}
/*
void handleActionPower() {
  String message;

  // on
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  int p = webServer.arg("value").toInt();

  int err = light.control(p);
  if (err < 0) {
    json["error"] = "Cannot turn on/off light while schedule is set.";
  }
  json["power"] = light.power();
  json["status"] = light.status();
  json["timestamp"] = timestamp();  
  json.printTo(message);
  webServer.send(200, "application/json", message);
}

void handleActionOff() {
  String message;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  // off
  int err = light.control(0);
  if (err < 0) {
    json["error"] = "Cannot turn on/off light while schedule is set.";
  }
  json["power"] = light.power();
  json["status"] = light.status();
  json["timestamp"] = timestamp();  
  json.printTo(message);
  webServer.send(200, "application/json", message);
}

void handleConfig() {
  String message,argname,argv;
  int en,on_h,on_m,off_h,off_m,dim;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  on_h = on_m = off_h = off_m = dim = -1;

  for (int i = 0; i < webServer.args(); i++) {
    argname = webServer.argName(i);
    argv = webServer.arg(i);
#ifdef DEBUG
    Serial.print("argname:");
    Serial.print(argname);
    Serial.print(" = ");
    Serial.println(argv);
#endif
    if (argname == "enable") {
       en = (argv ==  "true" ? 1 : 0);
       light.enable(en);
       EEPROM.write(EEPROM_SCHEDULE_ADDR, char(light.enable()));
       EEPROM.commit();
   } else if (argname == "on_h") {
       on_h  = argv.toInt();
    } else if (argname == "on_m") {
       on_m  = argv.toInt();
    } else if (argname == "off_h") {
       off_h  = argv.toInt();
    } else if (argname == "off_m") {
       off_m  = argv.toInt();
    } else if (argname == "dim") {
       dim  = argv.toInt();
       light.dim(dim);
       EEPROM.write(EEPROM_DIM_ADDR,   char(light.dim() & 0xFF));
       EEPROM.write(EEPROM_DIM_ADDR+1,   char(light.dim() >> 8));
       EEPROM.commit();
    }
  }

  if (!(on_h < 0 || on_m < 0 || off_h < 0 || off_m < 0)) {
    light.schedule(on_h,on_m,off_h,off_m);
    EEPROM.write(EEPROM_SCHEDULE_ADDR + 1, char(light.on_h()));
    EEPROM.write(EEPROM_SCHEDULE_ADDR + 2, char(light.on_m()));
    EEPROM.write(EEPROM_SCHEDULE_ADDR + 3, char(light.off_h()));
    EEPROM.write(EEPROM_SCHEDULE_ADDR + 4, char(light.off_m()));
    EEPROM.commit();
  }
  
  json["enable"] = boolstr[light.enable()];
  json["dim"] = light.dim();
  json["on_h"] = light.on_h();
  json["on_m"] = light.on_m();
  json["off_h"] = light.off_h();
  json["off_m"] = light.off_m();
  json["timestamp"] = timestamp();  
  json.printTo(message);
  webServer.send(200, "application/json", message);
}
*/

void send_fs (String path,String contentType) {
  if(SPIFFS.exists(path)){
    File file = SPIFFS.open(path, "r");
    size_t sent = webServer.streamFile(file, contentType);
    file.close();
  } else{
    webServer.send(500, "text/plain", "BAD PATH");
  }  
}

String makePage(String title, String contents) {
  String s = R"=====(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<link rel="stylesheet" href="/pure.css">
)=====";  
  s += "<title>";
  s += title;
  s += "</title></head><body>";
  s += contents;
  s += R"=====(
<div class="footer l-box">
<p>WiFi 10x7Seg clock by @omzn 2018 / All rights researved</p>
</div>
)=====";
  s += "</body></html>";
  return s;
}

String urlDecode(String input) {
  String s = input;
  s.replace("%20", " ");
  s.replace("+", " ");
  s.replace("%21", "!");
  s.replace("%22", "\"");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%25", "%");
  s.replace("%26", "&");
  s.replace("%27", "\'");
  s.replace("%28", "(");
  s.replace("%29", ")");
  s.replace("%30", "*");
  s.replace("%31", "+");
  s.replace("%2C", ",");
  s.replace("%2E", ".");
  s.replace("%2F", "/");
  s.replace("%2C", ",");
  s.replace("%3A", ":");
  s.replace("%3A", ";");
  s.replace("%3C", "<");
  s.replace("%3D", "=");
  s.replace("%3E", ">");
  s.replace("%3F", "?");
  s.replace("%40", "@");
  s.replace("%5B", "[");
  s.replace("%5C", "\\");
  s.replace("%5D", "]");
  s.replace("%5E", "^");
  s.replace("%5F", "-");
  s.replace("%60", "`");
  return s;
}

