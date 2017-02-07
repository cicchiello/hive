#include <sdcard_fexist.t.h>

#include <sdcard_write.t.h>

#define NDEBUG
#include <strutils.h>

#include <sdutils.h>

#include <SdFat.h>


#define FILENAME "TEST.TXT"

bool SDCardFExist::setup() {
    PF("SDCardFExist::setup; ");
    PHL("Initializing SD card...");

    SdFat sd;
    return SDUtils::initSd(sd);
}


static unsigned long timeToAct = 100l;
static bool success = true;

bool SDCardFExist::loop() {
    PF("SDCardFExist::loo; ");
    unsigned long now = millis();
    if (now > timeToAct && !m_didIt) {
	m_didIt = true;

	SdFat sd;
	SDUtils::initSd(sd);
	
	success = sd.exists(FILENAME);
	if (!success) {
	    PH("File ");
	    P(FILENAME);
	    PL(" doesn't exists; creating it to test exist test");
	    
	    // it might have failed because the file doesn't exist or because the existence
	    // test doesn't work...  assume the file doesn't exist -- so create it and try again 
	    SDCardWrite::makeFile(FILENAME);
	    
	    success = sd.exists(FILENAME);
	    if (!success)
	        PHL("It still doesn't exist!?!?");
	}
    }
    return success;
}

