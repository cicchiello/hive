#include <cloudpipe.h>

#include <Arduino.h>

#include "Adafruit_BluefruitLE_SPI.h"

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

#define assert(c,msg) if (!(c)) {WDT_TRACE(msg); while(1);}


#include <platformutils.h>


/* STATIC */
const char *CloudPipe::SensorLogDb = "hive-sensor-log";     // couchdb name


/* STATIC */
CloudPipe CloudPipe::s_singleton;


void CloudPipe::initMacAddress(Adafruit_BluefruitLE_SPI &ble)
{
    ble.println("AT+BLEGETADDR");
    ble.readline();
    mMacAddress = new Str(ble.buffer);
    //DL(mac->c_str());
    if (! ble.waitForOK() ) {
        PL(F("Failed to send?"));
    }
    for (char *p = (char*) mMacAddress->c_str(); *p; p++)
        if (*p == ':')
	    *p = '-';
    //DL(mac->c_str());
}

void CloudPipe::getMacAddress(Str *mac) const
{
    assert(mMacAddress, "CloudPipe::initMacAddress must be called before ::getMacAddress");
    *mac = *mMacAddress;
}

