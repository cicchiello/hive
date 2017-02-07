#include <wifi_connect.t.h>

#include <wifiutils.h>

#define NDEBUG
#include <strutils.h>

#include <Trace.h>

#include <MyWiFi.h>

//
// Most code taken from Adafruit_WINC1500/examples/ConnectWithWPA.ino
//

static int status = WL_IDLE_STATUS;     // the Wifi radio's status


static bool success = true;

static unsigned long timeToAct = 500l;
static int state = 0;
static bool connected = false;
static WifiUtils::Context *s_ctxt = NULL;
static int cycles = 2;
bool WifiConnect::loop() {
    TF("WifiConnect::loop");
    TRACE("entry");

    unsigned long now = millis();
    if (now > timeToAct && !m_didIt) {
        unsigned long callMeBackIn_ms = 0;

	if (s_ctxt == NULL) {
	    s_ctxt = new WifiUtils::Context();

	    // check for the presence of the shield:
	    if (s_ctxt->getWifi().status() == WL_NO_SHIELD) {
	        TRACE("WiFi shield not present");
		return success = false;
	    }

	    // attempt to connect to Wifi network:
	    if (s_ctxt->getWifi().status() == WL_CONNECTED) {
	        TRACE("WiFi shield is already connected!?!?");
		return success = false;
	    }

	    PL("attempting connect");
	}

	if (!connected) {
	    TRACE("calling connector");
	    WifiUtils::ConnectorStatus cstat =
	      WifiUtils::connector(*s_ctxt, WifiConnect::ssid, WifiConnect::pass,
				   &state, &callMeBackIn_ms);
	    if (cstat == WifiUtils::ConnectRetry) {
	        //TRACE("scheduling retry");
	        timeToAct = now + callMeBackIn_ms;
	    } else if (cstat == WifiUtils::ConnectSucceed) {
	        TRACE("WiFi module connected!");
		connected = true;
		WifiUtils::printWifiStatus();
		timeToAct = now + 5000l; 
		state = 0;
		TRACE("Scheduling disconnection in 5s");
		PL("Scheduling disconnection in 5s");
	    } else {
	        m_didIt = true;
	        success = false;
		delete s_ctxt;
		s_ctxt = NULL;
	        if (cstat == WifiUtils::ConnectTimeout) {
		    TRACE("WiFi shield couldn't connect!");
		} else {
		    TRACE2("Unexpected state: ", state);
		}
	    }
	} else {
	    TRACE("calling disconnector");
	    WifiUtils::DisconnectorStatus dstat = WifiUtils::disconnector(*s_ctxt, &state,
									  &callMeBackIn_ms);
	    if (dstat == WifiUtils::DisconnectRetry) {
	        TRACE("scheduling retry");
	        timeToAct = now + callMeBackIn_ms;
	    } else {
	        TRACE("WiFi module disconnected!");
		delete s_ctxt;
		s_ctxt = NULL;
	        if (dstat == WifiUtils::DisconnectSucceed) {
		    TRACE("WiFi module disconnected!");
		    state = 0;
		    connected = false;
		    if (--cycles == 0) {
		        m_didIt = true;
		    } else {
		        timeToAct = now + 5000l; 
		    }
		} else {
		    m_didIt = true;
		    success = false;
		    if (dstat == WifiUtils::DisconnectTimeout) {
		        TRACE("WiFi disconnect attempt timed out!");
		    } else {
		        TRACE2("Unexpected state: ", state);
		    }
		}
	    }
	}
    }

    return success;
}


