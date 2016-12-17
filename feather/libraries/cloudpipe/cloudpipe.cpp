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


static const char *sensorLogDb = "hive-sensor-log";     // couchdb name


/* STATIC */
CloudPipe CloudPipe::s_singleton;


CloudPipe::CloudPipe()
{
}


void CloudPipe::requestTimestamp(Adafruit_BluefruitLE_SPI &ble) const
{
    ble.print("AT+BLEUARTTX=");
    ble.print("cmd|GETTIME");
    ble.println("\\n");

    // check response status
    if ( ble.waitForOK() ) {
        //DL("got ok after sending GETTIME");
    } else {
        PL("didn't get ok after sending GETTIME");
    }
}


bool CloudPipe::isTimestampResponse(const char *rsp) const
{
    //DL("CloudPipe::isTimestampResponse");
    const char *prefix = "rply|GETTIME|";
    return (strncmp(rsp, prefix, strlen(prefix)) == 0);
}


bool CloudPipe::processTimestampResponse(const char *rsp, unsigned long *timestamp) const
{
    DL("CloudPipe::processTimestampResponse");
    const char *prefix = "rply|GETTIME|";
    char *endPtr;
    *timestamp = strtol(rsp + strlen(prefix), &endPtr, 10);
    Serial.print("Received reply to GETTIME: ");
    Serial.println(*timestamp);
    return endPtr > rsp+strlen(prefix);
}


bool CloudPipe::isSensorUploadResponse(const char *rsp) const
{
    const char *prefix = "rply|POST|";
    return (strncmp(rsp, prefix, strlen(prefix)) == 0);
}


bool CloudPipe::processSensorUploadResponse(const char *rsp) const
{
    const char *prefix = "rply|POST|";
    Str response(rsp + strlen(prefix));
    D("Received reply to POST: ");
    DL(response.c_str());
    return strcmp(response.c_str(), "success") == 0;
}

static void getMacAddress(Adafruit_BluefruitLE_SPI &ble, Str *mac)
{
    ble.println("AT+BLEGETADDR");
    ble.readline();
    *mac = ble.buffer;
    //DL(mac->c_str());
    if (! ble.waitForOK() ) {
        PL(F("Failed to send?"));
    }
    for (char *p = (char*) mac->c_str(); *p; p++)
      if (*p == ':')
	*p = '-';
    //DL(mac->c_str());
}

void CloudPipe::uploadSensorReading(Adafruit_BluefruitLE_SPI &ble,
				    const char *sensorName, const char *value, const char *timestamp) const
{
    static bool acquiredMacAddress = false;
    static Str macAddress;
    if (!acquiredMacAddress) {
        getMacAddress(ble, &macAddress);
	acquiredMacAddress = true;
    }
    
    ble.print("AT+BLEUARTTX=");
    ble.print("cmd|POST|");
    ble.print(sensorLogDb);
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

