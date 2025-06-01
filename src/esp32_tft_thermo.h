#ifndef ESP32_TFT_THERMO_H
#define ESP32_TFT_THERMO_H

#include "Free_Fonts.h" // Include the header file attached to this sketch
#include "SPI.h"
#include "TFT_eSPI.h"
#include "ESP32Loggable.h"

#include <WiFiClient.h>
#include <WebServer.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>



struct Coord {
  unsigned int x, y;  
};

Coord zones[3] = {
    {80, 22},
    {80, 72},
    {80, 122}
};

// There follows a crude way of flagging that this example sketch needs fonts which
// have not been enabled in the User_Setup.h file inside the TFT_HX8357 library.
//
// These lines produce errors during compile time if settings in User_Setup are not correct
//
// The error will be "does not name a type" but ignore this and read the text between ''
// it will indicate which font or feature needs to be enabled
//
// Either delete all the following lines if you do not want warnings, or change the lines
// to suit your sketch modifications.

#ifndef LOAD_GLCD
//ERROR_Please_enable_LOAD_GLCD_in_User_Setup
#endif

#ifndef LOAD_GFXFF
ERROR_Please_enable_LOAD_GFXFF_in_User_Setup!
#endif



class ESP32TftThermo : public ESP32Loggable {
    public:
        ESP32TftThermo() :  ESP32Loggable("ESP32TftThermo") {
        };

        // Static method to get the instance of the class.
        static ESP32TftThermo& getInstance() {
            // This guarantees that the instance is created only once.
            static ESP32TftThermo instance;
            return instance;
        }

        // Delete the copy constructor and the assignment operator to prevent cloning.
        ESP32TftThermo(const ESP32TftThermo&) = delete;
        ESP32TftThermo& operator=(const ESP32TftThermo&) = delete;

        void clearScreen();

        void redrawZone(u8_t zone, const char *text, const GFXfont *f = FSS18);
        void redraw(uint16_t c, uint16_t b, Coord& cd, const char* text, const GFXfont *f = FSS18);



        void appLoop();

        void redrawZones(char temp[7]);

        static WebServer webServer;
        static WiFiClient espClient;

        //wifi
        void setupWifi();
        void setupSerial();
        void setupTFT();
        void setupLogging();
        void setupOTA();

    protected:
        // Use hardware SPI
        TFT_eSPI tft = TFT_eSPI();



        void get_network_info();

    private:

        unsigned long drawTime = 0;
        unsigned int slightMove = 1;

        unsigned long clearTime=20000;
        unsigned long previousMoveMillis = 0;

        unsigned long moveTime=2000;
        unsigned long previousClearMillis = 0;

        void wifiTime();
        String getTime();
        String getDate();
        String getDateTime();

};

#endif
