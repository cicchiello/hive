#include <cloudpipe.h>

#include <Arduino.h>

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"


#include <platformutils.h>

#define NDEBUG
#include <str.h>

#define P(args) Serial.print(args)
#define PL(args) Serial.println(args)

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


CloudPipe::CloudPipe()
{
}


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
	for (char *p = (char*) mac->c_str(); *p; p++)
	    if (*p == ':')
	        *p = '-';
	//DL(mac->c_str());
	s_acquiredMacAddress = true;
    }

    *mac = s_macAddress;
}

void CloudPipe::uploadSensorReading(Adafruit_BluefruitLE_SPI &ble,
				    const char *sensorName, const char *value, const char *timestamp) const
{
    Str macAddress;
    getMacAddress(ble, &macAddress);
  
    ble.print("AT+BLEUARTTX=");
    ble.print("cmd|POST|");
    ble.print(SensorLogDb);
    ble.print("|");
    ble.print("{\"hiveid\":\"");
    ble.print(macAddress.c_str());
    ble.print("\",\"sensor\":\"");
    ble.print(sensorName);
    ble.print("\",\"timestamp\":\"");
    ble.print(timestamp);
    ble.print("\",\"value\":\"");
    ble.print(value);
    ble.print("\"}");
    ble.println("\\n");

    // check response status
    if ( ble.waitForOK() ) {
    } else {
        PL(F("Failed to send sensor message"));
    }
}

