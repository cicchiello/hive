#include <wdt.t.h>

#define HEADLESS
#define NDEBUG
#include <Trace.h>

#include <strutils.h>

#include <platformutils.h>

#include <Arduino.h>

static bool success = true;

static unsigned long timeToAct = 50l;

void wdt_get_config_defaults(struct wdt_conf * const config);

static PlatformUtils::WDT_EarlyWarning_Func s_prevTrigger = NULL;
static volatile bool s_earlyWarningTriggered = false;
void wdt_earlyWarning()
{
    PlatformUtils::nonConstSingleton().clearWDT();
    s_earlyWarningTriggered = true;
}


bool WDTTest::setup()
{
    TF("WDTTest::setup");
    
    unsigned long now = millis();
    timeToAct += now;

    PH("Initializing WDT");
    s_prevTrigger = PlatformUtils::nonConstSingleton().initWDT(&wdt_earlyWarning);
    
    return success;
}


static int allIsWellCnt = 0;
bool WDTTest::loop() {
    TF("WDTTest::loop");
    unsigned long now = millis();
    if (now > timeToAct && !m_didIt) {
        if (allIsWellCnt < 3) {
            // if the WDT has already triggered, that's a failure
	    if (s_earlyWarningTriggered) {
	        m_didIt = true;
		PlatformUtils::nonConstSingleton().shutdownWDT(s_prevTrigger);
		PH("WDT EarlyWarning triggered before expected");
		return success = false;
	    }
	    
	    PH("Trying a delay of ");
	    P(PlatformUtils::WDT_MaxTime_ms-250);
	    PL(" ms");
	    
	    PlatformUtils::nonConstSingleton().clearWDT();
	    delay(PlatformUtils::WDT_MaxTime_ms-250); // WDT time base is only approximate
	    
	    // if the WDT has triggered, that's a failure
	    if (s_earlyWarningTriggered) {
	        m_didIt = true;
		PlatformUtils::nonConstSingleton().shutdownWDT(s_prevTrigger);
		PH("WDT EarlyWarning triggered before expected");
		return success = false;
	    } else 
	        PH("The WDT Earlywarning did *not* trigger (as it shouldn't have)!");
	    
	    allIsWellCnt++;
	} else {
	    m_didIt = true;
	    
	    PH("Trying a delay of ");
	    P(PlatformUtils::WDT_MaxTime_ms+250);
	    PL(" ms");
	    
	    PlatformUtils::nonConstSingleton().clearWDT();
	    delay(PlatformUtils::WDT_MaxTime_ms+250); // WDT time base is only approximate

	    // this time it *should* trigger

	    // either way, shutdown the WDT system
	    PlatformUtils::nonConstSingleton().shutdownWDT(s_prevTrigger);
		
	    // if the WDT Early Warning has *not* triggered, that's a failure
	    if (!s_earlyWarningTriggered) {
	        PH("WDT EarlyWarning did *not* trigger when expected");
		return success = false;
	    } else 
	        PH("The WDT EarlyWarning *did* trigger (as it should have)!");
	}
    }

    return success;
}


