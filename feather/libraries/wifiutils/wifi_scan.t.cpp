#include <wifi_scan.t.h>

#include <wifiutils.h>

#define NDEBUG
#include <strutils.h>
#include <str.h>

#include <Trace.h>


//
// Most code taken from Adafruit_WINC1500/examples/ScanNetwork.ino
//


#include <SPI.h>
#include <MyWiFi.h>


static bool success = true;

bool WifiScan::setup() {
    TF("WifiScan::setup");
    TRACE("entry");
    
    WifiUtils::Context ctxt;

    m_didIt = true;
    
    // check for the presence of the shield:
    if (ctxt.getWifi().status() == WL_NO_SHIELD) {
        TRACE("WiFi shield not present");
	return success = false;
    }

    // Print WiFi MAC address:
    char buf[18];
    WifiUtils::getMacAddress(ctxt.getWifi(), buf);
    TRACE2("WiFi MAC address: ", buf);

    // scan for existing networks:
    TRACE("Scanning available networks...");
    WifiUtils::SSID *networks = NULL;
    int numNetworks = 0;
    WifiUtils::getNetworks(ctxt.getWifi(), &networks, &numNetworks);
    for (int i = 0; i < numNetworks; i++) {
        char buf[100];
	sprintf(buf, "%d) %s\tSignal: %d dBm\tEncryption: %s",
		i, networks[i].name, networks[i].strength,
		WifiUtils::encryptionTypename(networks[i].encryptionType));
	TRACE(buf);
    }
    if (networks != NULL)
        delete [] networks;

    return success = (numNetworks > 0);
}


bool WifiScan::loop() {
    return success;
}


