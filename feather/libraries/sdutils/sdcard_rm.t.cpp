#include <sdcard_rm.t.h>

#include <sdcard_write.t.h>

#include <sdutils.h>

#define NDEBUG
#include <Trace.h>

#include <strutils.h>

#include <SdFat.h>



#define FILENAME "/TEST.TXT"

bool SDCardRm::setup() {
    TF("SDCardRm::setup");
    
    PH("Initializing SD card...");

    SdFat sd;
    return SDUtils::initSd(sd);
}


static unsigned long timeToAct = 1000l;
static bool success = true;

bool SDCardRm::loop() {
    unsigned long now = millis();
    if (now > timeToAct && !m_didIt) {
	m_didIt = true;

	SdFat sd;
	SDUtils::initSd(sd);

	// file may or may not exist to begin with
	success = sd.exists(FILENAME);
	if (success) {
	    PL("Found file to exist on first try; removing it now");
	    success = sd.remove(FILENAME);
	    if (success) PL("SdFat::remove reported true");
	    else PL("SdFat::remove reported false");
	} else {
	    P("Found file didn't exist; creating ");
	    PL(FILENAME);
	    SDCardWrite::makeFile(FILENAME);

	    success = sd.exists(FILENAME);
	    if (success) {
	        PL("Found file to exist on second try; removing it now");
	        success = sd.remove(FILENAME);
		if (success) PL("SdFat::remove reported true");
		else PL("SdFat::remove reported false");
	    } else {
	        PL("Couldn't create file to test removal");
	    }
	}
	success &= !sd.exists(FILENAME);
    }
    
    return success;
}

