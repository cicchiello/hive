#include <sdcard_write2.t.h>

#include <sdutils.h>

#define NDEBUG
#include <Trace.h>

#include <strutils.h>

#include <SdFat.h>


#define FILENAME "/TEST2.TXT"

bool SDCardWrite2::setup() {
    TF("SDCardWrite2::setup");
    PH("Initializing SD card...");

    SdFat sd;
    return SDUtils::initSd(sd);
}


/* STATIC */
bool SDCardWrite2::makeFile(const char *filename)
{
    TF("SDCardWrite2::makeFile");
    
    SdFile f;
    if (!f.open(filename, O_CREAT | O_WRITE)) {
        PH("Couldn't open file");
	return false;
    }

    const char *line1 = "This is a test!\n";
    const char *line2 = "Line2 of the test!\n";
    const char *line3 = "This is a long-ish line to test the readline function's ability to deal with a buffer that is not initially large enough\n";

    char nullTerminator = 0;
    for (int i = 0; i < strlen(line1); i++) {
        int written = f.write(line1[i]);
	if (written != 1) {
	    PH("Failed to write a character to the file");
	    return false;
	}
    }
    
    for (int i = 0; i < strlen(line2); i++) {
        int written = f.write(line2[i]);
	if (written != 1) {
	    PH("Failed to write a character to the file");
	    return false;
	}
    }
    
    for (int i = 0; i < strlen(line3); i++) {
        int written = f.write(line3[i]);
	if (written != 1) {
	    PH("Failed to write a character to the file");
	    return false;
	}
    }

    f.close();

    return true;
}

static unsigned long timeToAct = 1000l;
static bool success = true;

bool SDCardWrite2::loop() {
    TF("SDCardWrite2::loop");
    
    unsigned long now = millis();
    if (now > timeToAct && !m_didIt) {
	m_didIt = true;

	SdFat sd;
	SDUtils::initSd(sd);
	
	// file mustn't exist to begin with
	bool exists = sd.exists(FILENAME);
	if (exists) {
	    sd.remove(FILENAME);
	    exists = sd.exists(FILENAME);
	}

	if (exists) {
	    PH("file that I'm supposed to create already exists and I couldn't delete it");
	    success = false;
	    return false;
	}

	makeFile(FILENAME);
    }
    
    return success;
}

