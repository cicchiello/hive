#include <TimestampEntry.h>

#include <Arduino.h>

#define P(args) Serial.print(args)
#define PL(args) Serial.println(args)


#ifndef NDEBUG
#define D(args) P(args)
#define DL(args) PL(args)
#else
#define D(args)
#define DL(args)
#endif

#include <str.h>

#include <cloudpipe.h>


void TimestampEntry::post(class Adafruit_BluefruitLE_SPI &ble)
{
    PL("TimestampEntry::post");
    CloudPipe::singleton().requestTimestamp(ble);
}

