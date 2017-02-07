#include <sdcard_fnexist.t.h>

#define NDEBUG
#include <strutils.h>

#include <sdutils.h>
#include <SdFat.h>


bool SDCardFNExist::setup() {
    PF("SDCardFNExist::setup; ");
    
    PHL("Initializing SD card...");

    SdFat sd;
    return SDUtils::initSd(sd);
}


static unsigned long timeToAct = 1000l;
static bool success = true;

bool SDCardFNExist::loop() {
    PF("SDCardFNexist::loop; ");
    unsigned long now = millis();
    if (now > timeToAct && !m_didIt) {
	m_didIt = true;

	SdFat sd;
	SDUtils::initSd(sd);

	// NONEXIST.TXT should never exist!
	success = !sd.exists("/NONEXIST.TXT");
	if (!success) 
	    PHL("File that shouldn't exist does!?!  /NONEXIST.TXT");
    }
    
    return success;
}

