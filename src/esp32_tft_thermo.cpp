/*

  --------------------------- NOTE ----------------------------------------
  The free font encoding format does not lend itself easily to plotting
  the background without flicker. For values that changes on screen it is
  better to use Fonts 1- 8 which are encoded specifically for rapid
  drawing with background.
  -------------------------------------------------------------------------

  #########################################################################
  ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
  ######       TO SELECT YOUR DISPLAY TYPE AND ENABLE FONTS          ######
  #########################################################################
*/

#include "esp32_tft_thermo.h"
//#include "esp32_secrets_default.h"
#include "esp32_secrets.h"
#include <WiFiClient.h>
#include <WebServer.h>

#include <logging.hpp>
#include <ets-appender.hpp>

ESP32TftThermo& thermoapp = ESP32TftThermo::getInstance();
WiFiClient ESP32TftThermo::espClient = WiFiClient();
WebServer ESP32TftThermo::webServer(80);


void ESP32TftThermo::get_network_info(){
    if(WiFi.status() == WL_CONNECTED) {
        Serial.print("[*] Network information for ");
        Serial.println(ESP_32_WIFI_SSID);

        Serial.println("[+] BSSID : " + WiFi.BSSIDstr());
        Serial.print("[+] Gateway IP : ");
        Serial.println(WiFi.gatewayIP());
        Serial.print("[+] Subnet Mask : ");
        Serial.println(WiFi.subnetMask());
        Serial.println((String)"[+] RSSI : " + WiFi.RSSI() + " dB");
        Serial.print("[+] ESP32 IP : ");
        Serial.println(WiFi.localIP());
        Serial.print("[+] ESP32 DNS IP : ");
        Serial.println(WiFi.dnsIP(0));
    }
}


void ESP32TftThermo::setupSerial() {
  Serial.begin(115200);
  delay(500);
  logV("setupSerial\n");

}

void ESP32TftThermo::setupWifi() {

  // Build Hostname
  char sapString[20]="";
  snprintf(sapString, 20, "esptherm-%08X", ESP.getEfuseMac());
  logD(sapString);

  // Attempt to connect to the AP stored on board, if not, start in SoftAP mode
  WiFi.mode(WIFI_STA);
  delay(2000);

  logD("setHostname");
  WiFi.hostname(String(sapString));

  logD("wifi begin with ssid(%s) and password (.......)", ESP_32_WIFI_SSID);
  WiFi.begin(ESP_32_WIFI_SSID, ESP_32_WIFI_PASSWORD); 
  logD("Connecting");

  for  (int w=0; w<=10 || WiFi.status() != WL_CONNECTED; w++) {
    delay(500);
    logD(".wifi.");
  }
  logV("Connected to the WiFi network");

  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  get_network_info();

  wifiTime();

}


void ESP32TftThermo::wifiTime() {
  const long  gmtOffset_sec = 3600;   // offset seconds, this depends on your time zone (3600 is GMT +1)
  const int   daylightOffset_sec = 3600;  // daylight saving offset seconds
  logD("Setting time via %s, gmt: %d, dst: %d ", ESP_32_WIFI_NTPHOSTNAME, gmtOffset_sec, daylightOffset_sec);

  configTime(gmtOffset_sec, daylightOffset_sec, ESP_32_WIFI_NTPHOSTNAME);

  String t = getTime();
  logI("Time %s" , t.c_str());

}


String ESP32TftThermo::getTime() {
  struct tm timeinfo;
  for (int i=0;i<5;i++) {
    logD(".time.");
    if(getLocalTime(&timeinfo)) {
      char charTime[64];
      strftime(charTime, sizeof(charTime), "%A, %B %d %Y %H:%M:%S", &timeinfo);
      logD("now : %s ", charTime);  
      return String(charTime);
    }
    delay(500);
  }
  logE("failed to obtain time");
  return String("");
}


void ESP32TftThermo::setupTFT() {
  tft.begin();
  tft.setRotation(1);

  clearScreen();

}

void ESP32TftThermo::setupLogging() {
  Logging::setLevel(esp32m::Debug);
  Logging::addAppender(&ETSAppender::instance());
#ifdef SYSLOG_HOST
  udpappender.setMode(UDPAppender::Format::Syslog);
  Logging::addAppender(&udpappender);
#endif
  Serial.println("added appenders");
}

void setup(void) {
  //setup serial 
  thermoapp.setupSerial();

  //setup logging
  thermoapp.setupLogging();

  //connect to wifi and try to get ntp time
  thermoapp.setupWifi();

  //setup the TFT display (and blank out)
  thermoapp.setupTFT();

}


void ESP32TftThermo::appLoop() {
  //current millis
  unsigned long currentMillis = millis();

  //clear every 20seconds (or so)
  if (currentMillis - previousClearMillis >= clearTime) {
    clearScreen();
    //assign previous before we go out of the loop
    previousClearMillis = currentMillis;
  }

  //move  every 2seconds (or so)
  if (currentMillis - previousMoveMillis >= moveTime) {
      //clear previous drawing

      char text[]="23.2 C";
      redraw(TFT_BLACK, TFT_BLACK, zones[0], text);

      slightMove++;
      if (slightMove>10) slightMove=0;
      //draw new one
      redraw(TFT_WHITE, TFT_BLACK, zones[0], text);

    //assign previous before we go out of the loop
    previousMoveMillis = currentMillis;
    }


}

void loop() {
  thermoapp.appLoop();
}

void ESP32TftThermo::clearScreen() {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.fillScreen(TFT_BLACK);            // Clear screen

}

void ESP32TftThermo::redraw(uint16_t c, uint16_t b, Coord& cd, const char* text) {
    // Set text datum to middle centre
  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(c, b);
  tft.setFreeFont(FSS18);                 // Select the font
  tft.drawString(text, cd.x + slightMove, cd.y + slightMove, GFXFF);// Print the string name of the font

}


