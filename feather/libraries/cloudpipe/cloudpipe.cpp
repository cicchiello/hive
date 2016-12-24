#include <cloudpipe.h>

#include <Arduino.h>

#include "Adafruit_BluefruitLE_SPI.h"


#include <platformutils.h>

#define HEADLESS
#define NDEBUG

#ifndef HEADLESS
#define P(args) Serial.print(args)
#define PL(args) Serial.println(args)
#else
#define P(args) 
#define PL(args) 
#endif

#ifndef NDEBUG
#define D(args) P(args)
#define DL(args) PL(args)
#else
#define D(args)
#define DL(args)
#endif


/* STATIC */
const char *CloudPipe::SensorLogDb = "hive-sensor-log";     // couchdb name


/* STATIC */
CloudPipe CloudPipe::s_singleton;


void CloudPipe::getMacAddress(Adafruit_BluefruitLE_SPI &ble, Str *mac) const
{
    static bool s_acquiredMacAddress = false;
    static Str s_macAddress;
    if (!s_acquiredMacAddress) {
        ble.println("AT+BLEGETADDR");
	ble.readline();
	s_macAddress = ble.buffer;
	//DL(mac->c_str());
	if (! ble.waitForOK() ) {
	    PL(F("Failed to send?"));
	}
	for (char *p = (char*) s_macAddress.c_str(); *p; p++)
	    if (*p == ':')
	        *p = '-';
	//DL(mac->c_str());
	s_acquiredMacAddress = true;
    }

    *mac = s_macAddress;
}

