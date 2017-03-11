#include <sdcard_read2.t.h>

#define HEADLESS
#define NDEBUG
#include <Trace.h>

#include <strutils.h>

#include <sdcard_read.t.h>

#include <sdcard_write2.t.h>

#include <sdutils.h>

#include <SdFat.h>


#define FILENAME "/TEST2.TXT"

bool SDCardRead2::setup()
{
    TF("SDCardRead2::setup; ");
    
    PH("Initializing SD card...");

    return true;
}


static unsigned long timeToAct = 1000l;
static bool success = true;

bool SDCardRead2::loop() {
    TF("SDCardRead2::loop");
    
    unsigned long now = millis();
    if (now > timeToAct && !m_didIt) {
	m_didIt = true;

	SdFat sd;
	SDUtils::initSd(sd);

	// file must exist to begin with
	bool exists = sd.exists(FILENAME);
	if (!exists) {
	    PH("TEST.TXT doesn't exist; I'll try to create it.");
	    SDCardWrite2::makeFile(FILENAME);
	}

	exists = sd.exists(FILENAME);
	if (exists) {
	    success = SDCardRead::testFile(FILENAME);
	} else {
	    PH("failed at test");
	    success = false;
	}
    }
    
    return success;
}


