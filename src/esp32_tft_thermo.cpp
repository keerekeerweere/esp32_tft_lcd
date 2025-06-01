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

#include <logging.hpp>
#include <ets-appender.hpp>

#define DISABLE_BROWNOUT_DETECTOR

#ifdef DISABLE_BROWNOUT_DETECTOR

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#endif

// in setup()


ESP32TftThermo& thermoapp = ESP32TftThermo::getInstance();
WiFiClient ESP32TftThermo::espClient = WiFiClient();
WebServer ESP32TftThermo::webServer(80);


void ESP32TftThermo::get_network_info(){
    if(WiFi.status() == WL_CONNECTED) {
        logI("[*] Network information for %s ", ESP_32_WIFI_SSID);
        logI("[+] BSSID : %s" , WiFi.BSSIDstr().c_str());        
        logI("[+] Gateway IP : %s", ((String)WiFi.gatewayIP().toString()).c_str() );
        logI("[+] Subnet Mask : %s", ((String)WiFi.subnetMask().toString()).c_str() );
        logI("[+] RSSI : %i dB" , WiFi.RSSI());
        logI("[+] ESP32 IP : %s",  ((String)WiFi.localIP().toString()).c_str());
        //logI("[+] ESP32 DNS IP : %s"), ((String)WiFi.dnsIP(0).toString()).c_str();
    }
}


void ESP32TftThermo::setupSerial() {
  Serial.begin(115200);
  delay(500);
  logV("setupSerial");

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

  String t = getDateTime();
  logI("Time %s" , t.c_str());

}

String ESP32TftThermo::getTime() {
  struct tm timeinfo;
  for (int i=0;i<5;i++) {
    logV(".time.");
    if(getLocalTime(&timeinfo)) {
      char charTime[64];
      strftime(charTime, sizeof(charTime), "%H:%M", &timeinfo);
      logV("now : %s ", charTime);  
      return String(charTime);
    }
    delay(500);
  }
  logE("failed to obtain time");
  return String("");
}


String ESP32TftThermo::getDate() {
  struct tm timeinfo;
  for (int i=0;i<5;i++) {
    logV(".time.");
    if(getLocalTime(&timeinfo)) {
      char charTime[64];
      strftime(charTime, sizeof(charTime), "%d/%m/%y", &timeinfo);
      logV("now : %s ", charTime);  
      return String(charTime);
    }
    delay(500);
  }
  logE("failed to obtain time");
  return String("");
}


String ESP32TftThermo::getDateTime() {
  struct tm timeinfo;
  for (int i=0;i<5;i++) {
    logD(".time.");
    if(getLocalTime(&timeinfo)) {
      char charTime[64];
      strftime(charTime, sizeof(charTime), "%A, %B  %Y %H:%M:%S", &timeinfo);
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

#ifdef DISABLE_BROWNOUT_DETECTOR

  //override brownout detected
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

#endif

  //setup serial 
  thermoapp.setupSerial();

  //setup logging
  thermoapp.setupLogging();

  //connect to wifi and try to get ntp time
  thermoapp.setupWifi();

  //setup OTA 
  thermoapp.setupOTA();

  //setup the TFT display (and blank out)
  thermoapp.setupTFT();

}

void ESP32TftThermo::setupOTA() {
  /*
  ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
      })
      .onEnd([]() {
        Serial.println("\nEnd");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
      });

  ArduinoOTA.begin();  
  */
}


void ESP32TftThermo::appLoop() {
  /*
  //handle OTA 
  ArduinoOTA.handle();
  */

  //current millis
  unsigned long currentMillis = millis();

  char temp[]="23.2 C";

  //clear every 20seconds (or so)
  if (currentMillis - previousClearMillis >= clearTime) {
    clearScreen();
    redrawZones(temp);
    
    //assign previous before we go out of the loop
    previousClearMillis = currentMillis;
  }

  //move  every 2seconds (or so)
  if (currentMillis - previousMoveMillis >= moveTime) {
      //clear previous drawing

      redrawZones(temp);

      // assign previous before we go out of the loop
      previousMoveMillis = currentMillis;
    }
}

void ESP32TftThermo::redrawZones(char temp[7])
{
  redrawZone(0, temp, FSS24);
  redrawZone(1, getTime().c_str());
  redrawZone(2, getDate().c_str(), FSS12);
}

void ESP32TftThermo::redrawZone(u8_t zone, const char *text, const GFXfont *f)
{
    logD("redraw zone %i with %s", zone, text);
  // draw new one
  redraw(TFT_WHITE, TFT_BLACK, zones[zone], text, f);

  slightMove++;
  if (slightMove > 10)
    slightMove = 0;

}

void loop() {
  thermoapp.appLoop();
}

void ESP32TftThermo::clearScreen() {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.fillScreen(TFT_BLACK);            // Clear screen

}

void ESP32TftThermo::redraw(uint16_t c, uint16_t b, Coord& cd, const char* text, const GFXfont *f) {
    // Set text datum to middle centre
  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(c, b);
  tft.setFreeFont(f);                 // Select the font
  //tft.drawString(text, cd.x + slightMove, cd.y + slightMove, GFXFF);// Print the string name of the font
  tft.drawString(text, cd.x , cd.y , GFXFF);// Print the string name of the font

}


