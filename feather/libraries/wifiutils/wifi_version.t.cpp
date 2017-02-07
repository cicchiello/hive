#include <wifi_version.t.h>

#include <Arduino.h>

#include <wifiutils.h>

#define NDEBUG

#include <Trace.h>

#include <strutils.h>

#include <SPI.h>
#include <MyWiFi.h>

static bool success = true;

bool WifiVersion::setup() {
    TF("WifiVersion::setup");
    TRACE("entry");

    WifiUtils::Context ctxt;

    // Print a welcome message
    TRACE("WINC1500 firmware check.");
    TRACE("");

    m_didIt = true;
    if (ctxt.getWifi().status() == WL_NO_SHIELD) {
        TRACE("NOT PRESENT");
	return success = false; // don't continue
    }
    TRACE("DETECTED");

    // Print firmware version on the shield
    const char *fv = WiFi.firmwareVersion();
    TRACE2("Firmware version installed: ", fv);

    // Print required firmware version
    TRACE2("Firmware version required : ", WIFI_FIRMWARE_REQUIRED);

    // Check if the required version is installed
    TRACE("");
    if (strcmp(fv, WIFI_FIRMWARE_REQUIRED) == 0) {
        TRACE("Firmware version check result: PASSED");
	return success = true;
    } else {
        TRACE("Firmware version check result: NOT PASSED");
	TRACE(" - The firmware version on the WINC1500 do not match the");
	TRACE("   version required by the library, you may experience");
	TRACE("   issues or failures.");
	return success = false;
    }
}


bool WifiVersion::loop() {
    return success;
}



