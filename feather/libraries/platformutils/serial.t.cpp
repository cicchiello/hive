#include <serial.t.h>

#define NDEBUG
#include <Trace.h>

#include <strutils.h>

#include <Arduino.h>

#include <platformutils.h>

static bool success = true;

bool SerialTest::setup() {
    TF("SerialTest::setup");
    
    // Print a welcome message
    PH("Feather M0 serial# test");
    PL();

    PH("My serial number string is: ");
    PL(PlatformUtils::singleton().serialNumber());

    m_didIt = true;

    return success;
}


bool SerialTest::loop() {
    return success;
}


